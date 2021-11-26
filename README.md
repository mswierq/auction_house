# Auction house

This is a simple auction house simulator where clients can sell and buy their imaginary commodities.

## Client interface

A user can connect through a telnet client to the auction house simulator server. In order to interact with the server, a client can type the following commands:

- `HELP` - lists the available commands.
- `LOGIN <username>` - logs in the user by its `<username>`. Only one user with the same `<username>` can be logged in at a time. Creates a new account if it doesn't exist.
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
- When a user bids an item, the bid amount is deducted automatically from user's funds. If a user is outbid, funds come back to its account.
- User can't bind if it doesn't have enough funds.
- User can store more than one item with the same name.
- User can sell, deposit or withdraw only one item of the same name at a time.
- Funds are represented as integer values.

## Server design

The high-level overview how the server application is implemented.

### Threads 
- **Session processor** - handles new client connections, reads user data from a socket and spawns a new asynchronous task for each data packet. Puts the related future object into the **Tasks queue**. 
- **Auction processor** - process auction events, monitors if an auction has been list. Spawns a new asynchronous task to handle user notification about the auction status. Puts the related future object into the **Tasks queue**.
- **Tasks processor** - pops queued futures and process them and then sends results to the connected users.

### Tasks

Tasks are asynchronous and are being executed synchronously by the Tasks processor.
The **Tasks processor** awaits the future objects in the **Tasks queue** to be finished and sends the result back to a user.
Each task represents processing of a single command or notification from the **Auction** processor.

There are following queues:
1. Tasks queue - queue of future objects that represents results of asynchronous tasks spawned by the **Session and Auction processors**.

There are following data structures:
1. Auctions list - list of items put to an auction. Items are put here in the result of user's command and removed when an auction comes to an end.
2. User accounts map - username is the key and the value is user account which keeps info about funds and list of items.
3. Sessions map - keeps entries for each connection with information about socket descriptor, session id and username if logged-in.