#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

/* ===================== MARKET DATA ===================== */

#define INITIAL_CAPACITY 10
#define GRAPH_ROWS 15
#define GRAPH_COLS 24
#define GRAPHS_PER_LINE 3
#define GRAPH_GAP 2
#define MAX_COINS 10

#define SAVE_FILE "market.txt"
#define SAVE_TMP "market.tmp"
#define SAVE_MAGIC "CRYPTOSIM"
#define SAVE_VERSION 1
#define MAX_SAVED_HISTORY 100000

#define ACCOUNT_FILE "account.txt"
#define ACCOUNT_TMP "account.tmp"
#define ACCOUNT_MAGIC "CRYPTOACC"
#define ACCOUNT_VERSION 1

#define WALLET_FILE "wallet.txt"
#define WALLET_TMP "wallet.tmp"
#define WALLET_MAGIC "CRYPTOWAL"
#define WALLET_VERSION 2

#define PKR_RATE 83.33
#define MAX_TRADES 100

#define MAX_AMOUNT 1000000000.0
#define MAX_CASH 1000000000000.0

#define TRADE_BUY 0
#define TRADE_SELL 1

typedef struct
{
    int id;
    char name[30];
    char symbol[10];
    double price;

    double *history;
    int count;
    int capacity;
} Crypto;

Crypto coins[MAX_COINS] =
{
    {1, "Bitcoin", "BTC", 67420.0, NULL, 0, 0},
    {2, "Ethereum", "ETH", 3420.0, NULL, 0, 0},
    {3, "Solana", "SOL", 142.0, NULL, 0, 0},
    {4, "BNB", "BNB", 610.0, NULL, 0, 0},
    {5, "XRP", "XRP", 0.52, NULL, 0, 0},
    {6, "Cardano", "ADA", 0.35, NULL, 0, 0},
    {7, "Dogecoin", "DOGE", 0.15, NULL, 0, 0},
    {8, "Avalanche", "AVAX", 27.0, NULL, 0, 0},
    {9, "Polkadot", "DOT", 4.0, NULL, 0, 0},
    {10, "Litecoin", "LTC", 85.0, NULL, 0, 0}
};

/* ===================== TAX ===================== */

double tax(double amount) {
    double taxes;
    taxes = amount * 0.05;
    printf("\n5%% tax = %.2f\n", taxes);
    return taxes;
}

/* ===================== STOCK EXCHANGE ===================== */

void stock_exchange(void) {
    int choice, currency, c;
    double amount, value, taxAmount, remaining;

    Crypto *selectedCoin;

    printf("\n========== CRYPTOCURRENCY CONVERSION ==========\n");

    /* Display all 10 cryptocurrencies */

    for (int i = 0; i < MAX_COINS; i++) {
        printf("%d. %s (%s) - $%.2f\n",
               i + 1,
               coins[i].name,
               coins[i].symbol,
               coins[i].price);
    }

    /* Validate cryptocurrency choice */

    while (1) {
        printf("\nChoose the cryptocurrency you have: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input! Enter a number.\n");
            while ((c = getchar()) != '\n' && c != EOF)
                ;
            continue;
        }

        if (choice >= 1 && choice <= MAX_COINS)
            break;

        printf("Invalid choice! Select between 1 and 10.\n");
    }

    /* Access the selected cryptocurrency structure */

    selectedCoin = &coins[choice - 1];

    printf("\nYou selected %s (%s).\n",
           selectedCoin->name,
           selectedCoin->symbol);

    printf("Current price: $%.2f\n",
           selectedCoin->price);

    /* Validate amount */

    while (1) {
        printf("\nEnter the amount of %s you have: ",selectedCoin->name);
        if (scanf("%lf", &amount) != 1) {
            printf("Invalid input! Enter a number.\n");
            while ((c = getchar()) != '\n' && c != EOF)
                ;
            continue;
        }
        if (isfinite(amount) && amount > 0 && amount <= MAX_AMOUNT)
            break;
        printf("Amount must be greater than zero and not too large.\n");
    }

    while (1) {
        printf("\nEnter the currency you want to convert to:\n");
        printf("1. USD\n");
        printf("2. PKR\n");
        printf("Choice: ");

        if (scanf("%d", &currency) != 1) {
            printf("Invalid input! Enter a number.\n");

            while ((c = getchar()) != '\n' && c != EOF)
                ;

            continue;
        }

        if (currency == 1 || currency == 2)
            break;

        printf("Invalid choice! Enter 1 or 2.\n");
    }

    if (currency == 1)
    {
        value = amount * selectedCoin->price;
        printf("\nValue in USD = $%.2f\n", value);
    }
    else
    {
        value = amount * selectedCoin->price * PKR_RATE;
        printf("\nValue in PKR = Rs. %.2f\n", value);
    }

    taxAmount = tax(value);

    remaining = value - taxAmount;

    printf("\nValue after tax = %.2f %s\n",
           remaining,
           currency == 1 ? "USD" : "PKR");
}

/* ===================== YOUR WALLET ===================== */

struct Holding
{
    char name[30];
    double amt;
    double money_invested;
};

struct Trade
{
    int type;               /* TRADE_BUY or TRADE_SELL */
    int coinIndex;
    int round;
    double amount;
    double priceUSD;        /* price per coin at the time of the trade */
    double paidPKR;         /* buy: total paid (with tax), sell: total received */
    double realizedPct;     /* sell: profit % on the sale, buy: 0 */
    time_t tradeTime;
};

struct Wallet
{
    int count;
    double cash;
    struct Holding coins[10];
    int tradeCount;
    struct Trade trades[MAX_TRADES];
};

/* Change % of a trade: market price now versus the price at that trade. */
double trade_change(const struct Trade *t)
{
    if (t->type == TRADE_SELL)
        return t->realizedPct;

    return (coins[t->coinIndex].price / t->priceUSD - 1.0) * 100.0;
}

