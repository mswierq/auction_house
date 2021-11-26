//
// Created by mswiercz on 24.11.2021.
//
#include "command.h"
#include "database.h"
#include <numeric>
#include <regex>
#include <spdlog/spdlog.h>

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
    "\\s+*(SELL)\\s+(\\w+)\\s+(\\d+)\\s+*(\\d+)?\\s+*", std::regex::icase);
static const auto bid_regex =
    std::regex("\\s+*(BID)\\s+(\\d+)\\s+(\\d+)\\s+*", std::regex::icase);
static const auto show_funds_regex =
    std::regex("\\s+*(SHOW)\\s+(FUNDS)\\s+*", std::regex::icase);
static const auto show_items_regex =
    std::regex("\\s+*(SHOW)\\s+(ITEMS)\\s+*", std::regex::icase);
static const auto show_sales_regex =
    std::regex("\\s+*(SHOW)\\s+(SALES)\\s+*", std::regex::icase);

class HelpCommand : public Command {
public:
  HelpCommand(IngressEvent &&event) : Command(std::move(event)) {}

  virtual EgressEvent execute(Database &) override {
    spdlog::info("user {}, session {}, asked for help",
                 _event.username.value_or(""), _event.session_id);
    return {_event.session_id, "print help here!"};
  }
};

class LoginCommand : public Command {
public:
  LoginCommand(IngressEvent &&event, std::string &&username)
      : Command(std::move(event)), _username(username) {}

  EgressEvent execute(Database &database) override {
    try {
      if (database.sessions.login(_event.session_id, _username)) {
        auto data = "Welcome " + _username + "!";
        spdlog::info("user {}, session {}, has logged in!",
                     _event.username.value_or(""), _event.session_id);
        return {_event.session_id, std::move(data)};
      }
    } catch (std::bad_optional_access &e) {
      spdlog::error("user {}, session {}, unexpected error: ",
                    _event.username.value_or(""), _event.session_id, e.what());
    }
    spdlog::info("user {}, session {}, asked for help",
                 _event.username.value_or(""), _event.session_id);
    return {_event.session_id, "Couldn't login as " + _username + "!"};
  }

private:
  std::string _username;
};

class LogoutCommand : public Command {
public:
  LogoutCommand(IngressEvent &&event) : Command(std::move(event)) {}

  EgressEvent execute(Database &database) override {
    try {
      if (database.sessions.logout(_event.session_id)) {
        auto data = "Good bay, " + _event.username.value() + "!";
        spdlog::info("user {}, session {}, has logged out!",
                     _event.username.value_or(""), _event.session_id);
        return {_event.session_id, std::move(data)};
      }
    } catch (std::bad_optional_access &e) {
      spdlog::error("user {}, session {}, unexpected error: ",
                    _event.username.value_or(""), _event.session_id, e.what());
    }
    return {_event.session_id, "You are not logged in!"};
  }
};

class LimitedAccess : public Command {
public:
  LimitedAccess(IngressEvent &&event) : Command(std::move(event)) {}

  EgressEvent execute(Database &database) final {
    if (!_event.username.has_value()) {
      spdlog::info("Not logged in session {}, tried to: {}", _event.session_id,
                   _event.data);
      return {_event.session_id, "You are not logged in!"};
    }
    return execute_impl(database);
  }

protected:
  virtual EgressEvent execute_impl(Database &database) = 0;
};

class DepositFundsCommand : public LimitedAccess {
public:
  DepositFundsCommand(IngressEvent &&event, std::string &&amount)
      : LimitedAccess(std::move(event)), _amount(amount) {}

protected:
  EgressEvent execute_impl(Database &database) override {
    try {
      if (database.accounts.deposit_funds(_event.username.value(),
                                          parse_funds(_amount))) {
        spdlog::info("user {}, session {}, deposited funds {} ",
                     _event.username.value_or(""), _event.session_id, _amount);
        return {_event.session_id,
                "Successful deposition of funds: " + _amount + "!"};
      }
    } catch (std::bad_optional_access &e) {
      spdlog::error("user {}, session {}, unexpected error: {}",
                    _event.username.value_or(""), _event.session_id, e.what());
      return {_event.session_id,
              "Deposition of funds has failed! Server error!"};
    } catch (std::invalid_argument &) {
    } catch (std::out_of_range &) {
    }
    spdlog::warn(
        "user {}, session {}, tried to deposit invalid amount of funds!",
        _event.username.value_or(""), _event.session_id);
    return {_event.session_id,
            "Deposition of funds has failed! Invalid amount!"};
  }

private:
  std::string _amount;
};

class DepositItemCommand : public LimitedAccess {
public:
  DepositItemCommand(IngressEvent &&event, std::string &&item)
      : LimitedAccess(std::move(event)), _item(item) {}

protected:
  EgressEvent execute_impl(Database &database) override {
    try {
      database.accounts.deposit_item(_event.username.value(), _item);
      spdlog::info("user {}, session {}, deposited item: {} ",
                   _event.username.value_or(""), _event.session_id, _item);
      return {_event.session_id,
              "Successful deposition of item: " + _item + "!"};
    } catch (std::bad_optional_access &e) {
      spdlog::error("user {}, session {}, unexpected error: {}",
                    _event.username.value_or(""), _event.session_id, e.what());
      return {_event.session_id,
              "Deposition of an item has failed! Server error!"};
    }
  }

private:
  std::string _item;
};

