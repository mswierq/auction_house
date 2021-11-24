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
            "Couldn't login as " + _username + "!"};
  }

private:
  std::string _username;
};

class LogoutCommand : public Command {
public:
  LogoutCommand(UserEvent &&event) : Command(std::move(event)) {}

  UserEvent execute(Database &database) override {
    try {
      if (database.sessions.logout(_event.session_id.value())) {
        auto data = "Good bay, " + _event.username.value() + "!";
        return {std::move(_event.username), _event.session_id, std::move(data)};
      }
    } catch (std::bad_optional_access &e) {
    }
    return {std::move(_event.username), _event.session_id,
            "You are not logged in!"};
  }
};

class DepositFundsCommand : public Command {
public:
  DepositFundsCommand(UserEvent &&event, std::string &&amount)
      : Command(std::move(event)), _amount(amount) {}

  UserEvent execute(Database &database) override {
    if (!_event.username.has_value()) {
      return {{},
              _event.session_id,
              "Deposition of funds has failed! Are you logged in?"};
    }
    try {
      if (database.accounts.deposit_funds(_event.username.value(),
                                          parse_funds(_amount))) {
        return {std::move(_event.username), _event.session_id,
                "Successful deposition of funds: " + _amount + "!"};
      }
    } catch (std::bad_optional_access &) {
      return {std::move(_event.username), _event.session_id,
              "Deposition of funds has failed! Server error!"};
    } catch (std::invalid_argument &) {
    } catch (std::out_of_range &) {
    }
    return {std::move(_event.username), _event.session_id,
            "Deposition of funds has failed! Invalid amount!"};
  }

private:
  std::string _amount;
};

class DepositItemCommand : public Command {
public:
  DepositItemCommand(UserEvent &&event, std::string &&item)
      : Command(std::move(event)), _item(item) {}

  UserEvent execute(Database &database) override {
    if (!_event.username.has_value()) {
      return {{},
              _event.session_id,
              "Deposition of an item has failed! Are you logged in?"};
    }
    try {
      database.accounts.deposit_item(_event.username.value(), _item);
      return {std::move(_event.username), _event.session_id,
              "Successful deposition of item: " + _item + "!"};
    } catch (std::bad_optional_access &) {
      return {std::move(_event.username), _event.session_id,
              "Deposition of an item has failed! Server error!"};
    }
  }

private:
  std::string _item;
};

class WithdrawFundsCommand : public Command {
public:
  WithdrawFundsCommand(UserEvent &&event, std::string &&amount)
      : Command(std::move(event)), _amount(amount) {}

  UserEvent execute(Database &database) override {
    if (!_event.username.has_value()) {
      return {{},
              _event.session_id,
              "Withdrawal of funds has failed! Are you logged in?"};
    }
    try {
      if (database.accounts.withdraw_funds(_event.username.value(),
                                          parse_funds(_amount))) {
        return {std::move(_event.username), _event.session_id,
                "Successfully withdrawn: " + _amount + "!"};
      } else {
        return {std::move(_event.username), _event.session_id,
                "Withdrawal of funds has failed! Insufficient funds!"};
      }
    } catch (std::bad_optional_access &) {
      return {std::move(_event.username), _event.session_id,
              "Withdrawal of funds has failed! Server error!"};
    } catch (std::invalid_argument &) {
    } catch (std::out_of_range &) {
    }
    return {std::move(_event.username), _event.session_id,
            "Withdrawal of funds has failed! Invalid amount!"};
  }

private:
  std::string _amount;
};

class WithdrawItemCommand : public Command {
public:
  WithdrawItemCommand(UserEvent &&event, std::string &&item)
      : Command(std::move(event)), _item(item) {}

  UserEvent execute(Database &database) override {
    if (!_event.username.has_value()) {
      return {{},
              _event.session_id,
              "Withdrawal of an item has failed! Are you logged in?"};
    }
    try {
      if(database.accounts.withdraw_item(_event.username.value(), _item)) {
        return {std::move(_event.username), _event.session_id,
                      "Successfully withdrawn item: " + _item + "!"};
      }
    } catch (std::bad_optional_access &) {
      return {std::move(_event.username), _event.session_id,
              "Withdrawal of an item has failed! Server error!"};
    }
    return {std::move(_event.username), _event.session_id,
            "Withdrawal of an item has failed! No such item: " + _item + "!"};
  }

private:
  std::string _item;
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
  std::smatch matches{};

  if (std::regex_match(event.data, help_regex)) {
    return CommandPtr{new HelpCommand{std::move(event)}};
  }

  if (std::regex_match(event.data, matches, login_regex)) {
    return CommandPtr{new LoginCommand{std::move(event), matches[2].str()}};
  }

  if (std::regex_match(event.data, logout_regex)) {
    return CommandPtr{new LogoutCommand{std::move(event)}};
  }

  if (std::regex_match(event.data, matches, deposit_funds_regex)) {
    return CommandPtr{
        new DepositFundsCommand{std::move(event), matches[3].str()}};
  }

  if (std::regex_match(event.data, matches, deposit_item_regex)) {
    return CommandPtr{
        new DepositItemCommand{std::move(event), matches[3].str()}};
  }

  if (std::regex_match(event.data, matches, withdraw_funds_regex)) {
    return CommandPtr{
        new WithdrawFundsCommand{std::move(event), matches[3].str()}};
  }

  if (std::regex_match(event.data, matches, withdraw_item_regex)) {
    return CommandPtr{
        new WithdrawItemCommand{std::move(event), matches[3].str()}};
  }

  return CommandPtr{new WrongCommand{std::move(event)}};
}
} // namespace auction_engine