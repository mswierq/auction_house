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

  void receive_data(const uint16_t port = 9999);

  void send_data(ConnectionId connection, const std::string &data);

private:
  std::list<Connection> _connections;
  Database& _database;
  TasksQueue &_queue;
  SessionId _next_session_id = 0;
};
} // namespace auction_engine
