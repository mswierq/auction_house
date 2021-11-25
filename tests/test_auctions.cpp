//
// Created by mswiercz on 22.11.2021.
//
#include "auctions.h"
#include <catch2/catch.hpp>
#include <thread>

using namespace auction_engine;
using Catch::Matchers::UnorderedEquals;

auto auction_equal = [](const Auction &a, const Auction &b) {
  return a.buyer == b.buyer && a.price == b.price && a.owner == b.owner &&
         a.item == b.item && a.expiration_time == b.expiration_time;
};

TEST_CASE("Single thread Auction lists manipulations", "[Auctions]") {
  auto time_zero = Clock::now();

  AuctionList auctions;
  REQUIRE(auctions.add_auction(
      {"owner", {}, 100, "item", time_zero + std::chrono::milliseconds(100)}));
  REQUIRE(auctions.add_auction({"owner_2",
                                {},
                                200,
                                "item_2",
                                time_zero + std::chrono::milliseconds(100)}));
  REQUIRE(auctions.add_auction(
      {"owner_3", {}, 200, "item_3", time_zero + std::chrono::hours(24)}));

  SECTION("Try to outbid") {
    auto result_0 = auctions.bid_item(0, 120, "new_buyer");
    auto result_1 = auctions.bid_item(1, 100, "new_buyer");
    auto result_2 = auctions.bid_item(1, 100, "owner_2");
    auto result_3 = auctions.bid_item(100, 100, "owner_2");

    REQUIRE(result_0 == BidResult::Successful);
    REQUIRE(result_1 == BidResult::TooLowPrice);
    REQUIRE(result_2 == BidResult::OwnerBid);
    REQUIRE(result_3 == BidResult::DoesNotExist);
  }

  SECTION("Collect a list of expired auctions") {
    auto start_at = time_zero + std::chrono::milliseconds(200);

    auctions.wait_for_expired();
    auto expired = auctions.collect_expired();

    REQUIRE(expired.size() == 2);

    std::vector expected = {
        Auction{"owner",
                {},
                100,
                "item",
                time_zero + std::chrono::milliseconds(100)},
        Auction{"owner_2",
                {},
                200,
                "item_2",
                time_zero + std::chrono::milliseconds(100)},
    };

    REQUIRE(std::is_permutation(expired.begin(), expired.end(),
                                expected.begin(), auction_equal));
  }

  SECTION("Bid and then collect expired auctions") {
    auctions.bid_item(7, 500, "new_buyer");

    auto start_at = time_zero + std::chrono::milliseconds(200);

    auctions.wait_for_expired();
    auto expired = auctions.collect_expired();

    REQUIRE(expired.size() == 2);

    std::vector expected = {
        Auction{"owner",
                {},
                100,
                "item",
                time_zero + std::chrono::milliseconds(100)},
        Auction{"owner_2", "new_buyer", 500, "item_2",
                time_zero + std::chrono::milliseconds(100)},
    };

    REQUIRE(std::is_permutation(expired.begin(), expired.end(),
                                expected.begin(), auction_equal));
  }

  SECTION("Print the current auctions list") {
    auctions.bid_item(11, 500, "new_buyer");

    REQUIRE_THAT(
        auctions.get_printable_list(),
        UnorderedEquals<std::string>(
            {{"ID: 9; ITEM: item; OWNER: owner; PRICE: 100; BUYER: "},
             {"ID: 10; ITEM: item_2; OWNER: owner_2; PRICE: 200; BUYER: "},
             {"ID: 11; ITEM: item_3; OWNER: owner_3; PRICE: 500; BUYER: "
              "new_buyer"}}));
  }

  SECTION("Add new auctions, bid, collect expired, then print the current "
          "auctions list") {
    auto start_at = time_zero + std::chrono::milliseconds(200);

    auctions.bid_item(14, 500, "new_buyer");

    REQUIRE_THAT(
        auctions.get_printable_list(),
        UnorderedEquals<std::string>(
            {{"ID: 12; ITEM: item; OWNER: owner; PRICE: 100; BUYER: "},
             {"ID: 13; ITEM: item_2; OWNER: owner_2; PRICE: 200; BUYER: "},
             {"ID: 14; ITEM: item_3; OWNER: owner_3; PRICE: 500; BUYER: "
              "new_buyer"}}));

    auctions.wait_for_expired();
    auctions.collect_expired();
    REQUIRE_THAT(
        auctions.get_printable_list(),
        UnorderedEquals<std::string>(
            {{"ID: 14; ITEM: item_3; OWNER: owner_3; PRICE: 500; BUYER: "
              "new_buyer"}}));

    auctions.add_auction({"owner_4",
                          {},
                          400,
                          "pretty_item",
                          time_zero - std::chrono::milliseconds(100)});
    REQUIRE_THAT(
        auctions.get_printable_list(),
        UnorderedEquals<std::string>(
            {{"ID: 14; ITEM: item_3; OWNER: owner_3; PRICE: 500; BUYER: "
              "new_buyer"},
             {"ID: 15; ITEM: pretty_item; OWNER: owner_4; PRICE: 400; "
              "BUYER: "}}));

    auctions.wait_for_expired();
    auto expired = auctions.collect_expired();
    REQUIRE(expired.size() == 1);

    auto it = expired.begin();
    REQUIRE(it != expired.end());
    REQUIRE(!it->buyer.has_value());
    REQUIRE(it->owner == "owner_4");
    REQUIRE(it->price == 400);
    REQUIRE(it->item == "pretty_item");
    REQUIRE(it->expiration_time == time_zero - std::chrono::milliseconds(100));

    REQUIRE_THAT(
        auctions.get_printable_list(),
        UnorderedEquals<std::string>(
            {{"ID: 14; ITEM: item_3; OWNER: owner_3; PRICE: 500; BUYER: "
              "new_buyer"}}));
  }
}

TEST_CASE("Multi-thread auction lists manipulations", "[Auctions]") {
  AuctionList auctions;

  auto t = std::thread([&auctions]() {
    auctions.wait_for_expired();
    auctions.collect_expired();
  });

  auto time_zero = Clock::now();

  REQUIRE(auctions.add_auction(
      {"owner", {}, 100, "item", time_zero + std::chrono::milliseconds(100)}));
  REQUIRE(auctions.add_auction({"owner_2",
                                {},
                                200,
                                "item_2",
                                time_zero + std::chrono::milliseconds(100)}));

  t.join();

  REQUIRE(auctions.get_printable_list().empty());
}