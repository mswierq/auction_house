//
// Created by mswiercz on 25.11.2021.
//
#include "user_event.h"

namespace auction_engine {
class Database;
class Auction;

using Notifications =
    std::pair<std::optional<UserEvent>, std::optional<UserEvent>>;

// Consumes the expired auctions and processes them
Notifications process_auction(Database &database, Auction &&auction);

} // namespace auction_engine
