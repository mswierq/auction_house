//
// Created by mswiercz on 24.11.2021.
//
#include "command.h"
#include "database.h"
#include <numeric>
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
    "\\s+*(SELL)\\s+(\\w+)\\s+(\\d+)\\s+*(?:\\d+)?\\s+*", std::regex::icase);
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
      if (database.accounts.withdraw_item(_event.username.value(), _item)) {
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

class SellItemCommand : public Command {
public:
  SellItemCommand(UserEvent &&event, std::string &&item, std::string &&price,
                  std::string &&time)
      : Command(std::move(event)), _item(item), _price(price), _time(time) {}

  UserEvent execute(Database &database) override {
    if (!_event.username.has_value()) {
      return {{},
              _event.session_id,
              "Selling of an item has failed! Are you logged in?"};
    }
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
    } catch (std::bad_optional_access &) {
      data = "Selling of an item has failed! Server error!";
    } catch (std::invalid_argument &) {
      data = "You can't sell your item, invalid argument!";
    } catch (std::out_of_range &) {
      data = "You can't sell your item, invalid argument!";
    }

    return {std::move(_event.username), _event.session_id, std::move(data)};
  }

private:
  static constexpr FundsType FEE = 1;
  std::string _item;
  std::string _price;
  std::string _time;
};

class BidItemCommand : public Command {
public:
  BidItemCommand(UserEvent &&event, std::string &&auction_id,
                 std::string &&new_price)
      : Command(std::move(event)), _auction_id(auction_id),
        _new_price(new_price) {}

  UserEvent execute(Database &database) override {
    if (!_event.username.has_value()) {
      return {{},
              _event.session_id,
              "Bidding an item has failed! Are you logged in?"};
    }
    std::string data{};
    try {
      auto &new_buyer = _event.username.value();
      auto auction_id = std::stoull(_auction_id);
      auto new_price = parse_funds(_new_price);
      if (database.accounts.withdraw_funds(new_buyer, new_price)) {
        auto result =
            database.auctions.bid_item(auction_id, new_price, new_buyer);
        switch (result) {
        case BidResult::Successful:
          data = "You are winning the auction " + _auction_id + "!";
          break;
        case BidResult::TooLowPrice:
          database.accounts.deposit_funds(new_buyer, new_price);
          data = "Your offer for the auction " + _auction_id + " was too low!";
          break;
        case BidResult::OwnerBid:
          database.accounts.deposit_funds(new_buyer, new_price);
          data = "You can't bid on the auction " + _auction_id +
                 ", you are the seller!";
          break;
        case BidResult::DoesNotExist:
          database.accounts.deposit_funds(new_buyer, new_price);
          data = "There is no such auction!";
          break;
        }
      } else {
        data = "You can't bid on the auction " + _auction_id +
               ", you don't have enough funds!";
      }
    } catch (std::bad_optional_access &) {
      data = "Bidding of an item has failed! Server error!";
    } catch (std::invalid_argument &) {
      data = "The bid arguments are invalid!";
    } catch (std::out_of_range &) {
      data = "The bid arguments are invalid!";
    }

    return {std::move(_event.username), _event.session_id, std::move(data)};
  }

private:
  std::string _auction_id;
  std::string _new_price;
};

class ShowItemsCommand : public Command {
public:
  ShowItemsCommand(UserEvent &&event) : Command(std::move(event)) {}

  UserEvent execute(Database &database) override {
    if (!_event.username.has_value()) {
      return {{}, _event.session_id, "You are not logged in!"};
    }
    auto egress_event = std::move(_event);
    egress_event.data =
        "Your items:\n" + database.accounts.get_items(egress_event.username.value());
    return egress_event;
  }
};

class ShowFundsCommand : public Command {
public:
  ShowFundsCommand(UserEvent &&event) : Command(std::move(event)) {}

  UserEvent execute(Database &database) override {
    if (!_event.username.has_value()) {
      return {{}, _event.session_id, "You are not logged in!"};
    }
    auto egress_event = std::move(_event);
    egress_event.data =
        "Your funds: " +
        std::to_string(database.accounts.get_funds(egress_event.username.value()));
    return egress_event;
  }
};

class ShowSalesCommand : public Command {
public:
  ShowSalesCommand(UserEvent &&event) : Command(std::move(event)) {}

  UserEvent execute(Database &database) override {
    if (!_event.username.has_value()) {
      return {{}, _event.session_id, "You are not logged in!"};
    }
    auto egress_event = std::move(_event);
    auto auctions = database.auctions.get_printable_list();
    egress_event.data =
        "SALES:\n" + std::accumulate(auctions.begin(), auctions.end(),
                                     std::string{}, [](auto &a, auto &b) {
                                       if (a.empty()) {
                                         return b;
                                       }
                                       return a + "\n" + b;
                                     });
    return egress_event;
  }
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