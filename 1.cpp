/*
 * 凡人修仙传 · 五子棋（人机对战）
 * 使用 EasyX 图形库开发
 * 功能：完善账号系统、强化AI、丰富规则、修仙风格界面
 * 编译：g++ 1.cpp -o 1.exe -leasyx -lgdi32 -limm32 -lmsimg32 -lole32 -loleaut32 -finput-charset=UTF-8 -fexec-charset=UTF-8
 */

#include <graphics.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <windows.h>

#define BOARD_SIZE 15
#define CELL_SIZE  48
#define MARGIN     55
#define WIN_WIDTH  1000
#define WIN_HEIGHT 820
#define BTN_W      260
#define BTN_H      52
#define MAX_HISTORY (BOARD_SIZE * BOARD_SIZE)
#define MODE_GOMOKU   0
#define MODE_CONNECT6 1
#define SAVE_VERSION  20260518
#define CHALLENGE_COUNT 3
#define ADMIN_KEY_FILE "admin_key.txt"
#define DEFAULT_RESET_PASSWORD "123456"

/* ===================== 修仙古风配色 ===================== */
COLORREF LINE_COLOR   = RGB(180, 160, 120);  // 古铜金线
COLORREF BLACK_CHESS  = RGB(25, 25, 25);     // 玄黑子
COLORREF WHITE_CHESS  = RGB(245, 245, 240);  // 玉白子
COLORREF TEXT_GOLD    = RGB(255, 215, 0);    // 纯金文字
COLORREF TEXT_WHITE   = RGB(240, 240, 230);  // 玉白文字
COLORREF ACCENT_GOLD  = RGB(255, 195, 50);   // 亮金
COLORREF BTN_BG       = RGB(45, 38, 30);     // 深木底
COLORREF BTN_HOVER    = RGB(85, 68, 48);     // 悬停木色
COLORREF BOARD_BG     = RGB(75, 65, 55);     // 棋盘底 暖木色
COLORREF LAST_MOVE    = RGB(255, 60, 60);    // 落子标记
COLORREF PANEL_BG     = RGB(15, 15, 22);     // 信息面板底
COLORREF PANEL_BORDER = RGB(160, 140, 100);  // 面板边框

/* ===================== 背景图片 ===================== */
IMAGE imgLogin, imgMenu, imgGame, imgWin;

/* ===================== 数据结构 ===================== */
struct Move { int x, y; };

struct UserInfo {
    char name[64];
    char pwdHash[33];
    int exp;
    int level;
    long long regTime;
};

struct GameSave {
    int board[BOARD_SIZE][BOARD_SIZE];
    Move history[MAX_HISTORY];
    int historyCount;
    int difficulty;
    int useForbidden;
    int playerColor;
    int currentTurn;
    int gameOver;
    int result;
    int version;
    int gameMode;
    int showHints;
    int turnPlaced;
    char owner[64];
    long long saveTime;
};

struct GameSaveV1 {
    int board[BOARD_SIZE][BOARD_SIZE];
    Move history[MAX_HISTORY];
    int historyCount;
    int difficulty;
    int useForbidden;
    int playerColor;
    int currentTurn;
    int gameOver;
    int result;
    int version;
    int gameMode;
    int showHints;
    int turnPlaced;
};

struct LegacyGameSave {
    int board[BOARD_SIZE][BOARD_SIZE];
    Move history[MAX_HISTORY];
    int historyCount;
    int difficulty;
    int useForbidden;
    int playerColor;
    int currentTurn;
    int gameOver;
    int result;
};

struct Button {
    int x, y, w, h;
    const wchar_t* text;
};

/* ===================== 全局变量 ===================== */
int board[BOARD_SIZE][BOARD_SIZE];
Move history[MAX_HISTORY];
int historyCount = 0;
char currentUser[64] = "";
int currentUserLevel = 1;
int currentUserExp = 0;
int difficulty = 2;
int useForbidden = 0;
int playerColor = 1;
int aiColor = 2;
int soundOn = 1;
int gameRunning = 0;
int stepCount = 0;
DWORD stepStartTime = 0;
int timeLimit = 0;
int aiThinking = 0;
int gameMode = MODE_GOMOKU;
int showHints = 1;
int turnPlacedThisRound = 0;
int canvasWidth = WIN_WIDTH;
int canvasHeight = WIN_HEIGHT;
int viewOffsetX = 0;
int viewOffsetY = 0;
int viewDrawW = WIN_WIDTH;
int viewDrawH = WIN_HEIGHT;
int challengeMode = 0;
int challengeIndex = 0;
int challengeMoveLimit = 0;
int challengePlayerMoves = 0;
int challengeBaseHistory = 0;
int challengeBasePlayerStones = 0;

const wchar_t* challengeNames[CHALLENGE_COUNT] = {
    L"残局·双活杀", L"残局·攻守劫", L"残局·乱战局"
};

const wchar_t* challengeGoals[CHALLENGE_COUNT] = {
    L"两手内破双线杀，默认不显示提示。",
    L"白棋干扰更多，关键落点更隐蔽。",
    L"乱战中找唯一攻势，容错更低。"
};

int challengeLimits[CHALLENGE_COUNT] = {2, 2, 2};
int challengeNoiseMin[CHALLENGE_COUNT] = {18, 26, 34};
int challengeNoiseMax[CHALLENGE_COUNT] = {26, 36, 46};

int checkWin(int x, int y, int color);
int isBoardFull();
int evaluatePointAdvanced(int x, int y, int color);

/* ===================== 工具函数 ===================== */
unsigned int simpleHash(const char* str) {
    unsigned int h = 0;
    while (*str) {
        h = (h << 5) + h + (unsigned char)(*str++);
    }
    return h;
}

void hashPassword(const char* pwd, const char* salt, char* out) {
    unsigned int h1 = simpleHash(pwd);
    unsigned int h2 = simpleHash(salt);
    unsigned int h3 = h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    unsigned int h4 = h3 ^ 0xA5A5A5A5;
    for (int i = 0; i < 8; i++) {
        unsigned int v = (h3 >> (i * 4)) & 0xF;
        out[i] = (v < 10) ? ('0' + v) : ('A' + v - 10);
    }
    for (int i = 0; i < 8; i++) {
        unsigned int v = (h4 >> (i * 4)) & 0xF;
        out[8 + i] = (v < 10) ? ('0' + v) : ('A' + v - 10);
    }
    out[16] = 0;
}

void makeStoredPassword(const char* pwd, const char* salt, char* out) {
    char oldHash[33] = {0};
    hashPassword(pwd, salt, oldHash);
    snprintf(out, 33, "H1_%s", oldHash);
    out[32] = 0;
}

void asciiToWchar(const char* src, wchar_t* dst, int maxLen) {
    if (!src || !dst || maxLen <= 0) return;
    dst[0] = 0;
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, src, -1, dst, maxLen);
    if (n <= 0) n = MultiByteToWideChar(CP_ACP, 0, src, -1, dst, maxLen);
    if (n <= 0) {
        int i;
        for (i = 0; i < maxLen - 1 && src[i]; i++) dst[i] = (unsigned char)src[i];
        dst[i] = 0;
    } else {
        dst[maxLen - 1] = 0;
    }
}

int inputCharCount(const char* src) {
    wchar_t tmp[128];
    if (MultiByteToWideChar(CP_ACP, 0, src, -1, tmp, 128) <= 0) asciiToWchar(src, tmp, 128);
    return (int)wcslen(tmp);
}

void removeLastInputChar(char* buffer, int* len) {
    if (!buffer || !len || *len <= 0) return;
    (*len)--;
    while (*len > 0 && (((unsigned char)buffer[*len] & 0xC0) == 0x80)) (*len)--;
    if (*len > 0 && ((unsigned char)buffer[*len] >= 0x80) && ((unsigned char)buffer[*len - 1] >= 0x80)) {
        (*len)--;
    }
    buffer[*len] = 0;
}

void playSoundEffect(int type) {
    if (!soundOn) return;
    if (type == 0) {
        Beep(1200, 60);
    } else if (type == 1) {
        Beep(880, 150);
        Beep(1100, 150);
        Beep(1320, 300);
    } else if (type == 2) {
        Beep(300, 300);
        Beep(250, 400);
    }
}

void safeCopy(char* dst, const char* src, int maxLen) {
    if (maxLen <= 0) return;
    strncpy(dst, src, maxLen - 1);
    dst[maxLen - 1] = 0;
}

void inputToStoredText(const char* src, char* dst, int maxLen) {
    if (!src || !dst || maxLen <= 0) return;
    dst[0] = 0;
    wchar_t wide[128];
    int wn = MultiByteToWideChar(CP_ACP, 0, src, -1, wide, 128);
    if (wn <= 0) {
        safeCopy(dst, src, maxLen);
        return;
    }
    int n = WideCharToMultiByte(CP_UTF8, 0, wide, -1, dst, maxLen, NULL, NULL);
    if (n <= 0) safeCopy(dst, src, maxLen);
    else dst[maxLen - 1] = 0;
}

int isGuestUser() {
    return currentUser[0] == 0 || strcmp(currentUser, "Guest") == 0;
}

void trimLineEnd(char* text) {
    if (!text) return;
    int len = (int)strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r' ||
                       text[len - 1] == ' ' || text[len - 1] == '\t')) {
        text[--len] = 0;
    }
}

void buildUserSavePath(const char* name, char* path, int maxLen) {
    const char* owner = (name && name[0]) ? name : "Guest";
    snprintf(path, maxLen, "save_%08X.dat", simpleHash(owner));
    path[maxLen - 1] = 0;
}

void ensureAdminKeyFile() {
    FILE* fp = fopen(ADMIN_KEY_FILE, "r");
    if (fp) {
        fclose(fp);
        return;
    }
    fp = fopen(ADMIN_KEY_FILE, "w");
    if (fp) fclose(fp);
}

void loadAdminKey(char* out, int maxLen) {
    if (!out || maxLen <= 0) return;
    out[0] = 0;
    ensureAdminKeyFile();
    FILE* fp = fopen(ADMIN_KEY_FILE, "r");
    if (fp) {
        if (fgets(out, maxLen, fp)) trimLineEnd(out);
        fclose(fp);
    }
}

int isValidUsername(const char* name) {
    int len = (int)strlen(name);
    if (len < 1 || len > 63) return 0;
    if (strcmp(name, "Guest") == 0) return 0;

    wchar_t wname[64];
    asciiToWchar(name, wname, 64);
    int chars = (int)wcslen(wname);
    if (chars < 1 || chars > 16) return 0;
    for (int i = 0; i < chars; i++) {
        wchar_t ch = wname[i];
        if (ch <= 32 || ch == L'/' || ch == L'\\' || ch == L':' || ch == L'*' ||
            ch == L'?' || ch == L'"' || ch == L'<' || ch == L'>' || ch == L'|') return 0;
        if (ch < 128 && !isalnum((unsigned char)ch) && ch != L'_') return 0;
    }
    return 1;
}

int isValidPassword(const char* pwd) {
    int len = (int)strlen(pwd);
    if (len < 4 || len > 31) return 0;
    int hasVisible = 0;
    for (int i = 0; i < len; i++) {
        if (pwd[i] != ' ') hasVisible = 1;
    }
    return hasVisible;
}

const wchar_t* getModeName() {
    return (gameMode == MODE_CONNECT6) ? L"六子棋" : L"五子棋";
}

int getWinLength() {
    return (gameMode == MODE_CONNECT6) ? 6 : 5;
}

int getTurnMoveNeed(int color) {
    if (gameMode != MODE_CONNECT6) return 1;
    if (historyCount == 0 && color == 1) return 1;
    return 2;
}

const wchar_t* getThreatName(int score) {
    if (score >= 10000000) return L"一手成阵";
    if (score >= 1000000) return L"临门成势";
    if (score >= 100000) return L"强攻";
    if (score >= 10000) return L"可造势";
    if (score >= 1000) return L"可布局";
    return L"平稳";
}

int toLogicalX(int x) { return x - viewOffsetX; }
int toLogicalY(int y) { return y - viewOffsetY; }

int hitButton(const Button* btn, int x, int y) {
    return x >= btn->x && x <= btn->x + btn->w && y >= btn->y && y <= btn->y + btn->h;
}

void updateOrigin() {
    setorigin(viewOffsetX, viewOffsetY);
}

