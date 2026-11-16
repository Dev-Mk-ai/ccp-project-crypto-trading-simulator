/* Web backend for the crypto simulator.
   Reuses crypto_simulator.c (market, wallet, account files) and exposes it
   as a tiny JSON API + serves index.html on http://127.0.0.1:8080
   Linux/macOS: gcc server.c -o server -lm
   Windows (MinGW): gcc server.c -o server.exe -lws2_32 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
typedef SOCKET sock_t;
#define closesock closesocket
#else
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
typedef int sock_t;
#define closesock close
#endif

#define main cli_main
#include "crypto_simulator.c"
#undef main

#ifndef MAX_AMOUNT
#define MAX_AMOUNT 1000000000.0
#endif
#ifndef MAX_CASH
#define MAX_CASH 1000000000000.0
#endif
#ifndef WALLET_BACKUP
#define WALLET_BACKUP "wallet.bak"
#endif

#define PORT 8080
#define LOGIN_FAILS 3
#define LOCK_SECONDS 60
#define SELL_EPS 2e-6

static struct Wallet W;
static char user[30], pass[30];
static int hasAccount = 0, loggedIn = 0;
static char out[1 << 17];
static int ol;

static void J(const char *f, ...)
{
    va_list a;
    int r;
    if (ol >= (int)sizeof out - 1) return;
    va_start(a, f);
    r = vsnprintf(out + ol, sizeof out - ol, f, a);
    va_end(a);
    if (r < 0) return;
    ol += r;
    if (ol > (int)sizeof out - 1) ol = (int)sizeof out - 1;
}

static void urldecode(char *s)
{
    char *o = s;
    while (*s) {
        if (*s == '+') {
            *o++ = ' ';
            s++;
        } else if (*s == '%' && isxdigit((unsigned char)s[1]) && isxdigit((unsigned char)s[2])) {
            char h[3];
            h[0] = s[1]; h[1] = s[2]; h[2] = 0;
            *o++ = (char)strtol(h, NULL, 16);
            s += 3;
        } else {
            *o++ = *s++;
        }
    }
    *o = 0;
}

/* read key from "a=1&b=2" style text (values are url-decoded) */
static int P(const char *b, const char *k, char *v, int n)
{
    size_t kl = strlen(k);
    const char *p = b;
    while (p && *p) {
        if (!strncmp(p, k, kl) && p[kl] == '=') {
            int i = 0;
            p += kl + 1;
            while (*p && *p != '&' && i < n - 1) v[i++] = *p++;
            v[i] = 0;
            urldecode(v);
            return 1;
        }
        p = strchr(p, '&');
        if (p) p++;
    }
    v[0] = 0;
    return 0;
}

static int num(const char *b, const char *k, double max, double *outv)
{
    char v[64], *end;
    double x;
    if (!P(b, k, v, sizeof v)) return 0;
    errno = 0;
    x = strtod(v, &end);
    if (end == v || *end != '\0' || errno != 0 || !isfinite(x) || !(x > 0) || x > max)
        return 0;
    *outv = x;
    return 1;
}

static int hfind(const char *n)
{
    for (int i = 0; i < W.count; i++)
        if (!strcmp(W.coins[i].name, n)) return i;
    return -1;
}

static void addTrade(int type, int ci, double amt, double pkr, double pct)
{
    struct Trade *t;
    if (W.tradeCount == MAX_TRADES) {
        memmove(&W.trades[0], &W.trades[1], (MAX_TRADES - 1) * sizeof W.trades[0]);
        W.tradeCount = MAX_TRADES - 1;
    }
    t = &W.trades[W.tradeCount++];
    t->type = type;
    t->coinIndex = ci;
    t->round = coins[ci].count;
    t->amount = amt;
    t->priceUSD = coins[ci].price;
    t->paidPKR = pkr;
    t->realizedPct = pct;
    t->tradeTime = time(NULL);
}

