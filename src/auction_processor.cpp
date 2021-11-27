//
// Created by mswiercz on 25.11.2021.
//
#include "auction_processor.h"
#include "database.h"

namespace auction_house::engine {

EgressEvent process_auction(Database &database, Auction &&auction) {
  auto seller_session = database.sessions.get_session_id(auction.owner);

  if (auction.buyer.has_value()) {
    auto &buyer = auction.buyer.value();

    // Buyer doesn't have funds to pay
    if (!database.accounts.withdraw_funds(buyer, auction.price)) {
      database.accounts.deposit_item(auction.owner, auction.item);
      return {seller_session, "Your item: " + auction.item +
                                  ", hasn't been sold! The " + buyer +
                                  " couldn't pay for it!"};
    }

    // Seller hasn't accepted the payment
    if (!database.accounts.deposit_funds(auction.owner, auction.price)) {
      // doesn't handle the result there has to be capacity for the just
      // withdrawn funds, the tasks from all users and auctions are being
      // processed in FIFO order
      if (!database.accounts.deposit_funds(buyer, auction.price)) {
        // TODO error log here
      }
      database.accounts.deposit_item(auction.owner, auction.item);
      return {seller_session,
              "Your item: " + auction.item +
                  ", hasn't been sold! You didn't accept the payment from " +
                  buyer + "!"};
    }

    // Successfully conclude the auction
    database.accounts.deposit_item(auction.buyer.value(), auction.item);
    return {seller_session,
            "Your item: " + auction.item + ", has been sold for " +
                std::to_string(auction.price) + " by " + buyer + "!"};
  } else { // there is no buyer
    database.accounts.deposit_item(auction.owner, auction.item);

    return {seller_session,
            "Your item: " + auction.item + ", hasn't been sold!"};
  };
}

} // namespace auction_house::engine