void DisplayWallet(struct Wallet *myWallet)
{
    double total = 0;
    int i;

    printf("\n========== YOUR WALLET ==========\n");

    if (myWallet->count == 0)
    {
        printf("You have not purchased any cryptocurrency yet.\n");
    }

    for (i = 0; i < myWallet->count; i++)
    {
        printf("%s: %.4f coins, Rs. %.2f invested.\n",
               myWallet->coins[i].name,
               myWallet->coins[i].amt,
               myWallet->coins[i].money_invested);

        total += myWallet->coins[i].money_invested;
    }

    printf("\nTotal money invested = Rs. %.2f\n", total);
    printf("Cash balance left = Rs. %.2f\n", myWallet->cash);
}

void your_wallet(struct Wallet *myWallet)
{
    DisplayWallet(myWallet);
}

/* ===================== TRADING HISTORY ===================== */

void trading_history(struct Wallet *myWallet)
{
    int i;
    double nowUSD, change, totalPaid = 0, totalReceived = 0;
    struct Trade *t;

    printf("\n========== TRADING HISTORY ==========\n");

    if (myWallet->tradeCount == 0)
    {
        printf("You have not made any trades yet.\n");
        return;
    }

    printf("%-3s %-5s %-5s %-9s %-10s %-12s %-13s %-12s %-8s\n",
           "#", "Type", "Round", "Coin", "Amount", "Price($)",
           "Rs. Value", "Now($)", "Change");

    for (i = 0; i < myWallet->tradeCount; i++)
    {
        t = &myWallet->trades[i];
        nowUSD = coins[t->coinIndex].price;
        change = trade_change(t);

        printf("%-3d %-5s %-5d %-9s %-10.4f %-12.4f %-13.2f %-12.4f %+.2f%%\n",
               i + 1,
               t->type == TRADE_SELL ? "SELL" : "BUY",
               t->round,
               coins[t->coinIndex].symbol,
               t->amount,
               t->priceUSD,
               t->paidPKR,
               nowUSD,
               change);

        if (t->type == TRADE_SELL)
            totalReceived += t->paidPKR;
        else
            totalPaid += t->paidPKR;
    }

    printf("\n(Change: BUY = market now vs your price, SELL = profit realised on the sale)\n");
    printf("\nTotal trades: %d\n", myWallet->tradeCount);
    printf("Total spent on purchases (including tax): Rs. %.2f\n", totalPaid);
    printf("Total received from sales: Rs. %.2f\n", totalReceived);
    printf("Current market round: %d\n", coins[0].count);
}

/* ===================== SORT HISTORY ===================== */

void sort_by_amount(struct Wallet *myWallet) {
    int i, j;
    struct Trade temp;
    for (i = 0; i < myWallet->tradeCount - 1; i++) {
        for (j = 0; j < myWallet->tradeCount - i - 1; j++) {
            if (myWallet->trades[j].paidPKR < myWallet->trades[j + 1].paidPKR) {
                temp = myWallet->trades[j];
                myWallet->trades[j] = myWallet->trades[j + 1];
                myWallet->trades[j + 1] = temp; }
        }
    }
}

void sort_by_profit_loss(struct Wallet *myWallet) {
    int i, j;
    struct Trade temp;

    for (i = 0; i < myWallet->tradeCount - 1; i++) {
        for (j = 0; j < myWallet->tradeCount - i - 1; j++) {

            if (trade_change(&myWallet->trades[j]) <
                trade_change(&myWallet->trades[j + 1])) {
                temp = myWallet->trades[j];
                myWallet->trades[j] = myWallet->trades[j + 1];
                myWallet->trades[j + 1] = temp; }
        }
    }
}

void sort_by_date(struct Wallet *myWallet) {
    int i, j;
    struct Trade temp;

    for (i = 0; i < myWallet->tradeCount - 1; i++) {
        for (j = 0; j < myWallet->tradeCount - i - 1; j++) {
            if (myWallet->trades[j].tradeTime < myWallet->trades[j + 1].tradeTime) {
                temp = myWallet->trades[j];
                myWallet->trades[j] = myWallet->trades[j + 1];
                myWallet->trades[j + 1] = temp; }
        }
    }
}

void sort_history(struct Wallet *myWallet) {
    int sortChoice;
    int c;

    printf("\n========== SORT HISTORY ==========\n");
    printf("1. Sort by date\n");
    printf("2. Sort by amount invested\n");
    printf("3. Sort by profit/loss\n");
    printf("Enter your choice: ");

    if (scanf("%d", &sortChoice) != 1) {
        printf("Invalid input! Please enter a valid choice.\n");
        while ((c = getchar()) != '\n' && c != EOF)
            ;
        return;
    }

    if (sortChoice < 1 || sortChoice > 3) {
        printf("Invalid choice! Please enter a valid option.\n");
        return;
    }

    switch (sortChoice) {
        case 1:
            sort_by_date(myWallet);
            break;
        case 2:
            sort_by_amount(myWallet);
            break;
        case 3:
            sort_by_profit_loss(myWallet);
            break;
    }

    trading_history(myWallet);
}

/* ===================== DEPOSIT CASH ===================== */

double cash_deposit(double deposit, double balance) {
    double updated_balance = deposit + balance;
    return updated_balance;
}

void deposit_cash(struct Wallet *myWallet) {
    double deposit;
    int c;
    printf("\nCurrent balance = Rs. %.2f\n", myWallet->cash);

    while (1) {
        printf("Enter amount to deposit: ");
        if (scanf("%lf", &deposit) != 1) {
            printf("Invalid input! Enter a number.\n");

            while ((c = getchar()) != '\n' && c != EOF)
                ;

            continue;
        }

        if (isfinite(deposit) && deposit > 0 &&
            myWallet->cash + deposit <= MAX_CASH)
            break;

        printf("Deposit must be greater than zero and not too large.\n");
    }

    myWallet->cash = cash_deposit(deposit, myWallet->cash);

    printf("\nDeposit successful!\n");
    printf("Updated balance = Rs. %.2f\n", myWallet->cash);
}

