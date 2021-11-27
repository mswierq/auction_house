//
// Created by mswiercz on 23.11.2021.
//
#include "session.h"
#include <mutex>

namespace auction_house::engine {
bool SessionManager::start_session(const SessionId id,
                                   const ConnectionId conn_id) {
  std::unique_lock _l{_mutex};
  if (_sessions.find(id) == _sessions.end()) {
    _sessions[id] = {conn_id, {}};
    return true;
  }
  return false;
}

bool SessionManager::end_session(const SessionId id) {
  std::unique_lock _l{_mutex};
  auto session_it = _sessions.find(id);
  if (session_it == _sessions.end()) {
    return false;
  }
  auto username_opt = session_it->second.username;
  if (username_opt) {
    _logged_users.erase(username_opt.value());
  }
  _sessions.erase(session_it);
  return true;
}

bool SessionManager::login(const SessionId id, const std::string &username) {
  std::unique_lock _l{_mutex};
  auto session_it = _sessions.find(id);
  auto logged_it = _logged_users.find(username);
  if (session_it == _sessions.end() || logged_it != _logged_users.end() ||
      username.empty()) {
    return false;
  }
  _logged_users[username] = id;
  session_it->second.username = username;
  return true;
}

bool SessionManager::logout(const SessionId id) {
  std::unique_lock _l{_mutex};
  auto session_it = _sessions.find(id);
  if (session_it == _sessions.end()) {
    return false;
  }
  auto username =
      session_it->second.username.value_or(""); // empty username doesn't exit
  auto logged_it = _logged_users.find(username);
  if (logged_it == _logged_users.end()) {
    return false;
  }
  _logged_users.erase(logged_it);
  session_it->second.username = {};
  return true;
}

std::optional<std::string> SessionManager::get_username(const SessionId id) {
  std::shared_lock _l{_mutex};
  auto it = _sessions.find(id);
  if (it == _sessions.end()) {
    return {};
  }
  return it->second.username;
}

std::optional<SessionId>
SessionManager::get_session_id(const std::string &username) {
  std::shared_lock _l{_mutex};
  if (_logged_users.find(username) == _logged_users.end()) {
    return {};
  }
  return _logged_users[username];
}

std::optional<ConnectionId>
SessionManager::get_connection_id(const SessionId id) {
  std::shared_lock _l{_mutex};
  auto session_it = _sessions.find(id);
  if (session_it != _sessions.end()) {
    return session_it->second.connection;
  }
  return {};
}
} // namespace auction_house::engine