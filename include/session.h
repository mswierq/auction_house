//
// Created by mswiercz on 23.11.2021.
//
#pragma once
#include "session_id.h"
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace auction_engine {
class SessionManager {
public:
  // Creates a new session when a user connects
  bool start_session(const SessionId id);

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

private:
  // keeps the current sessions and if user is logged in
  std::unordered_map<SessionId, std::optional<std::string>> _sessions;
  std::unordered_map<std::string, SessionId> _logged_users;
  std::shared_mutex _mutex;
};
} // namespace auction_engine