/* ===================== BUY AND SELL ===================== */

double rupees(double PKR, double amount) {
    return PKR * amount;
}

double dollars(double USD, double amount) {
    return USD * amount;
}

double convertToDollars(double PKR) {
    return PKR / PKR_RATE;
}

int currencyChoice(void) {
    int buy;
    int c;
    printf("\nEnter 1 to buy in rupees or 2 to buy in dollars: ");

    if (scanf("%d", &buy) != 1) {
        printf("Invalid input! Enter a number.\n");

        while ((c = getchar()) != '\n' && c != EOF)
            ;

        return currencyChoice();
    }

    if (buy == 1 || buy == 2)
        return buy;

    printf("Invalid choice! Try again.\n");
    return currencyChoice();
}

void buy_and_sell(struct Wallet *myWallet) {
    int coin_choice, buy, i, c, found = -1;
    double amount, totalCost, taxAmount, finalCost;
    double finalCostPKR, priceUSD, pricePKR;
    struct Trade *t;

    printf("\nEnter the coin you want to buy:\n");

    for (i = 0; i < MAX_COINS; i++) {
        printf("%d. %s (%s)\n",
               i + 1,
               coins[i].name,
               coins[i].symbol);
    }

    while (1) {
        printf("\nEnter your choice: ");

        if (scanf("%d", &coin_choice) != 1) {
            printf("Invalid input! Enter a number.\n");

            while ((c = getchar()) != '\n' && c != EOF)
                ;

            continue;
        }

        if (coin_choice >= 1 && coin_choice <= 10)
            break;

        printf("Invalid selection! Choose between 1 and 10.\n");
    }

    priceUSD = coins[coin_choice - 1].price;
    pricePKR = priceUSD * PKR_RATE;

    printf("\nThe price of %s (%s) is:\n",
           coins[coin_choice - 1].name,
           coins[coin_choice - 1].symbol);

    printf("$%.2f\n", priceUSD);
    printf("Rs. %.2f\n", pricePKR);

    while (1) {
        printf("\nAmount of %s you want to buy: ",
               coins[coin_choice - 1].name);

        if (scanf("%lf", &amount) != 1) {
            printf("Invalid input! Enter a number.\n");

            while ((c = getchar()) != '\n' && c != EOF)
                ;

            continue;
        }

        if (isfinite(amount) && amount > 0 && amount <= MAX_AMOUNT)
            break;

        printf("Amount must be greater than zero and not too large.\n");
    }

    buy = currencyChoice();

    if (buy == 1) {
        totalCost = rupees(pricePKR, amount);
        printf("\nPurchase cost in rupees: Rs. %.2f\n", totalCost);

        taxAmount = tax(totalCost);
        finalCost = totalCost + taxAmount;

        printf("\nTotal amount including tax: Rs. %.2f\n", finalCost);

        finalCostPKR = finalCost;
    }
    else {
        totalCost = dollars(priceUSD, amount);
        printf("\nPurchase cost in dollars: $%.2f\n", totalCost);

        taxAmount = tax(totalCost);
        finalCost = totalCost + taxAmount;

        printf("\nTotal amount including tax: $%.2f\n", finalCost);

        finalCostPKR = finalCost * PKR_RATE;
    }

    if (finalCostPKR > myWallet->cash) {
        printf("\nInsufficient balance!\n");
        printf("Your balance: Rs. %.2f\n", myWallet->cash);
        printf("Required amount: Rs. %.2f\n", finalCostPKR);
        return;
    }

    for (i = 0; i < myWallet->count; i++) {
        if (strcmp(myWallet->coins[i].name,
                   coins[coin_choice - 1].name) == 0) {
            found = i;
            break;
        }
    }

    if (found == -1 && myWallet->count == 10) {
        printf("\nYour wallet is full!\n");
        return;
    }

    if (myWallet->tradeCount == MAX_TRADES) {
        printf("\nTrading history is full!\n");
        return;
    }

    myWallet->cash -= finalCostPKR;

    if (found == -1) {
        found = myWallet->count;

        strcpy(myWallet->coins[found].name,
               coins[coin_choice - 1].name);

        myWallet->coins[found].amt = amount;
        myWallet->coins[found].money_invested = finalCostPKR;
        myWallet->count++;
    }
    else {
        myWallet->coins[found].amt += amount;
        myWallet->coins[found].money_invested += finalCostPKR;
    }

    t = &myWallet->trades[myWallet->tradeCount];

    t->type = TRADE_BUY;
    t->tradeTime = time(NULL);
    t->coinIndex = coin_choice - 1;
    t->round = coins[coin_choice - 1].count;
    t->amount = amount;
    t->priceUSD = priceUSD;
    t->paidPKR = finalCostPKR;
    t->realizedPct = 0.0;

    myWallet->tradeCount++;

    printf("\nPurchase successful!\n");
    printf("Coins purchased: %.4f %s\n",
           amount,
           coins[coin_choice - 1].symbol);

    printf("Amount deducted: Rs. %.2f\n", finalCostPKR);
    printf("Remaining balance: Rs. %.2f\n", myWallet->cash);
}

/* ===================== SELL ===================== */

