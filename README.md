# Auction house

This is a simple auction house simulator where clients can sell and buy their imaginary commodities.

## Client interface

A user can connect through a telnet client to the auction house simulator server. In order to interact with the server, a client can type the following commands:

- `HELP` - lists the available commands.
- `LOGIN <username>` - logs in the user by its `<username>`. Only one user with the same `<username>` can be logged in at a time.
- `LOGOUT` - logs out the user.
- `DEPOSIT FUNDS <amount>` - deposits `<amount>` of funds in user's account. Works only if logged in.
- `DEPOSIT ITEM <item>` - deposits an `<item>` in user's account. Works only if logged in.
- `WITHDRAWS FUNDS <amount>` - withdraws `<amount>` of funds in user's account. Works only if logged in.
- `WITHDRAWS FUNDS <item>` - withdraws an `<item>` in user's account. Works only if logged in.
- `SELL <item> <starting-price> [<expiration-time>]` - puts an `<item>` into an auction with the `<starting-price>`. The optional argument `[<expiration-time>]` is seconds from putting the `<item>` into sale, default value is `300` (5 minutes). Works only if logged in.
- `BID <auction-id> <new-price>` - bids an item tagged with the `<auction-id>` with the `<new-price>`. A user can't bid its own item. Works only if logged in.
- `SHOW FUNDS` - shows user's funds. Works only if logged in.
- `SHOW ITEMS` - shows user's items. Works only if logged in.
- `SHOW SALES` - shows sales. Works only if logged in.

The commands are case-insensitive, but the `<arguments>` are case-sensitive.

### Limitations and requirements
 
- Putting an item into an auction charges the seller a fee equals to `1`, which is deducted instantly. If a user doesn't have funds to put an item into an auction it will fail.
- When a user bids an item, the bid amount is deducted when auction has expired. If a user doesn't have enough funds to cover the bid, the item comes back to the seller.
- If the seller cannot accept the payment from the buyer for some reason (e.g. maximum number of funds has been reached), the item goes back to the seller account and the buyer is not charged.
- User can store more than one item with the same name.
- Funds are represented as integer values.
- Usernames and item names are alphanumerics.

## Server design

The high-level overview how the server application is implemented.

### Threads 
- **Session processor** - handles new client connections, reads user data from a socket and spawns a new asynchronous task for each data packet. Puts the related future object into the **Tasks queue**. 
- **Auction processor** - process auction events, monitors if an auction has been expired. Spawns a new asynchronous task to handle user notification about the auction status. Puts the related future object into the **Tasks queue**.
- **Tasks processor** - pops queued futures and process them and then sends results to the connected users. All async tasks use the deffered policy, which means the tasks are executed by this thread.

### Tasks

Tasks are asynchronous and are being executed synchronously by the Tasks processor.
The **Tasks processor** awaits the future objects in the **Tasks queue** to be finished and sends the result back to a user.
Each task represents processing of a single command or notification from the **Auction** processor.

There are following queues:
1. Tasks queue - queue of future objects that represents results of asynchronous tasks spawned by the **Session and Auction processors**.

There are following data structures:
1. AuctionList - list of items put to an auction. Items are put here in the result of user's command and removed when an auction comes to an end.
2. Accounts - username is the key and the value is user account which keeps info about funds and list of items.
3. SessionsManager - keeps entries for each connection with information about socket descriptor, session id and a map of logged-in usernames to session ids.

## Project

This is a CMake based project, **which was developed and tested under Linux**.

### Libraries
Used libraries in the project:
- Catch2 (v2.13.7) - for testing,
- spdlog (v1.9.2) - for logging.

Both are fetched automatically by the CMake FetchContent. **Git and network connection is required.**

The project uses Socket API for handling network traffic.

### Build

This project can be imported to your favourite IDE that supports CMake and just compile it. Two targets should be detected:
- auction_house - server binary,
- tests - a binary for running tests.

This project has been built with the following compilers:
- Clang 13,
- Clang 11,
- GCC 11.1.0.

### Docker
A docker image can be produced with the server app, by running:
```bash
docker build . -t auction_house
```

There is also a bash script that builds the image and runs it automatically, just run:
```bash
./build_and_run.sh
```
The default port is **10000**, to change it, please run:
```bash
./build_and_run.sh <port>
```

### Run
If you want to run the server binary without the docker image, just run:
```bash
./auction_house
```
To change the default **10000** port or set the logs level to debug, you can add these arguments:
```bash
./auction_house --port <port> --debug
```

### Windows support

This project has been successfully build on Windows machine using Visual Studio 2019 16.11.

Unfortunately there are still some bugs that deteriorates user expierence. Please, use Putty as telnet client and set Telnet negotiation mode to **Passive**.