void lockWindowSize() {
    HWND hwnd = GetHWnd();
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style &= ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
    SetWindowLongPtr(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    canvasWidth = WIN_WIDTH;
    canvasHeight = WIN_HEIGHT;
    viewOffsetX = 0;
    viewOffsetY = 0;
    viewDrawW = WIN_WIDTH;
    viewDrawH = WIN_HEIGHT;
    updateOrigin();
}

/* ===================== 修仙境界 ===================== */
const wchar_t* getXianLevelName(int exp) {
    if (exp < 50)  return L"炼气初期";
    if (exp < 100) return L"炼气中期";
    if (exp < 150) return L"炼气后期";
    if (exp < 250) return L"筑基初期";
    if (exp < 350) return L"筑基中期";
    if (exp < 500) return L"筑基后期";
    if (exp < 700) return L"结丹初期";
    if (exp < 900) return L"结丹中期";
    if (exp < 1200) return L"结丹后期";
    if (exp < 1500) return L"元婴初期";
    if (exp < 1900) return L"元婴中期";
    if (exp < 2400) return L"元婴后期";
    if (exp < 3000) return L"化神初期";
    if (exp < 3700) return L"化神中期";
    if (exp < 4500) return L"化神后期";
    if (exp < 5400) return L"炼虚初期";
    if (exp < 6400) return L"炼虚中期";
    if (exp < 7500) return L"炼虚后期";
    if (exp < 8700) return L"合体初期";
    if (exp < 10000) return L"合体中期";
    if (exp < 12000) return L"合体后期";
    if (exp < 15000) return L"大乘初期";
    if (exp < 20000) return L"大乘中期";
    if (exp < 30000) return L"大乘后期";
    if (exp < 50000) return L"渡劫期";
    return L"真仙";
}

/* ===================== 用户与文件系统 ===================== */
void normalizeUserInfo(UserInfo* u) {
    if (!u) return;
    if (u->exp < 0) u->exp = 0;
    int computedLevel = 1 + u->exp / 100;
    if (u->level < computedLevel) u->level = computedLevel;
    if (u->level < 1) u->level = 1;
    if (u->regTime <= 0) u->regTime = (long long)time(NULL);
}

void saveUser(const UserInfo* u) {
    UserInfo fixed = *u;
    normalizeUserInfo(&fixed);
    FILE* fp = fopen("users.dat", "a");
    if (fp) {
        fprintf(fp, "%s %s %d %d %lld\n", fixed.name, fixed.pwdHash, fixed.exp, fixed.level, fixed.regTime);
        fclose(fp);
    }
}

int loadUsers(UserInfo out[], int maxCount) {
    FILE* fp = fopen("users.dat", "r");
    if (!fp) return 0;
    int count = 0;
    while (count < maxCount) {
        char line[256];
        if (!fgets(line, sizeof(line), fp)) break;
        UserInfo tmp;
        if (sscanf(line, "%63s %32s %d %d %lld", tmp.name, tmp.pwdHash, &tmp.exp, &tmp.level, &tmp.regTime) == 5) {
            normalizeUserInfo(&tmp);
            out[count++] = tmp;
        } else if (sscanf(line, "%63s %32s", tmp.name, tmp.pwdHash) == 2) {
            tmp.exp = 0; tmp.level = 1; tmp.regTime = 0;
            normalizeUserInfo(&tmp);
            out[count++] = tmp;
        }
    }
    fclose(fp);
    return count;
}

int findUser(const char* name, UserInfo* out) {
    UserInfo users[200];
    int n = loadUsers(users, 200);
    int found = 0;
    for (int i = 0; i < n; i++) {
        if (strcmp(users[i].name, name) == 0) {
            if (out) *out = users[i];
            found = 1;
        }
    }
    return found;
}

void updateUser(const UserInfo* u) {
    UserInfo users[200];
    int n = loadUsers(users, 200);
    UserInfo fixed = *u;
    normalizeUserInfo(&fixed);
    int found = 0;
    for (int i = 0; i < n; i++) {
        if (strcmp(users[i].name, fixed.name) == 0) {
            users[i] = fixed;
            found = 1;
        }
    }
    if (!found) {
        if (n >= 200) return;
        users[n++] = fixed;
    }
    FILE* fp = fopen("users.dat", "w");
    if (!fp) return;
    for (int i = 0; i < n; i++) {
        fprintf(fp, "%s %s %d %d %lld\n", users[i].name, users[i].pwdHash, users[i].exp, users[i].level, users[i].regTime);
    }
    fclose(fp);
}

int verifyUserPassword(UserInfo* u, const char* pwd) {
    char newHash[33] = {0};
    char legacyHash[33] = {0};
    makeStoredPassword(pwd, u->name, newHash);
    hashPassword(pwd, u->name, legacyHash);
    if (strcmp(u->pwdHash, newHash) == 0) return 1;

    // 兼容旧 users.dat：原哈希或早期明文密码登录成功后自动升级。
    if (strcmp(u->pwdHash, legacyHash) == 0 || strcmp(u->pwdHash, pwd) == 0) {
        safeCopy(u->pwdHash, newHash, 33);
        updateUser(u);
        return 1;
    }
    return 0;
}

void addUserExp(const char* name, int delta) {
    UserInfo u;
    if (findUser(name, &u)) {
        u.exp += delta;
        int newLevel = 1 + u.exp / 100;
        if (newLevel > u.level) u.level = newLevel;
        updateUser(&u);
        if (strcmp(name, currentUser) == 0) {
            currentUserExp = u.exp;
            currentUserLevel = u.level;
        }
    }
}

void saveStats(int result) {
    if (challengeMode) return;
    FILE* fp = fopen("stats.dat", "a");
    if (fp) {
        fprintf(fp, "%s %d %d %d %d %d %d\n",
                currentUser, difficulty, result, (int)time(NULL), stepCount, gameMode, getWinLength());
        fclose(fp);
    }
    if (strcmp(currentUser, "Guest") != 0) {
        int delta = 5;
        if (result == 1) delta = 18 + difficulty * 6 + (gameMode == MODE_CONNECT6 ? 8 : 0);
        else if (result == 2) delta = 8 + difficulty * 2;
        addUserExp(currentUser, delta);
    }
}

void resetUserStats() {
    if (currentUser[0] == 0) return;
    FILE* fp = fopen("stats.dat", "r");
    if (!fp) return;
    char lines[4000][128];
    int lineCount = 0;
    char line[128];
    while (fgets(line, sizeof(line), fp) && lineCount < 4000) {
        safeCopy(lines[lineCount], line, 128);
        lineCount++;
    }
    fclose(fp);
    fp = fopen("stats.dat", "w");
    if (!fp) return;
    for (int i = 0; i < lineCount; i++) {
        char user[64] = {0};
        sscanf(lines[i], "%63s", user);
        if (strcmp(user, currentUser) == 0) continue;
        fputs(lines[i], fp);
    }
    fclose(fp);
}

void removeStatsForUser(const char* name) {
    if (!name || name[0] == 0) return;
    FILE* fp = fopen("stats.dat", "r");
    if (!fp) return;
    char lines[4000][128];
    int lineCount = 0;
    char line[128];
    while (fgets(line, sizeof(line), fp) && lineCount < 4000) {
        safeCopy(lines[lineCount], line, 128);
        lineCount++;
    }
    fclose(fp);
    fp = fopen("stats.dat", "w");
    if (!fp) return;
    for (int i = 0; i < lineCount; i++) {
        char user[64] = {0};
        sscanf(lines[i], "%63s", user);
        if (strcmp(user, name) == 0) continue;
        fputs(lines[i], fp);
    }
    fclose(fp);
}

void removeSaveForUser(const char* name) {
    char path[64];
    buildUserSavePath(name, path, 64);
    remove(path);
}

int deleteUserAccount(const char* name) {
    if (!name || name[0] == 0 || strcmp(name, "Guest") == 0) return 0;
    UserInfo users[200];
    int n = loadUsers(users, 200);
    int found = 0;
    FILE* fp = fopen("users.dat", "w");
    if (!fp) return 0;
    for (int i = 0; i < n; i++) {
        if (strcmp(users[i].name, name) == 0) {
            found = 1;
            continue;
        }
        fprintf(fp, "%s %s %d %d %lld\n",
                users[i].name, users[i].pwdHash, users[i].exp, users[i].level, users[i].regTime);
    }
    fclose(fp);
    if (found) {
        removeStatsForUser(name);
        removeSaveForUser(name);
    }
    return found;
}

void resetGuestSessionData() {
    removeStatsForUser("Guest");
    removeSaveForUser("Guest");
    if (strcmp(currentUser, "Guest") == 0) {
        currentUserLevel = 1;
        currentUserExp = 0;
    }
}

int saveGameToFile(const GameSave* gs) {
    char path[64];
    const char* owner = (currentUser[0] ? currentUser : "Guest");
    buildUserSavePath(owner, path, 64);

    GameSave fixed = *gs;
    safeCopy(fixed.owner, owner, 64);
    fixed.saveTime = (long long)time(NULL);
    fixed.version = SAVE_VERSION;

    FILE* fp = fopen(path, "wb");
    if (!fp) return 0;
    fwrite(&fixed, sizeof(GameSave), 1, fp);
    fclose(fp);
    return 1;
}

int saveStreamUsableForCurrentUser(FILE* fp, long fsize) {
    if (!fp) return 0;
    if (fsize == (long)sizeof(LegacyGameSave) || fsize == (long)sizeof(GameSaveV1)) return 1;
    if (fsize < (long)sizeof(GameSave)) return 0;
    GameSave tmp;
    long pos = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    size_t n = fread(&tmp, sizeof(GameSave), 1, fp);
    fseek(fp, pos, SEEK_SET);
    if (n != 1) return 0;
    if (tmp.owner[0] && strcmp(tmp.owner, (currentUser[0] ? currentUser : "Guest")) != 0) return 0;
    return 1;
}

int openCurrentSaveFile(FILE** out, long* fsize) {
    char path[64];
    const char* owner = (currentUser[0] ? currentUser : "Guest");
    buildUserSavePath(owner, path, 64);
    FILE* fp = fopen(path, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        *fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (saveStreamUsableForCurrentUser(fp, *fsize)) {
            *out = fp;
            return 1;
        }
        fclose(fp);
    }
    fp = fopen("save.dat", "rb"); // 兼容旧版单存档
    if (!fp) return 0;
    fseek(fp, 0, SEEK_END);
    *fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (!saveStreamUsableForCurrentUser(fp, *fsize)) {
        fclose(fp);
        return 0;
    }
    *out = fp;
    return 1;
}

void sanitizeLoadedSave(GameSave* gs) {
    if (!gs) return;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (gs->board[i][j] < 0 || gs->board[i][j] > 2) gs->board[i][j] = 0;
        }
    }
    if (gs->historyCount < 0) gs->historyCount = 0;
    if (gs->historyCount > MAX_HISTORY) gs->historyCount = MAX_HISTORY;
    int validHistory = 0;
    for (int i = 0; i < gs->historyCount; i++) {
        Move m = gs->history[i];
        if (m.x < 0 || m.x >= BOARD_SIZE || m.y < 0 || m.y >= BOARD_SIZE) continue;
        gs->history[validHistory++] = m;
    }
    gs->historyCount = validHistory;
    if (gs->difficulty < 1 || gs->difficulty > 3) gs->difficulty = 2;
    gs->useForbidden = gs->useForbidden ? 1 : 0;
    if (gs->playerColor != 1 && gs->playerColor != 2) gs->playerColor = 1;
    if (gs->currentTurn != 1 && gs->currentTurn != 2) gs->currentTurn = gs->playerColor;
    gs->gameOver = gs->gameOver ? 1 : 0;
    if (gs->result < -1 || gs->result > 2) gs->result = -1;
    if (gs->gameMode != MODE_GOMOKU && gs->gameMode != MODE_CONNECT6) gs->gameMode = MODE_GOMOKU;
    if (gs->gameMode == MODE_CONNECT6) gs->useForbidden = 0;
    gs->showHints = gs->showHints ? 1 : 0;
    if (gs->turnPlaced < 0 || gs->turnPlaced > 2) gs->turnPlaced = 0;
}

int loadGameFromFile(GameSave* gs) {
    FILE* fp = NULL;
    long fsize = 0;
    if (!openCurrentSaveFile(&fp, &fsize)) return 0;
    memset(gs, 0, sizeof(GameSave));
    if (fsize == (long)sizeof(LegacyGameSave)) {
        LegacyGameSave oldSave;
        size_t n = fread(&oldSave, sizeof(LegacyGameSave), 1, fp);
        fclose(fp);
        if (n != 1) return 0;
        for (int i = 0; i < BOARD_SIZE; i++)
            for (int j = 0; j < BOARD_SIZE; j++)
                gs->board[i][j] = oldSave.board[i][j];
        gs->historyCount = oldSave.historyCount;
        if (gs->historyCount < 0) gs->historyCount = 0;
        if (gs->historyCount > MAX_HISTORY) gs->historyCount = MAX_HISTORY;
        for (int i = 0; i < gs->historyCount && i < MAX_HISTORY; i++) gs->history[i] = oldSave.history[i];
        gs->difficulty = oldSave.difficulty;
        gs->useForbidden = oldSave.useForbidden;
        gs->playerColor = oldSave.playerColor;
        gs->currentTurn = oldSave.currentTurn;
        gs->gameOver = oldSave.gameOver;
        gs->result = oldSave.result;
        gs->version = 0;
        gs->gameMode = MODE_GOMOKU;
        gs->showHints = showHints;
        gs->turnPlaced = 0;
        gs->owner[0] = 0;
        gs->saveTime = 0;
        sanitizeLoadedSave(gs);
        return 1;
    }
    if (fsize == (long)sizeof(GameSaveV1)) {
        GameSaveV1 oldSave;
        size_t n = fread(&oldSave, sizeof(GameSaveV1), 1, fp);
        fclose(fp);
        if (n != 1) return 0;
        for (int i = 0; i < BOARD_SIZE; i++)
            for (int j = 0; j < BOARD_SIZE; j++)
                gs->board[i][j] = oldSave.board[i][j];
        gs->historyCount = oldSave.historyCount;
        for (int i = 0; i < oldSave.historyCount && i < MAX_HISTORY; i++) gs->history[i] = oldSave.history[i];
        gs->difficulty = oldSave.difficulty;
        gs->useForbidden = oldSave.useForbidden;
        gs->playerColor = oldSave.playerColor;
        gs->currentTurn = oldSave.currentTurn;
        gs->gameOver = oldSave.gameOver;
        gs->result = oldSave.result;
        gs->version = oldSave.version;
        gs->gameMode = oldSave.gameMode;
        gs->showHints = oldSave.showHints;
        gs->turnPlaced = oldSave.turnPlaced;
        gs->owner[0] = 0;
        gs->saveTime = 0;
        sanitizeLoadedSave(gs);
        return 1;
    }
    if (fsize < (long)sizeof(GameSave)) {
        fclose(fp);
        return 0;
    }
    size_t n = fread(gs, sizeof(GameSave), 1, fp);
    fclose(fp);
    if (n != 1) return 0;
    if (gs->owner[0] && strcmp(gs->owner, (currentUser[0] ? currentUser : "Guest")) != 0) return 0;
    sanitizeLoadedSave(gs);
    return 1;
}

int savePathUsableForCurrentUser(const char* path) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return 0;
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    int ok = saveStreamUsableForCurrentUser(fp, fsize);
    fclose(fp);
    return ok;
}

int hasSaveFile() {
    char path[64];
    const char* owner = (currentUser[0] ? currentUser : "Guest");
    buildUserSavePath(owner, path, 64);
    if (savePathUsableForCurrentUser(path)) return 1;
    return savePathUsableForCurrentUser("save.dat");
}

/* ===================== 棋盘基础 ===================== */
void initBoard() {
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            board[i][j] = 0;
    historyCount = 0;
    stepCount = 0;
    stepStartTime = GetTickCount();
    aiThinking = 0;
    turnPlacedThisRound = 0;
}

int normalizeChallengeIndex(int index) {
    if (index < 0 || index >= CHALLENGE_COUNT) return 0;
    return index;
}

const wchar_t* getChallengeName() {
    return challengeNames[normalizeChallengeIndex(challengeIndex)];
}

const wchar_t* getChallengeGoal() {
    return challengeGoals[normalizeChallengeIndex(challengeIndex)];
}

int countColorStones(int color) {
    int count = 0;
    for (int y = 0; y < BOARD_SIZE; y++)
        for (int x = 0; x < BOARD_SIZE; x++)
            if (board[y][x] == color) count++;
    return count;
}

int countChallengePlayerMoves() {
    int count = countColorStones(playerColor) - challengeBasePlayerStones;
    return count < 0 ? 0 : count;
}

void seedChallengeStone(int x, int y, int color) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) return;
    if (board[y][x] != 0 || historyCount >= MAX_HISTORY) return;
    board[y][x] = color;
    history[historyCount].x = x;
    history[historyCount].y = y;
    historyCount++;
}

int isInsideBoard(int x, int y) {
    return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
}

int isReservedPoint(int x, int y, Move* reserved, int count) {
    for (int i = 0; i < count; i++)
        if (reserved[i].x == x && reserved[i].y == y) return 1;
    return 0;
}

int boardHasWin(int color) {
    for (int y = 0; y < BOARD_SIZE; y++)
        for (int x = 0; x < BOARD_SIZE; x++)
            if (board[y][x] == color && checkWin(x, y, color)) return 1;
    return 0;
}

int countImmediateWinsForColor(int color) {
    int count = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board[y][x] != 0) continue;
            board[y][x] = color;
            if (checkWin(x, y, color)) count++;
            board[y][x] = 0;
        }
    }
    return count;
}

void rebuildChallengeHistory() {
    historyCount = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board[y][x] == 0 || historyCount >= MAX_HISTORY) continue;
            history[historyCount].x = x;
            history[historyCount].y = y;
            historyCount++;
        }
    }
}

int bestChallengeScoreForColor(int color) {
    int best = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board[y][x] != 0) continue;
            int score = evaluatePointAdvanced(x, y, color);
            if (score > best) best = score;
        }
    }
    return best;
}

