//
// Created by mswiercz on 21.11.2021.
//
#pragma once
#include "session_id.h"
#include <optional>
#include "funds_type.h"

namespace auction_engine {
//An ingress/egress event
struct UserEvent {
  //A logged-in user has to fill this field
  std::optional<std::string> username;
  //The auction processor doesn't know the session ids, only usernames
  std::optional<SessionId> session_id;
  std::string data;
};
} // namespace auction_engine
