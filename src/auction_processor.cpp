//
// Created by mswiercz on 25.11.2021.
//
#include "auction_processor.h"
#include "database.h"

namespace auction_engine {

EgressEvent process_auction(Database &database, Auction &&auction) {
  auto seller_session = database.sessions.get_session_id(auction.owner);

  if (auction.buyer.has_value()) {
    database.accounts.deposit_item(auction.buyer.value(), auction.item);
    database.accounts.deposit_funds(auction.owner, auction.price);

    return EgressEvent{seller_session, "Your item: " + auction.item +
                                           ", has been sold for " +
                                           std::to_string(auction.price) + "!"};
  } else { // there is no buyer
    database.accounts.deposit_item(auction.owner, auction.item);

    return EgressEvent{seller_session,
                       "Your item: " + auction.item + ", hasn't been sold!"};
  };
}

} // namespace auction_engine