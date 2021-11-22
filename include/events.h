//
// Created by mswiercz on 21.11.2021.
//
#pragma once
#include "session_id.h"
#include <optional>

namespace auction_engine {
struct Event {
  //A logged-in user hast to fill this field
  std::optional<std::string> username;
  SessionId session_id;
  std::string data;
};
} // namespace auction_engine