int tryBuildRandomChallenge(int style) {
    int dirs[4][2] = {{1,0},{0,1},{1,1},{1,-1}};
    int noiseMin = challengeNoiseMin[style];
    int noiseMax = challengeNoiseMax[style];
    int noiseTarget = noiseMin + rand() % (noiseMax - noiseMin + 1);
    int minWhitePressure[CHALLENGE_COUNT] = {1800, 4500, 4500};
    int minBlackStones[CHALLENGE_COUNT] = {12, 15, 18};
    int minWhiteStones[CHALLENGE_COUNT] = {8, 12, 16};
    int requiredForks = (style >= 2) ? 3 : 2;

    for (int attempt = 0; attempt < 450; attempt++) {
        initBoard();
        int keyX = 4 + rand() % 7;
        int keyY = 4 + rand() % 7;
        int d1 = rand() % 4;
        int d2 = rand() % 4;
        while (d2 == d1) d2 = rand() % 4;
        int sign1 = (rand() % 2) ? 1 : -1;
        int sign2 = (rand() % 2) ? 1 : -1;
        int usedDirs[2] = {d1, d2};
        int usedSigns[2] = {sign1, sign2};
        Move reserved[3];
        reserved[0].x = keyX; reserved[0].y = keyY;
        int reservedCount = 1;
        int ok = 1;

        for (int k = 0; k < 2 && ok; k++) {
            int dx = dirs[usedDirs[k]][0] * usedSigns[k];
            int dy = dirs[usedDirs[k]][1] * usedSigns[k];
            int tx = keyX + dx, ty = keyY + dy;
            if (!isInsideBoard(tx, ty)) { ok = 0; break; }
            reserved[reservedCount].x = tx;
            reserved[reservedCount].y = ty;
            reservedCount++;
            for (int s = 1; s <= 3; s++) {
                int x = keyX - dx * s;
                int y = keyY - dy * s;
                if (!isInsideBoard(x, y) || board[y][x] == 2) { ok = 0; break; }
                board[y][x] = 1;
            }
        }
        if (!ok) continue;
        if (reserved[1].x == reserved[2].x && reserved[1].y == reserved[2].y) continue;

        int added = 0;
        int spread = 5 + style * 2;
        for (int tries = 0; tries < 1800 && added < noiseTarget; tries++) {
            int x, y;
            if (rand() % 100 < 78) {
                x = keyX + (rand() % (spread * 2 + 1)) - spread;
                y = keyY + (rand() % (spread * 2 + 1)) - spread;
            } else {
                x = rand() % BOARD_SIZE;
                y = rand() % BOARD_SIZE;
            }
            if (!isInsideBoard(x, y) || board[y][x] != 0) continue;
            if (isReservedPoint(x, y, reserved, reservedCount)) continue;

            int blackChance = 42 - style * 4;
            int color = (rand() % 100 < blackChance) ? 1 : 2;
            board[y][x] = color;
            if (boardHasWin(1) || boardHasWin(2) ||
                countImmediateWinsForColor(1) > 0 ||
                countImmediateWinsForColor(2) > 0) {
                board[y][x] = 0;
                continue;
            }
            added++;
        }

        if (added < noiseMin) continue;
        if (boardHasWin(1) || boardHasWin(2)) continue;
        if (countImmediateWinsForColor(1) > 0 || countImmediateWinsForColor(2) > 0) continue;
        if (countColorStones(1) < minBlackStones[style] || countColorStones(2) < minWhiteStones[style]) continue;
        if (bestChallengeScoreForColor(2) < minWhitePressure[style]) continue;

        board[keyY][keyX] = 1;
        int winsAfterKey = countImmediateWinsForColor(1);
        int keyAlreadyWins = checkWin(keyX, keyY, 1);
        board[keyY][keyX] = 0;
        if (keyAlreadyWins || winsAfterKey < requiredForks) continue;

        rebuildChallengeHistory();
        return 1;
    }
    return 0;
}

void loadChallengeBoard(int index) {
    index = normalizeChallengeIndex(index);
    challengeMode = 1;
    challengeIndex = index;
    challengeMoveLimit = challengeLimits[index];
    challengePlayerMoves = 0;
    gameMode = MODE_GOMOKU;
    useForbidden = 0;
    playerColor = 1;
    aiColor = 2;
    timeLimit = 0;
    showHints = 0;

    if (!tryBuildRandomChallenge(index)) {
        initBoard();
        seedChallengeStone(4, 7, 1); seedChallengeStone(5, 7, 1); seedChallengeStone(6, 7, 1);
        seedChallengeStone(7, 4, 1); seedChallengeStone(7, 5, 1); seedChallengeStone(7, 6, 1);
        seedChallengeStone(4, 6, 2); seedChallengeStone(6, 6, 2); seedChallengeStone(8, 8, 2);
        seedChallengeStone(9, 7, 2); seedChallengeStone(8, 5, 2);
        seedChallengeStone(3, 4, 2); seedChallengeStone(10, 4, 2); seedChallengeStone(11, 8, 2);
        seedChallengeStone(5, 10, 2); seedChallengeStone(9, 10, 2); seedChallengeStone(2, 8, 1);
        seedChallengeStone(10, 2, 1); seedChallengeStone(12, 6, 1); seedChallengeStone(4, 11, 1);
    }

    challengeBaseHistory = historyCount;
    challengeBasePlayerStones = countColorStones(playerColor);
    stepCount = historyCount;
    stepStartTime = GetTickCount();
    turnPlacedThisRound = 0;
}

int getCellX(int col) { return MARGIN + col * CELL_SIZE; }
int getCellY(int row) { return MARGIN + row * CELL_SIZE; }

void getMouseCell(int mx, int my, int* col, int* row) {
    int x = mx - MARGIN + CELL_SIZE / 2;
    int y = my - MARGIN + CELL_SIZE / 2;
    *col = x / CELL_SIZE;
    *row = y / CELL_SIZE;
    if (*col < 0) *col = 0;
    if (*col >= BOARD_SIZE) *col = BOARD_SIZE - 1;
    if (*row < 0) *row = 0;
    if (*row >= BOARD_SIZE) *row = BOARD_SIZE - 1;
}

/* ===================== 胜负判断 ===================== */
int checkWin(int x, int y, int color) {
    int target = getWinLength();
    int dirs[4][2] = {{1,0},{0,1},{1,1},{1,-1}};
    for (int d = 0; d < 4; d++) {
        int cnt = 1;
        int dx = dirs[d][0], dy = dirs[d][1];
        for (int step = 1; step < target; step++) {
            int nx = x + dx * step, ny = y + dy * step;
            if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) break;
            if (board[ny][nx] == color) cnt++;
            else break;
        }
        for (int step = 1; step < target; step++) {
            int nx = x - dx * step, ny = y - dy * step;
            if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) break;
            if (board[ny][nx] == color) cnt++;
            else break;
        }
        if (cnt >= target) return 1;
    }
    return 0;
}

int isBoardFull() {
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (board[i][j] == 0) return 0;
    return 1;
}

/* ===================== 禁手规则 ===================== */
int isForbiddenMove(int x, int y, int color) {
    if (gameMode != MODE_GOMOKU || !useForbidden || color != 1 || board[y][x] != 0) return 0;
    board[y][x] = color;
    int overline = 0, fours = 0, threes = 0;
    int dirs[4][2] = {{1,0},{0,1},{1,1},{1,-1}};
    for (int d = 0; d < 4; d++) {
        int dx = dirs[d][0], dy = dirs[d][1];
        int cnt = 1;
        int leftOpen = 0, rightOpen = 0;
        int nx = x - dx, ny = y - dy;
        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && board[ny][nx] == color) {
            cnt++; nx -= dx; ny -= dy;
        }
        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && board[ny][nx] == 0) leftOpen = 1;
        nx = x + dx; ny = y + dy;
        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && board[ny][nx] == color) {
            cnt++; nx += dx; ny += dy;
        }
        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && board[ny][nx] == 0) rightOpen = 1;
        if (cnt > 5) overline = 1;
        else if (cnt == 4 && (leftOpen || rightOpen)) fours++;
        else if (cnt == 3 && leftOpen && rightOpen) threes++;
    }
    board[y][x] = 0;
    if (overline) return 1;
    if (fours >= 2) return 1;
    if (threes >= 2) return 1;
    return 0;
}

/* ===================== 绘图函数 ===================== */
void drawBackground(IMAGE* img) {
    setorigin(0, 0);
    setfillcolor(RGB(0, 0, 0));
    solidrectangle(0, 0, WIN_WIDTH, WIN_HEIGHT);
    putimage(0, 0, WIN_WIDTH, WIN_HEIGHT, img, 0, 0);
    updateOrigin();
}

void drawBoard() {
    int left = getCellX(0) - 12;
    int top = getCellY(0) - 12;
    int right = getCellX(BOARD_SIZE - 1) + 12;
    int bottom = getCellY(BOARD_SIZE - 1) + 12;
    setfillcolor(RGB(28, 22, 18));
    fillroundrect(left - 8, top - 8, right + 8, bottom + 8, 22, 22);
    setfillcolor(BOARD_BG);
    fillroundrect(left, top, right, bottom, 16, 16);
    setlinestyle(PS_SOLID, 3);
    setlinecolor(RGB(120, 88, 42));
    roundrect(left + 2, top + 2, right - 2, bottom - 2, 14, 14);
    setlinestyle(PS_SOLID, 2);
    setlinecolor(LINE_COLOR);
    for (int i = 0; i < BOARD_SIZE; i++) {
        int y = getCellY(i);
        line(getCellX(0), y, getCellX(BOARD_SIZE - 1), y);
    }
    for (int i = 0; i < BOARD_SIZE; i++) {
        int x = getCellX(i);
        line(x, getCellY(0), x, getCellY(BOARD_SIZE - 1));
    }
    int stars[5][2] = {{3,3},{3,11},{7,7},{11,3},{11,11}};
    setfillcolor(LINE_COLOR);
    for (int i = 0; i < 5; i++) {
        int cx = getCellX(stars[i][0]);
        int cy = getCellY(stars[i][1]);
        solidcircle(cx, cy, 5);
    }
    HDC hdc = GetImageHDC(NULL);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(210, 190, 140));
    settextstyle(14, 0, "Consolas");
    wchar_t buf[8];
    for (int i = 0; i < BOARD_SIZE; i++) {
        wsprintfW(buf, L"%c", L'A' + i);
        TextOutW(hdc, getCellX(i) - 5, top - 22, buf, wcslen(buf));
        wsprintfW(buf, L"%d", i + 1);
        TextOutW(hdc, left - 28, getCellY(i) - 8, buf, wcslen(buf));
    }
}

void drawChess(int col, int row, int color) {
    int cx = getCellX(col);
    int cy = getCellY(row);
    int r = CELL_SIZE / 2 - 3;
    if (color == 1) {
        // 玄黑子：外圈阴影+本体+高光
        setfillcolor(RGB(10, 10, 10));
        solidcircle(cx, cy, r);
        setfillcolor(BLACK_CHESS);
        solidcircle(cx, cy, r - 2);
        setfillcolor(RGB(90, 90, 90));
        solidcircle(cx - r/3, cy - r/3, r/4);
    } else {
        // 玉白子：外圈阴影+本体+高光
        setfillcolor(RGB(180, 180, 180));
        solidcircle(cx, cy, r + 1);
        setfillcolor(WHITE_CHESS);
        solidcircle(cx, cy, r);
        setlinecolor(RGB(200, 200, 210));
        circle(cx, cy, r);
        setfillcolor(RGB(255, 255, 255));
        solidcircle(cx - r/3, cy - r/3, r/4);
    }
}

void drawLastMoveMarker(int col, int row) {
    if (col < 0 || col >= BOARD_SIZE || row < 0 || row >= BOARD_SIZE) return;
    int cx = getCellX(col);
    int cy = getCellY(row);
    setlinecolor(LAST_MOVE);
    setlinestyle(PS_SOLID, 2);
    circle(cx, cy, CELL_SIZE / 2 - 2);
}

void drawHover(int col, int row) {
    if (col < 0 || col >= BOARD_SIZE || row < 0 || row >= BOARD_SIZE) return;
    if (board[row][col] != 0) return;
    int cx = getCellX(col);
    int cy = getCellY(row);
    int r = CELL_SIZE / 2 - 6;
    if (playerColor == 1) {
        setfillcolor(RGB(80, 80, 80));
        solidcircle(cx, cy, r);
    } else {
        setfillcolor(RGB(220, 220, 220));
        solidcircle(cx, cy, r);
    }
}

void drawHintMarkers();

void redrawAll(int hoverCol, int hoverRow, int lastCol, int lastRow) {
    drawBackground(&imgGame);
    drawBoard();
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (board[i][j] != 0)
                drawChess(j, i, board[i][j]);
    if (lastCol >= 0 && lastRow >= 0) drawLastMoveMarker(lastCol, lastRow);
    drawHintMarkers();
    drawHover(hoverCol, hoverRow);
}

/* 带阴影的文字 */
void drawCenterTextShadow(int y, const wchar_t* str, int height, COLORREF c) {
    settextstyle(height, 0, "SimHei");
    HDC hdc = GetImageHDC(NULL);
    SetBkMode(hdc, TRANSPARENT);
    SIZE sz;
    GetTextExtentPoint32W(hdc, str, wcslen(str), &sz);
    int x = (WIN_WIDTH - sz.cx) / 2;
    SetTextColor(hdc, RGB(0, 0, 0));
    TextOutW(hdc, x + 2, y + 2, str, wcslen(str));
    SetTextColor(hdc, c);
    TextOutW(hdc, x, y, str, wcslen(str));
}

void drawCenterText(int y, const wchar_t* str, int height, COLORREF c) {
    settextstyle(height, 0, "SimHei");
    HDC hdc = GetImageHDC(NULL);
    SetBkMode(hdc, TRANSPARENT);
    SIZE sz;
    GetTextExtentPoint32W(hdc, str, wcslen(str), &sz);
    int x = (WIN_WIDTH - sz.cx) / 2;
    SetTextColor(hdc, RGB(0, 0, 0));
    TextOutW(hdc, x + 2, y + 2, str, wcslen(str));
    SetTextColor(hdc, c);
    TextOutW(hdc, x, y, str, wcslen(str));
}

void drawResultOverlay(int result) {
    HDC hdc = GetImageHDC(NULL);
    SetBkMode(hdc, TRANSPARENT);

    if (result == 1) {
        int x = 132, y = 168, w = 560, h = 316;
        setfillcolor(PANEL_BG);
        setlinecolor(PANEL_BORDER);
        fillroundrect(x - 10, y - 10, x + w + 10, y + h + 92, 12, 12);
        putimage(x, y, w, h, &imgWin, 0, 0);
        setfillcolor(RGB(18, 16, 24));
        solidrectangle(x, y + h - 76, x + w, y + h + 58);
        SetTextColor(hdc, ACCENT_GOLD);
        settextstyle(34, 0, "SimHei");
        const wchar_t* winTitle = challengeMode ? L"破局成功" : L"道友胜";
        TextOutW(hdc, x + 30, y + h - 64, winTitle, wcslen(winTitle));
        SetTextColor(hdc, TEXT_WHITE);
        settextstyle(18, 0, "SimHei");
        const wchar_t* winLine1 = challengeMode ? L"天机：此阵竟被你寻到生门。" : L"天机：此局是本座失算。";
        const wchar_t* winLine2 = challengeMode ? L"这一手，算得上破局。" : L"道友这一手，算得上漂亮。";
        TextOutW(hdc, x + 30, y + h - 22, winLine1, wcslen(winLine1));
        TextOutW(hdc, x + 30, y + h + 8, winLine2, wcslen(winLine2));
        SetTextColor(hdc, ACCENT_GOLD);
        TextOutW(hdc, x + 30, y + h + 38, L"点击继续", wcslen(L"点击继续"));
        return;
    }

    int x = 258, y = 248, w = 460, h = 190;
    setfillcolor(PANEL_BG);
    setlinecolor(PANEL_BORDER);
    fillroundrect(x, y, x + w, y + h, 14, 14);
    roundrect(x + 6, y + 6, x + w - 6, y + h - 6, 12, 12);
    const wchar_t* title = (result == 0) ? L"天机胜" : L"平局";
    const wchar_t* line1 = (result == 0)
        ? (challengeMode ? L"天机：残局最忌贪心，错一手便无路。" : L"天机：区区炼气期，也敢挑战本座？")
        : L"天机：今日算你命大，且饶你一局。";
    const wchar_t* line2 = (result == 0)
        ? (challengeMode ? L"换一招，再来破阵。" : L"回去再修三百年吧。")
        : L"";
    SetTextColor(hdc, result == 0 ? RGB(255, 120, 100) : TEXT_WHITE);
    settextstyle(34, 0, "SimHei");
    TextOutW(hdc, x + 46, y + 38, title, wcslen(title));
    SetTextColor(hdc, TEXT_WHITE);
    settextstyle(18, 0, "SimHei");
    TextOutW(hdc, x + 46, y + 92, line1, wcslen(line1));
    if (wcslen(line2) > 0) TextOutW(hdc, x + 46, y + 120, line2, wcslen(line2));
    SetTextColor(hdc, ACCENT_GOLD);
    TextOutW(hdc, x + 46, y + 144, L"点击继续", wcslen(L"点击继续"));
}

