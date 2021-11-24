//
// Created by mswiercz on 24.11.2021.
//
#include "command.h"
#include "database.h"
#include <regex>

namespace auction_engine {

static const auto help_regex =
    std::regex("\\s+*(HELP)\\s+*", std::regex::icase);
static const auto login_regex =
    std::regex("\\s+*(LOGIN)\\s+(\\w+)\\s+*", std::regex::icase);
static const auto logout_regex =
    std::regex("\\s+*(LOGOUT)\\s+*", std::regex::icase);
static const auto deposit_funds_regex =
    std::regex("\\s+*(DEPOSIT)\\s+(FUNDS)\\s+(\\d+)\\s+*", std::regex::icase);
static const auto deposit_item_regex =
    std::regex("\\s+*(DEPOSIT)\\s+(ITEM)\\s+(\\w+)\\s+*", std::regex::icase);
static const auto withdraw_funds_regex =
    std::regex("\\s+*(WITHDRAW)\\s+(FUNDS)\\s+(\\d+)\\s+*", std::regex::icase);
static const auto withdraw_item_regex =
    std::regex("\\s+*(WITHDRAW)\\s+(ITEM)\\s+(\\w+)\\s+*", std::regex::icase);
static const auto sell_regex = std::regex(
    "\\s+*(SELL)\\s+(\\w+)\\s+(\\d+)\\s+(?:\\d+)?\\s+*", std::regex::icase);
static const auto bid_regex =
    std::regex("\\s+*(BID)\\s+(\\w+)\\s+(\\d+)\\s+*", std::regex::icase);
static const auto show_funds_regex =
    std::regex("\\s+*(SHOW)\\s+(FUNDS)\\s+*", std::regex::icase);
static const auto show_items_regex =
    std::regex("\\s+*(SHOW)\\s+(ITEMS)\\s+*", std::regex::icase);
static const auto show_sales_regex =
    std::regex("\\s+*(SHOW)\\s+(SALES)\\s+*", std::regex::icase);

class HelpCommand : public Command {
public:
  HelpCommand(UserEvent &&event) : Command(std::move(event)) {}

  virtual UserEvent execute(Database &) override {
    return {std::move(_event.username), _event.session_id, "print help here!"};
  }
};

class LoginCommand : public Command {
public:
  LoginCommand(UserEvent &&event, std::string &&username)
      : Command(std::move(event)), _username(username) {}

  UserEvent execute(Database &database) override {
    try {
      if (database.sessions.login(_event.session_id.value(), _username)) {
        auto data = "Welcome " + _username + "!";
        return {std::move(_username), _event.session_id, std::move(data)};
      }
    } catch (std::bad_optional_access &e) {
    }
    return {std::move(_event.username), _event.session_id,
            "Couldn't login as" + _username};
  }

private:
  std::string _username;
};

class WrongCommand : public Command {
public:
  WrongCommand(UserEvent &&event) : Command(std::move(event)) {}

  UserEvent execute(Database &) override {
    auto egress_event = std::move(_event);
    egress_event.data.insert(0, "WRONG COMMAND: ");
    return egress_event;
  }
};

CommandPtr Command::parse(UserEvent &&event) {
  if (std::regex_match(event.data, help_regex)) {
    return CommandPtr{new HelpCommand{std::move(event)}};
  }

  std::smatch matches{};
  if (std::regex_match(event.data, matches, login_regex)) {
    return CommandPtr{new LoginCommand{std::move(event), matches[2].str()}};
  }

  return CommandPtr{new WrongCommand{std::move(event)}};
}
} // namespace auction_engine