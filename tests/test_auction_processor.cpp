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

    SECTION("The seller is logged in") {
      sessions.login(seller_session, seller);
      auto seller_event = process_auction(database, std::move(auction));

      REQUIRE(seller_event.session_id == seller_session);
      REQUIRE(seller_event.data == "Your item: item, has been sold for 100!");
      REQUIRE(accounts.get_funds(seller) == price);
      REQUIRE(accounts.get_items(buyer) == item);
    }

    SECTION("The seller is not logged in") {
      auto seller_event = process_auction(database, std::move(auction));
      REQUIRE(!seller_event.session_id.has_value());
      REQUIRE(accounts.get_funds(seller) == price);
      REQUIRE(accounts.get_items(buyer) == item);
    }
  }

  SECTION("Nobody bought an item") {
    Auction auction = {seller, {}, price, item, time};

    SECTION("The seller is logged in") {
      sessions.login(seller_session, seller);
      auto seller_event = process_auction(database, std::move(auction));

      REQUIRE(seller_event.session_id.value() == seller_session);
      REQUIRE(seller_event.data == "Your item: item, hasn't been sold!");
      REQUIRE(accounts.get_funds(seller) == 0);
      REQUIRE(accounts.get_items(seller) == item);
    }

    SECTION("The seller is not logged in") {
      auto seller_event = process_auction(database, std::move(auction));
      REQUIRE(!seller_event.session_id.has_value());
      REQUIRE(accounts.get_funds(seller) == 0);
      REQUIRE(accounts.get_items(seller) == item);
    }
  }
}