void redrawScene(int hoverCol, int hoverRow, int lastCol, int lastRow, int gameOver, int result) {
    redrawAll(hoverCol, hoverRow, lastCol, lastRow);
    if (gameOver && result >= 0) drawResultOverlay(result);
}

void drawLeftText(int x, int y, const wchar_t* str, int height, COLORREF c) {
    settextstyle(height, 0, "SimHei");
    HDC hdc = GetImageHDC(NULL);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    TextOutW(hdc, x + 2, y + 2, str, wcslen(str));
    SetTextColor(hdc, c);
    TextOutW(hdc, x, y, str, wcslen(str));
}

int evaluatePointAdvanced(int x, int y, int color);

int findBestMoveForColor(int color, int* bestX, int* bestY, int* bestScore) {
    int found = 0;
    int topScore = -1;
    int topX = -1, topY = -1;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board[y][x] != 0) continue;
            if (gameMode == MODE_GOMOKU && useForbidden && color == 1 && isForbiddenMove(x, y, color)) continue;
            int score = evaluatePointAdvanced(x, y, color);
            board[y][x] = color;
            if (checkWin(x, y, color)) score += 10000000;
            board[y][x] = 0;
            int dist = abs(x - BOARD_SIZE / 2) + abs(y - BOARD_SIZE / 2);
            score -= dist;
            if (!found || score > topScore) {
                found = 1;
                topScore = score;
                topX = x;
                topY = y;
            }
        }
    }
    if (bestX) *bestX = topX;
    if (bestY) *bestY = topY;
    if (bestScore) *bestScore = topScore;
    return found;
}

int hasImmediateWin(int color, int* outX, int* outY) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board[y][x] != 0) continue;
            if (gameMode == MODE_GOMOKU && useForbidden && color == 1 && isForbiddenMove(x, y, color)) continue;
            board[y][x] = color;
            int win = checkWin(x, y, color);
            board[y][x] = 0;
            if (win) {
                if (outX) *outX = x;
                if (outY) *outY = y;
                return 1;
            }
        }
    }
    return 0;
}

int getSituationText(wchar_t* out, COLORREF* color) {
    int x, y;
    if (hasImmediateWin(playerColor, &x, &y)) {
        wcscpy(out, L"局势: 可一手胜");
        *color = ACCENT_GOLD;
        return 1;
    }
    if (hasImmediateWin(aiColor, &x, &y)) {
        wcscpy(out, L"局势: 需立即防");
        *color = RGB(255, 100, 90);
        return 1;
    }

    int ps = 0, as = 0;
    findBestMoveForColor(playerColor, NULL, NULL, &ps);
    findBestMoveForColor(aiColor, NULL, NULL, &as);
    int diff = ps - as;
    if (ps < 10000 && as < 10000) {
        wcscpy(out, L"局势: 均势");
        *color = TEXT_WHITE;
    } else if (diff > 50000) {
        wcscpy(out, L"局势: 道友占优");
        *color = ACCENT_GOLD;
    } else if (diff < -50000) {
        wcscpy(out, L"局势: 天机占优");
        *color = RGB(255, 130, 110);
    } else {
        wcscpy(out, L"局势: 有攻防");
        *color = TEXT_WHITE;
    }
    return 1;
}

void drawHintMarkers() {
    if (!showHints || !gameRunning || aiThinking) return;
    int ax, ay, as, dx, dy, ds;
    int hasAttack = findBestMoveForColor(playerColor, &ax, &ay, &as);
    int hasDefense = findBestMoveForColor(aiColor, &dx, &dy, &ds);
    HDC hdc = GetImageHDC(NULL);
    SetBkMode(hdc, TRANSPARENT);
    settextstyle(16, 0, "SimHei");
    if (hasDefense && ds >= 10000) {
        int cx = getCellX(dx), cy = getCellY(dy);
        setlinestyle(PS_SOLID, 3);
        setlinecolor(RGB(255, 90, 75));
        circle(cx, cy, CELL_SIZE / 2 - 8);
        SetTextColor(hdc, RGB(255, 120, 110));
        TextOutW(hdc, cx - 8, cy - 8, L"守", 1);
    }
    if (hasAttack && as >= 1000 && !(hasDefense && ds >= 10000 && ax == dx && ay == dy)) {
        int cx = getCellX(ax), cy = getCellY(ay);
        setlinestyle(PS_SOLID, 2);
        setlinecolor(RGB(95, 210, 160));
        circle(cx, cy, CELL_SIZE / 2 - 12);
        SetTextColor(hdc, RGB(120, 235, 185));
        TextOutW(hdc, cx - 8, cy - 8, L"攻", 1);
    }
}

void drawButton(const Button* btn, int hover) {
    // 外框高光
    setlinestyle(PS_SOLID, 1);
    setlinecolor(hover ? RGB(255, 230, 150) : RGB(80, 65, 45));
    if (hover) setfillcolor(BTN_HOVER);
    else setfillcolor(BTN_BG);
    fillroundrect(btn->x, btn->y, btn->x + btn->w, btn->y + btn->h, 16, 16);
    // 内框
    setlinestyle(PS_SOLID, 2);
    setlinecolor(hover ? ACCENT_GOLD : LINE_COLOR);
    roundrect(btn->x + 2, btn->y + 2, btn->x + btn->w - 2, btn->y + btn->h - 2, 14, 14);
    HDC hdc = GetImageHDC(NULL);
    SetTextColor(hdc, hover ? ACCENT_GOLD : TEXT_WHITE);
    SetBkMode(hdc, TRANSPARENT);
    settextstyle(22, 0, "SimHei");
    SIZE sz;
    GetTextExtentPoint32W(hdc, btn->text, wcslen(btn->text), &sz);
    int tx = btn->x + (btn->w - sz.cx) / 2;
    int ty = btn->y + (btn->h - sz.cy) / 2;
    TextOutW(hdc, tx, ty, btn->text, wcslen(btn->text));
}

void drawSmallButton(const Button* btn, int hover, int fontSize) {
    setlinestyle(PS_SOLID, 1);
    setlinecolor(hover ? RGB(255, 230, 150) : RGB(80, 65, 45));
    if (hover) setfillcolor(BTN_HOVER);
    else setfillcolor(BTN_BG);
    fillroundrect(btn->x, btn->y, btn->x + btn->w, btn->y + btn->h, 10, 10);
    setlinestyle(PS_SOLID, 1);
    setlinecolor(hover ? ACCENT_GOLD : LINE_COLOR);
    roundrect(btn->x + 1, btn->y + 1, btn->x + btn->w - 1, btn->y + btn->h - 1, 8, 8);
    HDC hdc = GetImageHDC(NULL);
    SetTextColor(hdc, hover ? ACCENT_GOLD : TEXT_WHITE);
    SetBkMode(hdc, TRANSPARENT);
    settextstyle(fontSize, 0, "SimHei");
    SIZE sz;
    GetTextExtentPoint32W(hdc, btn->text, wcslen(btn->text), &sz);
    int tx = btn->x + (btn->w - sz.cx) / 2;
    int ty = btn->y + (btn->h - sz.cy) / 2;
    TextOutW(hdc, tx, ty, btn->text, wcslen(btn->text));
}

/* ===================== 右侧信息面板 ===================== */
void drawInfoPanel(int gameOver, int result, int currentTurn) {
    HDC hdc = GetImageHDC(NULL);
    SetBkMode(hdc, TRANSPARENT);
    int px = 760, py = 90, pw = 210;
    setfillcolor(PANEL_BG);
    setlinecolor(PANEL_BORDER);
    fillroundrect(px - 12, py - 15, px + pw + 12, py + 540, 14, 14);
    wchar_t buf[128];
    drawLeftText(px, py, L"对局信息", 24, ACCENT_GOLD);
    py += 42;
    wchar_t wuser[64]; asciiToWchar(currentUser, wuser, 64);
    wsprintfW(buf, L"道友: %s", wuser);
    drawLeftText(px, py, buf, 18, TEXT_WHITE);
    py += 32;
    const wchar_t* realm = getXianLevelName(currentUserExp);
    wsprintfW(buf, L"境界: %s", realm);
    drawLeftText(px, py, buf, 18, ACCENT_GOLD);
    py += 32;
    const wchar_t* diffName[4] = {L"", L"简单", L"中等", L"困难"};
    wsprintfW(buf, L"难度: %s", diffName[difficulty]);
    drawLeftText(px, py, buf, 18, TEXT_WHITE);
    py += 32;
    wsprintfW(buf, L"模式: %s", challengeMode ? L"残局挑战" : getModeName());
    drawLeftText(px, py, buf, 18, TEXT_WHITE);
    py += 32;
    wsprintfW(buf, L"目标: 连%d成阵", getWinLength());
    drawLeftText(px, py, buf, 18, TEXT_WHITE);
    py += 32;
    if (challengeMode) {
        wsprintfW(buf, L"残局: %s", getChallengeName());
        drawLeftText(px, py, buf, 18, ACCENT_GOLD);
        py += 32;
        int remainMoves = challengeMoveLimit - challengePlayerMoves;
        if (remainMoves < 0) remainMoves = 0;
        wsprintfW(buf, L"限手: 剩%d/%d", remainMoves, challengeMoveLimit);
        drawLeftText(px, py, buf, 18, remainMoves <= 1 ? RGB(255, 120, 100) : TEXT_WHITE);
        py += 32;
    }
    wsprintfW(buf, L"执子: %s", (playerColor == 1) ? L"玄黑(先)" : L"玉白(后)");
    drawLeftText(px, py, buf, 18, TEXT_WHITE);
    py += 32;
    wsprintfW(buf, L"规则: %s", (gameMode == MODE_CONNECT6) ? L"六连无禁手" : (useForbidden ? L"有禁手" : L"无禁手"));
    drawLeftText(px, py, buf, 18, TEXT_WHITE);
    py += 32;
    wsprintfW(buf, L"心眼: %s", showHints ? L"开启" : L"关闭");
    drawLeftText(px, py, buf, 18, showHints ? ACCENT_GOLD : TEXT_WHITE);
    py += 32;
    wsprintfW(buf, L"步数: %d", stepCount);
    drawLeftText(px, py, buf, 18, TEXT_WHITE);
    py += 32;
    if (!gameOver) {
        wsprintfW(buf, L"回合: %s", (currentTurn == playerColor) ? L"道友" : L"天机");
        drawLeftText(px, py, buf, 18, ACCENT_GOLD);
        py += 32;
        int need = getTurnMoveNeed(currentTurn);
        int remain = need - turnPlacedThisRound;
        if (remain < 1) remain = 1;
        wsprintfW(buf, L"本回合: 还落%d子", remain);
        drawLeftText(px, py, buf, 18, TEXT_WHITE);
        py += 32;
    }
    if (gameRunning && !gameOver && timeLimit > 0) {
        DWORD elapsed = (GetTickCount() - stepStartTime) / 1000;
        int remain = timeLimit - (int)elapsed;
        if (remain < 0) remain = 0;
        wsprintfW(buf, L"剩余: %d秒", remain);
        COLORREF tc = (remain <= 5) ? RGB(255, 80, 80) : TEXT_WHITE;
        drawLeftText(px, py, buf, 18, tc);
        py += 32;
    }
    if (!gameOver && showHints) {
        COLORREF sitColor = TEXT_WHITE;
        if (getSituationText(buf, &sitColor)) {
            drawLeftText(px, py, buf, 18, sitColor);
            py += 32;
        }
    }
}

void drawStatusBar(const wchar_t* extra) {
    HDC hdc = GetImageHDC(NULL);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, TEXT_WHITE);
    settextstyle(18, 0, "SimHei");
    TextOutW(hdc, 20, WIN_HEIGHT - 32, L"ESC-返回菜单  R-悔棋  H-心眼提示", wcslen(L"ESC-返回菜单  R-悔棋  H-心眼提示"));
    if (extra && wcslen(extra) > 0) {
        settextstyle(22, 0, "SimHei");
        SetTextColor(hdc, ACCENT_GOLD);
        SIZE sz;
        GetTextExtentPoint32W(hdc, extra, wcslen(extra), &sz);
        TextOutW(hdc, (WIN_WIDTH - sz.cx) / 2, WIN_HEIGHT / 2, extra, wcslen(extra));
    }
}

/* ===================== 悔棋 ===================== */
void undoMove() {
    if (historyCount <= 0) return;
    int removed = 0;
    int minHistory = challengeMode ? challengeBaseHistory : 0;
    if (gameMode == MODE_GOMOKU) {
        if (historyCount - minHistory < 2) return;
        for (int i = 0; i < 2 && historyCount > minHistory; i++) {
            historyCount--;
            int x = history[historyCount].x;
            int y = history[historyCount].y;
            board[y][x] = 0;
            removed++;
        }
    } else {
        if (turnPlacedThisRound > 0) {
            if (historyCount <= minHistory) return;
            historyCount--;
            int x = history[historyCount].x;
            int y = history[historyCount].y;
            board[y][x] = 0;
            removed++;
            turnPlacedThisRound--;
        } else {
            int lastColor = board[history[historyCount - 1].y][history[historyCount - 1].x];
            while (historyCount > minHistory && board[history[historyCount - 1].y][history[historyCount - 1].x] == lastColor) {
                historyCount--;
                int x = history[historyCount].x;
                int y = history[historyCount].y;
                board[y][x] = 0;
                removed++;
            }
            if (historyCount > minHistory) {
                int prevColor = board[history[historyCount - 1].y][history[historyCount - 1].x];
                while (historyCount > minHistory && board[history[historyCount - 1].y][history[historyCount - 1].x] == prevColor) {
                    historyCount--;
                    int x = history[historyCount].x;
                    int y = history[historyCount].y;
                    board[y][x] = 0;
                    removed++;
                }
            }
        }
    }
    stepCount -= removed;
    if (stepCount < minHistory) stepCount = minHistory;
    if (challengeMode) challengePlayerMoves = countChallengePlayerMoves();
    stepStartTime = GetTickCount();
}

int canUndoNow() {
    int minHistory = challengeMode ? challengeBaseHistory : 0;
    if (gameMode == MODE_GOMOKU) return historyCount - minHistory >= 2;
    if (turnPlacedThisRound > 0) return historyCount > minHistory;
    if (playerColor == 2 && historyCount <= minHistory + 1) return 0;
    return historyCount - minHistory >= 2;
}


