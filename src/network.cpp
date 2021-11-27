//
// Created by mswiercz on 26.11.2021.
//
#include "database.h"
#include "spdlog/spdlog.h"
#include "tasks_queue.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

namespace auction_engine {

constexpr auto MAX_CONNECTIONS = 100;
constexpr auto INVALID_CONNECTION = -1;
constexpr auto ERROR = -1;

static fd_set read_fds{};
static fd_set connected_fds{};
static ConnectionId max_connection_id = INVALID_CONNECTION;

//Initializes the server sockets, binds to port etc.
static ConnectionId init_server_socket(const uint16_t port) {
  auto server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == INVALID_CONNECTION) {
    spdlog::error("Couldn't create a server socket!");
    std::exit(1);
  }

  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                 reinterpret_cast<void *>(&opt), sizeof(opt)) == ERROR) {
    spdlog::error("Couldn't reuse address!");
    exit(1);
  }

  sockaddr_in server_address = {AF_INET, htons(port), INADDR_ANY, {0}};
  if (bind(server_fd, reinterpret_cast<sockaddr *>(&server_address),
           sizeof(sockaddr)) == ERROR) {
    spdlog::error("Couldn't bind! Port {}", port);
    std::exit(1);
  }

  if ((listen(server_fd, MAX_CONNECTIONS)) == -1) {
    spdlog::error("Listening on port {} has failed!", port);
    std::exit(1);
  }

  FD_ZERO(&read_fds);
  FD_ZERO(&connected_fds);
  FD_SET(server_fd, &connected_fds);

  char address_buffer[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(server_address.sin_addr), address_buffer,
            INET_ADDRSTRLEN);
  spdlog::info("Successfully started the server {}:{}!",
               std::string(address_buffer, std::strlen(address_buffer)),
               htons(server_address.sin_port));

  max_connection_id = server_fd;

  return server_fd;
}

//Closes a connection
static void close_connection(const ConnectionId connection) {
  spdlog::info("Closing connection {}!", connection);
  close(connection);
  FD_CLR(connection, &connected_fds);
}

//Handles a new connection
static ConnectionId handle_new_connection(const ConnectionId server_fd) {
  if (FD_ISSET(server_fd, &read_fds)) {
    sockaddr_in client_address{};
    socklen_t sin_size = sizeof(sockaddr_in);
    auto client_fd = accept(
        server_fd, reinterpret_cast<sockaddr *>(&client_address), &sin_size);

    if (client_fd != ERROR) {
      char address_buffer[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(client_address.sin_addr), address_buffer,
                INET_ADDRSTRLEN);
      spdlog::info("Received new connection from {}:{}",
                   std::string(address_buffer, std::strlen(address_buffer)),
                   htons(client_address.sin_port));

      FD_SET(client_fd, &connected_fds);
      max_connection_id = std::max(max_connection_id, client_fd);
      return client_fd;
    } else {
      spdlog::error("Accepting a new connection has failed!");
    }
  }
  return INVALID_CONNECTION;
}

// Returns user data or none when user hasn't sent anything yet, empty data
// means data a user hung up
std::optional<std::string> receive_data(const ConnectionId connection) {
  std::optional<std::string> result = {};
  if (FD_ISSET(connection, &read_fds)) {
    spdlog::debug("Receiving data for connection {}", connection);
    std::array<char, 1024> buffer;
    std::string data{};
    std::size_t n_bytes = 0;
    if ((n_bytes = recv(connection, buffer.begin(), buffer.size(), 0)) > 0) {
      data.append(buffer.begin(), n_bytes);
    }
    if (n_bytes == ERROR) {
      spdlog::error("Something went wrong while reading data for connection {}",
                    connection);
    }
    return {std::move(data)};
  }
  return result;
}

//Awaits for new connections or user data to receive
void wait_for_network_traffic() {
  spdlog::debug("Waiting for incoming connections/data!");
  read_fds = connected_fds;
  if ((select(max_connection_id + 1, &read_fds, nullptr, nullptr, nullptr)) ==
      ERROR) {
    spdlog::error("Waiting for incoming connections/data has failed!");
  }
}

void Network::serve_ingress(const uint16_t port) {
  auto server_socket = init_server_socket(port);

  for (;;) {
    wait_for_network_traffic();

    _scan_connections();

    auto new_connection = handle_new_connection(server_socket);
    if (new_connection != INVALID_CONNECTION) {
      if (!_create_new_session(new_connection)) {
        close_connection(new_connection);
      }
    }
  }
}

void Network::send_data(const ConnectionId connection, std::string &&data) {
  data.append("\nCMD>>");
  data.insert(0, "RESP>> ");
  const auto *data_ptr = data.data();
  std::size_t n_bytes_to_send = data.size();
  std::size_t n_sent_bytes = 0;
  do {
    n_sent_bytes = send(connection, data.data(), data.size(), 0);
    if (n_sent_bytes == ERROR) {
      spdlog::warn("Sending data for connection {} has failed", connection);
      return;
    }
    n_bytes_to_send -= n_sent_bytes;
  } while (n_bytes_to_send > 0);
}

bool Network::_create_new_session(const ConnectionId connection_id) {
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

void Network::_end_connection(const ConnectionId connection_id,
                             const SessionId session_id) {
  spdlog::info("Closing session {}!", session_id);
  if (!_database.sessions.end_session(session_id)) {
    spdlog::error("Couldn't end session {} for connection {}!", session_id,
                  connection_id);
  }
  close_connection(connection_id);
}

void Network::_serve_user_data(std::string &&data, const SessionId session_id) {
  data.pop_back(); // remove new line sign
  auto username = _database.sessions.get_username(session_id);
  spdlog::debug("Creating new task for session: {}, "
                "username: {}, received data size {}!",
                session_id, username.value_or(""), data.size());
  _queue.enqueue(
      create_command_task({username, session_id, std::move(data)}, _database));
}

void Network::_scan_connections() {
  for (auto connection_it = _connections.begin();
       connection_it != _connections.end();) {
    spdlog::debug(
        "Checking if there are data to receive for connection {} session {}!",
        connection_it->connection, connection_it->session_id);

    auto user_data = receive_data(connection_it->connection);

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
} // namespace auction_engine