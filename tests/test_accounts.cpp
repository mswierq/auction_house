//
// Created by mswiercz on 21.11.2021.
//
#include "user_account.h"
#include <array>
#include <catch2/catch.hpp>
#include <future>

using namespace auction_engine;

TEST_CASE("Create a new user account", "[Accounts]") {
  Accounts accounts;
  std::string username{"username"};

  SECTION("by deposition of funds") {
    REQUIRE(accounts.deposit_funds(username, 10));
    REQUIRE(accounts.get_funds(username) == 10);
    REQUIRE(accounts.get_items(username).empty());
  }

  SECTION("by deposition of an item") {
    std::string item{"item"};
    accounts.deposit_item(username, item);
    REQUIRE(accounts.get_items(username) == item);
    REQUIRE(accounts.get_funds(username) == 0);
  }

  SECTION("by deposition of multiple items") {
    std::array items{"item", "item2", "item3"};
    for (auto &item : items) {
      accounts.deposit_item(username, item);
    }
    REQUIRE(accounts.get_items(username) ==
            std::accumulate(items.cbegin(), items.cend(), std::string{},
                            [](std::string &a, const std::string &b) {
                              if (!a.empty()) {
                                a.append("\n");
                              }
                              return a.append(b);
                            }));
    REQUIRE(accounts.get_funds(username) == 0);
  }

  SECTION("by withdrawing funds") {
    REQUIRE(accounts.withdraw_funds(username, 10) == false);
    REQUIRE(accounts.get_funds(username) == 0);
    REQUIRE(accounts.get_items(username).empty());
  }

  SECTION("by withdrawing item that doesn't exist") {
    REQUIRE(accounts.withdraw_item(username, "item") == false);
    REQUIRE(accounts.get_funds(username) == 0);
    REQUIRE(accounts.get_items(username).empty());
  }

  SECTION("by getting funds") {
    REQUIRE(accounts.get_funds(username) == 0);
    REQUIRE(accounts.get_items(username).empty());
  }

  SECTION("by getting items") {
    REQUIRE(accounts.get_items(username).empty());
    REQUIRE(accounts.get_funds(username) == 0);
  }
}

TEST_CASE("Operate on multiple users accounts", "[Accounts]") {
  Accounts accounts;
  std::vector users{"user_0", "user_1", "user_2"};

  for (auto &user : users) {
    REQUIRE(accounts.get_funds(user) == 0);
    REQUIRE(accounts.get_items(user).empty());
  }

  SECTION("Add a new account") {
    auto new_user{"new_user"};
    REQUIRE(accounts.get_funds(new_user) == 0);
    REQUIRE(accounts.get_items(new_user).empty());

    users.push_back(new_user);
    for (auto &user : users) {
      REQUIRE(accounts.get_funds(user) == 0);
      REQUIRE(accounts.get_items(user).empty());
    }
  }

  SECTION("Operate on multiple accounts in parallel") {
    std::vector<std::future<std::pair<FundsType, std::string>>> futures;
    futures.push_back(std::async(std::launch::async, [&accounts]() {
      auto user = "user_0";
      accounts.deposit_item(user, "item");
      accounts.deposit_funds(user, 10);
      accounts.deposit_item(user, "item_2");
      accounts.withdraw_funds(user, 5);
      accounts.withdraw_funds(user, 7);
      return std::make_pair(accounts.get_funds(user), accounts.get_items(user));
    }));
    futures.push_back(std::async(std::launch::async, [&accounts]() {
      auto user = "user_1";
      accounts.deposit_funds(user, 20);
      accounts.deposit_item(user, "item");
      accounts.deposit_item(user, "item_2");
      accounts.withdraw_funds(user, 7);
      accounts.withdraw_item(user, "item_2");
      return std::make_pair(accounts.get_funds(user), accounts.get_items(user));
    }));
    futures.push_back(std::async(std::launch::async, [&accounts]() {
      auto user = "user_2";
      accounts.deposit_item(user, "item");
      accounts.deposit_item(user, "item_3");
      accounts.withdraw_funds(user, 7);
      accounts.deposit_item(user, "item_2");
      return std::make_pair(accounts.get_funds(user), accounts.get_items(user));
    }));
    futures.push_back(std::async(std::launch::async, [&accounts]() {
      auto user = "new_user";
      accounts.deposit_funds(user, 30);
      accounts.deposit_item(user, "item");
      accounts.deposit_item(user, "item_2");
      accounts.deposit_item(user, "item_2");
      accounts.withdraw_item(user, "item_2");
      return std::make_pair(accounts.get_funds(user), accounts.get_items(user));
    }));

    std::vector expected = {
        std::make_tuple("user_0", 5, "item\nitem_2"),
        std::make_tuple("user_1", 13, "item"),
        std::make_tuple("user_2", 0, "item\nitem_3\nitem_2"),
        std::make_tuple("new_user", 30, "item\nitem_2"),
    };

    for (auto &f : futures) {
      f.wait();
    }

    for (auto i = 0; i < futures.size(); ++i) {
      auto &[user, exp_funds, exp_items] = expected[i];
      auto [funds, items] = futures[i].get();
      REQUIRE(exp_funds == funds);
      REQUIRE(exp_items == items);
      // Double check after all parallel operations are done
      REQUIRE(accounts.get_funds(user) == funds);
      REQUIRE(accounts.get_items(user) == items);
    }
  }
}

TEST_CASE("Error cases", "[Accounts]") {
  Accounts accounts;
  auto user{"user"};
  accounts.deposit_funds(user, 10);
  SECTION("Overflow funds during deposit") {
    REQUIRE(
        !accounts.deposit_funds(user, std::numeric_limits<FundsType>::max()));
    REQUIRE(accounts.get_funds(user) == 10);
  }
}