void sell_coin(struct Wallet *myWallet) {
    int coin_choice, marketIndex = -1, c, i, k, j;
    double amount, money_received, costBasis;
    struct Trade *t;

    if (myWallet->count == 0) {
        printf("\nYou have no coins to sell!\n");
        return;
    }

    if (myWallet->tradeCount == MAX_TRADES) {
        printf("\nTrading history is full!\n");
        return;
    }

    for (i = 0; i < myWallet->count; i++) {
        printf("%d. %s - %.4f coins\n", i + 1,
               myWallet->coins[i].name, myWallet->coins[i].amt);
    }

    while (1) {
        printf("Choose which coin you want to sell from 1-%d: ", myWallet->count);

        if (scanf("%d", &coin_choice) != 1) {
            printf("Invalid input! Enter a number.\n");

            while ((c = getchar()) != '\n' && c != EOF)
                ;

            continue;
        }

        if (coin_choice > 0 && coin_choice <= myWallet->count)
            break;

        printf("Invalid selection\n");
    }

    printf("You chose: %s\n", myWallet->coins[coin_choice - 1].name);

    for (k = 0; k < MAX_COINS; k++) {
        if (strcmp(myWallet->coins[coin_choice - 1].name, coins[k].name) == 0) {
            marketIndex = k;
            break;
        }
    }

    if (marketIndex == -1) {
        printf("Error: Coin not found in market!\n");
        return;
    }

    printf("Current price of %s: $ %.2f\n",
           myWallet->coins[coin_choice - 1].name, coins[marketIndex].price);
    printf("Current price of %s: Rs. %.2f\n",
           myWallet->coins[coin_choice - 1].name, coins[marketIndex].price * PKR_RATE);

    while (1) {
        printf("How many amount of %s you want to sell?\n", coins[marketIndex].name);

        if (scanf("%lf", &amount) != 1) {
            printf("Invalid input! Enter a number.\n");

            while ((c = getchar()) != '\n' && c != EOF)
                ;

            continue;
        }

        if (isfinite(amount) && amount > 0 && amount <= myWallet->coins[coin_choice - 1].amt)
            break;

        printf("Insufficient amount.\nTry again!\n");
    }

    money_received = (coins[marketIndex].price * PKR_RATE) * amount;
    printf("You will receive: Rs. %.2f\n", money_received);

    /* record the sale in the trading history */
    t = &myWallet->trades[myWallet->tradeCount];

    t->type = TRADE_SELL;
    t->tradeTime = time(NULL);
    t->coinIndex = marketIndex;
    t->round = coins[marketIndex].count;
    t->amount = amount;
    t->priceUSD = coins[marketIndex].price;
    costBasis = myWallet->coins[coin_choice - 1].money_invested * (amount / myWallet->coins[coin_choice - 1].amt);
    t->paidPKR = money_received;
    t->realizedPct = costBasis > 0 ? (money_received / costBasis - 1.0) * 100.0 : 0.0;

    myWallet->tradeCount++;

    myWallet->cash += money_received;
    myWallet->coins[coin_choice - 1].money_invested *= 1 - (amount / myWallet->coins[coin_choice - 1].amt);
    myWallet->coins[coin_choice - 1].amt -= amount;

    if (myWallet->coins[coin_choice - 1].amt <= 0.0001) {
        for (j = coin_choice - 1; j < myWallet->count - 1; j++) {
            myWallet->coins[j] = myWallet->coins[j + 1];
        }
        myWallet->count -= 1;
    }

    printf("Coin sold successfully!\n");
    printf("Remaining balance: Rs. %.2f\n", myWallet->cash);
}

int validatePassword(char password[]);
int saveAccount(const char username[], const char password[]);

/* ===================== CHANGE PASSWORD ===================== */

void change_password(char username[], char password[])
{
    char old_password[30];
    char new_password[30];
    char confirm_password[30];

    printf("Change password\n");

    printf("Enter your current password:\n ");

    if (scanf("%29s", old_password) != 1){
        printf("Invalid input! Please enter a valid password.\n");
        return;
    }

    if (strcmp(old_password, password) != 0) {
        printf("Incorrect current password!\n");
    }
    else {
        printf("Enter new password: ");
        if (scanf("%29s", new_password) != 1) {
            printf("Invalid input! Please enter a valid password.\n");
            return;
        }

        printf("Confirm your new password: ");
        if (scanf("%29s", confirm_password) != 1) {
            printf("Invalid input! Please enter a valid password.\n");
            return;
        }

        if (!validatePassword(new_password)){
            printf("Password must be at least 10 characters long and contain at least one uppercase letter, one lowercase letter, and one digit.\n");
            return;
        }

        if (strcmp(new_password, confirm_password) == 0) {
            strcpy(password, new_password);

            if (!saveAccount(username, password))
                printf("Warning: could not save account to %s\n", ACCOUNT_FILE);

            printf("Password changed successfully\n");
        }
        else {
            printf("Passwords do not match\n");
        }
    }
}

/* ===================== FUNCTION DECLARATIONS ===================== */

void drawLine(char graph[GRAPH_ROWS][GRAPH_COLS],
              int x1, int y1, int x2, int y2);

void buildGraph(int coinIndex,
                char graph[GRAPH_ROWS][GRAPH_COLS],
                double *lowOut,
                double *highOut);

void printCellText(const char *text, int isLast);

void showPriceGraph(void);

int initializeHistory(Crypto *coin);

int saveHistory(Crypto *coin);

void updatePrice(Crypto *coin);

void newMarketRound(void);

void freeHistory(void);

int saveToFile(void);

int loadFromFile(void);


void showMarket(void);

void mainmenu(char username[], char password[], struct Wallet *myWallet);

/* ===================== MARKET DISPLAY ===================== */

void showMarket(void)
{
    int i;

    printf("\n========== CRYPTOCURRENCY MARKET ==========\n");

    printf("%-5s %-15s %-10s %-15s %-15s\n",
           "ID", "Name", "Symbol", "Price (USD)", "Price (PKR)");

    for (i = 0; i < MAX_COINS; i++)
    {
        printf("%-5d %-15s %-10s $%-14.2f Rs. %-12.2f\n",
               coins[i].id,
               coins[i].name,
               coins[i].symbol,
               coins[i].price,
               coins[i].price * PKR_RATE);
    }
}

/* ===================== GRAPH ===================== */

