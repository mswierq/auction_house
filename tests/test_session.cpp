//
// Created by mswiercz on 23.11.2021.
//
#include "session.h"
#include <catch2/catch.hpp>

using namespace auction_engine;

TEST_CASE("Check session operators", "[Session]") {
  SessionManager manager;

  SECTION("Start, login, logout and remove single session") {
    REQUIRE(manager.start_session(1));
    REQUIRE(manager.login(1, "username"));
    REQUIRE(manager.get_username(1).value() == "username");
    REQUIRE(manager.get_session_id("username").value() == 1);
    REQUIRE(manager.logout(1));
    REQUIRE(manager.end_session(1));
  }

  SECTION("Try to get username without logging in") {
    REQUIRE(manager.start_session(1));
    REQUIRE(!manager.get_username(1).has_value());
    REQUIRE(!manager.get_session_id("username").has_value());
    REQUIRE(!manager.logout(1));
    REQUIRE(manager.end_session(1));
  }

  SECTION("Try to logging to already logged in account") {
    REQUIRE(manager.start_session(1));
    REQUIRE(manager.start_session(2));
    REQUIRE(manager.login(1, "username"));
    REQUIRE(!manager.login(2, "username"));
    REQUIRE(manager.get_session_id("username").value() == 1);
    REQUIRE(manager.get_username(1).value() == "username");
    REQUIRE(!manager.get_username(2).has_value());
    REQUIRE(manager.end_session(1));
    REQUIRE(manager.end_session(2));
  }

  SECTION("Try to logout without logging in first") {
    REQUIRE(manager.start_session(1));
    REQUIRE(!manager.logout(1));
    REQUIRE(!manager.get_session_id("username").has_value());
    REQUIRE(!manager.get_username(1).has_value());
    REQUIRE(manager.end_session(1));
  }

  SECTION("Try to login with empty username") {
    REQUIRE(manager.start_session(1));
    REQUIRE(!manager.login(1, ""));
    REQUIRE(manager.end_session(1));
  }

  SECTION("Try to login after ending a session") {
    REQUIRE(manager.start_session(1));
    REQUIRE(manager.end_session(1));
    REQUIRE(!manager.login(1, "username"));
  }

  SECTION("Try to start the same session twice") {
    REQUIRE(manager.start_session(1));
    REQUIRE(!manager.start_session(1));
    REQUIRE(manager.end_session(1));
  }
}