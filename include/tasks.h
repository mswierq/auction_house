//
// Created by mswiercz on 24.11.2021.
//
#pragma once
#include <future>

namespace auction_engine {
class Database;
class UserEvent;

using Task = std::future<UserEvent>;

// Consumes an event and returns a task that process it
Task create_command_task(UserEvent &&event, Database& database);
} // namespace auction_engine