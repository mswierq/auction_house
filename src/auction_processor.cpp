//
// Created by mswiercz on 25.11.2021.
//
#include "auction_processor.h"
#include "database.h"

namespace auction_engine {
Notifications process_auction(Database &database, Auction &&auction) {
  std::optional<UserEvent> seller_event{};
  std::optional<UserEvent> buyer_event{};

  if (auction.buyer.has_value()) {
    auto &buyer = auction.buyer.value();

    database.accounts.deposit_item(buyer, auction.item);
    database.accounts.deposit_funds(auction.owner, auction.price); // TODO

    auto seller_session = database.sessions.get_session_id(auction.owner);
    if (seller_session.has_value()) {
      seller_event =
          UserEvent{std::move(auction.owner), seller_session.value(),
                    "Your item: " + auction.item + ", has been sold for " +
                        std::to_string(auction.price) + "!"};
    }

    auto buyer_session = database.sessions.get_session_id(buyer);
    if (buyer_session.has_value()) {
      buyer_event = UserEvent{std::move(buyer), buyer_session.value(),
                              "You have bought: " + auction.item + ", for " +
                                  std::to_string(auction.price) + "!"};
    }
  } else { // there is no buyer
    database.accounts.deposit_item(auction.owner, auction.item);

    auto seller_session = database.sessions.get_session_id(auction.owner);
    if (seller_session.has_value()) {
      seller_event =
          UserEvent{std::move(auction.owner), seller_session.value(),
                    "Your item: " + auction.item + ", hasn't been sold!"};
    }
  }

  return {seller_event, buyer_event};
}
} // namespace auction_engine