/* ===================== AI 算法 ===================== */
int evaluatePointAdvanced(int x, int y, int color) {
    if (board[y][x] != 0) return 0;
    int old = board[y][x];
    board[y][x] = color;
    int target = getWinLength();
    int dirs[4][2] = {{1,0},{0,1},{1,1},{1,-1}};
    int score = 0;
    int openThreats = 0;
    for (int d = 0; d < 4; d++) {
        int dx = dirs[d][0], dy = dirs[d][1];
        int cnt = 1;
        int leftOpen = 0, rightOpen = 0;
        int nx = x - dx, ny = y - dy;
        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && board[ny][nx] == color) {
            cnt++; nx -= dx; ny -= dy;
        }
        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && board[ny][nx] == 0) leftOpen = 1;
        nx = x + dx; ny = y + dy;
        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && board[ny][nx] == color) {
            cnt++; nx += dx; ny += dy;
        }
        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && board[ny][nx] == 0) rightOpen = 1;
        int open = leftOpen + rightOpen;
        if (cnt >= target) score += 10000000;
        else if (cnt == target - 1 && open == 2) { score += 1000000; openThreats++; }
        else if (cnt == target - 1 && open == 1) score += 120000;
        else if (cnt == target - 2 && open == 2) { score += 30000; openThreats++; }
        else if (cnt == target - 2 && open == 1) score += 4500;
        else if (cnt == target - 3 && open == 2) score += 1800;
        else if (cnt == target - 3 && open == 1) score += 400;
        else if (cnt >= 2 && open == 2) score += 180;
        else if (cnt >= 2 && open == 1) score += 60;
        else if (cnt == 1 && open == 2) score += 20;
    }
    if (openThreats >= 2) score += 60000;
    board[y][x] = old;
    return score;
}

struct Cand { int x, y, s; };

int generateMoves(int moves[][2], int maxMoves, int turnColor) {
    int nearby[BOARD_SIZE][BOARD_SIZE];
    memset(nearby, 0, sizeof(nearby));
    int hasStone = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board[y][x] != 0) {
                hasStone = 1;
                for (int dy = -2; dy <= 2; dy++) {
                    for (int dx = -2; dx <= 2; dx++) {
                        int nx = x + dx, ny = y + dy;
                        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE)
                            nearby[ny][nx] = 1;
                    }
                }
            }
        }
    }
    if (!hasStone) {
        moves[0][0] = BOARD_SIZE / 2;
        moves[0][1] = BOARD_SIZE / 2;
        return 1;
    }
    Cand cands[BOARD_SIZE * BOARD_SIZE];
    int cnum = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board[y][x] != 0) continue;
            if (!nearby[y][x]) continue;
            if (gameMode == MODE_GOMOKU && useForbidden && turnColor == 1 && isForbiddenMove(x, y, 1)) continue;
            int myColor = turnColor;
            int opColor = 3 - turnColor;
            int s = evaluatePointAdvanced(x, y, myColor) * 2;
            s += evaluatePointAdvanced(x, y, opColor);
            board[y][x] = myColor;
            if (checkWin(x, y, myColor)) s += 100000000;
            board[y][x] = opColor;
            if (checkWin(x, y, opColor)) s += 90000000;
            board[y][x] = 0;
            int dist = abs(x - BOARD_SIZE/2) + abs(y - BOARD_SIZE/2);
            s -= dist * 3;
            cands[cnum].x = x; cands[cnum].y = y; cands[cnum].s = s;
            cnum++;
        }
    }
    int take = cnum < maxMoves ? cnum : maxMoves;
    for (int i = 0; i < take; i++) {
        int maxIdx = i;
        for (int j = i + 1; j < cnum; j++) {
            if (cands[j].s > cands[maxIdx].s) maxIdx = j;
        }
        if (maxIdx != i) {
            Cand tmp = cands[i];
            cands[i] = cands[maxIdx];
            cands[maxIdx] = tmp;
        }
        moves[i][0] = cands[i].x;
        moves[i][1] = cands[i].y;
    }
    return take;
}

int alphaBeta(int depth, int alpha, int beta, int color, int maxDepth) {
    (void)maxDepth;
    int opColor = 3 - color;
    int moves[25][2];
    int n = generateMoves(moves, 25, color);
    if (n == 0) return 0;
    for (int i = 0; i < n; i++) {
        int x = moves[i][0], y = moves[i][1];
        board[y][x] = color;
        int win = checkWin(x, y, color);
        board[y][x] = 0;
        if (win) return 100000000;
    }
    if (depth == 0) {
        int myScore = 0, opScore = 0;
        for (int i = 0; i < n; i++) {
            int x = moves[i][0], y = moves[i][1];
            int s1 = evaluatePointAdvanced(x, y, color);
            int s2 = evaluatePointAdvanced(x, y, opColor);
            if (s1 > myScore) myScore = s1;
            if (s2 > opScore) opScore = s2;
        }
        return myScore - (int)(opScore * 0.9);
    }
    for (int i = 0; i < n; i++) {
        int x = moves[i][0], y = moves[i][1];
        board[y][x] = color;
        int score = -alphaBeta(depth - 1, -beta, -alpha, opColor, maxDepth);
        board[y][x] = 0;
        if (score > alpha) alpha = score;
        if (alpha >= beta) break;
    }
    return alpha;
}

void AI_move(int* outX, int* outY) {
    int bestX = -1, bestY = -1;
    int myColor = aiColor;
    int opColor = 3 - aiColor;
    if (difficulty == 1) {
        int moves[30][2];
        int n = generateMoves(moves, 30, myColor);
        if (n == 0) { *outX = -1; *outY = -1; return; }
        if (rand() % 100 < 35) {
            int idx = rand() % n;
            *outX = moves[idx][0]; *outY = moves[idx][1];
            return;
        }
        int bestScore = -99999999;
        for (int i = 0; i < n; i++) {
            int x = moves[i][0], y = moves[i][1];
            int score = evaluatePointAdvanced(x, y, myColor);
            score += evaluatePointAdvanced(x, y, opColor);
            int dist = abs(x - BOARD_SIZE/2) + abs(y - BOARD_SIZE/2);
            score -= dist * 5;
            if (score > bestScore) { bestScore = score; bestX = x; bestY = y; }
        }
    } else if (difficulty == 2) {
        int moves[20][2];
        int n = generateMoves(moves, 20, myColor);
        if (n == 0) { *outX = -1; *outY = -1; return; }
        int bestScore = -99999999;
        for (int i = 0; i < n; i++) {
            int x = moves[i][0], y = moves[i][1];
            board[y][x] = myColor;
            int score = 0;
            if (checkWin(x, y, myColor)) score = 100000000;
            else {
                int opMoves[20][2];
                int opN = generateMoves(opMoves, 20, opColor);
                int opBest = -99999999;
                for (int j = 0; j < opN; j++) {
                    int ox = opMoves[j][0], oy = opMoves[j][1];
                    board[oy][ox] = opColor;
                    int opScore = evaluatePointAdvanced(ox, oy, opColor);
                    if (checkWin(ox, oy, opColor)) opScore = 100000000;
                    board[oy][ox] = 0;
                    if (opScore > opBest) opBest = opScore;
                }
                score = evaluatePointAdvanced(x, y, myColor) - opBest;
            }
            board[y][x] = 0;
            if (score > bestScore) { bestScore = score; bestX = x; bestY = y; }
        }
    } else {
        int moves[20][2];
        int n = generateMoves(moves, 20, myColor);
        if (n == 0) { *outX = -1; *outY = -1; return; }
        int bestScore = -99999999;
        for (int i = 0; i < n; i++) {
            int x = moves[i][0], y = moves[i][1];
            board[y][x] = myColor;
            int win = checkWin(x, y, myColor);
            int score;
            if (win) score = 100000000;
            else {
                score = -alphaBeta(2, -99999999, 99999999, opColor, 2);
            }
            board[y][x] = 0;
            if (score > bestScore) { bestScore = score; bestX = x; bestY = y; }
        }
    }
    *outX = bestX; *outY = bestY;
}

int placeStone(int x, int y, int color) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) return 0;
    if (board[y][x] != 0 || historyCount >= MAX_HISTORY) return 0;
    board[y][x] = color;
    history[historyCount].x = x;
    history[historyCount].y = y;
    historyCount++;
    stepCount++;
    return 1;
}

int playAITurn(int* lastCol, int* lastRow) {
    int need = getTurnMoveNeed(aiColor);
    turnPlacedThisRound = 0;
    for (int i = 0; i < need; i++) {
        int aix, aiy;
        if (historyCount == 0 && aiColor == 1) {
            aix = BOARD_SIZE / 2;
            aiy = BOARD_SIZE / 2;
            if (difficulty == 1) { aix += rand()%3 - 1; aiy += rand()%3 - 1; }
        } else {
            AI_move(&aix, &aiy);
        }
        if (!placeStone(aix, aiy, aiColor)) return -1;
        *lastCol = aix;
        *lastRow = aiy;
        turnPlacedThisRound++;
        playSoundEffect(0);
        if (checkWin(aix, aiy, aiColor)) return 1;
        if (isBoardFull()) return 2;
    }
    turnPlacedThisRound = 0;
    return 0;
}

/* ===================== 输入界面 ===================== */
void inputScreen(const wchar_t* prompt, char* out, int maxLen, int isPassword) {
    int confirmed = 0;
    int cancelled = 0;
    Button btnOK, btnCancel;
    btnOK.x = WIN_WIDTH / 2 - 120; btnOK.y = 450; btnOK.w = 100; btnOK.h = 42; btnOK.text = L"确定";
    btnCancel.x = WIN_WIDTH / 2 + 20; btnCancel.y = 450; btnCancel.w = 100; btnCancel.h = 42; btnCancel.text = L"取消";
    int hover = -1;
    ExMessage msg;

    HWND parent = GetHWnd();
    int editX = WIN_WIDTH / 2 - 170, editY = 296, editW = 340, editH = 38;
    DWORD editStyle = WS_CHILD | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL;
    if (isPassword) editStyle |= ES_PASSWORD;
    HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", editStyle,
                                editX, editY, editW, editH, parent, NULL, GetModuleHandleW(NULL), NULL);
    HFONT editFont = CreateFontW(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
    SendMessageW(edit, WM_SETFONT, (WPARAM)editFont, TRUE);
    SendMessageW(edit, EM_LIMITTEXT, (WPARAM)(isPassword ? 31 : 16), 0);
    if (isPassword) SendMessageW(edit, EM_SETPASSWORDCHAR, L'*', 0);
    int needRedraw = 1;

    while (1) {
        if (needRedraw) {
            ShowWindow(edit, SW_HIDE);
            drawBackground(&imgLogin);
            drawCenterTextShadow(200, prompt, 30, TEXT_GOLD);
            setfillcolor(RGB(30, 30, 30));
            setlinecolor(LINE_COLOR);
            fillroundrect(WIN_WIDTH / 2 - 180, 290, WIN_WIDTH / 2 + 180, 340, 8, 8);
            drawButton(&btnOK, hover == 0);
            drawButton(&btnCancel, hover == 1);
            FlushBatchDraw();
            SetWindowPos(edit, HWND_TOP, editX, editY, editW, editH, SWP_SHOWWINDOW);
            ShowWindow(edit, SW_SHOW);
            InvalidateRect(edit, NULL, TRUE);
            UpdateWindow(edit);
            SetFocus(edit);
            needRedraw = 0;
        }

        while (peekmessage(&msg, EX_MOUSE)) {
            if (msg.message == WM_MOUSEMOVE || msg.message == WM_LBUTTONDOWN) {
                int mx = toLogicalX(msg.x);
                int my = toLogicalY(msg.y);
                if (msg.message == WM_MOUSEMOVE) {
                    int oldHover = hover;
                    hover = -1;
                    if (hitButton(&btnOK, mx, my)) hover = 0;
                    else if (hitButton(&btnCancel, mx, my)) hover = 1;
                    if (hover != oldHover) needRedraw = 1;
                } else if (msg.message == WM_LBUTTONDOWN) {
                    if (hitButton(&btnOK, mx, my)) {
                        confirmed = 1; break;
                    }
                    if (hitButton(&btnCancel, mx, my)) {
                        cancelled = 1; break;
                    }
                }
            }
        }
        MSG winMsg;
        while (PeekMessageW(&winMsg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&winMsg);
            DispatchMessageW(&winMsg);
        }
        if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0) {
            while ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0) Sleep(10);
            cancelled = 1;
        }
        if ((GetAsyncKeyState(VK_RETURN) & 0x8000) != 0) {
            while ((GetAsyncKeyState(VK_RETURN) & 0x8000) != 0) Sleep(10);
            confirmed = 1;
        }
        if (confirmed || cancelled) break;
        Sleep(10);
    }
    if (confirmed) {
        wchar_t wideText[128] = {0};
        char utf8Text[256] = {0};
        GetWindowTextW(edit, wideText, 128);
        int n = WideCharToMultiByte(CP_UTF8, 0, wideText, -1, utf8Text, 256, NULL, NULL);
        if (n > 0) safeCopy(out, utf8Text, maxLen);
        else out[0] = 0;
    } else {
        out[0] = 0;
    }
    DestroyWindow(edit);
    DeleteObject(editFont);
    SetFocus(parent);
}