void drawLine(char graph[GRAPH_ROWS][GRAPH_COLS],
              int x1, int y1, int x2, int y2)
{
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);

    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    int err = dx - dy;
    int e2;

    char symbol;

    if (dx == 0)
        symbol = '|';
    else if (dy == 0)
        symbol = '-';
    else if ((x2 - x1) * (y2 - y1) < 0)
        symbol = '/';
    else
        symbol = '\\';

    while (x1 != x2 || y1 != y2)
    {
        e2 = 2 * err;

        if (e2 > -dy)
        {
            err -= dy;
            x1 += sx;
        }

        if (e2 < dx)
        {
            err += dx;
            y1 += sy;
        }

        if (x1 != x2 || y1 != y2)
            graph[y1][x1] = symbol;
    }
}

void buildGraph(int coinIndex,
                char graph[GRAPH_ROWS][GRAPH_COLS],
                double *lowOut,
                double *highOut)
{
    int xPoints[GRAPH_COLS], yPoints[GRAPH_COLS];

    int i, j, count, start;

    double minPrice, maxPrice, range, price;

    double *h;

    for (i = 0; i < GRAPH_ROWS; i++)
    {
        for (j = 0; j < GRAPH_COLS; j++)
            graph[i][j] = ' ';
    }

    *lowOut = *highOut = 0.0;

    if (coins[coinIndex].history == NULL ||
        coins[coinIndex].count == 0)
        return;

    start = 0;
    count = coins[coinIndex].count;

    if (count > GRAPH_COLS)
    {
        start = count - GRAPH_COLS;
        count = GRAPH_COLS;
    }

    h = coins[coinIndex].history + start;

    minPrice = maxPrice = h[0];

    for (i = 1; i < count; i++)
    {
        if (h[i] < minPrice)
            minPrice = h[i];

        if (h[i] > maxPrice)
            maxPrice = h[i];
    }

    range = maxPrice - minPrice;

    for (i = 0; i < count; i++)
    {
        price = h[i];

        if (count == 1)
            xPoints[i] = GRAPH_COLS / 2;
        else
            xPoints[i] = i * (GRAPH_COLS - 1) / (count - 1);

        if (range == 0.0)
            yPoints[i] = GRAPH_ROWS / 2;
        else
            yPoints[i] = (int)((maxPrice - price) /
                               range * (GRAPH_ROWS - 1) + 0.5);

        graph[yPoints[i]][xPoints[i]] = '*';
    }

    for (i = 0; i < count - 1; i++)
    {
        drawLine(graph,
                 xPoints[i], yPoints[i],
                 xPoints[i + 1], yPoints[i + 1]);
    }

    for (i = 0; i < count; i++)
        graph[yPoints[i]][xPoints[i]] = '*';

    *lowOut = minPrice;
    *highOut = maxPrice;
}

void printCellText(const char *text, int isLast)
{
    int cellWidth = GRAPH_COLS + 1;

    printf("%-*.*s", cellWidth, cellWidth, text);

    if (!isLast)
        printf("%*s", GRAPH_GAP, "");
}

void showPriceGraph(void)
{
    char graphs[GRAPHS_PER_LINE][GRAPH_ROWS][GRAPH_COLS];

    double lows[GRAPHS_PER_LINE], highs[GRAPHS_PER_LINE];

    char line[64];

    int first, k, n, row, j;

    printf("\n--- CRYPTOCURRENCY LINE GRAPHS ---\n");

    for (first = 0; first < MAX_COINS; first += GRAPHS_PER_LINE)
    {
        n = MAX_COINS - first;

        if (n > GRAPHS_PER_LINE)
            n = GRAPHS_PER_LINE;

        for (k = 0; k < n; k++)
            buildGraph(first + k, graphs[k], &lows[k], &highs[k]);

        putchar('\n');

        for (k = 0; k < n; k++)
        {
            snprintf(line, sizeof(line), "%s (%s)",
                     coins[first + k].name,
                     coins[first + k].symbol);

            printCellText(line, k == n - 1);
        }

        putchar('\n');

        for (k = 0; k < n; k++)
        {
            snprintf(line, sizeof(line), "Now:  $%.4f",
                     coins[first + k].price);

            printCellText(line, k == n - 1);
        }

        putchar('\n');

        for (k = 0; k < n; k++)
        {
            snprintf(line, sizeof(line), "High: $%.4f", highs[k]);

            printCellText(line, k == n - 1);
        }

        putchar('\n');

        for (k = 0; k < n; k++)
        {
            snprintf(line, sizeof(line), "Low:  $%.4f", lows[k]);

            printCellText(line, k == n - 1);
        }

        putchar('\n');

        for (row = 0; row < GRAPH_ROWS; row++)
        {
            for (k = 0; k < n; k++)
            {
                putchar('|');

                for (j = 0; j < GRAPH_COLS; j++)
                    putchar(graphs[k][row][j]);

                if (k < n - 1)
                    printf("%*s", GRAPH_GAP, "");
            }

            putchar('\n');
        }

        for (k = 0; k < n; k++)
        {
            putchar('+');

            for (j = 0; j < GRAPH_COLS; j++)
                putchar('-');

            if (k < n - 1)
                printf("%*s", GRAPH_GAP, "");
        }

        putchar('\n');
    }

    printf("\n(each graph: left = older prices, right = newer prices)\n");
}
/* ===================== PRICE HISTORY ===================== */

int initializeHistory(Crypto *coin)
{
    coin->capacity = INITIAL_CAPACITY;

    coin->history = (double *)malloc(
        coin->capacity * sizeof(double));

    if (coin->history == NULL)
    {
        coin->capacity = 0;
        coin->count = 0;

        return 0;
    }

    coin->history[0] = coin->price;

    coin->count = 1;

    return 1;
}

int increaseHistory(double **history, int newCapacity)
{
    double *temp;

    temp = (double *)realloc(
        *history,
        newCapacity * sizeof(double));

    if (temp == NULL)
        return 0;

    *history = temp;

    return 1;
}

int saveHistory(Crypto *coin)
{
    if (coin->count == coin->capacity)
    {
        int newCapacity = coin->capacity * 2;
        if (!increaseHistory(&((*coin).history),newCapacity))
            return 0;
        coin->capacity = newCapacity;
    }
    coin->history[coin->count] = coin->price;
    coin->count++;
    return 1;
}

