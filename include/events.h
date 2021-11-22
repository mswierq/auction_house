//
// Created by mswiercz on 21.11.2021.
//
#pragma once
#include "session_id.h"
#include <optional>

namespace auction_engine {
struct Event {
  //A logged-in user has to fill this field
  std::optional<std::string> username;
  //A session id isn't set by the Auction processor
  std::optional<SessionId> session_id;
  std::string data;
};
} // namespace auction_engine