/* ===================== 登录界面 ===================== */
int loginScreen() {
    const int LOGIN_BTN_COUNT = 5;
    Button btns[LOGIN_BTN_COUNT];
    btns[0].x = (WIN_WIDTH - BTN_W) / 2; btns[0].y = 280; btns[0].w = BTN_W; btns[0].h = BTN_H; btns[0].text = L"登录";
    btns[1].x = (WIN_WIDTH - BTN_W) / 2; btns[1].y = 350; btns[1].w = BTN_W; btns[1].h = BTN_H; btns[1].text = L"注册";
    btns[2].x = (WIN_WIDTH - BTN_W) / 2; btns[2].y = 420; btns[2].w = BTN_W; btns[2].h = BTN_H; btns[2].text = L"修改密码";
    btns[3].x = (WIN_WIDTH - BTN_W) / 2; btns[3].y = 490; btns[3].w = BTN_W; btns[3].h = BTN_H; btns[3].text = L"忘记密码";
    btns[4].x = (WIN_WIDTH - BTN_W) / 2; btns[4].y = 560; btns[4].w = BTN_W; btns[4].h = BTN_H; btns[4].text = L"游客登录";
    int hover = -1;
    int needRedraw = 1;
    ExMessage msg;
    while (1) {
        if (needRedraw) {
            drawBackground(&imgLogin);
            drawCenterTextShadow(80, L"凡人修仙传 · 五子棋", 48, ACCENT_GOLD);
            drawCenterTextShadow(160, L"江西财经大学 · 信敏廉毅", 26, TEXT_WHITE);
            for (int i = 0; i < LOGIN_BTN_COUNT; i++) drawButton(&btns[i], i == hover);
            FlushBatchDraw();
            needRedraw = 0;
        }
        if (peekmessage(&msg, EX_MOUSE)) {
            int mx = toLogicalX(msg.x);
            int my = toLogicalY(msg.y);
            if (msg.message == WM_MOUSEMOVE) {
                int oldHover = hover;
                hover = -1;
                for (int i = 0; i < LOGIN_BTN_COUNT; i++) {
                    if (hitButton(&btns[i], mx, my)) {
                        hover = i; break;
                    }
                }
                if (hover != oldHover) needRedraw = 1;
            } else if (msg.message == WM_LBUTTONDOWN) {
                for (int i = 0; i < LOGIN_BTN_COUNT; i++) {
                    if (hitButton(&btns[i], mx, my)) {
                        if (i == 0) {
                            char name[64] = {0}, pwd[64] = {0};
                            inputScreen(L"请输入用户名", name, 64, 0);
                            if (strlen(name) == 0) { needRedraw = 1; continue; }
                            if (!isValidUsername(name)) {
                                MessageBoxW(GetHWnd(), L"用户名需为1-16个字符，可用中文、字母、数字或下划线。", L"提示", MB_OK);
                                needRedraw = 1; continue;
                            }
                            inputScreen(L"请输入密码", pwd, 64, 1);
                            if (strlen(pwd) == 0) { needRedraw = 1; continue; }
                            UserInfo u;
                            if (findUser(name, &u)) {
                                if (verifyUserPassword(&u, pwd)) {
                                    safeCopy(currentUser, name, 64);
                                    currentUserLevel = u.level;
                                    currentUserExp = u.exp;
                                    return 1;
                                }
                                MessageBoxW(GetHWnd(), L"密码错误", L"错误", MB_OK);
                            } else {
                                MessageBoxW(GetHWnd(), L"账号不存在", L"错误", MB_OK);
                            }
                            needRedraw = 1;
                        } else if (i == 1) {
                            char name[64] = {0}, pwd[64] = {0}, pwd2[64] = {0}, hash[33] = {0};
                            inputScreen(L"请输入新用户名", name, 64, 0);
                            if (strlen(name) == 0) { needRedraw = 1; continue; }
                            if (!isValidUsername(name)) {
                                MessageBoxW(GetHWnd(), L"用户名需为1-16个字符，可用中文、字母、数字或下划线，不可使用Guest。", L"提示", MB_OK);
                                needRedraw = 1; continue;
                            }
                            if (findUser(name, NULL)) {
                                MessageBoxW(GetHWnd(), L"用户已存在", L"错误", MB_OK);
                                needRedraw = 1; continue;
                            }
                            inputScreen(L"请输入密码", pwd, 64, 1);
                            if (!isValidPassword(pwd)) {
                                MessageBoxW(GetHWnd(), L"密码需为4-31位，不能全为空格。", L"提示", MB_OK);
                                needRedraw = 1; continue;
                            }
                            inputScreen(L"请再次输入密码", pwd2, 64, 1);
                            if (strcmp(pwd, pwd2) != 0) {
                                MessageBoxW(GetHWnd(), L"两次输入的密码不一致。", L"提示", MB_OK);
                                needRedraw = 1; continue;
                            }
                            makeStoredPassword(pwd, name, hash);
                            UserInfo u;
                            safeCopy(u.name, name, 64);
                            safeCopy(u.pwdHash, hash, 33);
                            u.exp = 0; u.level = 1; u.regTime = time(NULL);
                            saveUser(&u);
                            safeCopy(currentUser, name, 64);
                            currentUserLevel = 1; currentUserExp = 0;
                            MessageBoxW(GetHWnd(), L"注册成功", L"成功", MB_OK);
                            return 1;
                        } else if (i == 2) {
                            char name[64] = {0}, oldPwd[64] = {0}, newPwd[64] = {0}, newPwd2[64] = {0}, hashNew[33] = {0};
                            inputScreen(L"请输入用户名", name, 64, 0);
                            if (strlen(name) == 0) { needRedraw = 1; continue; }
                            inputScreen(L"请输入旧密码", oldPwd, 64, 1);
                            inputScreen(L"请输入新密码", newPwd, 64, 1);
                            if (!isValidPassword(newPwd)) {
                                MessageBoxW(GetHWnd(), L"新密码需为4-31位，不能全为空格。", L"提示", MB_OK);
                                needRedraw = 1; continue;
                            }
                            inputScreen(L"请再次输入新密码", newPwd2, 64, 1);
                            if (strcmp(newPwd, newPwd2) != 0) {
                                MessageBoxW(GetHWnd(), L"两次输入的新密码不一致。", L"提示", MB_OK);
                                needRedraw = 1; continue;
                            }
                            UserInfo u;
                            if (findUser(name, &u)) {
                                if (verifyUserPassword(&u, oldPwd)) {
                                    makeStoredPassword(newPwd, name, hashNew);
                                    safeCopy(u.pwdHash, hashNew, 33);
                                    updateUser(&u);
                                    MessageBoxW(GetHWnd(), L"密码修改成功", L"成功", MB_OK);
                                } else {
                                    MessageBoxW(GetHWnd(), L"旧密码错误", L"错误", MB_OK);
                                }
                            } else {
                                MessageBoxW(GetHWnd(), L"用户不存在", L"错误", MB_OK);
                            }
                            needRedraw = 1;
                        } else if (i == 3) {
                            char name[64] = {0}, adminKey[64] = {0}, savedKey[64] = {0}, hashNew[33] = {0};
                            inputScreen(L"请输入要重置的账号", name, 64, 0);
                            if (strlen(name) == 0) { needRedraw = 1; continue; }
                            UserInfo u;
                            if (!findUser(name, &u)) {
                                MessageBoxW(GetHWnd(), L"用户不存在", L"错误", MB_OK);
                                needRedraw = 1; continue;
                            }
                            inputScreen(L"请输入管理员密匙", adminKey, 64, 1);
                            if (strlen(adminKey) == 0) { needRedraw = 1; continue; }
                            loadAdminKey(savedKey, 64);
                            if (strcmp(adminKey, savedKey) != 0) {
                                MessageBoxW(GetHWnd(), L"管理员密匙错误，密码未重置。", L"错误", MB_OK);
                                needRedraw = 1; continue;
                            }
                            makeStoredPassword(DEFAULT_RESET_PASSWORD, name, hashNew);
                            safeCopy(u.pwdHash, hashNew, 33);
                            updateUser(&u);
                            MessageBoxW(GetHWnd(), L"密码已重置为 123456。", L"成功", MB_OK);
                            needRedraw = 1;
                        } else if (i == 4) {
                            safeCopy(currentUser, "Guest", 64);
                            currentUserLevel = 1; currentUserExp = 0;
                            return 1;
                        }
                    }
                }
            }
        }
        Sleep(10);
    }
}


/* ===================== 主菜单 ===================== */
int mainMenu() {
    Button btns[8];
    btns[0].x = (WIN_WIDTH - BTN_W) / 2; btns[0].y = 200; btns[0].w = BTN_W; btns[0].h = BTN_H; btns[0].text = L"开始游戏";
    btns[1].x = (WIN_WIDTH - BTN_W) / 2; btns[1].y = 258; btns[1].w = BTN_W; btns[1].h = BTN_H; btns[1].text = L"残局挑战";
    btns[2].x = (WIN_WIDTH - BTN_W) / 2; btns[2].y = 316; btns[2].w = BTN_W; btns[2].h = BTN_H; btns[2].text = L"继续游戏";
    btns[3].x = (WIN_WIDTH - BTN_W) / 2; btns[3].y = 374; btns[3].w = BTN_W; btns[3].h = BTN_H; btns[3].text = L"游戏设置";
    btns[4].x = (WIN_WIDTH - BTN_W) / 2; btns[4].y = 432; btns[4].w = BTN_W; btns[4].h = BTN_H; btns[4].text = L"个人数据";
    btns[5].x = (WIN_WIDTH - BTN_W) / 2; btns[5].y = 490; btns[5].w = BTN_W; btns[5].h = BTN_H; btns[5].text = L"排行榜";
    btns[6].x = (WIN_WIDTH - BTN_W) / 2; btns[6].y = 548; btns[6].w = BTN_W; btns[6].h = BTN_H; btns[6].text = L"切换账号";
    btns[7].x = (WIN_WIDTH - BTN_W) / 2; btns[7].y = 606; btns[7].w = BTN_W; btns[7].h = BTN_H; btns[7].text = L"退出游戏";
    int hover = -1;
    int needRedraw = 1;
    ExMessage msg;
    while (1) {
        if (needRedraw) {
            drawBackground(&imgMenu);
            drawCenterTextShadow(60, L"凡人修仙传", 44, ACCENT_GOLD);
            drawCenterTextShadow(130, getModeName(), 36, TEXT_WHITE);
            for (int i = 0; i < 8; i++) drawButton(&btns[i], i == hover);
            FlushBatchDraw();
            needRedraw = 0;
        }
        if (peekmessage(&msg, EX_MOUSE)) {
            int mx = toLogicalX(msg.x);
            int my = toLogicalY(msg.y);
            if (msg.message == WM_MOUSEMOVE) {
                int oldHover = hover;
                hover = -1;
                for (int i = 0; i < 8; i++) {
                    if (hitButton(&btns[i], mx, my)) {
                        hover = i; break;
                    }
                }
                if (hover != oldHover) needRedraw = 1;
            } else if (msg.message == WM_LBUTTONDOWN) {
                for (int i = 0; i < 8; i++) {
                    if (hitButton(&btns[i], mx, my)) {
                        if (i == 2 && !hasSaveFile()) {
                            MessageBoxW(GetHWnd(), L"没有存档", L"提示", MB_OK);
                            needRedraw = 1; continue;
                        }
                        return i + 1;
                    }
                }
            }
        }
        Sleep(10);
    }
}

/* ===================== 残局挑战 ===================== */
int challengeMenu() {
    Button btns[CHALLENGE_COUNT + 1];
    wchar_t btnTexts[CHALLENGE_COUNT + 1][80];
    int startY = 230;
    for (int i = 0; i < CHALLENGE_COUNT; i++) {
        btns[i].x = (WIN_WIDTH - 340) / 2;
        btns[i].y = startY + i * 88;
        btns[i].w = 340;
        btns[i].h = 48;
        wsprintfW(btnTexts[i], L"%s  ·  限%d手", challengeNames[i], challengeLimits[i]);
        btns[i].text = btnTexts[i];
    }
    int backIndex = CHALLENGE_COUNT;
    btns[backIndex].x = (WIN_WIDTH - 260) / 2;
    btns[backIndex].y = startY + CHALLENGE_COUNT * 88 + 10;
    btns[backIndex].w = 260;
    btns[backIndex].h = 48;
    wcscpy(btnTexts[backIndex], L"返回");
    btns[backIndex].text = btnTexts[backIndex];

    int hover = -1;
    int needRedraw = 1;
    ExMessage msg;
    while (1) {
        if (needRedraw) {
            drawBackground(&imgMenu);
            drawCenterTextShadow(58, L"残局挑战", 40, ACCENT_GOLD);
            drawCenterText(116, L"限定手数内破局，残局不会写入普通存档。", 18, RGB(220, 205, 160));
            for (int i = 0; i < CHALLENGE_COUNT; i++) {
                drawSmallButton(&btns[i], i == hover, 18);
                drawCenterText(btns[i].y + 54, challengeGoals[i], 16, TEXT_WHITE);
            }
            drawButton(&btns[backIndex], hover == backIndex);
            FlushBatchDraw();
            needRedraw = 0;
        }
        if (peekmessage(&msg, EX_MOUSE)) {
            int mx = toLogicalX(msg.x);
            int my = toLogicalY(msg.y);
            if (msg.message == WM_MOUSEMOVE) {
                int oldHover = hover;
                hover = -1;
                for (int i = 0; i <= CHALLENGE_COUNT; i++) {
                    if (hitButton(&btns[i], mx, my)) {
                        hover = i; break;
                    }
                }
                if (hover != oldHover) needRedraw = 1;
            } else if (msg.message == WM_LBUTTONDOWN) {
                for (int i = 0; i <= CHALLENGE_COUNT; i++) {
                    if (hitButton(&btns[i], mx, my)) {
                        if (i == backIndex) return -1;
                        return i;
                    }
                }
            }
        }
        Sleep(10);
    }
}

/* ===================== 游戏设置 ===================== */
void gameSettings() {
    const int MAX_BTN_COUNT = 9;
    int guestMode = isGuestUser();
    int btnCount = guestMode ? 8 : 9;
    int logoutIndex = guestMode ? -1 : 7;
    int backIndex = guestMode ? 7 : 8;
    Button btns[MAX_BTN_COUNT];
    wchar_t btnTexts[MAX_BTN_COUNT][40];
    for (int i = 0; i < btnCount; i++) {
        btns[i].x = (WIN_WIDTH - 300) / 2;
        btns[i].y = 152 + i * 58;
        btns[i].w = 300;
        btns[i].h = 46;
        btns[i].text = btnTexts[i];
    }
    if (!guestMode) wcscpy(btnTexts[logoutIndex], L"注销账号");
    wcscpy(btnTexts[backIndex], L"返回");
    int hover = -1;
    int needRedraw = 1;
    ExMessage msg;
    wchar_t diffNames[4][16] = {L"", L"简单", L"中等", L"困难"};
    wchar_t timeNames[4][16] = {L"不限", L"10秒", L"30秒", L"60秒"};
    int timeVals[4] = {0, 10, 30, 60};
    int timeIdx = 0;
    for (int i = 0; i < 4; i++) if (timeVals[i] == timeLimit) timeIdx = i;
    while (1) {
        if (needRedraw) {
            wsprintfW(btnTexts[0], L"难度: %s", diffNames[difficulty]);
            wsprintfW(btnTexts[1], L"模式: %s", getModeName());
            wcscpy(btnTexts[2], gameMode == MODE_CONNECT6 ? L"禁手: 六子棋无禁手" : (useForbidden ? L"禁手: 开启" : L"禁手: 关闭"));
            wcscpy(btnTexts[3], (playerColor == 1) ? L"执子: 玄黑(先)" : L"执子: 玉白(后)");
            wcscpy(btnTexts[4], showHints ? L"心眼提示: 开启" : L"心眼提示: 关闭");
            wcscpy(btnTexts[5], soundOn ? L"音效: 开启" : L"音效: 关闭");
            wsprintfW(btnTexts[6], L"步时: %s", timeNames[timeIdx]);
            drawBackground(&imgMenu);
            drawCenterTextShadow(62, L"游戏设置", 38, ACCENT_GOLD);
            drawCenterText(116, L"六子棋: 黑先落1子，之后双方每回合落2子，先连6获胜。", 18, RGB(220, 205, 160));
            for (int i = 0; i < btnCount; i++) drawSmallButton(&btns[i], i == hover, 18);
            FlushBatchDraw();
            needRedraw = 0;
        }
        if (peekmessage(&msg, EX_MOUSE)) {
            int mx = toLogicalX(msg.x);
            int my = toLogicalY(msg.y);
            if (msg.message == WM_MOUSEMOVE) {
                int oldHover = hover;
                hover = -1;
                for (int i = 0; i < btnCount; i++) {
                    if (hitButton(&btns[i], mx, my)) {
                        hover = i; break;
                    }
                }
                if (hover != oldHover) needRedraw = 1;
            } else if (msg.message == WM_LBUTTONDOWN) {
                for (int i = 0; i < btnCount; i++) {
                    if (hitButton(&btns[i], mx, my)) {
                        if (i == 0) { difficulty++; if (difficulty > 3) difficulty = 1; needRedraw = 1; }
                        else if (i == 1) {
                            gameMode = (gameMode == MODE_GOMOKU) ? MODE_CONNECT6 : MODE_GOMOKU;
                            if (gameMode == MODE_CONNECT6) useForbidden = 0;
                            needRedraw = 1;
                        }
                        else if (i == 2) {
                            if (gameMode == MODE_GOMOKU) useForbidden = !useForbidden;
                            needRedraw = 1;
                        }
                        else if (i == 3) { playerColor = 3 - playerColor; needRedraw = 1; }
                        else if (i == 4) { showHints = !showHints; needRedraw = 1; }
                        else if (i == 5) { soundOn = !soundOn; needRedraw = 1; }
                        else if (i == 6) { timeIdx = (timeIdx + 1) % 4; timeLimit = timeVals[timeIdx]; needRedraw = 1; }
                        else if (!guestMode && i == logoutIndex) {
                            char pwd[64] = {0};
                            UserInfo u;
                            if (!findUser(currentUser, &u)) {
                                MessageBoxW(GetHWnd(), L"当前账号不存在，无法注销。", L"错误", MB_OK);
                                needRedraw = 1; continue;
                            }
                            inputScreen(L"请输入当前账号密码", pwd, 64, 1);
                            if (strlen(pwd) == 0) { needRedraw = 1; continue; }
                            if (!verifyUserPassword(&u, pwd)) {
                                MessageBoxW(GetHWnd(), L"密码错误，账号未注销。", L"错误", MB_OK);
                                needRedraw = 1; continue;
                            }
                            int ans = MessageBoxW(GetHWnd(), L"确认永久注销当前账号？账号、排行榜记录和该账号存档将被删除。", L"注销账号", MB_YESNO);
                            if (ans == IDYES) {
                                char deletedName[64];
                                safeCopy(deletedName, currentUser, 64);
                                if (deleteUserAccount(deletedName)) {
                                    MessageBoxW(GetHWnd(), L"账号已注销。", L"成功", MB_OK);
                                    safeCopy(currentUser, "Guest", 64);
                                    currentUserLevel = 1;
                                    currentUserExp = 0;
                                    loginScreen();
                                    return;
                                }
                                MessageBoxW(GetHWnd(), L"注销失败，请稍后重试。", L"错误", MB_OK);
                            }
                            needRedraw = 1;
                        }
                        else if (i == backIndex) return;
                    }
                }
            }
        }
        Sleep(10);
    }
}

