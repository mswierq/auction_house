//
// Created by mswiercz on 24.11.2021.
//
#include "tasks.h"
#include "database.h"
#include "events.h"
#include <future>

namespace auction_engine {
Task create_command_task(UserEvent &&event, Database& database) {
  return std::async([]() { return UserEvent{{}, {}, ""}; });
}
} // namespace auction_engine