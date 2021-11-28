//
// Created by mswiercz on 26.11.2021.
//
#include "session_processor.h"
#include "database.h"
#include "network.h"
#include "spdlog/spdlog.h"
#include "tasks_queue.h"
#include <thread>

namespace auction_house::engine {

void SessionProcessor::serve_ingress(const uint16_t port) {
  auto server_socket = network::init_server_socket(port);

  for (;;) {
    network::wait_for_traffic();

    _scan_connections();

    auto new_connection = network::handle_new_connection(server_socket);
    if (new_connection != network::INVALID_CONNECTION) {
      if (!_create_new_session(new_connection)) {
        network::close_connection(new_connection);
      }
    }
  }
}

bool SessionProcessor::_create_new_session(const ConnectionId connection_id) {
  auto session_id = _next_session_id++;
  if (_database.sessions.start_session(session_id, connection_id)) {
    _connections.push_back({connection_id, session_id});
    _queue.enqueue(create_command_task({{}, session_id, "HELP"}, _database));
    spdlog::debug("Started new session {} for connection {}", session_id,
                  connection_id);
    return true;
  }
  spdlog::error("Starting new session {} for connection {} has failed!",
                session_id, connection_id);
  return false;
}

void SessionProcessor::_end_connection(const ConnectionId connection_id,
                                       const SessionId session_id) {
  spdlog::info("Closing session {}!", session_id);
  if (!_database.sessions.end_session(session_id)) {
    spdlog::error("Couldn't end session {} for connection {}!", session_id,
                  connection_id);
  }
  network::close_connection(connection_id);
}

void SessionProcessor::_serve_user_data(std::string &&data,
                                        const SessionId session_id) {
  if (data.back() == '\n') {
    data.pop_back();
  }
  auto username = _database.sessions.get_username(session_id);
  spdlog::debug("Creating new task for session: {}, "
                "username: {}, received data size {}!",
                session_id, username.value_or(""), data.size());
  _queue.enqueue(
      create_command_task({username, session_id, std::move(data)}, _database));
}

void SessionProcessor::_scan_connections() {
  for (auto connection_it = _connections.begin();
       connection_it != _connections.end();) {
    spdlog::debug(
        "Checking if there are data to receive for connection {} session {}!",
        connection_it->connection, connection_it->session_id);

    auto user_data = network::receive_data(connection_it->connection);

    if (user_data.has_value()) {
      auto &data = user_data.value();
      if (data.empty()) {
        _end_connection(connection_it->connection, connection_it->session_id);
        connection_it = _connections.erase(connection_it);
        continue; // go to next connection
      } else {
        _serve_user_data(std::move(data), connection_it->session_id);
      }
    }
    ++connection_it;
  }
}
} // namespace auction_house::engine