class WithdrawFundsCommand : public LimitedAccess {
public:
  WithdrawFundsCommand(IngressEvent &&event, std::string &&amount)
      : LimitedAccess(std::move(event)), _amount(amount) {}

protected:
  EgressEvent execute_impl(Database &database) override {
    try {
      if (database.accounts.withdraw_funds(_event.username.value(),
                                           parse_funds(_amount))) {
        spdlog::info("user {}, session {}, withdrawn funds: {} ",
                     _event.username.value_or(""), _event.session_id, _amount);
        return {_event.session_id, "Successfully withdrawn: " + _amount + "!"};
      } else {
        spdlog::info("user {}, session {}, tried to withdraw funds: {} ",
                     _event.username.value_or(""), _event.session_id, _amount);
        return {_event.session_id,
                "Withdrawal of funds has failed! Insufficient funds!"};
      }
    } catch (std::bad_optional_access &e) {
      spdlog::error("user {}, session {}, unexpected error: {}",
                    _event.username.value_or(""), _event.session_id, e.what());
      return {_event.session_id,
              "Withdrawal of funds has failed! Server error!"};
    } catch (std::invalid_argument &) {
    } catch (std::out_of_range &) {
    }
    spdlog::warn(
        "user {}, session {}, tried to withdraw invalid amount of funds!",
        _event.username.value_or(""), _event.session_id);
    return {_event.session_id,
            "Withdrawal of funds has failed! Invalid amount!"};
  }

private:
  std::string _amount;
};

class WithdrawItemCommand : public LimitedAccess {
public:
  WithdrawItemCommand(IngressEvent &&event, std::string &&item)
      : LimitedAccess(std::move(event)), _item(item) {}

protected:
  EgressEvent execute_impl(Database &database) override {
    try {
      if (database.accounts.withdraw_item(_event.username.value(), _item)) {
        spdlog::info("user {}, session {}, withdrawn item: {} ",
                     _event.username.value_or(""), _event.session_id, _item);
        return {_event.session_id,
                "Successfully withdrawn item: " + _item + "!"};
      }
    } catch (std::bad_optional_access &e) {
      spdlog::error("user {}, session {}, unexpected error: {}",
                    _event.username.value_or(""), _event.session_id, e.what());
      return {_event.session_id,
              "Withdrawal of an item has failed! Server error!"};
    }
    spdlog::info("user {}, session {}, tried to withdraw item: {} ",
                 _event.username.value_or(""), _event.session_id, _item);
    return {_event.session_id,
            "Withdrawal of an item has failed! No such item: " + _item + "!"};
  }

private:
  std::string _item;
};

class SellItemCommand : public LimitedAccess {
public:
  SellItemCommand(IngressEvent &&event, std::string &&item, std::string &&price,
                  std::string &&time)
      : LimitedAccess(std::move(event)), _item(item), _price(price),
        _time(time) {}

protected:
  EgressEvent execute_impl(Database &database) override {
    std::string data{};
    try {
      auto &username = _event.username.value();
      auto price = parse_funds(_price);
      auto expiration_time =
          Clock::now() + std::chrono::seconds(std::stoi(_time));
      if (database.accounts.withdraw_item(username, _item)) {
        if (database.accounts.withdraw_funds(username, FEE)) {
          if (database.auctions.add_auction(
                  {username, {}, price, _item, expiration_time})) {
            data = "Your item " + _item + " is being auctioned off!";
          } else {
            database.accounts.deposit_item(username, _item);
            database.accounts.deposit_funds(username, FEE);
            data = "Selling of an item has failed! Server error!";
          }
        } else {
          data = "You can't sell your item, you don't have funds to cover the "
                 "fee!";
          database.accounts.deposit_item(username, _item);
        }
      } else {
        data = "You can't sell your item, there is no " + _item + "!";
      }
    } catch (std::bad_optional_access &e) {
      spdlog::error("user {}, session {}, unexpected error: {}",
                    _event.username.value_or(""), _event.session_id, e.what());
      data = "Selling of an item has failed! Server error!";
    } catch (std::invalid_argument &) {
      data = "You can't sell your item, invalid argument!";
    } catch (std::out_of_range &) {
      data = "You can't sell your item, invalid argument!";
    }

    spdlog::info("user {}, session {}, put item: {} on sale for {}, it "
                 "will expire in {} seconds, transaction result: {}",
                 _event.username.value_or(""), _event.session_id, _item, _price,
                 _time, data);

    return {_event.session_id, std::move(data)};
  }

private:
  static constexpr FundsType FEE = 1;
  std::string _item;
  std::string _price;
  std::string _time;
};

