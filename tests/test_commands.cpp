//
// Created by mswiercz on 24.11.2021.
//
#include "command.h"
#include "database.h"
#include <catch2/catch.hpp>

using namespace auction_engine;
using Catch::Matchers::UnorderedEquals;

TEST_CASE("Test execution of user account related commands", "[Commands]") {
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
      event.data =
          "DEPOSIT FUNDS "
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
      REQUIRE(egress_event.data ==
              "Successful deposition of item: my_pretty_item!");
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
      REQUIRE(egress_event.data == "Successfully withdrawn: 100!");
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
      event.data =
          "WITHDRAW FUNDS "
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
      REQUIRE(egress_event.data ==
              "Successfully withdrawn item: my_ugly_item!");
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

TEST_CASE("Test execution of auction commands", "[Commands]") {
  Accounts accounts;
  AuctionList auctions;
  SessionManager sessions;
  Database database{accounts, auctions, sessions};

  const SessionId user_0_sess_id = 1;
  const ConnectionId user_0_conn_id = 1;
  const SessionId user_1_sess_id = 2;
  const ConnectionId user_1_conn_id = 2;
  const auto username_0 = "username_0";
  const auto username_1 = "username_1";
  REQUIRE(sessions.start_session(user_0_sess_id, user_0_sess_id));
  REQUIRE(sessions.start_session(user_1_sess_id, user_1_sess_id));
  REQUIRE(sessions.login(user_0_sess_id, username_0));
  REQUIRE(sessions.login(user_1_sess_id, username_1));
  REQUIRE(accounts.deposit_funds(username_0, 1000));
  REQUIRE(accounts.deposit_funds(username_1, 1000));
  accounts.deposit_item(username_0, "item_0");
  accounts.deposit_item(username_0, "item_1");
  accounts.deposit_item(username_0, "item_0");
  accounts.deposit_item(username_1, "item_2");
  accounts.deposit_item(username_1, "item_3");

  UserEvent event_0 = {username_0, user_0_sess_id, ""};
  UserEvent event_1 = {username_1, user_1_sess_id, ""};

  SECTION("Successfully put an item into sale - default expiration time") {
    event_0.data = "SELL item_0 100";
    auto egress_event = Command::parse(std::move(event_0))->execute(database);
    REQUIRE(egress_event.username.value() == username_0);
    REQUIRE(egress_event.session_id.value() == user_0_sess_id);
    REQUIRE(egress_event.data == "Your item item_0 is being auctioned off!");
    REQUIRE(accounts.get_items(username_0) == "item_1\nitem_0");
    REQUIRE(accounts.get_funds(username_0) == 999); // charge for selling an
                                                    // item
    REQUIRE_THAT(
        auctions.get_printable_list(),
        UnorderedEquals<std::string>(
            {{"ID: 0; ITEM: item_0; OWNER: username_0; PRICE: 100; BUYER: "}}));
  }

  SECTION("Successfully put an item into sale") {
    event_0.data = "SELL item_0 100 1";
    auto egress_event = Command::parse(std::move(event_0))->execute(database);
    REQUIRE(egress_event.username.value() == username_0);
    REQUIRE(egress_event.session_id.value() == user_0_sess_id);
    REQUIRE(egress_event.data == "Your item item_0 is being auctioned off!");
    REQUIRE(accounts.get_funds(username_0) == 999); // charge for selling an
                                                    // item
    REQUIRE_THAT(
        auctions.get_printable_list(),
        UnorderedEquals<std::string>(
            {{"ID: 0; ITEM: item_0; OWNER: username_0; PRICE: 100; BUYER: "}}));

    SECTION("Then successfully bid it by other user") {
      event_1.data = "BID 0 200";
      auto egress_event = Command::parse(std::move(event_1))->execute(database);
      REQUIRE(egress_event.username.value() == username_1);
      REQUIRE(egress_event.session_id.value() == user_1_sess_id);
      REQUIRE(egress_event.data == "You are winning the auction 0!");
      REQUIRE(accounts.get_funds(username_1) == 800);
      REQUIRE_THAT(auctions.get_printable_list(),
                   UnorderedEquals<std::string>(
                       {{std::string("ID: 0; ITEM: item_0; OWNER: username_0; "
                                     "PRICE: 200; BUYER: ") +
                         username_1}}));
    }

    SECTION("Then fail the bid due too low offer") {
      event_1.data = "BID 0 100";
      auto egress_event = Command::parse(std::move(event_1))->execute(database);
      REQUIRE(egress_event.username.value() == username_1);
      REQUIRE(egress_event.session_id.value() == user_1_sess_id);
      REQUIRE(egress_event.data == "Your offer for the auction 0 was too low!");
      REQUIRE(accounts.get_funds(username_1) == 1000);
      REQUIRE_THAT(
          auctions.get_printable_list(),
          UnorderedEquals<std::string>({{"ID: 0; ITEM: item_0; OWNER: "
                                         "username_0; PRICE: 100; BUYER: "}}));
    }

    SECTION("Then fail the bid due to lack of an auction") {
      event_1.data = "BID 1 200";
      auto egress_event = Command::parse(std::move(event_1))->execute(database);
      REQUIRE(egress_event.username.value() == username_1);
      REQUIRE(egress_event.session_id.value() == user_1_sess_id);
      REQUIRE(egress_event.data == "There is no such auction!");
      REQUIRE(accounts.get_funds(username_1) == 1000);
      REQUIRE_THAT(
          auctions.get_printable_list(),
          UnorderedEquals<std::string>({{"ID: 0; ITEM: item_0; OWNER: "
                                         "username_0; PRICE: 100; BUYER: "}}));
    }

    SECTION("Then fail the bid due to lack of an auction - auction id auto of "
            "bounds") {
      event_1.data = "BID "
                     "100000000000000000000000000000000000000000000000000000000"
                     "000000000000000000000000000000000000000000000000000000000"
                     "0000000000000000000000000 200";
      auto egress_event = Command::parse(std::move(event_1))->execute(database);
      REQUIRE(egress_event.username.value() == username_1);
      REQUIRE(egress_event.session_id.value() == user_1_sess_id);
      REQUIRE(egress_event.data == "The bid arguments are invalid!");
      REQUIRE(accounts.get_funds(username_1) == 1000);
      REQUIRE_THAT(
          auctions.get_printable_list(),
          UnorderedEquals<std::string>({{"ID: 0; ITEM: item_0; OWNER: "
                                         "username_0; PRICE: 100; BUYER: "}}));
    }

    SECTION("Then fail the bid due to lack of an auction - price auto of "
            "bounds") {
      event_1.data =
          "BID 0 "
          "10000000000000000000000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000000000000000000000"
          "0000000000000000000000000";
      auto egress_event = Command::parse(std::move(event_1))->execute(database);
      REQUIRE(egress_event.username.value() == username_1);
      REQUIRE(egress_event.session_id.value() == user_1_sess_id);
      REQUIRE(egress_event.data == "The bid arguments are invalid!");
      REQUIRE(accounts.get_funds(username_1) == 1000);
      REQUIRE_THAT(
          auctions.get_printable_list(),
          UnorderedEquals<std::string>({{"ID: 0; ITEM: item_0; OWNER: "
                                         "username_0; PRICE: 100; BUYER: "}}));
    }

    SECTION("Try to bid an item when not being logged in") {
      event_1.username = {};
      event_1.data = "BID 0 10000";
      auto egress_event = Command::parse(std::move(event_1))->execute(database);
      REQUIRE(!egress_event.username.has_value());
      REQUIRE(egress_event.session_id.value() == user_1_sess_id);
      REQUIRE(egress_event.data ==
              "Bidding an item has failed! Are you logged in?");
      REQUIRE(accounts.get_funds(username_1) == 1000);
      REQUIRE_THAT(
          auctions.get_printable_list(),
          UnorderedEquals<std::string>({{"ID: 0; ITEM: item_0; OWNER: "
                                         "username_0; PRICE: 100; BUYER: "}}));
    }

    SECTION("Try to bid as a seller of the item") {
      UserEvent event{username_0, user_0_sess_id, "BID 0 200"};
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(egress_event.username.value() == username_0);
      REQUIRE(egress_event.session_id.value() == user_0_sess_id);
      REQUIRE(egress_event.data ==
              "You can't bid on the auction 0, you are the seller!");
      REQUIRE(accounts.get_funds(username_0) == 999);
      REQUIRE_THAT(
          auctions.get_printable_list(),
          UnorderedEquals<std::string>({{"ID: 0; ITEM: item_0; OWNER: "
                                         "username_0; PRICE: 100; BUYER: "}}));
    }

    SECTION("Try to bid without enough funds") {
      UserEvent event{username_1, user_1_sess_id, "BID 0 2000"};
      auto egress_event = Command::parse(std::move(event))->execute(database);
      REQUIRE(egress_event.username.value() == username_1);
      REQUIRE(egress_event.session_id.value() == user_1_sess_id);
      REQUIRE(egress_event.data ==
              "You can't bid on the auction 0, you don't have enough funds!");
      REQUIRE(accounts.get_funds(username_1) == 1000);
      REQUIRE_THAT(
          auctions.get_printable_list(),
          UnorderedEquals<std::string>({{"ID: 0; ITEM: item_0; OWNER: "
                                         "username_0; PRICE: 100; BUYER: "}}));
    }
  }

  SECTION("Fail at putting an item into sale - no sufficient funds to pay the "
          "fee") {
    REQUIRE(accounts.withdraw_funds(username_0, 1000)); // set balance to 0
    event_0.data = "SELL item_0 100 1";
    auto egress_event = Command::parse(std::move(event_0))->execute(database);
    REQUIRE(egress_event.username.value() == username_0);
    REQUIRE(egress_event.session_id.value() == user_0_sess_id);
    REQUIRE(egress_event.data ==
            "You can't sell your item, you don't have funds to cover the fee!");
    REQUIRE(accounts.get_funds(username_0) == 0);
    REQUIRE(accounts.get_items(username_0) == "item_1\nitem_0\nitem_0");
    REQUIRE(auctions.get_printable_list().empty());
  }

  SECTION("Fail at putting an item into sale - no such item!") {
    event_0.data = "SELL item_3 100 1";
    auto egress_event = Command::parse(std::move(event_0))->execute(database);
    REQUIRE(egress_event.username.value() == username_0);
    REQUIRE(egress_event.session_id.value() == user_0_sess_id);
    REQUIRE(egress_event.data == "You can't sell your item, there is no item_3!");
    REQUIRE(accounts.get_funds(username_0) == 1000);
    REQUIRE(accounts.get_items(username_0) == "item_0\nitem_1\nitem_0");
    REQUIRE(auctions.get_printable_list().empty());
  }

  SECTION("Fail at putting an item into sale - price out of bounds!") {
    event_0.data =
        "SELL item_1 "
        "1000000000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000 1";
    auto egress_event = Command::parse(std::move(event_0))->execute(database);
    REQUIRE(egress_event.username.value() == username_0);
    REQUIRE(egress_event.session_id.value() == user_0_sess_id);
    REQUIRE(egress_event.data == "You can't sell your item, invalid argument!");
    REQUIRE(accounts.get_funds(username_0) == 1000);
    REQUIRE(accounts.get_items(username_0) == "item_0\nitem_1\nitem_0");
    REQUIRE(auctions.get_printable_list().empty());
  }

  SECTION("Fail at putting an item into sale - time out of bounds!") {
    event_0.data =
        "SELL item_1 100"
        "1000000000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000";
    auto egress_event = Command::parse(std::move(event_0))->execute(database);
    REQUIRE(egress_event.username.value() == username_0);
    REQUIRE(egress_event.session_id.value() == user_0_sess_id);
    REQUIRE(egress_event.data ==
            "You can't sell your item, invalid argument!");
    REQUIRE(accounts.get_funds(username_0) == 1000);
    REQUIRE(accounts.get_items(username_0) == "item_0\nitem_1\nitem_0");
    REQUIRE(auctions.get_printable_list().empty());
  }

  SECTION("Fail at putting an item into sale - when a user is not logged in!") {
    event_0.username = {};
    event_0.data = "SELL item_1 100";
    auto egress_event = Command::parse(std::move(event_0))->execute(database);
    REQUIRE(!egress_event.username.has_value());
    REQUIRE(egress_event.session_id.value() == user_0_sess_id);
    REQUIRE(egress_event.data ==
            "Selling of an item has failed! Are you logged in?");
    REQUIRE(auctions.get_printable_list().empty());
  }
}

TEST_CASE("Test execution of show commands", "[Commands]") {
  Accounts accounts;
  AuctionList auctions;
  SessionManager sessions;
  Database database{accounts, auctions, sessions};

  const SessionId user_0_sess_id = 1;
  const ConnectionId user_0_conn_id = 1;
  const SessionId user_1_sess_id = 2;
  const ConnectionId user_1_conn_id = 2;
  const auto username_0 = "username_0";
  const auto username_1 = "username_1";
  REQUIRE(sessions.start_session(user_0_sess_id, user_0_sess_id));
  REQUIRE(sessions.start_session(user_1_sess_id, user_1_sess_id));
  REQUIRE(sessions.login(user_0_sess_id, username_0));
  REQUIRE(sessions.login(user_1_sess_id, username_1));
  REQUIRE(accounts.deposit_funds(username_0, 1000));
  REQUIRE(accounts.deposit_funds(username_1, 1000));
  accounts.deposit_item(username_0, "item_0");
  accounts.deposit_item(username_0, "item_1");
  accounts.deposit_item(username_0, "item_0");
  accounts.deposit_item(username_1, "item_2");
  accounts.deposit_item(username_1, "item_3");

  REQUIRE(auctions.add_auction({username_0, {}, 100, "item_4", Clock::now()}));
  REQUIRE(auctions.add_auction({username_0, {}, 200, "item_5", Clock::now()}));
  REQUIRE(auctions.add_auction({username_1, {}, 400, "item_6", Clock::now()}));
  REQUIRE(auctions.add_auction({username_1, {}, 300, "item_7", Clock::now()}));
  REQUIRE(auctions.add_auction({username_1, {}, 500, "item_8", Clock::now()}));

  REQUIRE(auctions.bid_item(1, 500, username_1) == BidResult::Successful);

  SECTION("Show user's items") {

  }

  SECTION("Show user's funds") {

  }

  SECTION("Show auctions") {

  }
}