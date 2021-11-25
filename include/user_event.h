//
// Created by mswiercz on 21.11.2021.
//
#pragma once
#include "funds_type.h"
#include "session_id.h"
#include <optional>

namespace auction_engine {
// An ingress/egress event
struct UserEvent {
  // A logged-in user has to fill this field
  std::optional<std::string> username;
  SessionId session_id;
  std::string data;
};
} // namespace auction_engines
