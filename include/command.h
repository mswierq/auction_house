//
// Created by mswiercz on 24.11.2021.
//
#pragma once
#include "session_id.h"
#include "tasks.h"
#include "user_event.h"
#include <memory>
#include <optional>
#include <string>

namespace auction_engine {
class Command;
using CommandPtr = std::unique_ptr<Command>;

class Database;

class Command {
public:
  Command(UserEvent&& event): _event(std::move(event)) {}

  // Consume the event and return a command executor
  static CommandPtr parse(UserEvent &&event);

  virtual UserEvent execute(Database &database) = 0;

  virtual ~Command() = default;

protected:
  UserEvent _event;
};
} // namespace auction_engine
