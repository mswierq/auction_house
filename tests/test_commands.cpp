//
// Created by mswiercz on 24.11.2021.
//
#include "command.h"
#include "database.h"
#include <catch2/catch.hpp>

using namespace auction_engine;

TEST_CASE("Test execution of a command", "[Commands]") {
  Accounts accounts;
  AuctionList auctions;
  SessionManager sessions;
  Database database{accounts, auctions, sessions};

  const SessionId session_id = 1;
  const ConnectionId connection_id = 1;
  sessions.start_session(session_id, connection_id);
  UserEvent event{{}, session_id, ""};

  SECTION("Login and logout") {
    SECTION("Successfully login a user") {
      event.data = "LOGIN username";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(egress_event.username.value() == "username");
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data == "Welcome username!");
      REQUIRE(sessions.get_username(session_id).value() == "username");
      REQUIRE(sessions.get_session_id("username").value() == session_id);
    }

    SECTION("Fail at login a user") {
      sessions.start_session(2, 2);
      sessions.login(2, "username");
      event.data = "LOGIN username";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(!egress_event.username.has_value());
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data == "Couldn't login as username!");
      REQUIRE(!sessions.get_username(session_id).has_value());
      REQUIRE(sessions.get_session_id("username").value() == 2);
    }

    SECTION("Successfully logout a user") {
      sessions.login(session_id, "username");
      event.username = "username";
      event.data = "LOGOUT";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(egress_event.username.value() == "username");
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data == "Good bay, username!");
      REQUIRE(!sessions.get_username(session_id).has_value());
      REQUIRE(!sessions.get_session_id("username").has_value());
    }

    SECTION("Fail at logging out a user") {
      event.data = "LOGOUT";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(!egress_event.username.has_value());
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data == "You are not logged in!");
      REQUIRE(!sessions.get_username(session_id).has_value());
    }
  }

  SECTION("Fail at parsing an unknown command") {
    event.data = "LOGOUT 1 ? whatIs it!!";
    auto egress_event = Command::parse(std::move(event))->execute(database);
    REQUIRE(!egress_event.username.has_value());
    REQUIRE(egress_event.session_id.value() == session_id);
    REQUIRE(egress_event.data == "WRONG COMMAND: LOGOUT 1 ? whatIs it!!");
  }

  SECTION("Funds deposits") {
    SECTION("Successfully deposit funds") {
      sessions.login(session_id, "username");
      event.username = "username";
      event.data = "DEPOSIT FUNDS 100";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(egress_event.username.value() == "username");
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data == "Successful deposition of funds: 100!");
      REQUIRE(accounts.get_funds("username") == 100);
    }

    SECTION("Try to deposit funds, without being logged in") {
      event.data = "DEPOSIT FUNDS 100";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(!egress_event.username.has_value());
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data ==
              "Deposition of funds has failed! Are you logged in?");
      REQUIRE(accounts.get_funds("username") == 0);
    }

    SECTION("Try to deposit invalid amount") {
      sessions.login(session_id, "username");
      event.username = "username";
      event.data = "DEPOSIT FUNDS invalid100";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(egress_event.username.value() == "username");
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data == "WRONG COMMAND: DEPOSIT FUNDS invalid100");
      REQUIRE(accounts.get_funds("username") == 0);
    }

    SECTION("Try to deposit too much") {
      sessions.login(session_id, "username");
      event.username = "username";
      event.data = "DEPOSIT FUNDS "
                   "1000000000000000000000000000000000000000000000000000000000000"
                   "000000000000000000000";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(egress_event.username.value() == "username");
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data ==
              "Deposition of funds has failed! Invalid amount!");
      REQUIRE(accounts.get_funds("username") == 0);
    }
  }

  SECTION("Items deposits") {
    SECTION("Successfully deposit an item") {
      sessions.login(session_id, "username");
      event.username = "username";
      event.data = "DEPOSIT ITEM my_pretty_item";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(egress_event.username.value() == "username");
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data == "Successful deposition of item: my_pretty_item!");
      REQUIRE(accounts.get_items("username") == "my_pretty_item");
    }

    SECTION("Try to deposit an item, without being logged in") {
      event.data = "DEPOSIT ITEM my_pretty_item";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(!egress_event.username.has_value());
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data ==
              "Deposition of an item has failed! Are you logged in?");
      REQUIRE(accounts.get_items("username").empty());
    }
  }

  SECTION("Funds withdrawals") {
    SECTION("Successful withdrawal of funds") {
      sessions.login(session_id, "username");
      accounts.deposit_funds("username", 1000);
      event.username = "username";
      event.data = "WITHDRAW FUNDS 100";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(egress_event.username.value() == "username");
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data ==
              "Successfully withdrawn: 100!");
      REQUIRE(accounts.get_funds("username") == 900);
    }

    SECTION("Try to withdraw funds, without being logged in") {
      event.data = "WITHDRAW FUNDS 100";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(!egress_event.username.has_value());
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data ==
              "Withdrawal of funds has failed! Are you logged in?");
      REQUIRE(accounts.get_funds("username") == 0);
    }

    SECTION("Try to withdraw invalid amount") {
      sessions.login(session_id, "username");
      accounts.deposit_funds("username", 1000);
      event.username = "username";
      event.data = "WITHDRAW FUNDS invalid100";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(egress_event.username.value() == "username");
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data == "WRONG COMMAND: WITHDRAW FUNDS invalid100");
      REQUIRE(accounts.get_funds("username") == 1000);
    }

    SECTION("Try to withdraw too much - invalid amount") {
      sessions.login(session_id, "username");
      accounts.deposit_funds("username", 1000);
      event.username = "username";
      event.data = "WITHDRAW FUNDS "
                   "1000000000000000000000000000000000000000000000000000000000000"
                   "000000000000000000000";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(egress_event.username.value() == "username");
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data ==
              "Withdrawal of funds has failed! Invalid amount!");
      REQUIRE(accounts.get_funds("username") == 1000);
    }

    SECTION("Try to withdraw too much - insufficient funds") {
      sessions.login(session_id, "username");
      accounts.deposit_funds("username", 1000);
      event.username = "username";
      event.data = "WITHDRAW FUNDS "
                   "2000";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(egress_event.username.value() == "username");
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data ==
              "Withdrawal of funds has failed! Insufficient funds!");
      REQUIRE(accounts.get_funds("username") == 1000);
    }
  }

  SECTION("Items withdrawals") {
    SECTION("Successfully withdraw an item") {
      sessions.login(session_id, "username");
      accounts.deposit_item("username", "my_pretty_item");
      accounts.deposit_item("username", "my_ugly_item");
      event.username = "username";
      event.data = "WITHDRAW ITEM my_ugly_item";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(egress_event.username.value() == "username");
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data == "Successfully withdrawn item: my_ugly_item!");
      REQUIRE(accounts.get_items("username") == "my_pretty_item");
    }

    SECTION("Try to withdraw an item, without being logged in") {
      event.data = "WITHDRAW ITEM my_pretty_item";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(!egress_event.username.has_value());
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data ==
              "Withdrawal of an item has failed! Are you logged in?");
      REQUIRE(accounts.get_items("username").empty());
    }

    SECTION("Try to withdraw an item that doesn't exist") {
      sessions.login(session_id, "username");
      accounts.deposit_item("username", "my_pretty_item");
      event.data = "DEPOSIT ITEM my_ugly_item";
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(!egress_event.username.has_value());
      REQUIRE(egress_event.session_id.value() == session_id);
      REQUIRE(egress_event.data ==
              "Deposition of an item has failed! Are you logged in?");
      REQUIRE(accounts.get_items("username") == "my_pretty_item");
    }
  }
}