void updatePrice(Crypto *coin)
{
    double change = (rand() % 1001 - 500) / 10000.0;

    coin->price *= (1.0 + change);
}

void newMarketRound(void)
{
    int i;

    for (i = 0; i < MAX_COINS; i++)
    {
        updatePrice(&coins[i]);

        if (!saveHistory(&coins[i]))
            printf("Warning: could not save history for %s\n",
                   coins[i].name);
    }

    printf("New market round generated.\n");
}

void freeHistory(void)
{
    int i;

    for (i = 0; i < MAX_COINS; i++)
    {
        free(coins[i].history);

        coins[i].history = NULL;

        coins[i].count = coins[i].capacity = 0;
    }
}

/* ===================== FILE HANDLING ===================== */

int saveToFile(void)
{
    FILE *fp;

    int i, j, ok;

    fp = fopen(SAVE_TMP, "w");

    if (fp == NULL)
        return 0;

    fprintf(fp, "%s %d\n%d\n",
            SAVE_MAGIC, SAVE_VERSION, MAX_COINS);

    for (i = 0; i < MAX_COINS; i++)
    {
        fprintf(fp, "%d %d\n",
                coins[i].id,
                coins[i].count);

        for (j = 0; j < coins[i].count; j++)
            fprintf(fp, "%.17g\n", coins[i].history[j]);
    }

    ok = !ferror(fp);

    if (fclose(fp) != 0)
        ok = 0;

    if (!ok)
    {
        remove(SAVE_TMP);
        return 0;
    }

    remove(SAVE_FILE);

    if (rename(SAVE_TMP, SAVE_FILE) != 0)
        return 0;

    return 1;
}

int loadFromFile(void) {
    FILE *fp;
    char magic[32];
    int version, n, i, j, id, count;
    double value;
    double *tmp[MAX_COINS] = {NULL};
    int tmpCount[MAX_COINS] = {0};

    fp = fopen(SAVE_FILE, "r");

    if (fp == NULL)
        return 0;

    if (fscanf(fp, "%31s %d", magic, &version) != 2 ||
        strcmp(magic, SAVE_MAGIC) != 0 ||
        version != SAVE_VERSION)
        goto fail;

    if (fscanf(fp, "%d", &n) != 1 || n != MAX_COINS)
        goto fail;

    for (i = 0; i < MAX_COINS; i++)
    {
        if (fscanf(fp, "%d %d", &id, &count) != 2)
            goto fail;

        if (id != coins[i].id ||
            count < 1 ||
            count > MAX_SAVED_HISTORY)
            goto fail;

        tmp[i] = (double *)malloc(count * sizeof(double));

        if (tmp[i] == NULL)
            goto fail;

        for (j = 0; j < count; j++)
        {
            if (fscanf(fp, "%lf", &value) != 1 ||
                !(value > 0.0) || !isfinite(value))
                goto fail;

            tmp[i][j] = value;
        }

        tmpCount[i] = count;
    }

    fclose(fp);

    freeHistory();

    for (i = 0; i < MAX_COINS; i++)
    {
        coins[i].history = tmp[i];

        coins[i].count = tmpCount[i];

        coins[i].capacity = tmpCount[i];

        coins[i].price = tmp[i][tmpCount[i] - 1];
    }

    return 1;

fail:

    fclose(fp);

    for (i = 0; i < MAX_COINS; i++)
        free(tmp[i]);

    return 0;
}

int saveWallet(struct Wallet *myWallet) {
    FILE *fp;
    int ok;
    struct Trade *t; 
    fp=fopen(WALLET_TMP,"w");
    if (fp==NULL) 
        return 0;

    fprintf(fp, "%s %d\n",WALLET_MAGIC,WALLET_VERSION);
    fprintf(fp, "%.17g %d\n",myWallet->cash,myWallet->count);

    for(int i=0;i<myWallet->count;i++) {
        fprintf(fp,"%s %.17g %.17g\n",myWallet->coins[i].name,myWallet->coins[i].amt,myWallet->coins[i].money_invested);
    }

    fprintf(fp, "%d\n",myWallet->tradeCount);

    for(int i=0;i<myWallet->tradeCount;i++) {
        t=&myWallet->trades[i];
        fprintf(fp,"%d %d %d %.17g %.17g %.17g %.17g %lld\n",t->type,t->coinIndex,t->round,t->amount,t->priceUSD,t->paidPKR,t->realizedPct,(long long)t->tradeTime);
    }

    ok = !ferror(fp);

    if (fclose(fp) != 0) 
        ok = 0;

    if (!ok) {
        remove(WALLET_TMP);
        return 0;
    }

    remove(WALLET_FILE);

    if (rename(WALLET_TMP, WALLET_FILE) != 0)
        return 0;

    return 1;
}

