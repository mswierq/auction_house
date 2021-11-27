//
// Created by mswiercz on 23.11.2021.
//
#pragma once
#include "session_id.h"
#include "connection_id.h"
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace auction_house::engine {
struct Session {
  ConnectionId connection;
  std::optional<std::string> username;
};

class SessionManager {
public:
  // Creates a new session when a user connects
  bool start_session(const SessionId id, const ConnectionId conn_id);

  // Ends a session when a user disconnects
  bool end_session(const SessionId id);

  // Logs in a user, returns false when other session has logged for given
  // username
  bool login(const SessionId id, const std::string &username);

  // Logs out a user, returns false if user wasn't logged in.
  bool logout(const SessionId id);

  // Returns username for the given session, none if user isn't logged in
  std::optional<std::string> get_username(const SessionId id);

  // Returns SessionId for the given username, none if user isn't logged in
  std::optional<SessionId> get_session_id(const std::string &username);

  // Returns a connection id for the given session
  std::optional<ConnectionId> get_connection_id(const SessionId id);

private:
  // keeps the current sessions and if user is logged in
  std::unordered_map<SessionId, Session> _sessions;
  std::unordered_map<std::string, SessionId> _logged_users;
  std::shared_mutex _mutex;
};
} // namespace auction_house::engine
