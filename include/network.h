//
// Created by mswiercz on 24.11.2021.
//
#pragma once
#include "session_id.h"
#include <list>
#include <string>

namespace auction_engine {
class Database;
class TasksQueue;

using ConnectionId = int; // socket desc

struct Connection {
  ConnectionId connection;
  SessionId session_id;
};

class Network {
public:
  Network(Database &database, TasksQueue &queue)
      : _database(database), _queue(queue) {}

  // Receives data from already connected users, waits for new connections
  // and starts new sessions for them, closes connections and remove unused
  // sessions when a user hangs up
  void serve_ingress(const uint16_t port = 10000);

  // Tries to send data to user with given connection id, if fails just drops
  // the message
  void send_data(const ConnectionId connection, std::string &&data);

  //Creates new session for a new connection
  bool create_new_session(const ConnectionId connection_id);

private:
  std::list<Connection> _connections;
  Database &_database;
  TasksQueue &_queue;
  SessionId _next_session_id = 0;
};
} // namespace auction_engine