int loadWallet(struct Wallet *myWallet) {
    FILE *fp;
    char magic [32];
    int version,count,i,tradeCount;
    double cash;
    struct Wallet temp={0};
    long long tt;

    fp=fopen(WALLET_FILE,"r");

    if (fp==NULL) 
        return 0;
    
    if (fscanf(fp,"%31s %d",magic,&version) !=2 ) {
        fclose(fp);
        return 0;
    }

    if(strcmp(magic,WALLET_MAGIC) != 0 || (version != 1 && version != WALLET_VERSION)){
            fclose(fp);
            return 0; }

    if (fscanf(fp,"%lf %d",&cash,&count) !=2) {
        fclose(fp);
        return 0;
    }

    if (count<0 || count>10 || !(cash>=0) || !isfinite(cash)) {
        fclose(fp);
        return 0;
    }
    temp.cash=cash;
    temp.count=count;

    for (i = 0; i < count; i++) {

        if (fscanf(fp, "%29s %lf %lf",
            temp.coins[i].name,
            &temp.coins[i].amt,
            &temp.coins[i].money_invested) != 3) {

            fclose(fp);
            return 0;
        }
        int found = 0;

        for (int j = 0; j < MAX_COINS; j++) {
            if (strcmp(temp.coins[i].name, coins[j].name) == 0) {
                found = 1;
                break;
            }
        }

        if (!found ||
            !(temp.coins[i].amt > 0) || !isfinite(temp.coins[i].amt) ||
            !(temp.coins[i].money_invested >= 0) ||
            !isfinite(temp.coins[i].money_invested)) {
            fclose(fp);
            return 0;
        }

        for (int j = 0; j < i; j++) {
            if (strcmp(temp.coins[i].name, temp.coins[j].name) == 0) {
                fclose(fp);
                return 0;
            }
        }
    }

    if (fscanf(fp,"%d",&tradeCount) != 1 || tradeCount<0 || tradeCount>MAX_TRADES){
        fclose(fp);
        return 0;
    }
    temp.tradeCount=tradeCount;

    for(i=0;i<tradeCount;i++) {
        temp.trades[i].realizedPct = 0.0;
        if (version >= 2) {
            if (fscanf(fp,"%d %d %d %lf %lf %lf %lf %lld",&temp.trades[i].type,&temp.trades[i].coinIndex,&temp.trades[i].round,&temp.trades[i].amount,
                                                &temp.trades[i].priceUSD,&temp.trades[i].paidPKR,&temp.trades[i].realizedPct,&tt) !=8) {
                fclose(fp);
                return 0; }
        }
        else if (fscanf(fp,"%d %d %d %lf %lf %lf %lld",&temp.trades[i].type,&temp.trades[i].coinIndex,&temp.trades[i].round,&temp.trades[i].amount,
                                                &temp.trades[i].priceUSD,&temp.trades[i].paidPKR,&tt) !=7) {
            fclose(fp);
            return 0; }

        if (temp.trades[i].coinIndex<0
            || temp.trades[i].coinIndex >= MAX_COINS || (temp.trades[i].type != 0 && temp.trades[i].type !=1)
            || !(temp.trades[i].priceUSD > 0) || !isfinite(temp.trades[i].priceUSD)
            || !(temp.trades[i].amount > 0) || !isfinite(temp.trades[i].amount)
            || !(temp.trades[i].paidPKR >= 0) || !isfinite(temp.trades[i].paidPKR)
            || !isfinite(temp.trades[i].realizedPct)) {
            fclose(fp);
            return 0; }
        temp.trades[i].tradeTime = (time_t)tt;
    }

    fclose(fp);
    *myWallet = temp;
    return 1;
}


/* ===================== AUTHENTICATION ===================== */


int validateUsername(char username[])
{
    int i;

    if (strlen(username) != 10)
        return 0;

    for (i = 0; username[i] != '\0'; i++)
    {
        if (!isalpha((unsigned char)username[i]))
            return 0;
    }

    return 1;
}

int validatePassword(char password[])
{
    int i;

    int digit = 0, upper = 0, lower = 0;

    if (strlen(password) != 10)
        return 0;

    for (i = 0; password[i] != '\0'; i++)
    {
        if (isdigit((unsigned char)password[i]))
            digit++;

        else if (isupper((unsigned char)password[i]))
            upper++;

        else if (islower((unsigned char)password[i]))
            lower++;
    }

    if (digit > 0 && upper > 0 && lower > 0 &&
        digit + upper + lower == 10)
        return 1;

    return 0;
}

/* Writes the account to a temp file, then renames it over the real file
   so a failed write never destroys an existing account. */
int saveAccount(const char username[], const char password[])
{
    FILE *fp;
    int ok;

    fp = fopen(ACCOUNT_TMP, "w");

    if (fp == NULL)
        return 0;

    fprintf(fp, "%s %d\n%s\n%s\n",
            ACCOUNT_MAGIC, ACCOUNT_VERSION, username, password);

    ok = !ferror(fp);

    if (fclose(fp) != 0)
        ok = 0;

    if (!ok)
    {
        remove(ACCOUNT_TMP);
        return 0;
    }

    remove(ACCOUNT_FILE);

    if (rename(ACCOUNT_TMP, ACCOUNT_FILE) != 0)
        return 0;

    return 1;
}

/* Loads a previously saved account. Returns 1 only if the file exists,
   has the right header, and the stored credentials are still valid. */
int loadAccount(char username[], char password[])
{
    FILE *fp;
    char magic[32];
    char fileUser[30];
    char filePass[30];
    int version;

    fp = fopen(ACCOUNT_FILE, "r");

    if (fp == NULL)
        return 0;

    if (fscanf(fp, "%31s %d", magic, &version) != 2 ||
        strcmp(magic, ACCOUNT_MAGIC) != 0 ||
        version != ACCOUNT_VERSION ||
        fscanf(fp, "%29s %29s", fileUser, filePass) != 2 ||
        !validateUsername(fileUser) ||
        !validatePassword(filePass))
    {
        fclose(fp);
        return 0;
    }

    fclose(fp);

    strcpy(username, fileUser);
    strcpy(password, filePass);

    return 1;
}

int signUp(char username[], char password[])
{
    printf("\n========== SIGN UP ==========\n");

    printf("Create a username (10 letters): ");

    if (scanf("%29s", username) != 1){
        printf("Invalid input! Please enter a valid username.\n");
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
        return 0;
    }

    if (!validateUsername(username))
    {
        printf("Invalid username! It must contain exactly 10 letters.\n");

        return 0;
    }

    printf("Create a password (10 characters): ");

    if (scanf("%29s", password) != 1){
        printf("Invalid input! Please enter a valid password.\n");
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
        return 0;
    }

    if (!validatePassword(password))
    {
        printf("Invalid password! It must contain exactly 10 characters, including an uppercase letter, lowercase letter and digit.\n");

        return 0;
    }

    if (!saveAccount(username, password))
        printf("Warning: could not save account to %s\n", ACCOUNT_FILE);

    printf("\nAccount created successfully! Please log in.\n");

    return 1;
}