static void state(const char *msg, int ok)
{
    int i, j, s;
    ol = 0;
    J("{\"ok\":%d,\"msg\":\"%s\",\"hasAccount\":%d,\"loggedIn\":%d,\"user\":\"%s\"",
      ok, msg, hasAccount, loggedIn, loggedIn ? user : "");
    if (loggedIn) {
        J(",\"rate\":%.2f,\"round\":%d,\"cash\":%.2f,\"coins\":[", PKR_RATE, coins[0].count, W.cash);
        for (i = 0; i < MAX_COINS; i++) {
            J("%s{\"id\":%d,\"name\":\"%s\",\"sym\":\"%s\",\"price\":%.6f,\"h\":[",
              i ? "," : "", coins[i].id, coins[i].name, coins[i].symbol, coins[i].price);
            s = coins[i].count > 60 ? coins[i].count - 60 : 0;
            for (j = s; j < coins[i].count; j++)
                J("%s%.6f", j > s ? "," : "", coins[i].history[j]);
            J("]}");
        }
        J("],\"hold\":[");
        for (i = 0; i < W.count; i++)
            J("%s{\"name\":\"%s\",\"amt\":%.6f,\"inv\":%.2f}", i ? "," : "",
              W.coins[i].name, W.coins[i].amt, W.coins[i].money_invested);
        J("],\"trades\":[");
        for (i = 0; i < W.tradeCount; i++) {
            struct Trade *t = &W.trades[i];
            J("%s{\"t\":\"%s\",\"s\":\"%s\",\"r\":%d,\"a\":%.6f,\"p\":%.6f,\"k\":%.2f,\"c\":%.4f,\"d\":%lld}",
              i ? "," : "", t->type == TRADE_SELL ? "SELL" : "BUY",
              coins[t->coinIndex].symbol, t->round, t->amount, t->priceUSD,
              t->paidPKR, trade_change(t), (long long)t->tradeTime);
        }
        J("]");
    }
    J("}");
}

static const char *act(const char *a, const char *b, int *ok)
{
    char v[64], w[64];
    double x, cost, rec, held;
    int id, h, i, sv, before;
    static int fails = 0;
    static time_t lockUntil = 0;

    *ok = 0;
    if (!strcmp(a, "state")) { *ok = 1; return ""; }

    if (!strcmp(a, "signup")) {
        char u[30] = "", p[30] = "";
        if (hasAccount) return "An account already exists. Please log in.";
        P(b, "u", u, 30); P(b, "p", p, 30);
        if (!validateUsername(u)) return "Username must be exactly 10 letters.";
        if (!validatePassword(p)) return "Password: 10 letters or digits with upper, lower and a digit.";
        if (!saveAccount(u, p)) return "Could not save the account to disk. Account was not created.";
        strcpy(user, u); strcpy(pass, p); hasAccount = 1;
        *ok = 1; return "Account created. Please log in.";
    }
    if (!strcmp(a, "login")) {
        if (!hasAccount) return "No account yet. Sign up first.";
        if (time(NULL) < lockUntil) return "Too many failed attempts. Try again in a minute.";
        P(b, "u", v, 64); P(b, "p", w, 64);
        if (strcmp(v, user) || strcmp(w, pass)) {
            if (++fails >= LOGIN_FAILS) {
                fails = 0;
                lockUntil = time(NULL) + LOCK_SECONDS;
                return "Too many failed attempts. Locked for 60 seconds.";
            }
            return "Incorrect username or password.";
        }
        fails = 0; loggedIn = 1; *ok = 1; return "Welcome back.";
    }
    if (!loggedIn) return "Please log in.";

    if (!strcmp(a, "logout")) {
        loggedIn = 0;
        sv = saveToFile();
        if (!saveWallet(&W)) sv = 0;
        *ok = 1;
        return sv ? "Logged out." : "Logged out, but some data could not be saved to disk.";
    }

    if (!strcmp(a, "deposit")) {
        if (!num(b, "amt", MAX_CASH, &x)) return "Enter a valid deposit amount.";
        if (W.cash + x > MAX_CASH) return "Deposit would exceed the maximum balance.";
        W.cash += x;
        sv = saveWallet(&W);
        *ok = sv;
        return sv ? "Deposit successful." : "Deposit done, but wallet.txt could not be saved.";
    }
    if (!strcmp(a, "round")) {
        before = coins[0].count;
        newMarketRound();
        if (coins[0].count == before) return "Could not generate a new round (out of memory).";
        sv = saveToFile();
        *ok = sv;
        return sv ? "New market round generated." : "New round generated, but market.txt could not be saved.";
    }
    if (!strcmp(a, "password")) {
        P(b, "o", v, 64); P(b, "n", w, 64);
        if (strcmp(v, pass)) return "Current password is incorrect.";
        if (!validatePassword(w)) return "Password: 10 letters or digits with upper, lower and a digit.";
        if (!saveAccount(user, w)) return "Could not save the new password. Password was not changed.";
        strcpy(pass, w);
        *ok = 1; return "Password changed.";
    }
    if (!strcmp(a, "buy") || !strcmp(a, "sell")) {
        P(b, "coin", v, 64); id = (int)strtol(v, NULL, 10);
        if (id < 1 || id > MAX_COINS || !num(b, "amt", MAX_AMOUNT, &x))
            return "Enter a valid coin and amount.";
        i = id - 1;
        h = hfind(coins[i].name);
        if (a[0] == 'b') {
            cost = coins[i].price * PKR_RATE * x * 1.05;
            if (!isfinite(cost) || cost > W.cash) return "Insufficient balance.";
            if (h < 0 && W.count == MAX_COINS) return "Your wallet is full.";
            W.cash -= cost;
            if (h < 0) {
                h = W.count++;
                strcpy(W.coins[h].name, coins[i].name);
                W.coins[h].amt = W.coins[h].money_invested = 0;
            }
            W.coins[h].amt += x;
            W.coins[h].money_invested += cost;
            addTrade(TRADE_BUY, i, x, cost, 0);
            sv = saveWallet(&W);
            *ok = sv;
            return sv ? "Purchase complete (5% tax included)." : "Purchase done, but wallet.txt could not be saved.";
        }
        if (h < 0) return "You do not hold that coin.";
        held = W.coins[h].amt;
        if (x > held + SELL_EPS) return "You do not hold that much.";
        if (x >= held - SELL_EPS) x = held;
        rec = coins[i].price * PKR_RATE * x;
        cost = W.coins[h].money_invested * (x / held);
        addTrade(TRADE_SELL, i, x, rec, cost > 0 ? (rec / cost - 1.0) * 100.0 : 0.0);
        W.cash += rec;
        if (x >= held) {
            for (; h < W.count - 1; h++) W.coins[h] = W.coins[h + 1];
            W.count--;
        } else {
            W.coins[h].money_invested -= cost;
            W.coins[h].amt = held - x;
        }
        sv = saveWallet(&W);
        *ok = sv;
        return sv ? "Sale complete." : "Sale done, but wallet.txt could not be saved.";
    }
    return "Unknown action.";
}

