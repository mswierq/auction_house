//
// Created by mswiercz on 25.11.2021.
//
#include "auction_processor.h"
#include "database.h"
#include <catch2/catch.hpp>

using namespace auction_engine;

TEST_CASE("Test processing expired auctions", "[AuctionProcessor]") {
  SessionManager sessions;
  AuctionList auctions;
  Accounts accounts;
  Database database{accounts, auctions, sessions};

  auto seller = "seller";
  auto buyer = "buyer";
  FundsType price = 100;
  auto item = "item";
  auto time = Clock::now();

  SessionId seller_session = 1;
  SessionId buyer_session = 2;
  sessions.start_session(seller_session, 1);
  sessions.start_session(buyer_session, 2);

  SECTION("An item has been bought") {
    Auction auction = {seller, buyer, price, item, time};
    SECTION("The seller and the buyer are logged in") {
      sessions.login(seller_session, seller);
      sessions.login(buyer_session, buyer);
      auto [seller_event, buyer_event] =
          process_auction(database, std::move(auction));

      REQUIRE(seller_event.has_value());
      REQUIRE(seller_event.value().username.value() == seller);
      REQUIRE(seller_event.value().session_id == seller_session);
      REQUIRE(seller_event.value().data == "Your item: item, has been sold for 100!");
      REQUIRE(accounts.get_funds(seller) == price);

      REQUIRE(buyer_event.has_value());
      REQUIRE(buyer_event.value().username.value() == buyer);
      REQUIRE(buyer_event.value().session_id == buyer_session);
      REQUIRE(buyer_event.value().data == "You have bought: item, for 100!");
      REQUIRE(accounts.get_items(buyer) == item);
    }

    SECTION("The seller is logged in") {
      sessions.login(seller_session, seller);
      auto [seller_event, buyer_event] =
          process_auction(database, std::move(auction));

      REQUIRE(seller_event.has_value());
      REQUIRE(seller_event.value().username.value() == seller);
      REQUIRE(seller_event.value().session_id == seller_session);
      REQUIRE(seller_event.value().data == "Your item: item, has been sold for 100!");
      REQUIRE(accounts.get_funds(seller) == price);

      REQUIRE(!buyer_event.has_value());
      REQUIRE(accounts.get_items(buyer) == item);
    }

    SECTION("The buyer is logged in") {
      sessions.login(buyer_session, buyer);
      auto [seller_event, buyer_event] =
          process_auction(database, std::move(auction));

      REQUIRE(!seller_event.has_value());
      REQUIRE(accounts.get_funds(seller) == price);

      REQUIRE(buyer_event.has_value());
      REQUIRE(buyer_event.value().username.value() == buyer);
      REQUIRE(buyer_event.value().session_id == buyer_session);
      REQUIRE(buyer_event.value().data == "You have bought: item, for 100!");
      REQUIRE(accounts.get_items(buyer) == item);
    }

    SECTION("Neither seller nor buyer are logged in") {
      auto [seller_event, buyer_event] =
          process_auction(database, std::move(auction));
      REQUIRE(!seller_event.has_value());
      REQUIRE(accounts.get_funds(seller) == price);

      REQUIRE(!buyer_event.has_value());
      REQUIRE(accounts.get_items(buyer) == item);
    }
  }

  SECTION("Nobody bought an item") {
    Auction auction = {seller, {}, price, item, time};
    SECTION("The seller and the buyer are logged in") {
      sessions.login(seller_session, seller);
      sessions.login(buyer_session, buyer);
      auto [seller_event, buyer_event] =
          process_auction(database, std::move(auction));

      REQUIRE(seller_event.has_value());
      REQUIRE(seller_event.value().username.value() == seller);
      REQUIRE(seller_event.value().session_id == seller_session);
      REQUIRE(seller_event.value().data == "Your item: item, hasn't been sold!");
      REQUIRE(accounts.get_funds(seller) == 0);
      REQUIRE(accounts.get_items(seller) == item);

      REQUIRE(!buyer_event.has_value());
    }

    SECTION("The seller is logged in") {
      sessions.login(seller_session, seller);
      auto [seller_event, buyer_event] =
          process_auction(database, std::move(auction));

      REQUIRE(seller_event.has_value());
      REQUIRE(seller_event.value().username.value() == seller);
      REQUIRE(seller_event.value().session_id == seller_session);
      REQUIRE(seller_event.value().data == "Your item: item, hasn't been sold!");
      REQUIRE(accounts.get_funds(seller) == 0);

      REQUIRE(!buyer_event.has_value());
    }

    SECTION("The buyer is logged in") {
      sessions.login(buyer_session, buyer);
      auto [seller_event, buyer_event] =
          process_auction(database, std::move(auction));

      REQUIRE(!seller_event.has_value());
      REQUIRE(accounts.get_funds(seller) == 0);

      REQUIRE(!buyer_event.has_value());
    }

    SECTION("Neither seller nor buyer are logged in") {
      auto [seller_event, buyer_event] =
          process_auction(database, std::move(auction));
      REQUIRE(!seller_event.has_value());
      REQUIRE(accounts.get_funds(seller) == 0);

      REQUIRE(!buyer_event.has_value());
    }
  }
}