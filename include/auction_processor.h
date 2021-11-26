//
// Created by mswiercz on 25.11.2021.
//
#include "events.h"
#include <optional>

namespace auction_engine {
class Database;
class Auction;

// Consumes the expired auctions and processes them
EgressEvent process_auction(Database &database, Auction &&auction);

} // namespace auction_engine
