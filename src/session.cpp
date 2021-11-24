//
// Created by mswiercz on 23.11.2021.
//
#include "session.h"
#include <mutex>

namespace auction_engine {
bool SessionManager::start_session(const SessionId id) {
  std::unique_lock _l{_mutex};
  if (_sessions.find(id) == _sessions.end()) {
    _sessions[id] = {};
    return true;
  }
  return false;
}

bool SessionManager::end_session(const SessionId id) {
  std::unique_lock _l{_mutex};
  if (_sessions.find(id) == _sessions.end()) {
    return false;
  }
  auto username_opt = _sessions[id];
  if (username_opt) {
    _logged_users.erase(username_opt.value());
  }
  _sessions.erase(id);
  return true;
}

bool SessionManager::login(const SessionId id, const std::string &username) {
  std::unique_lock _l{_mutex};
  if (_sessions.find(id) == _sessions.end() ||
      _logged_users.find(username) != _logged_users.end() || username.empty()) {
    return false;
  }
  _logged_users[username] = id;
  _sessions[id] = username;
  return true;
}

bool SessionManager::logout(const SessionId id) {
  std::unique_lock _l{_mutex};
  if (_sessions.find(id) == _sessions.end()) {
    return false;
  }
  auto username = _sessions[id].value_or(""); // empty username doesn't exit
  auto logged_it = _logged_users.find(username);
  if (logged_it == _logged_users.end()) {
    return false;
  }
  _logged_users.erase(logged_it);
  _sessions[id] = {};
  return true;
}

std::optional<std::string> SessionManager::get_username(const SessionId id) {
  std::shared_lock _l{_mutex};
  if (_sessions.find(id) == _sessions.end()) {
    return {};
  }
  return _sessions[id];
}

std::optional<SessionId>
SessionManager::get_session_id(const std::string &username) {
  std::shared_lock _l{_mutex};
  if (_logged_users.find(username) == _logged_users.end()) {
    return {};
  }
  return _logged_users[username];
}
} // namespace auction_engine