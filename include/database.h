//
#pragma once
#include "user_account.h"
#include "auctions.h"
#include "session.h"

namespace auction_house::engine {
//Simple struct to keep all references to data structs together
struct Database {
  Accounts& accounts;
  AuctionList& auctions;
  SessionManager& sessions;
};
}
