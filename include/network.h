//
// Created by mswiercz on 27.11.2021.
//
#pragma once
#include "connection_id.h"
#include <cstdint>
#include <optional>
#include <string>

namespace auction_house::network {
constexpr auto MAX_CONNECTIONS = 100;
constexpr auto INVALID_CONNECTION = -1;

// Initializes the server sockets, binds to port etc.
ConnectionId init_server_socket(const uint16_t port);

// Closes a connection
void close_connection(const ConnectionId connection);

// Handles a new connection
ConnectionId handle_new_connection(const ConnectionId server_fd);

// Returns user data or none when user hasn't sent anything yet, empty data
// means data a user hung up
std::optional<std::string> receive_data(const ConnectionId connection);

// Awaits for new connections or user data to receive
void wait_for_traffic();

// Sends data to user, in case of an error drops it
void send_data(ConnectionId connectionId, std::string &&data);
} // namespace auction_house::network
