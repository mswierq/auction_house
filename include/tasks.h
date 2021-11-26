//
// Created by mswiercz on 24.11.2021.
//
#pragma once
#include "events.h"
#include <future>

namespace auction_engine {
class Database;
class Auction;

using Task = std::future<EgressEvent>;

// Consumes a user event and returns a task that process it
Task create_command_task(IngressEvent &&event, Database &database);

// Consumes an expired auction and returns a task that process it
Task create_auction_task(Auction &&auction, Database &database);
} // namespace auction_engine