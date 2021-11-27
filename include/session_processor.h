//
// Created by mswiercz on 24.11.2021.
//
#pragma once
#include "connection_id.h"
#include "session_id.h"
#include <list>
#include <string>

namespace auction_house::engine {
class Database;
class TasksQueue;

struct Connection {
  ConnectionId connection;
  SessionId session_id;
};

class SessionProcessor {
public:
  SessionProcessor(Database &database, TasksQueue &queue)
      : _database(database), _queue(queue) {}

  // Receives data from already connected users, waits for new connections
  // and starts new sessions for them, closes connections and remove unused
  // sessions when a user hangs up
  void serve_ingress(const uint16_t port = 10000);

private:
  // Creates a new session for a new connection
  bool _create_new_session(const ConnectionId connection_id);

  // Ends a session and associated connection
  void _end_connection(const ConnectionId connection_id,
                       const SessionId session_id);

  // Prepares a task for received data and puts it on a queue
  void _serve_user_data(std::string &&data, const SessionId session_id);

  // Goes through the active connections, reads the data and prepares tasks,
  // closes inactive connections
  void _scan_connections();

  std::list<Connection> _connections;
  Database &_database;
  TasksQueue &_queue;
  SessionId _next_session_id = 0;
};
} // namespace auction_house::engine