class BidItemCommand : public LimitedAccess {
public:
  BidItemCommand(IngressEvent &&event, std::string &&auction_id,
                 std::string &&new_price)
      : LimitedAccess(std::move(event)), _auction_id(auction_id),
        _new_price(new_price) {}

protected:
  EgressEvent execute_impl(Database &database) override {
    std::string data{};
    try {
      auto &new_buyer = _event.username.value();
      auto auction_id = std::stoull(_auction_id);
      auto new_price = parse_funds(_new_price);
      auto result =
          database.auctions.bid_item(auction_id, new_price, new_buyer);
      switch (result) {
      case BidResult::Successful:
        data = "You are winning the auction " + _auction_id + "!";
        break;
      case BidResult::TooLowPrice:
        data = "Your offer for the auction " + _auction_id + " was too low!";
        break;
      case BidResult::OwnerBid:
        data = "You can't bid on the auction " + _auction_id +
               ", you are the seller!";
        break;
      case BidResult::DoesNotExist:
        data = "There is no such auction!";
        break;
      }
    } catch (std::bad_optional_access &e) {
      spdlog::error("user {}, session {}, unexpected error: {}",
                    _event.username.value_or(""), _event.session_id, e.what());
      data = "Bidding of an item has failed! Server error!";
    } catch (std::invalid_argument &) {
      data = "The bid arguments are invalid!";
    } catch (std::out_of_range &) {
      data = "The bid arguments are invalid!";
    }

    spdlog::info("user {}, session {}, bit auction: {} on sale for {}, "
                 "transaction result: {}",
                 _event.username.value_or(""), _event.session_id, _auction_id,
                 _new_price, data);

    return {_event.session_id, std::move(data)};
  }

private:
  std::string _auction_id;
  std::string _new_price;
};

class ShowItemsCommand : public LimitedAccess {
public:
  ShowItemsCommand(IngressEvent &&event) : LimitedAccess(std::move(event)) {}

protected:
  EgressEvent execute_impl(Database &database) override {
    spdlog::info("user {}, session {}, asked for items list",
                 _event.username.value_or(""), _event.session_id);
    return {_event.session_id, "Your items:\n" + database.accounts.get_items(
                                                     _event.username.value())};
  }
};

class ShowFundsCommand : public LimitedAccess {
public:
  ShowFundsCommand(IngressEvent &&event) : LimitedAccess(std::move(event)) {}

protected:
  EgressEvent execute_impl(Database &database) override {
    spdlog::info("user {}, session {}, asked for funds",
                 _event.username.value_or(""), _event.session_id);
    return {_event.session_id,
            "Your funds: " + std::to_string(database.accounts.get_funds(
                                 _event.username.value()))};
  }
};

class ShowSalesCommand : public LimitedAccess {
public:
  ShowSalesCommand(IngressEvent &&event) : LimitedAccess(std::move(event)) {}

protected:
  EgressEvent execute_impl(Database &database) override {
    spdlog::info("user {}, session {}, asked for sales",
                 _event.username.value_or(""), _event.session_id);
    auto auctions = database.auctions.get_printable_list();
    return {_event.session_id,
            "SALES:\n" + std::accumulate(auctions.begin(), auctions.end(),
                                         std::string{}, [](auto &a, auto &b) {
                                           if (a.empty()) {
                                             return b;
                                           }
                                           return a + "\n" + b;
                                         })};
  }
};

class WrongCommand : public Command {
public:
  WrongCommand(IngressEvent &&event) : Command(std::move(event)) {}

  EgressEvent execute(Database &) override {
    spdlog::info("user {}, session {}, typed wrong command: {}",
                 _event.username.value_or(""), _event.session_id, _event.data);
    return {_event.session_id, _event.data.insert(0, "WRONG COMMAND: ")};
  }
};

CommandPtr Command::parse(IngressEvent &&event) {
  std::smatch matches{};

  spdlog::debug("user {}, session {}, parsing command: {}",
                event.username.value_or(""), event.session_id, event.data);

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

  if (std::regex_match(event.data, matches, sell_regex)) {
    auto time =
        matches.size() > 4 ? matches[4].str() : "300"; // default 5 minutes
    return CommandPtr{new SellItemCommand{std::move(event), matches[2].str(),
                                          matches[3].str(), std::move(time)}};
  }

  if (std::regex_match(event.data, matches, bid_regex)) {
    return CommandPtr{new BidItemCommand{std::move(event), matches[2].str(),
                                         matches[3].str()}};
  }

  if (std::regex_match(event.data, matches, show_funds_regex)) {
    return CommandPtr{new ShowFundsCommand{std::move(event)}};
  }

  if (std::regex_match(event.data, matches, show_items_regex)) {
    return CommandPtr{new ShowItemsCommand{std::move(event)}};
  }

  if (std::regex_match(event.data, matches, show_sales_regex)) {
    return CommandPtr{new ShowSalesCommand{std::move(event)}};
  }

  return CommandPtr{new WrongCommand{std::move(event)}};
}
} // namespace auction_engine