/* ===================== 个人数据 ===================== */
void showPersonalStats() {
    int wins[4] = {0}, losses[4] = {0}, draws[4] = {0};
    int modeWins[2] = {0}, modeGames[2] = {0};
    int totalGames = 0;
    int currentStreak = 0, bestStreak = 0;
    UserInfo accountInfo;
    int hasAccount = (strcmp(currentUser, "Guest") != 0 && findUser(currentUser, &accountInfo));
    wchar_t regText[64] = L"游客模式";
    if (hasAccount) {
        time_t rt = (time_t)accountInfo.regTime;
        struct tm* tmv = localtime(&rt);
        if (tmv) wsprintfW(regText, L"%04d-%02d-%02d", tmv->tm_year + 1900, tmv->tm_mon + 1, tmv->tm_mday);
        else wcscpy(regText, L"未知");
    }
    FILE* fp = fopen("stats.dat", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            char user[64];
            int diff, result, t = 0, steps = 0, mode = MODE_GOMOKU;
            if (sscanf(line, "%63s %d %d %d %d %d", user, &diff, &result, &t, &steps, &mode) >= 3) {
                if (strcmp(user, currentUser) != 0) continue;
                if (diff < 1 || diff > 3) continue;
                if (result == 1) wins[diff]++;
                else if (result == 0) losses[diff]++;
                else draws[diff]++;
                if (mode != MODE_GOMOKU && mode != MODE_CONNECT6) mode = MODE_GOMOKU;
                modeGames[mode]++;
                if (result == 1) modeWins[mode]++;
                if (result == 1) {
                    currentStreak++;
                    if (currentStreak > bestStreak) bestStreak = currentStreak;
                } else {
                    currentStreak = 0;
                }
                totalGames++;
            }
        }
        fclose(fp);
    }
    Button btnReset, btnBack;
    btnReset.x = (WIN_WIDTH - BTN_W) / 2; btnReset.y = 600; btnReset.w = BTN_W; btnReset.h = BTN_H; btnReset.text = L"重置数据";
    btnBack.x = (WIN_WIDTH - BTN_W) / 2; btnBack.y = 670; btnBack.w = BTN_W; btnBack.h = BTN_H; btnBack.text = L"返回";
    int hover = -1;
    int needRedraw = 1;
    ExMessage msg;
    HDC hdc = GetImageHDC(NULL);
    while (1) {
        if (needRedraw) {
            drawBackground(&imgMenu);
            drawCenterTextShadow(60, L"个人数据", 38, ACCENT_GOLD);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, TEXT_WHITE);
            settextstyle(24, 0, "SimHei");
            const wchar_t* diffName[4] = {L"", L"简单", L"中等", L"困难"};
            int y = 150;
            wchar_t buf[256];
            wchar_t wuser[64]; asciiToWchar(currentUser, wuser, 64);
            wsprintfW(buf, L"道友: %s    注册: %s", wuser, regText);
            drawCenterText(y, buf, 22, ACCENT_GOLD); y += 45;
            wsprintfW(buf, L"境界: %s    经验: %d", getXianLevelName(currentUserExp), currentUserExp);
            drawCenterText(y, buf, 22, TEXT_WHITE); y += 45;
            wsprintfW(buf, L"总对局: %d", totalGames);
            drawCenterText(y, buf, 22, TEXT_WHITE); y += 45;
            for (int d = 1; d <= 3; d++) {
                wsprintfW(buf, L"%s:  胜 %d   负 %d   平 %d", diffName[d], wins[d], losses[d], draws[d]);
                drawCenterText(y, buf, 22, TEXT_WHITE);
                y += 42;
            }
            wsprintfW(buf, L"五子棋: %d局/%d胜    六子棋: %d局/%d胜", modeGames[0], modeWins[0], modeGames[1], modeWins[1]);
            drawCenterText(y, buf, 20, TEXT_WHITE); y += 38;
            wsprintfW(buf, L"当前连胜: %d    最高连胜: %d", currentStreak, bestStreak);
            drawCenterText(y, buf, 20, ACCENT_GOLD); y += 38;
            if (totalGames >= 20) drawCenterText(y, L"成就: 棋痴入道", 20, RGB(120, 235, 185));
            else if (bestStreak >= 3) drawCenterText(y, L"成就: 三胜凝气", 20, RGB(120, 235, 185));
            else if (modeWins[1] > 0) drawCenterText(y, L"成就: 六连初悟", 20, RGB(120, 235, 185));
            else drawCenterText(y, L"成就: 尚待破局", 20, RGB(220, 205, 160));
            drawButton(&btnReset, hover == 0);
            drawButton(&btnBack, hover == 1);
            FlushBatchDraw();
            needRedraw = 0;
        }
        if (peekmessage(&msg, EX_MOUSE)) {
            int mx = toLogicalX(msg.x);
            int my = toLogicalY(msg.y);
            if (msg.message == WM_MOUSEMOVE) {
                int oldHover = hover;
                hover = -1;
                if (hitButton(&btnReset, mx, my)) hover = 0;
                else if (hitButton(&btnBack, mx, my)) hover = 1;
                if (hover != oldHover) needRedraw = 1;
            } else if (msg.message == WM_LBUTTONDOWN) {
                if (hitButton(&btnReset, mx, my)) {
                    resetUserStats();
                    wins[1] = wins[2] = wins[3] = 0; losses[1] = losses[2] = losses[3] = 0; draws[1] = draws[2] = draws[3] = 0;
                    modeWins[0] = modeWins[1] = modeGames[0] = modeGames[1] = 0;
                    totalGames = 0; currentStreak = 0; bestStreak = 0;
                    MessageBoxW(GetHWnd(), L"数据已重置", L"提示", MB_OK);
                    needRedraw = 1;
                }
                if (hitButton(&btnBack, mx, my)) return;
            }
        }
        Sleep(10);
    }
}

/* ===================== 排行榜 ===================== */
struct PlayerStat { char name[64]; int wins, losses, draws; };

void showLeaderboard() {
    PlayerStat stats[100];
    int count = 0;
    UserInfo users[100];
    int userCount = loadUsers(users, 100);
    for (int i = 0; i < userCount && count < 100; i++) {
        safeCopy(stats[count].name, users[i].name, 64);
        stats[count].wins = 0; stats[count].losses = 0; stats[count].draws = 0;
        count++;
    }
    int found = 0;
    for (int i = 0; i < count; i++) if (strcmp(stats[i].name, currentUser) == 0) { found = 1; break; }
    if (!found && count < 100 && strcmp(currentUser, "Guest") != 0) {
        safeCopy(stats[count].name, currentUser, 64);
        stats[count].wins = 0; stats[count].losses = 0; stats[count].draws = 0;
        count++;
    }
    FILE* fp = fopen("stats.dat", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            char user[64]; int diff, result, t = 0, steps = 0, mode = MODE_GOMOKU;
            int parsed = sscanf(line, "%63s %d %d %d %d %d", user, &diff, &result, &t, &steps, &mode);
            if (parsed >= 3) {
                if (diff != 3) continue;
                if (parsed < 6) mode = MODE_GOMOKU;
                if (mode != gameMode) continue;
                for (int i = 0; i < count; i++) {
                    if (strcmp(stats[i].name, user) == 0) {
                        if (result == 1) stats[i].wins++;
                        else if (result == 0) stats[i].losses++;
                        else stats[i].draws++;
                        break;
                    }
                }
            }
        }
        fclose(fp);
    }
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            int total_i = stats[i].wins + stats[i].losses + stats[i].draws;
            int total_j = stats[j].wins + stats[j].losses + stats[j].draws;
            double rate_i = total_i > 0 ? (double)stats[i].wins / total_i : 0.0;
            double rate_j = total_j > 0 ? (double)stats[j].wins / total_j : 0.0;
            if (stats[j].wins > stats[i].wins ||
                (stats[j].wins == stats[i].wins && rate_j > rate_i) ||
                (stats[j].wins == stats[i].wins && rate_j == rate_i && stats[j].losses < stats[i].losses) ||
                (stats[j].wins == stats[i].wins && rate_j == rate_i && stats[j].losses == stats[i].losses && total_j > total_i)) {
                PlayerStat tmp = stats[i]; stats[i] = stats[j]; stats[j] = tmp;
            }
        }
    }
    Button backBtn;
    backBtn.x = (WIN_WIDTH - BTN_W) / 2; backBtn.y = 640; backBtn.w = BTN_W; backBtn.h = BTN_H; backBtn.text = L"返回";
    int hover = -1;
    int needRedraw = 1;
    ExMessage msg;
    HDC hdc = GetImageHDC(NULL);
    while (1) {
        if (needRedraw) {
            drawBackground(&imgMenu);
            wchar_t title[80];
            wsprintfW(title, L"排行榜（%s · 困难）", getModeName());
            drawCenterTextShadow(30, title, 38, ACCENT_GOLD);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, TEXT_WHITE);
            settextstyle(20, 0, "SimHei");
            drawLeftText(60, 85, L"排名", 20, TEXT_WHITE);
            drawLeftText(150, 85, L"道友", 20, TEXT_WHITE);
            drawLeftText(340, 85, L"胜", 20, TEXT_WHITE);
            drawLeftText(430, 85, L"负", 20, TEXT_WHITE);
            drawLeftText(520, 85, L"平", 20, TEXT_WHITE);
            drawLeftText(620, 85, L"胜率", 20, TEXT_WHITE);
            drawLeftText(740, 85, L"境界", 20, TEXT_WHITE);
            int y = 118;
            int showCount = count < 14 ? count : 14;
            for (int i = 0; i < showCount; i++) {
                wchar_t wname[64]; asciiToWchar(stats[i].name, wname, 64);
                wchar_t buf[32];
                int total = stats[i].wins + stats[i].losses + stats[i].draws;
                double rate = total > 0 ? (double)stats[i].wins / total * 100.0 : 0.0;
                UserInfo u; int exp = 0;
                if (findUser(stats[i].name, &u)) exp = u.exp;
                wsprintfW(buf, L"%d", i + 1); drawLeftText(60, y, buf, 18, TEXT_WHITE);
                drawLeftText(150, y, wname, 18, TEXT_WHITE);
                wsprintfW(buf, L"%d", stats[i].wins); drawLeftText(340, y, buf, 18, TEXT_WHITE);
                wsprintfW(buf, L"%d", stats[i].losses); drawLeftText(430, y, buf, 18, TEXT_WHITE);
                wsprintfW(buf, L"%d", stats[i].draws); drawLeftText(520, y, buf, 18, TEXT_WHITE);
                wsprintfW(buf, L"%.1f%%", rate); drawLeftText(620, y, buf, 18, TEXT_WHITE);
                const wchar_t* realm = getXianLevelName(exp);
                drawLeftText(740, y, realm, 18, TEXT_WHITE);
                y += 34;
            }
            drawButton(&backBtn, hover >= 0);
            FlushBatchDraw();
            needRedraw = 0;
        }
        if (peekmessage(&msg, EX_MOUSE)) {
            int mx = toLogicalX(msg.x);
            int my = toLogicalY(msg.y);
            if (msg.message == WM_MOUSEMOVE) {
                int oldHover = hover;
                hover = -1;
                if (hitButton(&backBtn, mx, my)) hover = 0;
                if (hover != oldHover) needRedraw = 1;
            } else if (msg.message == WM_LBUTTONDOWN) {
                if (hitButton(&backBtn, mx, my)) return;
            }
        }
        Sleep(10);
    }
}