int logIn(char username[], char password[])
{
    char enteredUsername[30];

    char enteredPassword[30];

    printf("\n========== LOG IN ==========\n");

    for (int attempts = 1; attempts <= 3; attempts++)
    {
        printf("\nEnter username: \n");

        if (scanf("%29s", enteredUsername) != 1){
            printf("Invalid input! Please enter a valid username.\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF)
                ;
            return 0;
        }

        if (strcmp(enteredUsername, username) == 0 )
        {
            break;
        }

        printf("Incorrect username !\n");

        printf("Attempts remaining: %d\n", 3 - attempts);
    }
    if (strcmp(enteredUsername, username) == 0 ){
        for (int attempt = 1; attempt <= 3; attempt++){

            printf("Enter password: \n");

            if (scanf("%29s", enteredPassword) != 1){
                printf("Invalid input! Please enter a valid password.\n");
                int c;
                while ((c = getchar()) != '\n' && c != EOF)
                    ;
                return 0;
            }

            if (strcmp(enteredPassword, password) == 0){
                printf("\nLogin successful!\n");

                return 1;
            }

            printf("Incorrect password!\n");
            printf("Attempts remaining: %d\n", 3 - attempt);
        }
    }
    printf("\nMaximum login attempts reached.\n");

    return 0;
}

void signUpMenu(char username[], char password[],
                int *accountCreated)
{
    if (*accountCreated == 0)
        *accountCreated = signUp(username, password);

    else
        printf("An account has already been created. Please log in.\n");
}

void logInMenu(char username[], char password[],
               int accountCreated, int *loginSuccessful)
{
    if (accountCreated == 0)
        printf("Please sign up first!");

    else
        *loginSuccessful = logIn(username, password);
}

/* ===================== MAIN ===================== */

int main(void)
{
    int i;
    int choice;
    int accountCreated = 0;
    int loginSuccessful = 0;
    char username[30];
    char password[30];
    struct Wallet myWallet = {0};

    if (loadAccount(username, password))
    {
        accountCreated = 1;
        printf("Existing account found in %s. Please log in.\n",
               ACCOUNT_FILE);
    }

    do
    {
        printf("\n========== CRYPTO SIMULATOR ==========\n");

        printf("1. Sign-up\n");

        printf("2. Log-in\n");

        printf("3. Exit\n");

        printf("Enter your choice: ");

        if (scanf("%d", &choice) != 1)
        {
            int c;

            while ((c = getchar()) != '\n' && c != EOF)
                ;

            choice = 0;
        }

        switch (choice)
        {
            case 1:

                signUpMenu(username, password, &accountCreated);
                break;

            case 2:

                logInMenu(username, password,
                          accountCreated, &loginSuccessful);

                break;

            case 3:

                printf("Exiting program...\n");

                break;

            default:

                printf("Invalid choice!\n");
        }

        if (loginSuccessful == 1)
            break;

    } while (choice != 3);

    if (loginSuccessful == 1)
    {
        srand((unsigned int)time(NULL));

        if (loadFromFile())
        {
            printf("Saved market data loaded from %s.\n\n",
                   SAVE_FILE);
        }
        else
        {
            for (i = 0; i < MAX_COINS; i++)
            {
                if (!initializeHistory(&coins[i]))
                {
                    printf("Out of memory.\n");

                    freeHistory();

                    return 1;
                }
            }

            printf("No valid save file found. Starting a new market.\n\n");
        }


        if (loadWallet(&myWallet))
            printf("Saved wallet loaded from %s.\n\n", WALLET_FILE);     

        mainmenu(username, password, &myWallet);

        if (!saveToFile())
            printf("Warning: could not save data to %s\n", SAVE_FILE);

        if (!saveWallet(&myWallet)) 
            printf("Warning: could not save data to %s\n",WALLET_FILE);

        freeHistory();
    }

    return 0;
}

/* ===================== MAIN MENU ===================== */
/* Order follows the flowchart: options 1-12, loops until 12 (Logout). */

void mainmenu(char username[], char password[], struct Wallet *myWallet)
{
    int mmoption = 0;

    int c;

    do
    {
        printf("\n=== CRYPTO SIMULATOR ===\n"
               "1.  Wallet\n"
               "2.  Deposit cash\n"
               "3.  Market prices\n"
               "4.  Price graph\n"
               "5.  Generate new market round\n"
               "6.  Buy coin\n"
               "7.  Sell coin\n"
               "8.  Stock exchange conversion\n"
               "9.  Trading history\n"
               "10. Sort history\n"
               "11. Change password\n"
               "12. Logout\n");

        printf("\nChoice: ");

        if (scanf("%d", &mmoption) != 1)
            mmoption = 0;

        while ((c = getchar()) != '\n' && c != EOF)
            ;

        switch (mmoption)
        {
            case 1:

                your_wallet(myWallet);
                break;

            case 2:

                deposit_cash(myWallet);
                if (!saveWallet(myWallet)) 
                    printf("Warning: could not save data to %s\n",WALLET_FILE); 
                break;

            case 3:

                showMarket();
                break;

            case 4:

                showPriceGraph();
                break;

            case 5:

                newMarketRound();

                if (!saveToFile())
                    printf("Warning: could not save data to %s\n",
                           SAVE_FILE);

                break;

            case 6:

                buy_and_sell(myWallet);
                if (!saveWallet(myWallet))
                    printf("Warning: could not save data to %s\n", WALLET_FILE);
                break;

            case 7:

                sell_coin(myWallet);
                if (!saveWallet(myWallet))
                    printf("Warning: could not save data to %s\n", WALLET_FILE);
                break;

            case 8:

                stock_exchange();
                break;

            case 9:

                trading_history(myWallet);
                break;

            case 10:

                sort_history(myWallet);
                break;

            case 11:

                change_password(username, password);
                break;

            case 12:

                printf("Logged out.\n");
                break;

            default:
                printf("Invalid Input\n");

        }

    } while (mmoption != 12);
}
