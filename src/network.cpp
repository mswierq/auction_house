//
// Created by mswiercz on 27.11.2021.
//
#include "network.h"
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <array>
#include <algorithm>
#ifndef WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <winsock.h>
#endif

#ifdef WIN32
#define close(a) closesocket(a)
using socklen_t = int;
#endif

namespace auction_house::network {
static fd_set read_fds{};
static fd_set connected_fds{};
ConnectionId max_connection_id = INVALID_CONNECTION;

ConnectionId init_server_socket(const uint16_t port) {
  #ifdef WIN32
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) {
    spdlog::error("WSAStartup failed.");
    exit(1);
  }
  #endif

  auto server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_fd == INVALID_CONNECTION) {
    spdlog::error("Couldn't create a server socket!");
    std::exit(1);
  }

  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                 reinterpret_cast<char *>(&opt), sizeof(opt)) == SOCKET_ERROR) {
    spdlog::error("Couldn't reuse address!");
    exit(1);
  }

  sockaddr_in server_address = {AF_INET, htons(port), INADDR_ANY, {0}};
  if (bind(server_fd, reinterpret_cast<sockaddr *>(&server_address),
           sizeof(sockaddr)) == SOCKET_ERROR) {
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

  #ifndef WIN32
  char address_buffer[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(server_address.sin_addr), address_buffer,
            INET_ADDRSTRLEN);
  spdlog::info("Successfully started the server {}:{}!",
               std::string(address_buffer, std::strlen(address_buffer)),
               htons(server_address.sin_port));
  #endif

  max_connection_id = server_fd;

  return server_fd;
}

void close_connection(const ConnectionId connection) {
  spdlog::info("Closing connection {}!", connection);
  close(connection);
  FD_CLR(connection, &connected_fds);
}

ConnectionId handle_new_connection(const ConnectionId server_fd) {
  if (FD_ISSET(server_fd, &read_fds)) {
    sockaddr_in client_address{};
    socklen_t sin_size = sizeof(sockaddr_in);
    auto client_fd = accept(
        server_fd, reinterpret_cast<sockaddr *>(&client_address), &sin_size);

    if (client_fd != SOCKET_ERROR) {
      #ifndef WIN32
      char address_buffer[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(client_address.sin_addr), address_buffer,
                INET_ADDRSTRLEN);
      spdlog::info("Received new connection from {}:{}",
                   std::string(address_buffer, std::strlen(address_buffer)),
                   htons(client_address.sin_port));
      #endif

      FD_SET(client_fd, &connected_fds);
      #ifndef WIN32
      max_connection_id = std::max(max_connection_id, client_fd);
      #else
      max_connection_id = max(max_connection_id, client_fd);
      #endif
      return client_fd;
    } else {
      spdlog::error("Accepting a new connection has failed!");
    }
  }
  return INVALID_CONNECTION;
}

std::optional<std::string> receive_data(const ConnectionId connection) {
  std::optional<std::string> result = {};
  if (FD_ISSET(connection, &read_fds)) {
    spdlog::debug("Receiving data for connection {}", connection);
    std::array<char, 1024> buffer;
    std::string data{};
    std::size_t n_bytes = 0;
    auto is_eol = false;
    do {
      n_bytes = recv(connection, buffer.data(), buffer.size(), 0);
      data.append(buffer.data(), n_bytes);
      is_eol = data.size() > 0 && data.back() == '\n';
    } while(!is_eol && n_bytes == buffer.size());
    if (n_bytes == SOCKET_ERROR) {
      spdlog::error("Something went wrong while reading data for connection {}",
                    connection);
    }
    return {std::move(data)};
  }
  return result;
}

void wait_for_traffic() {
  spdlog::debug("Waiting for incoming connections/data!");
  read_fds = connected_fds;
  if ((select(max_connection_id + 1, &read_fds, nullptr, nullptr, nullptr)) ==
      SOCKET_ERROR) {
    spdlog::error("Waiting for incoming connections/data has failed!");
  }
}

void send_data(ConnectionId connection, std::string &&data) {
  data.append("\nCMD>>");
  data.insert(0, "RESP>> ");
  const auto *data_ptr = data.data();
  std::size_t n_bytes_to_send = data.size();
  std::size_t n_sent_bytes = 0;
  do {
    n_sent_bytes = send(connection, data.data(), data.size(), 0);
    if (n_sent_bytes == SOCKET_ERROR) {
      spdlog::warn("Sending data for connection {} has failed", connection);
      return;
    }
    n_bytes_to_send -= n_sent_bytes;
  } while (n_bytes_to_send > 0);
}
} // namespace auction_house::network