static void sendAll(sock_t s, const char *d, int n)
{
    while (n > 0) {
        int r = (int)send(s, d, n, 0);
        if (r <= 0) return;
        d += r; n -= r;
    }
}

static void reply(sock_t s, const char *code, const char *type, const char *body, int n)
{
    char h[300];
    int k = snprintf(h, sizeof h,
        "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nCache-Control: no-store\r\n"
        "X-Content-Type-Options: nosniff\r\nX-Frame-Options: DENY\r\nConnection: close\r\n\r\n",
        code, type, n);
    sendAll(s, h, k);
    sendAll(s, body, n);
}

static void replyText(sock_t s, const char *code, const char *msg)
{
    reply(s, code, "text/plain", msg, (int)strlen(msg));
}

static int ieqn(const char *a, const char *b, size_t n)
{
    for (; n; n--, a++, b++) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        if (!*a) return 1;
    }
    return 1;
}

static int hdr(const char *buf, const char *name, char *o, int n)
{
    size_t nl = strlen(name);
    const char *p = strstr(buf, "\r\n");
    while (p) {
        p += 2;
        if (p[0] == '\r' || p[0] == '\0') break;
        if (ieqn(p, name, nl) && p[nl] == ':') {
            int i = 0;
            p += nl + 1;
            while (*p == ' ') p++;
            while (*p && *p != '\r' && i < n - 1) o[i++] = *p++;
            o[i] = 0;
            return 1;
        }
        p = strstr(p, "\r\n");
    }
    o[0] = 0;
    return 0;
}

static int readReq(sock_t s, char *buf, int cap, char **body)
{
    int n = 0, need = 0;
    char *he = NULL;
    while (n < cap - 1) {
        int r = (int)recv(s, buf + n, cap - 1 - n, 0);
        if (r <= 0) break;
        n += r;
        buf[n] = 0;
        if (!he) {
            he = strstr(buf, "\r\n\r\n");
            if (he) {
                char cl[16];
                need = hdr(buf, "Content-Length", cl, sizeof cl) ? atoi(cl) : 0;
                if (need < 0 || need > cap - 1 - (int)(he + 4 - buf)) return 0;
            }
        }
        if (he && n - (int)(he + 4 - buf) >= need) break;
    }
    if (!he || n - (int)(he + 4 - buf) < need) return 0;
    *body = he + 4;
    return 1;
}