/* ===================== 游戏主循环 ===================== */
void startGame(int isContinue) {
    aiColor = 3 - playerColor;
    int hoverCol = -1, hoverRow = -1;
    int lastCol = -1, lastRow = -1;
    int gameOver = 0;
    int result = -1;
    int currentTurn = 1;
    int drawOffered = 0;
    gameRunning = 1;
    turnPlacedThisRound = 0;

    if (isContinue && !challengeMode) {
        GameSave gs;
        if (loadGameFromFile(&gs)) {
            for (int i = 0; i < BOARD_SIZE; i++)
                for (int j = 0; j < BOARD_SIZE; j++)
                    board[i][j] = gs.board[i][j];
            historyCount = gs.historyCount;
            for (int i = 0; i < historyCount; i++) history[i] = gs.history[i];
            difficulty = gs.difficulty;
            useForbidden = gs.useForbidden;
            playerColor = gs.playerColor;
            aiColor = 3 - playerColor;
            gameMode = gs.gameMode;
            showHints = gs.showHints;
            if (gameMode == MODE_CONNECT6) useForbidden = 0;
            turnPlacedThisRound = gs.turnPlaced;
            currentTurn = gs.currentTurn;
            gameOver = gs.gameOver;
            result = gs.result;
            stepCount = historyCount;
            if (historyCount > 0) {
                lastCol = history[historyCount-1].x;
                lastRow = history[historyCount-1].y;
            }
        } else {
            MessageBoxW(GetHWnd(), L"当前账号没有可读取的存档，已新开一局。", L"继续游戏", MB_OK);
            initBoard();
        }
    } else {
        if (challengeMode) loadChallengeBoard(challengeIndex);
        else initBoard();
    }
    stepStartTime = GetTickCount();

    Button btnUndo, btnGiveUp, btnDraw, btnSave, btnBack;
    int bw = 110, bh = 36, gap = 14;
    int startX = (WIN_WIDTH - (5*bw + 4*gap)) / 2;
    int by = WIN_HEIGHT - 90;
    btnUndo.x = startX; btnUndo.y = by; btnUndo.w = bw; btnUndo.h = bh; btnUndo.text = L"悔棋(R)";
    btnGiveUp.x = startX + bw + gap; btnGiveUp.y = by; btnGiveUp.w = bw; btnGiveUp.h = bh; btnGiveUp.text = L"认输";
    btnDraw.x = startX + 2*(bw+gap); btnDraw.y = by; btnDraw.w = bw; btnDraw.h = bh; btnDraw.text = challengeMode ? L"提示(H)" : L"求和";
    btnSave.x = startX + 3*(bw+gap); btnSave.y = by; btnSave.w = bw; btnSave.h = bh; btnSave.text = challengeMode ? L"换局" : L"存档";
    btnBack.x = startX + 4*(bw+gap); btnBack.y = by; btnBack.w = bw; btnBack.h = bh; btnBack.text = L"返回";
    Button* bottomBtns[5] = {&btnUndo, &btnGiveUp, &btnDraw, &btnSave, &btnBack};
    int btnHover = -1;

    if (!isContinue && !challengeMode && playerColor == 2) {
        aiThinking = 1;
        redrawScene(-1, -1, lastCol, lastRow, gameOver, result);
        drawInfoPanel(gameOver, result, currentTurn);
        for (int i = 0; i < 5; i++) drawSmallButton(bottomBtns[i], -1, 16);
        drawStatusBar(L"");
        FlushBatchDraw();
        Sleep(300);
        int aiResult = playAITurn(&lastCol, &lastRow);
        if (aiResult == 1) { gameOver = 1; result = 0; saveStats(0); }
        else if (aiResult == 2) { gameOver = 1; result = 2; saveStats(2); }
        currentTurn = playerColor;
        stepStartTime = GetTickCount();
        aiThinking = 0;
    }

    redrawScene(-1, -1, lastCol, lastRow, gameOver, result);
    drawInfoPanel(gameOver, result, currentTurn);
    for (int i = 0; i < 5; i++) drawSmallButton(bottomBtns[i], -1, 16);
    drawStatusBar(L"");
    FlushBatchDraw();

    int doRedraw = 0;
    while (1) {
        if (doRedraw) {
            doRedraw = 0;
            redrawScene(hoverCol, hoverRow, lastCol, lastRow, gameOver, result);
            drawInfoPanel(gameOver, result, currentTurn);
            for (int i = 0; i < 5; i++) drawSmallButton(bottomBtns[i], i == btnHover, 16);
            // 结果仅通过弹窗显示
            drawStatusBar(L"");
            FlushBatchDraw();
        }
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            while (GetAsyncKeyState(VK_ESCAPE) & 0x8000) Sleep(10);
            int ans = MessageBoxW(GetHWnd(), L"返回主菜单？未保存的进度将丢失。", L"确认", MB_YESNO);
            if (ans == IDYES) { gameRunning = 0; return; }
            doRedraw = 1; continue;
        }
        if (!gameOver && !aiThinking && (GetAsyncKeyState('R') & 0x8000 || GetAsyncKeyState('r') & 0x8000)) {
            while ((GetAsyncKeyState('R') & 0x8000) || (GetAsyncKeyState('r') & 0x8000)) Sleep(10);
            if (canUndoNow()) {
                undoMove();
                if (historyCount > 0) { lastCol = history[historyCount-1].x; lastRow = history[historyCount-1].y; }
                else { lastCol = -1; lastRow = -1; }
                currentTurn = playerColor;
                drawOffered = 0;
                doRedraw = 1; continue;
            }
        }
        if (GetAsyncKeyState('H') & 0x8000 || GetAsyncKeyState('h') & 0x8000) {
            while ((GetAsyncKeyState('H') & 0x8000) || (GetAsyncKeyState('h') & 0x8000)) Sleep(10);
            showHints = !showHints;
            doRedraw = 1; continue;
        }
        if (!gameOver && gameRunning && timeLimit > 0 && currentTurn == playerColor) {
            DWORD elapsed = (GetTickCount() - stepStartTime) / 1000;
            if ((int)elapsed >= timeLimit) {
                gameOver = 1; result = 0; saveStats(0); playSoundEffect(2);
                doRedraw = 1; continue;
            }
        }

        ExMessage msg;
        while (peekmessage(&msg, EX_MOUSE)) {
            int mx = toLogicalX(msg.x);
            int my = toLogicalY(msg.y);
            if (msg.message == WM_MOUSEMOVE) {
                int oldCol = hoverCol, oldRow = hoverRow;
                getMouseCell(mx, my, &hoverCol, &hoverRow);
                int oldBtn = btnHover;
                btnHover = -1;
                for (int i = 0; i < 5; i++) {
                    if (hitButton(bottomBtns[i], mx, my)) {
                        btnHover = i; break;
                    }
                }
                if (oldCol != hoverCol || oldRow != hoverRow || oldBtn != btnHover) {
                    redrawScene(hoverCol, hoverRow, lastCol, lastRow, gameOver, result);
                    drawInfoPanel(gameOver, result, currentTurn);
                    for (int i = 0; i < 5; i++) drawSmallButton(bottomBtns[i], i == btnHover, 16);
                    if (gameOver && result >= 0) {
                        // 结果仅通过弹窗显示，此处不再绘制屏幕文字
                    }
                    drawStatusBar(L"");
                    FlushBatchDraw();
                }
            } else if (msg.message == WM_LBUTTONDOWN) {
                int btnClicked = -1;
                for (int i = 0; i < 5; i++) {
                    if (hitButton(bottomBtns[i], mx, my)) {
                        btnClicked = i; break;
                    }
                }
                if (btnClicked >= 0) {
                    if (btnClicked == 0) {
                        if (!gameOver && !aiThinking && canUndoNow()) {
                            undoMove();
                            if (historyCount > 0) { lastCol = history[historyCount-1].x; lastRow = history[historyCount-1].y; }
                            else { lastCol = -1; lastRow = -1; }
                            currentTurn = playerColor; drawOffered = 0;
                            doRedraw = 1; continue;
                        }
                    } else if (btnClicked == 1) {
                        if (!gameOver) {
                            gameOver = 1; result = 0; saveStats(0); playSoundEffect(2);
                            doRedraw = 1; continue;
                        }
                    } else if (btnClicked == 2) {
                        if (challengeMode) {
                            showHints = 1;
                            doRedraw = 1; continue;
                        }
                        if (!gameOver && !aiThinking) {
                            if (drawOffered) {
                                MessageBoxW(GetHWnd(), L"已提出求和，不可重复申请。", L"提示", MB_OK);
                                doRedraw = 1; continue;
                            }
                            drawOffered = 1;
                            int agree = 0;
                            if (difficulty == 1) agree = (rand() % 100 < 40);
                            else if (difficulty == 2) agree = (rand() % 100 < 60);
                            else {
                                int myScore = 0, opScore = 0;
                                int moves[20][2]; int n = generateMoves(moves, 20, aiColor);
                                for (int i = 0; i < n; i++) {
                                    int s1 = evaluatePointAdvanced(moves[i][0], moves[i][1], aiColor);
                                    int s2 = evaluatePointAdvanced(moves[i][0], moves[i][1], playerColor);
                                    if (s1 > myScore) myScore = s1;
                                    if (s2 > opScore) opScore = s2;
                                }
                                agree = (opScore > myScore + 5000);
                            }
                            if (agree) {
                                gameOver = 1; result = 2; saveStats(2); playSoundEffect(1);
                            } else {
                                MessageBoxW(GetHWnd(), L"本座拒绝和棋，继续！", L"天机", MB_OK);
                            }
                            doRedraw = 1; continue;
                        }
                    } else if (btnClicked == 3) {
                        if (challengeMode) {
                            loadChallengeBoard(challengeIndex);
                            gameOver = 0; result = -1; lastCol = -1; lastRow = -1;
                            currentTurn = playerColor; drawOffered = 0;
                            doRedraw = 1; continue;
                        }
                        if (gameOver) {
                            MessageBoxW(GetHWnd(), L"终局不再覆盖存档。点击棋盘可重开，或返回菜单。", L"存档", MB_OK);
                            doRedraw = 1; continue;
                        }
                        GameSave gs;
                        memset(&gs, 0, sizeof(gs));
                        for (int i = 0; i < BOARD_SIZE; i++)
                            for (int j = 0; j < BOARD_SIZE; j++)
                                gs.board[i][j] = board[i][j];
                        gs.historyCount = historyCount;
                        for (int i = 0; i < historyCount; i++) gs.history[i] = history[i];
                        gs.difficulty = difficulty; gs.useForbidden = useForbidden;
                        gs.playerColor = playerColor; gs.currentTurn = currentTurn;
                        gs.gameOver = gameOver; gs.result = result;
                        gs.version = SAVE_VERSION; gs.gameMode = gameMode;
                        gs.showHints = showHints; gs.turnPlaced = turnPlacedThisRound;
                        safeCopy(gs.owner, currentUser[0] ? currentUser : "Guest", 64);
                        gs.saveTime = (long long)time(NULL);
                        if (saveGameToFile(&gs)) {
                            MessageBoxW(GetHWnd(), L"存档成功", L"提示", MB_OK);
                        } else {
                            MessageBoxW(GetHWnd(), L"存档失败", L"错误", MB_OK);
                        }
                        doRedraw = 1; continue;
                    } else if (btnClicked == 4) {
                        int ans = MessageBoxW(GetHWnd(), L"返回主菜单？未保存的进度将丢失。", L"确认", MB_YESNO);
                        if (ans == IDYES) { gameRunning = 0; return; }
                        doRedraw = 1; continue;
                    }
                    continue;
                }

                if (gameOver) {
                    if (challengeMode) loadChallengeBoard(challengeIndex);
                    else initBoard();
                    gameOver = 0; result = -1; lastCol = -1; lastRow = -1; drawOffered = 0;
                    currentTurn = playerColor;
                    if (!challengeMode && playerColor == 2) {
                        aiThinking = 1;
                        redrawScene(-1, -1, lastCol, lastRow, gameOver, result);
                        drawInfoPanel(gameOver, result, currentTurn);
                        for (int i = 0; i < 5; i++) drawSmallButton(bottomBtns[i], -1, 16);
                        drawStatusBar(L""); FlushBatchDraw();
                        Sleep(300);
                        int aiResult = playAITurn(&lastCol, &lastRow);
                        if (aiResult == 1) { gameOver = 1; result = 0; saveStats(0); }
                        else if (aiResult == 2) { gameOver = 1; result = 2; saveStats(2); }
                        currentTurn = playerColor;
                        stepStartTime = GetTickCount();
                        aiThinking = 0;
                    }
                    doRedraw = 1; continue;
                }
                if (aiThinking) continue;
                if (currentTurn != playerColor) continue;
                int col, row;
                getMouseCell(mx, my, &col, &row);
                if (board[row][col] != 0) continue;
                if (useForbidden && playerColor == 1 && isForbiddenMove(col, row, 1)) {
                    MessageBoxW(GetHWnd(), L"禁手！此乃天道所不容之棋。", L"违规", MB_OK);
                    continue;
                }
                if (!placeStone(col, row, playerColor)) continue;
                lastCol = col; lastRow = row;
                turnPlacedThisRound++;
                if (challengeMode) challengePlayerMoves = countChallengePlayerMoves();
                drawOffered = 0;
                playSoundEffect(0);
                redrawScene(hoverCol, hoverRow, lastCol, lastRow, gameOver, result);
                drawInfoPanel(gameOver, result, currentTurn);
                for (int i = 0; i < 5; i++) drawSmallButton(bottomBtns[i], i == btnHover, 16);
                drawStatusBar(L""); FlushBatchDraw();
                if (checkWin(col, row, playerColor)) {
                    gameOver = 1; result = 1; saveStats(1); playSoundEffect(1);
                    doRedraw = 1; continue;
                }
                if (isBoardFull()) {
                    gameOver = 1; result = 2; saveStats(2); playSoundEffect(1);
                    doRedraw = 1; continue;
                }
                if (challengeMode) {
                    challengePlayerMoves = countChallengePlayerMoves();
                    if (challengePlayerMoves >= challengeMoveLimit) {
                        gameOver = 1; result = 0; playSoundEffect(2);
                        doRedraw = 1; continue;
                    }
                }
                if (turnPlacedThisRound < getTurnMoveNeed(playerColor)) {
                    stepStartTime = GetTickCount();
                    doRedraw = 1; continue;
                }
                turnPlacedThisRound = 0;
                currentTurn = aiColor;
                redrawScene(hoverCol, hoverRow, lastCol, lastRow, gameOver, result);
                drawInfoPanel(gameOver, result, currentTurn);
                for (int i = 0; i < 5; i++) drawSmallButton(bottomBtns[i], i == btnHover, 16);
                drawStatusBar(L""); FlushBatchDraw();
                aiThinking = 1;
                Sleep(200);
                int aiResult = playAITurn(&lastCol, &lastRow);
                currentTurn = playerColor; stepStartTime = GetTickCount();
                redrawScene(hoverCol, hoverRow, lastCol, lastRow, gameOver, result);
                drawInfoPanel(gameOver, result, currentTurn);
                for (int i = 0; i < 5; i++) drawSmallButton(bottomBtns[i], i == btnHover, 16);
                drawStatusBar(L""); FlushBatchDraw();
                if (aiResult == 1) {
                    gameOver = 1; result = 0; saveStats(0); playSoundEffect(2);
                    aiThinking = 0; doRedraw = 1; continue;
                }
                if (aiResult == 2 || isBoardFull()) {
                    gameOver = 1; result = 2; saveStats(2); playSoundEffect(1);
                    aiThinking = 0; doRedraw = 1; continue;
                }
                aiThinking = 0;
            }
        }
        Sleep(10);
    }
}

void startChallenge(int index) {
    int oldMode = gameMode;
    int oldForbidden = useForbidden;
    int oldPlayerColor = playerColor;
    int oldAiColor = aiColor;
    int oldTimeLimit = timeLimit;

    challengeMode = 1;
    challengeIndex = normalizeChallengeIndex(index);
    startGame(0);

    challengeMode = 0;
    challengeIndex = 0;
    challengeMoveLimit = 0;
    challengePlayerMoves = 0;
    challengeBaseHistory = 0;
    challengeBasePlayerStones = 0;
    gameMode = oldMode;
    useForbidden = oldForbidden;
    playerColor = oldPlayerColor;
    aiColor = oldAiColor;
    timeLimit = oldTimeLimit;
}

/* ===================== 主函数 ===================== */
int main() {
    srand((unsigned)time(NULL));
    ensureAdminKeyFile();
    initgraph(WIN_WIDTH, WIN_HEIGHT);
    lockWindowSize();
    setbkcolor(RGB(20, 20, 20));
    cleardevice();
    loadimage(&imgLogin, "bg_login.png", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&imgMenu, "bg_menu.png", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&imgGame, "bg_game.png", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&imgWin, "bg_win.png");
    BeginBatchDraw();
    loginScreen();
    while (1) {
        int choice = mainMenu();
        if (choice == 1) startGame(0);
        else if (choice == 2) {
            int index = challengeMenu();
            if (index >= 0) startChallenge(index);
        }
        else if (choice == 3) startGame(1);
        else if (choice == 4) gameSettings();
        else if (choice == 5) showPersonalStats();
        else if (choice == 6) showLeaderboard();
        else if (choice == 7) {
            if (isGuestUser()) resetGuestSessionData();
            loginScreen();
        }
        else if (choice == 8) {
            if (isGuestUser()) resetGuestSessionData();
            break;
        }
    }
    EndBatchDraw();
    closegraph();
    return 0;
}
