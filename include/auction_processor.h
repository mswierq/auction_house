//
// Created by mswiercz on 25.11.2021.
//
#include "events.h"

namespace auction_house::engine {
class Database;
class Auction;

// Consumes the expired auctions and processes them
EgressEvent process_auction(Database &database, Auction &&auction);

} // namespace auction_house::engine
