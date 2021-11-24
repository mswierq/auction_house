//
// Created by mswiercz on 24.11.2021.
//
#include <catch2/catch.hpp>
#include "command.h"
#include "database.h"

using namespace auction_engine;

TEST_CASE("Test execution of a command", "[Commands]") {
  Accounts accounts;
  AuctionList auctions;
  SessionManager sessions;
  Database database{accounts, auctions, sessions};

  const SessionId session_id = 1;
  const ConnectionId connection_id = 1;
  sessions.start_session(session_id, connection_id);
  UserEvent event {{}, session_id, ""};

  SECTION("Successfully login a user") {
    event.data = "LOGIN username";
    auto egress_event = Command::parse(std::move(event))->execute(database);
    REQUIRE(egress_event.username.value() == "username");
    REQUIRE(egress_event.session_id.value() == session_id);
    REQUIRE(egress_event.data == "Welcome username!");
    REQUIRE(sessions.get_username(session_id).value() == "username");
    REQUIRE(sessions.get_session_id("username").value() == session_id);
  }
}