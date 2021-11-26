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

constexpr auto MAX_CONNECTIONS = 100;
constexpr auto INVALID_CONNECTION = -1;
constexpr auto ERROR = -1;

namespace auction_engine {

void Network::receive_data(const uint16_t port) {
  auto server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == INVALID_CONNECTION) {
    spdlog::error("Couldn't create a server socket!");
    std::exit(1);
  }

  sockaddr_in server_address = {AF_INET, htons(port), INADDR_ANY, {0}};
  if (bind(server_fd, reinterpret_cast<sockaddr *>(&server_address),
           sizeof(sockaddr)) == -1) {
    spdlog::error("Couldn't bind! Port {}", port);
    std::exit(1);
  }

  if ((listen(server_fd, MAX_CONNECTIONS)) == -1) {
    spdlog::error("Listening on port {} has failed!", port);
    std::exit(1);
  }

  fd_set read_fds{};
  FD_ZERO(&read_fds);
  FD_SET(server_fd, &read_fds);
  auto max_connection_id = server_fd;

  char address_buffer[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(server_address.sin_addr), address_buffer,
            INET_ADDRSTRLEN);
  spdlog::info("Successfully started the server {}:{}!",
               std::string(address_buffer, std::strlen(address_buffer)),
               htons(server_address.sin_port));

  for (;;) {
    // Wait for a new connection or data on already opened sockets
    if ((select(max_connection_id + 1, &read_fds, nullptr, nullptr, nullptr)) ==
        -1) {
      spdlog::error("Waiting for incoming connections/data has failed!");
      std::exit(1);
    }

    // Handle new connection
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

        auto session_id = _next_session_id++;
        if (_database.sessions.start_session(session_id, client_fd)) {
          FD_SET(client_fd, &read_fds);
          max_connection_id = std::max(max_connection_id, client_fd);
          _connections.push_back({client_fd, session_id});
        } else {
          close(client_fd);
          spdlog::error("Starting new session {} for connection {} has failed!",
                        session_id, client_fd);
        }
      } else {
        spdlog::error("Accepting a new connection has failed!");
      }
    }

    // Check if there is data waiting from the current connections
    for (auto connection_it = _connections.begin();
         connection_it != _connections.end();) {
      if (FD_ISSET(connection_it->connection, &read_fds)) {
        // Receive data
        std::array<char, 1024> buffer;
        std::string data;
        std::size_t n_bytes = 0;
        if ((n_bytes = recv(connection_it->connection, buffer.begin(),
                            buffer.size(), 0)) > 0) {
          data.append(buffer.begin(), n_bytes);
        }

        if (data.empty()) { // Connection has been closed
          spdlog::info("Closing connection {} and session {}",
                       connection_it->connection, connection_it->session_id);
          close(connection_it->connection);
          FD_CLR(connection_it->connection, &read_fds);
          if (!_database.sessions.end_session(connection_it->connection)) {
            spdlog::error("Couldn't end session {} for connection {}!",
                          connection_it->session_id, connection_it->connection);
          }
          connection_it = _connections.erase(connection_it);

        } else { // Prepare task and send it to be processed
          auto username =
              _database.sessions.get_username(connection_it->connection);
          spdlog::debug("Creating new task for connection: {}, session: {}, "
                        "username: {}, received data size {}!",
                        connection_it->connection, connection_it->session_id,
                        username.value_or(""), data.size());
          _queue.enqueue(create_command_task(
              {username, connection_it->session_id, std::move(data)},
              _database));
          ++connection_it;
        }
      }
    }
  }
}

void Network::send_data(ConnectionId connection, const std::string &data) {
  const auto *data_ptr = data.data();
  std::size_t n_bytes_to_send = data.size();
  std::size_t sent_bytes = 0;
  do {
    sent_bytes = send(connection, data.data(), data.size(), 0);
    if (sent_bytes == ERROR) {
      spdlog::warn("Sending data for connection {} has failed", connection);
    }
    n_bytes_to_send -= sent_bytes;
  } while (n_bytes_to_send > 0);
}
} // namespace auction_engine