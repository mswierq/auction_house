//
// Created by mswiercz on 21.11.2021.
//
#pragma once
#include "funds_type.h"
#include "session_id.h"
#include <optional>

namespace auction_house::engine {
struct IngressEvent {
  // This is set when a user is logged in
  std::optional<std::string> username;
  SessionId session_id;
  std::string data;
};

struct EgressEvent {
  // Session id is either copied from an ingress event or set by auction
  // processing when seller is connected and logged in. Egress events with none
  // session id are dropped.
  std::optional<SessionId> session_id;
  std::string data;
};
} // namespace auction_house::engine
