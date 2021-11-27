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

static ConnectionId init_server(const uint16_t port) {
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

static void close_connection(const ConnectionId connection) {
  close(connection);
  spdlog::info("Closing connection {}!", connection);
}

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

void Network::serve_ingress(const uint16_t port) {
  auto server_fd = init_server(port);

  for (;;) {
    read_fds = connected_fds;
    spdlog::debug("Waiting for incoming connections/data!");
    // Wait for a new connection or data on already opened sockets
    if ((select(max_connection_id + 1, &read_fds, nullptr, nullptr, nullptr)) ==
        ERROR) {
      spdlog::error("Waiting for incoming connections/data has failed!");
    }

    // Check if there is data waiting from the previously connected users
    for (auto connection_it = _connections.begin();
         connection_it != _connections.end();) {
      bool remove_connection = false;
      spdlog::debug(
          "Check if there are data to receive for connection {} session {}!",
          connection_it->connection, connection_it->session_id);
      if (FD_ISSET(connection_it->connection, &read_fds)) {
        spdlog::debug("Receiving data for connection {}",
                      connection_it->connection);

        // Receive data
        std::array<char, 1024> buffer;
        std::string data{};
        std::size_t n_bytes = 0;
        if ((n_bytes = recv(connection_it->connection, buffer.begin(),
                            buffer.size(), 0)) > 0) {
          data.append(buffer.begin(), n_bytes);
        }

        if (n_bytes < 1) { // Connection has been closed or error occurred
          spdlog::info("Closing connection {} and session {}",
                       connection_it->connection, connection_it->session_id);
          close_connection(connection_it->connection);
          FD_CLR(connection_it->connection, &connected_fds);
          if (!_database.sessions.end_session(connection_it->session_id)) {
            spdlog::error("Couldn't end session {} for connection {}!",
                          connection_it->session_id, connection_it->connection);
          }
          remove_connection = true;
        } else {           // Prepare task and send it to be processed
          data.pop_back(); // remove new line sign
          auto username =
              _database.sessions.get_username(connection_it->session_id);
          spdlog::debug("Creating new task for connection: {}, session: {}, "
                        "username: {}, received data size {}!",
                        connection_it->connection, connection_it->session_id,
                        username.value_or(""), data.size());
          _queue.enqueue(create_command_task(
              {username, connection_it->session_id, std::move(data)},
              _database));
        }
      }
      if (remove_connection) {
        connection_it = _connections.erase(connection_it);
      } else {
        ++connection_it;
      }
    }

    // Handle new connection
    auto client_connection = handle_new_connection(server_fd);
    if (client_connection != INVALID_CONNECTION) {
      if (!create_new_session(client_connection)) {
        close_connection(client_connection);
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

bool Network::create_new_session(const ConnectionId connection_id) {
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
} // namespace auction_engine