static int hostOk(const char *h)
{
    char a[40], b[40];
    snprintf(a, sizeof a, "127.0.0.1:%d", PORT);
    snprintf(b, sizeof b, "localhost:%d", PORT);
    return !strcmp(h, a) || !strcmp(h, b);
}

static int originOk(const char *o)
{
    char a[60], b[60];
    snprintf(a, sizeof a, "http://127.0.0.1:%d", PORT);
    snprintf(b, sizeof b, "http://localhost:%d", PORT);
    return !strcmp(o, a) || !strcmp(o, b);
}

static void handle(sock_t s)
{
    static char buf[16384];
    char m[8] = "", p[128] = "", host[64], org[64];
    char *body = NULL;

    if (!readReq(s, buf, sizeof buf, &body)) {
        replyText(s, "400 Bad Request", "Bad request");
        return;
    }
    sscanf(buf, "%7s %127s", m, p);

    hdr(buf, "Host", host, sizeof host);
    if (!hostOk(host)) { replyText(s, "403 Forbidden", "Forbidden"); return; }
    if (hdr(buf, "Origin", org, sizeof org) && !originOk(org)) {
        replyText(s, "403 Forbidden", "Forbidden");
        return;
    }

    if (!strncmp(p, "/api/", 5)) {
        int ok;
        const char *msg;
        if (strcmp(m, "POST")) { replyText(s, "405 Method Not Allowed", "POST only"); return; }
        msg = act(p + 5, body, &ok);
        state(msg, ok);
        reply(s, "200 OK", "application/json", out, ol);
    } else if (!strcmp(m, "GET") && (!strcmp(p, "/") || !strcmp(p, "/index.html"))) {
        FILE *f = fopen("index.html", "rb");
        long len;
        char *data;
        if (!f) { replyText(s, "404 Not Found", "index.html not found next to the server"); return; }
        fseek(f, 0, SEEK_END); len = ftell(f); fseek(f, 0, SEEK_SET);
        data = len >= 0 ? (char *)malloc((size_t)len + 1) : NULL;
        if (data && fread(data, 1, (size_t)len, f) == (size_t)len)
            reply(s, "200 OK", "text/html; charset=utf-8", data, (int)len);
        else
            replyText(s, "500 Internal Server Error", "Could not read index.html");
        free(data); fclose(f);
    } else {
        replyText(s, "404 Not Found", "Not found");
    }
}

int main(void)
{
    sock_t srv, c;
    struct sockaddr_in addr;
    int one = 1, i, marketLoaded;
    FILE *wf;

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { printf("Winsock failed to start.\n"); return 1; }
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    srand((unsigned int)time(NULL));

    marketLoaded = loadFromFile();
    if (!marketLoaded) {
        for (i = 0; i < MAX_COINS; i++)
            if (!initializeHistory(&coins[i])) { printf("Out of memory.\n"); return 1; }
        if (!saveToFile()) printf("Warning: could not save data to %s\n", SAVE_FILE);
        wf = fopen(WALLET_FILE, "r");
        if (wf) {
            fclose(wf);
            remove(WALLET_BACKUP);
            if (rename(WALLET_FILE, WALLET_BACKUP) == 0)
                printf("Market data was missing, so the old wallet was moved to %s.\n", WALLET_BACKUP);
        }
    } else if (loadWallet(&W)) {
        printf("Saved wallet loaded from %s.\n", WALLET_FILE);
    }
    hasAccount = loadAccount(user, pass);

    srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv == (sock_t)-1) { printf("Could not create a socket.\n"); return 1; }
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof one);
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(PORT);
    if (bind(srv, (struct sockaddr *)&addr, sizeof addr) != 0 || listen(srv, 8) != 0) {
        printf("Could not listen on port %d (already in use?).\n", PORT);
        return 1;
    }
    printf("Crypto Simulator running at http://localhost:%d  (Ctrl+C to stop)\n", PORT);

    for (;;) {
#ifdef _WIN32
        int tv = 5000;
#else
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
#endif
        c = accept(srv, NULL, NULL);
        if (c == (sock_t)-1) continue;
        setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);
        handle(c);
        closesock(c);
    }
}
