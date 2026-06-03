/*
 * 凡人修仙传 · 五子棋（人机对战）
 * 使用 EasyX 图形库开发
 * 功能：完善账号系统、强化AI、丰富规则、修仙风格界面
 * 编译：g++ 1.cpp -o 1.exe -leasyx -lgdi32 -limm32 -lmsimg32 -lole32 -loleaut32 -lwinhttp -lcrypt32 -finput-charset=UTF-8 -fexec-charset=UTF-8
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
#include <winhttp.h>
#include <wincrypt.h>

#define BOARD_SIZE 15
#define CELL_SIZE  48
#define MARGIN     55
#define WIN_WIDTH  1220
#define WIN_HEIGHT 860
#define BTN_W      260
#define BTN_H      52
#define MAX_HISTORY (BOARD_SIZE * BOARD_SIZE)
#define MODE_GOMOKU   0
#define MODE_CONNECT6 1
#define SAVE_VERSION  20260518
#define CHALLENGE_COUNT 3
#define RUNTIME_DATA_DIR "data"
#define ADMIN_KEY_FILE "admin_key.txt"
#define USERS_FILE "users.dat"
#define STATS_FILE "stats.dat"
#define LEGACY_SAVE_FILE "save.dat"
#define BG_LOGIN_FILE "bg_login.png"
#define BG_MENU_FILE "bg_menu.png"
#define BG_GAME_FILE "bg_game.png"
#define BG_WIN_FILE "bg_win.png"
#define RUNTIME_PATH_MAX 1024
#define DEFAULT_RESET_PASSWORD "123456"
#define MODEL_API_KEY_MAX 256
#define MODEL_NAME_TEXT "glm-5.1"
#define MODEL_API_HOST L"open.bigmodel.cn"
#define MODEL_API_PATH L"/api/paas/v4/chat/completions"
#define MODEL_CANDIDATE_MAX 18

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

struct StatRecord {
    char user[64];
    int difficulty;
    int result;
    int timeValue;
    int steps;
    int mode;
    int winLength;
    int modelFlag;
};

struct StatSummary {
    int wins[4];
    int losses[4];
    int draws[4];
    int modeWins[2];
    int modeGames[2];
    int totalGames;
    int currentStreak;
    int bestStreak;
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
int modelDuelMode = 0;
int modelDuelPlayerScore = 0;
int modelDuelModelScore = 0;
int modelDuelDrawScore = 0;
wchar_t modelDuelStatus[128] = L"";
wchar_t aiDialogueText[256] = L"";

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
int isForbiddenMove(int x, int y, int color);
int isInsideBoard(int x, int y);
int hasImmediateWin(int color, int* outX, int* outY);
int isLegalMoveForColor(int x, int y, int color);
int tryBuildModelChallengeBoard(int index);
int tryBuildModelChallengeBoardResponsive(int index);
void setModelDuelStatus(const wchar_t* text);
int validateModelApiKey();
int validateModelApiKeyResponsive();
void setAiDialogueBySituation(int eventType);
int parseStatLine(const char* line, StatRecord* rec);

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

void getExeDir(char* out, int maxLen) {
    if (!out || maxLen <= 0) return;
    out[0] = 0;
    char modulePath[RUNTIME_PATH_MAX] = {0};
    DWORD len = GetModuleFileNameA(NULL, modulePath, RUNTIME_PATH_MAX);
    if (len == 0 || len >= RUNTIME_PATH_MAX) return;
    char* slash = strrchr(modulePath, '\\');
    char* altSlash = strrchr(modulePath, '/');
    if (!slash || (altSlash && altSlash > slash)) slash = altSlash;
    if (!slash) return;
    slash[1] = 0;
    snprintf(out, maxLen, "%s", modulePath);
    out[maxLen - 1] = 0;
}

void buildExeRelativePath(const char* fileName, char* out, int maxLen) {
    if (!out || maxLen <= 0) return;
    out[0] = 0;
    if (!fileName || fileName[0] == 0) return;

    char exeDir[RUNTIME_PATH_MAX] = {0};
    getExeDir(exeDir, RUNTIME_PATH_MAX);
    if (exeDir[0]) snprintf(out, maxLen, "%s%s", exeDir, fileName);
    else snprintf(out, maxLen, "%s", fileName);
    out[maxLen - 1] = 0;
}

FILE* fopenExeRelative(const char* fileName, const char* mode) {
    char path[RUNTIME_PATH_MAX];
    buildExeRelativePath(fileName, path, RUNTIME_PATH_MAX);
    return fopen(path, mode);
}

void ensureRuntimeDataDir() {
    char dir[RUNTIME_PATH_MAX];
    buildExeRelativePath(RUNTIME_DATA_DIR, dir, RUNTIME_PATH_MAX);
    if (dir[0] == 0) return;
    DWORD attrs = GetFileAttributesA(dir);
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) return;
    CreateDirectoryA(dir, NULL);
}

void buildRuntimeDataPath(const char* fileName, char* out, int maxLen) {
    if (!out || maxLen <= 0) return;
    out[0] = 0;
    if (!fileName || fileName[0] == 0) return;

    ensureRuntimeDataDir();
    char exeDir[RUNTIME_PATH_MAX] = {0};
    getExeDir(exeDir, RUNTIME_PATH_MAX);
    if (exeDir[0]) snprintf(out, maxLen, "%s%s\\%s", exeDir, RUNTIME_DATA_DIR, fileName);
    else snprintf(out, maxLen, "%s\\%s", RUNTIME_DATA_DIR, fileName);
    out[maxLen - 1] = 0;
}

void migrateRootRuntimeFileToData(const char* fileName) {
    if (!fileName || fileName[0] == 0) return;
    char oldPath[RUNTIME_PATH_MAX];
    char newPath[RUNTIME_PATH_MAX];
    buildExeRelativePath(fileName, oldPath, RUNTIME_PATH_MAX);
    buildRuntimeDataPath(fileName, newPath, RUNTIME_PATH_MAX);
    if (oldPath[0] == 0 || newPath[0] == 0 || strcmp(oldPath, newPath) == 0) return;
    if (GetFileAttributesA(newPath) != INVALID_FILE_ATTRIBUTES) return;

    DWORD oldAttrs = GetFileAttributesA(oldPath);
    if (oldAttrs == INVALID_FILE_ATTRIBUTES || (oldAttrs & FILE_ATTRIBUTE_DIRECTORY)) return;
    MoveFileA(oldPath, newPath);
}

FILE* fopenRuntimeData(const char* fileName, const char* mode) {
    migrateRootRuntimeFileToData(fileName);
    char path[RUNTIME_PATH_MAX];
    buildRuntimeDataPath(fileName, path, RUNTIME_PATH_MAX);
    return fopen(path, mode);
}

void migrateFixedRuntimeFilesToData() {
    ensureRuntimeDataDir();
    migrateRootRuntimeFileToData(ADMIN_KEY_FILE);
    migrateRootRuntimeFileToData(USERS_FILE);
    migrateRootRuntimeFileToData(STATS_FILE);
    migrateRootRuntimeFileToData(LEGACY_SAVE_FILE);
}

void buildUserSavePath(const char* name, char* path, int maxLen) {
    const char* owner = (name && name[0]) ? name : "Guest";
    char fileName[64];
    snprintf(fileName, sizeof(fileName), "save_%08X.dat", simpleHash(owner));
    fileName[sizeof(fileName) - 1] = 0;
    migrateRootRuntimeFileToData(fileName);
    buildRuntimeDataPath(fileName, path, maxLen);
}

int loadAdminKey(char* out, int maxLen) {
    if (!out || maxLen <= 0) return 0;
    out[0] = 0;
    FILE* fp = fopenRuntimeData(ADMIN_KEY_FILE, "r");
    if (fp) {
        if (fgets(out, maxLen, fp)) trimLineEnd(out);
        fclose(fp);
    }
    return out[0] != 0;
}

void buildUserModelKeyPath(const char* name, char* path, int maxLen) {
    const char* owner = (name && name[0]) ? name : "Guest";
    char fileName[64];
    snprintf(fileName, sizeof(fileName), "glm_key_%08X.dat", simpleHash(owner));
    fileName[sizeof(fileName) - 1] = 0;
    migrateRootRuntimeFileToData(fileName);
    buildRuntimeDataPath(fileName, path, maxLen);
}

void removeModelKeyForUser(const char* name) {
    const char* owner = (name && name[0]) ? name : "Guest";
    char fileName[64];
    snprintf(fileName, sizeof(fileName), "glm_key_%08X.dat", simpleHash(owner));
    fileName[sizeof(fileName) - 1] = 0;

    char path[RUNTIME_PATH_MAX];
    buildUserModelKeyPath(name, path, RUNTIME_PATH_MAX);
    remove(path);

    char oldPath[RUNTIME_PATH_MAX];
    buildExeRelativePath(fileName, oldPath, RUNTIME_PATH_MAX);
    if (strcmp(path, oldPath) != 0) remove(oldPath);
}

int saveModelApiKey(const char* apiKey) {
    if (!apiKey || apiKey[0] == 0) return 0;
    char path[RUNTIME_PATH_MAX];
    const char* owner = currentUser[0] ? currentUser : "Guest";
    buildUserModelKeyPath(owner, path, RUNTIME_PATH_MAX);

    DATA_BLOB in;
    DATA_BLOB out;
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.pbData = (BYTE*)apiKey;
    in.cbData = (DWORD)strlen(apiKey);
    if (!CryptProtectData(&in, L"wuziqi glm api key", NULL, NULL, NULL,
                          CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        return 0;
    }

    FILE* fp = fopen(path, "wb");
    if (!fp) {
        LocalFree(out.pbData);
        return 0;
    }
    size_t written = fwrite(out.pbData, 1, out.cbData, fp);
    fclose(fp);
    LocalFree(out.pbData);
    return written == out.cbData;
}

int loadModelApiKey(char* out, int maxLen) {
    if (!out || maxLen <= 0) return 0;
    out[0] = 0;
    char path[RUNTIME_PATH_MAX];
    const char* owner = currentUser[0] ? currentUser : "Guest";
    buildUserModelKeyPath(owner, path, RUNTIME_PATH_MAX);

    FILE* fp = fopen(path, "rb");
    if (!fp) return 0;
    unsigned char encrypted[2048];
    size_t len = fread(encrypted, 1, sizeof(encrypted), fp);
    int tooLarge = !feof(fp);
    fclose(fp);
    if (len == 0 || tooLarge) return 0;

    DATA_BLOB in;
    DATA_BLOB plain;
    memset(&in, 0, sizeof(in));
    memset(&plain, 0, sizeof(plain));
    in.pbData = encrypted;
    in.cbData = (DWORD)len;
    if (!CryptUnprotectData(&in, NULL, NULL, NULL, NULL,
                            CRYPTPROTECT_UI_FORBIDDEN, &plain)) {
        return 0;
    }
    int copyLen = (plain.cbData < (DWORD)(maxLen - 1)) ? (int)plain.cbData : maxLen - 1;
    memcpy(out, plain.pbData, copyLen);
    out[copyLen] = 0;
    LocalFree(plain.pbData);
    return out[0] != 0;
}

int hasModelApiKey() {
    char key[MODEL_API_KEY_MAX];
    return loadModelApiKey(key, MODEL_API_KEY_MAX);
}

void clearModelDuelSession() {
    modelDuelMode = 0;
    modelDuelPlayerScore = 0;
    modelDuelModelScore = 0;
    modelDuelDrawScore = 0;
    modelDuelStatus[0] = 0;
}

void resetModelDuelScore() {
    modelDuelPlayerScore = 0;
    modelDuelModelScore = 0;
    modelDuelDrawScore = 0;
    wcscpy(modelDuelStatus, L"GLM-5.1 已就绪");
}

void setAiDialogueBySituation(int eventType) {
    const wchar_t* lines[20];
    int count = 0;
    switch (eventType) {
        case 0:
            lines[count++] = L"哼，区区炼气期，也敢挑战本座？";
            lines[count++] = L"今日便让你见识一下，何为天机！";
            lines[count++] = L"落子无悔，这便是修仙界的规矩。";
            break;
        case 1:
            lines[count++] = L"此子倒是有点意思...";
            lines[count++] = L"雕虫小技，也敢班门弄斧？";
            lines[count++] = L"这步棋，倒是有几分韩老魔的风骨。";
            lines[count++] = L"本座倒要看看，你还能撑几手。";
            break;
        case 2:
            lines[count++] = L"本座这一手，你可看清楚了？";
            lines[count++] = L"莫要得意，本座还有后手。";
            lines[count++] = L"天机不可泄露，但这局...你输定了。";
            lines[count++] = L"这一子，便是你的劫数。";
            break;
        case 3:
            lines[count++] = L"竟然...竟然被你寻到了生门...";
            lines[count++] = L"此局是本座失算，道友这一手，算得上漂亮。";
            break;
        case 4:
            lines[count++] = L"回去再修三百年吧。";
            lines[count++] = L"区区炼气期，也敢挑战本座？";
            break;
        case 5:
            lines[count++] = L"今日算你命大，且饶你一局。";
            lines[count++] = L"天道无常，棋道亦然。";
            break;
    }
    if (count > 0) {
        wcscpy(aiDialogueText, lines[rand() % count]);
    }
}

void enterUserSession(const char* name, int level, int exp) {
    safeCopy(currentUser, name && name[0] ? name : "Guest", 64);
    currentUserLevel = level;
    currentUserExp = exp;
    clearModelDuelSession();
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

void pumpUiMessages() {
    MSG winMsg;
    while (PeekMessageW(&winMsg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&winMsg);
        DispatchMessageW(&winMsg);
    }
    ExMessage ignored;
    while (peekmessage(&ignored, EX_MOUSE)) {
    }
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

int parseUserLine(const char* line, UserInfo* out) {
    if (!line || !out) return 0;
    UserInfo tmp;
    if (sscanf(line, "%63s %32s %d %d %lld", tmp.name, tmp.pwdHash, &tmp.exp, &tmp.level, &tmp.regTime) == 5) {
        normalizeUserInfo(&tmp);
        *out = tmp;
        return 1;
    }
    if (sscanf(line, "%63s %32s", tmp.name, tmp.pwdHash) == 2) {
        tmp.exp = 0; tmp.level = 1; tmp.regTime = 0;
        normalizeUserInfo(&tmp);
        *out = tmp;
        return 1;
    }
    return 0;
}

int writeUserLine(FILE* fp, const UserInfo* u) {
    if (!fp || !u) return 0;
    return fprintf(fp, "%s %s %d %d %lld\n", u->name, u->pwdHash, u->exp, u->level, u->regTime) > 0;
}

int userNameExists(const char* name) {
    if (!name || name[0] == 0) return 0;
    FILE* fp = fopenRuntimeData(USERS_FILE, "r");
    if (!fp) return 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        UserInfo tmp;
        if (parseUserLine(line, &tmp) && strcmp(tmp.name, name) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int saveUser(const UserInfo* u) {
    if (!u || u->name[0] == 0 || userNameExists(u->name)) return 0;
    UserInfo fixed = *u;
    normalizeUserInfo(&fixed);
    FILE* fp = fopenRuntimeData(USERS_FILE, "a");
    if (!fp) return 0;
    int ok = writeUserLine(fp, &fixed);
    fclose(fp);
    return ok;
}

int replaceRuntimeDataFile(const char* tmpFileName, const char* targetFileName) {
    char tmpPath[RUNTIME_PATH_MAX];
    char targetPath[RUNTIME_PATH_MAX];
    buildRuntimeDataPath(tmpFileName, tmpPath, RUNTIME_PATH_MAX);
    buildRuntimeDataPath(targetFileName, targetPath, RUNTIME_PATH_MAX);
    if (tmpPath[0] == 0 || targetPath[0] == 0) return 0;
    if (MoveFileExA(tmpPath, targetPath, MOVEFILE_REPLACE_EXISTING)) return 1;
    remove(tmpPath);
    return 0;
}

int rewriteUserWithUpsert(const UserInfo* u) {
    if (!u || u->name[0] == 0) return 0;
    UserInfo fixed = *u;
    normalizeUserInfo(&fixed);

    FILE* in = fopenRuntimeData(USERS_FILE, "r");
    FILE* out = fopenRuntimeData("users.tmp", "w");
    if (!out) {
        if (in) fclose(in);
        return 0;
    }

    int found = 0;
    int ok = 1;
    if (in) {
        char line[256];
        while (fgets(line, sizeof(line), in)) {
            UserInfo tmp;
            if (parseUserLine(line, &tmp) && strcmp(tmp.name, fixed.name) == 0) {
                if (!writeUserLine(out, &fixed)) ok = 0;
                found = 1;
            } else {
                if (fputs(line, out) == EOF) ok = 0;
            }
        }
        fclose(in);
    }
    if (!found && !writeUserLine(out, &fixed)) ok = 0;
    if (fclose(out) != 0) ok = 0;
    if (!ok) {
        char tmpPath[RUNTIME_PATH_MAX];
        buildRuntimeDataPath("users.tmp", tmpPath, RUNTIME_PATH_MAX);
        remove(tmpPath);
        return 0;
    }
    return replaceRuntimeDataFile("users.tmp", USERS_FILE);
}

int rewriteUsersWithoutName(const char* name) {
    if (!name || name[0] == 0) return 0;
    FILE* in = fopenRuntimeData(USERS_FILE, "r");
    if (!in) return 0;
    FILE* out = fopenRuntimeData("users.tmp", "w");
    if (!out) {
        fclose(in);
        return 0;
    }

    int found = 0;
    int ok = 1;
    char line[256];
    while (fgets(line, sizeof(line), in)) {
        UserInfo tmp;
        if (parseUserLine(line, &tmp) && strcmp(tmp.name, name) == 0) {
            found = 1;
            continue;
        }
        if (fputs(line, out) == EOF) ok = 0;
    }
    fclose(in);
    if (fclose(out) != 0) ok = 0;
    if (!ok || !found) {
        char tmpPath[RUNTIME_PATH_MAX];
        buildRuntimeDataPath("users.tmp", tmpPath, RUNTIME_PATH_MAX);
        remove(tmpPath);
        return 0;
    }
    return replaceRuntimeDataFile("users.tmp", USERS_FILE);
}

int rewriteStatsForUser(const char* name, int filterModelFlag, int modelFlag) {
    if (!name || name[0] == 0) return 0;
    FILE* in = fopenRuntimeData(STATS_FILE, "r");
    if (!in) return 1;
    FILE* out = fopenRuntimeData("stats.tmp", "w");
    if (!out) {
        fclose(in);
        return 0;
    }

    int ok = 1;
    char line[512];
    while (fgets(line, sizeof(line), in)) {
        StatRecord rec;
        int removeLine = parseStatLine(line, &rec) &&
                         strcmp(rec.user, name) == 0 &&
                         (!filterModelFlag || rec.modelFlag == modelFlag);
        if (removeLine) continue;
        if (fputs(line, out) == EOF) ok = 0;
    }
    fclose(in);
    if (fclose(out) != 0) ok = 0;
    if (!ok) {
        char tmpPath[RUNTIME_PATH_MAX];
        buildRuntimeDataPath("stats.tmp", tmpPath, RUNTIME_PATH_MAX);
        remove(tmpPath);
        return 0;
    }
    return replaceRuntimeDataFile("stats.tmp", STATS_FILE);
}

int loadUsers(UserInfo out[], int maxCount) {
    FILE* fp = fopenRuntimeData(USERS_FILE, "r");
    if (!fp) return 0;
    int count = 0;
    while (count < maxCount) {
        char line[256];
        if (!fgets(line, sizeof(line), fp)) break;
        UserInfo tmp;
        if (parseUserLine(line, &tmp)) out[count++] = tmp;
    }
    fclose(fp);
    return count;
}

int findUser(const char* name, UserInfo* out) {
    if (!name || name[0] == 0) return 0;
    FILE* fp = fopenRuntimeData(USERS_FILE, "r");
    if (fp) {
        int found = 0;
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            UserInfo tmp;
            if (parseUserLine(line, &tmp) && strcmp(tmp.name, name) == 0) {
                if (out) *out = tmp;
                found = 1;
            }
        }
        fclose(fp);
        return found;
    }
    return 0;
}

void updateUser(const UserInfo* u) {
    rewriteUserWithUpsert(u);
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
    if (modelDuelMode) {
        if (result == 1) modelDuelPlayerScore++;
        else if (result == 0) modelDuelModelScore++;
        else if (result == 2) modelDuelDrawScore++;
    }
    FILE* fp = fopenRuntimeData(STATS_FILE, "a");
    if (fp) {
        fprintf(fp, "%s %d %d %d %d %d %d %d\n",
                currentUser, difficulty, result, (int)time(NULL), stepCount,
                gameMode, getWinLength(), modelDuelMode ? 1 : 0);
        fclose(fp);
    }
    if (strcmp(currentUser, "Guest") != 0) {
        int delta = 5;
        if (result == 1) delta = 18 + difficulty * 6 + (gameMode == MODE_CONNECT6 ? 8 : 0);
        else if (result == 2) delta = 8 + difficulty * 2;
        addUserExp(currentUser, delta);
    }
}

int parseStatLine(const char* line, StatRecord* rec) {
    if (!line || !rec) return 0;
    memset(rec, 0, sizeof(StatRecord));
    rec->mode = MODE_GOMOKU;
    rec->winLength = 5;
    rec->modelFlag = 0;
    int parsed = sscanf(line, "%63s %d %d %d %d %d %d %d",
                        rec->user, &rec->difficulty, &rec->result, &rec->timeValue,
                        &rec->steps, &rec->mode, &rec->winLength, &rec->modelFlag);
    if (parsed < 3) return 0;
    if (parsed < 6) rec->mode = MODE_GOMOKU;
    if (parsed < 7) rec->winLength = (rec->mode == MODE_CONNECT6) ? 6 : 5;
    if (parsed < 8) rec->modelFlag = 0;
    if (rec->difficulty < 1 || rec->difficulty > 3) return 0;
    if (rec->result < 0 || rec->result > 2) return 0;
    if (rec->mode != MODE_GOMOKU && rec->mode != MODE_CONNECT6) rec->mode = MODE_GOMOKU;
    rec->modelFlag = rec->modelFlag ? 1 : 0;
    return 1;
}

void initStatSummary(StatSummary* s) {
    if (!s) return;
    memset(s, 0, sizeof(StatSummary));
}

void addStatToSummary(StatSummary* s, const StatRecord* rec) {
    if (!s || !rec) return;
    int diff = rec->difficulty;
    if (rec->result == 1) s->wins[diff]++;
    else if (rec->result == 0) s->losses[diff]++;
    else s->draws[diff]++;
    s->modeGames[rec->mode]++;
    if (rec->result == 1) s->modeWins[rec->mode]++;
    if (rec->result == 1) {
        s->currentStreak++;
        if (s->currentStreak > s->bestStreak) s->bestStreak = s->currentStreak;
    } else {
        s->currentStreak = 0;
    }
    s->totalGames++;
}

void loadUserStatSummary(int modelFlag, StatSummary* out) {
    initStatSummary(out);
    FILE* fp = fopenRuntimeData(STATS_FILE, "r");
    if (!fp) return;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        StatRecord rec;
        if (!parseStatLine(line, &rec)) continue;
        if (strcmp(rec.user, currentUser) != 0) continue;
        if (rec.modelFlag != modelFlag) continue;
        addStatToSummary(out, &rec);
    }
    fclose(fp);
}

void resetUserStatsByModelFlag(int modelFlag) {
    if (currentUser[0] == 0) return;
    rewriteStatsForUser(currentUser, 1, modelFlag);
}

void resetUserStats() {
    if (currentUser[0] == 0) return;
    rewriteStatsForUser(currentUser, 0, 0);
}

void removeStatsForUser(const char* name) {
    if (!name || name[0] == 0) return;
    rewriteStatsForUser(name, 0, 0);
}

void removeSaveForUser(const char* name) {
    const char* owner = (name && name[0]) ? name : "Guest";
    char fileName[64];
    snprintf(fileName, sizeof(fileName), "save_%08X.dat", simpleHash(owner));
    fileName[sizeof(fileName) - 1] = 0;

    char path[RUNTIME_PATH_MAX];
    buildUserSavePath(name, path, RUNTIME_PATH_MAX);
    remove(path);

    char oldPath[RUNTIME_PATH_MAX];
    buildExeRelativePath(fileName, oldPath, RUNTIME_PATH_MAX);
    if (strcmp(path, oldPath) != 0) remove(oldPath);
}

int deleteUserAccount(const char* name) {
    if (!name || name[0] == 0 || strcmp(name, "Guest") == 0) return 0;
    int removed = rewriteUsersWithoutName(name);
    if (removed) {
        removeStatsForUser(name);
        removeSaveForUser(name);
        removeModelKeyForUser(name);
    }
    return removed;
}

void resetGuestSessionData() {
    clearModelDuelSession();
    removeStatsForUser("Guest");
    removeSaveForUser("Guest");
    removeModelKeyForUser("Guest");
    if (strcmp(currentUser, "Guest") == 0) {
        currentUserLevel = 1;
        currentUserExp = 0;
    }
}

int saveGameToFile(const GameSave* gs) {
    char path[RUNTIME_PATH_MAX];
    const char* owner = (currentUser[0] ? currentUser : "Guest");
    buildUserSavePath(owner, path, RUNTIME_PATH_MAX);

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
    char path[RUNTIME_PATH_MAX];
    const char* owner = (currentUser[0] ? currentUser : "Guest");
    buildUserSavePath(owner, path, RUNTIME_PATH_MAX);
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
    fp = fopenRuntimeData(LEGACY_SAVE_FILE, "rb"); // 兼容旧版单存档
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
    char path[RUNTIME_PATH_MAX];
    const char* owner = (currentUser[0] ? currentUser : "Guest");
    buildUserSavePath(owner, path, RUNTIME_PATH_MAX);
    if (savePathUsableForCurrentUser(path)) return 1;
    char legacyPath[RUNTIME_PATH_MAX];
    migrateRootRuntimeFileToData(LEGACY_SAVE_FILE);
    buildRuntimeDataPath(LEGACY_SAVE_FILE, legacyPath, RUNTIME_PATH_MAX);
    return savePathUsableForCurrentUser(legacyPath);
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

    int builtByModel = 0;
    if (modelDuelMode && !isGuestUser() && hasModelApiKey()) {
        builtByModel = tryBuildModelChallengeBoardResponsive(index);
        setModelDuelStatus(builtByModel ? L"GLM-5.1 已生成残局" : L"GLM残局失败，本地生成");
    } else if (modelDuelMode) {
        setModelDuelStatus(L"未配置API Key，本地生成残局");
    }

    if (!builtByModel && !tryBuildRandomChallenge(index)) {
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

int getBoardClickCell(int mx, int my, int* col, int* row) {
    int left = getCellX(0) - CELL_SIZE / 2;
    int right = getCellX(BOARD_SIZE - 1) + CELL_SIZE / 2;
    int top = getCellY(0) - CELL_SIZE / 2;
    int bottom = getCellY(BOARD_SIZE - 1) + CELL_SIZE / 2;
    if (mx < left || mx > right || my < top || my > bottom) {
        if (col) *col = -1;
        if (row) *row = -1;
        return 0;
    }
    getMouseCell(mx, my, col, row);
    return 1;
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
        const wchar_t* winLine1 = challengeMode ? L"天机：此阵竟被你寻到生门。"
            : (modelDuelMode ? L"GLM-5.1：此局我已记下。" : L"天机：此局是本座失算。");
        const wchar_t* winLine2 = challengeMode ? L"这一手，算得上破局。"
            : (modelDuelMode ? L"点击继续下一局，比分继续累积。" : L"道友这一手，算得上漂亮。");
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
    const wchar_t* title = (result == 0) ? (modelDuelMode && !challengeMode ? L"模型胜" : L"天机胜") : L"平局";
    const wchar_t* line1 = (result == 0)
        ? (challengeMode ? L"天机：残局最忌贪心，错一手便无路。"
                         : (modelDuelMode ? L"GLM-5.1：这一局由我拿下。" : L"天机：区区炼气期，也敢挑战本座？"))
        : (modelDuelMode ? L"GLM-5.1：此局平分秋色。" : L"天机：今日算你命大，且饶你一局。");
    const wchar_t* line2 = (result == 0)
        ? (challengeMode ? L"换一招，再来破阵。"
                         : (modelDuelMode ? L"点击继续下一局，比分继续累积。" : L"回去再修三百年吧。"))
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

void drawBoxCenterText(int x, int w, int y, const wchar_t* str, int height, COLORREF c) {
    settextstyle(height, 0, "SimHei");
    HDC hdc = GetImageHDC(NULL);
    SetBkMode(hdc, TRANSPARENT);
    SIZE sz;
    GetTextExtentPoint32W(hdc, str, wcslen(str), &sz);
    int tx = x + (w - sz.cx) / 2;
    SetTextColor(hdc, RGB(0, 0, 0));
    TextOutW(hdc, tx + 2, y + 2, str, wcslen(str));
    SetTextColor(hdc, c);
    TextOutW(hdc, tx, y, str, wcslen(str));
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
    (void)result;
    HDC hdc = GetImageHDC(NULL);
    SetBkMode(hdc, TRANSPARENT);
    int px = 770, py = 82, pw = WIN_WIDTH - 810;
    if (pw < 360) pw = 360;
    int panelBottom = WIN_HEIGHT - 180;
    setfillcolor(PANEL_BG);
    setlinecolor(PANEL_BORDER);
    fillroundrect(px - 14, py - 16, px + pw + 14, panelBottom, 14, 14);
    roundrect(px - 10, py - 12, px + pw + 10, panelBottom - 4, 12, 12);
    wchar_t buf[160];
    drawBoxCenterText(px, pw, py, L"对局信息", 26, ACCENT_GOLD);
    py += 40;
    setlinecolor(RGB(100, 85, 58));
    line(px + 28, py, px + pw - 28, py);
    py += 20;
    wchar_t wuser[64]; asciiToWchar(currentUser, wuser, 64);
    wsprintfW(buf, L"道友: %s", wuser);
    drawBoxCenterText(px, pw, py, buf, 18, TEXT_WHITE);
    py += 30;
    const wchar_t* realm = getXianLevelName(currentUserExp);
    wsprintfW(buf, L"境界: %s", realm);
    drawBoxCenterText(px, pw, py, buf, 18, ACCENT_GOLD);
    py += 30;
    const wchar_t* diffName[4] = {L"", L"简单", L"中等", L"困难"};
    wsprintfW(buf, L"难度: %s", diffName[difficulty]);
    drawBoxCenterText(px, pw, py, buf, 18, TEXT_WHITE);
    py += 34;
    setlinecolor(RGB(72, 62, 48));
    line(px + 48, py, px + pw - 48, py);
    py += 18;
    const wchar_t* panelModeText = challengeMode ? L"残局挑战" : (modelDuelMode ? L"模型对局" : getModeName());
    wsprintfW(buf, L"模式: %s", panelModeText);
    drawBoxCenterText(px, pw, py, buf, 18, TEXT_WHITE);
    py += 30;
    if (!challengeMode && modelDuelMode) {
        wsprintfW(buf, L"规则: %s", getModeName());
        drawBoxCenterText(px, pw, py, buf, 18, TEXT_WHITE);
        py += 30;
        drawBoxCenterText(px, pw, py, L"模型: GLM-5.1", 18, ACCENT_GOLD);
        py += 30;
        wsprintfW(buf, L"比分: 道友%d 模型%d 平%d",
                  modelDuelPlayerScore, modelDuelModelScore, modelDuelDrawScore);
        drawBoxCenterText(px, pw, py, buf, 16, TEXT_WHITE);
        py += 28;
        if (modelDuelStatus[0]) {
            drawBoxCenterText(px, pw, py, modelDuelStatus, 16, RGB(220, 205, 160));
            py += 28;
        }
    }
    wsprintfW(buf, L"目标: 连%d成阵", getWinLength());
    drawBoxCenterText(px, pw, py, buf, 18, TEXT_WHITE);
    py += 30;
    if (challengeMode) {
        wsprintfW(buf, L"残局: %s", getChallengeName());
        drawBoxCenterText(px, pw, py, buf, 18, ACCENT_GOLD);
        py += 30;
        int remainMoves = challengeMoveLimit - challengePlayerMoves;
        if (remainMoves < 0) remainMoves = 0;
        wsprintfW(buf, L"限手: 剩%d/%d", remainMoves, challengeMoveLimit);
        drawBoxCenterText(px, pw, py, buf, 18, remainMoves <= 1 ? RGB(255, 120, 100) : TEXT_WHITE);
        py += 30;
    }
    wsprintfW(buf, L"执子: %s", (playerColor == 1) ? L"玄黑(先)" : L"玉白(后)");
    drawBoxCenterText(px, pw, py, buf, 18, TEXT_WHITE);
    py += 30;
    wsprintfW(buf, L"规则: %s", (gameMode == MODE_CONNECT6) ? L"六连无禁手" : (useForbidden ? L"有禁手" : L"无禁手"));
    drawBoxCenterText(px, pw, py, buf, 18, TEXT_WHITE);
    py += 30;
    wsprintfW(buf, L"心眼: %s", showHints ? L"开启" : L"关闭");
    drawBoxCenterText(px, pw, py, buf, 18, showHints ? ACCENT_GOLD : TEXT_WHITE);
    py += 30;
    wsprintfW(buf, L"步数: %d", stepCount);
    drawBoxCenterText(px, pw, py, buf, 18, TEXT_WHITE);
    py += 34;
    setlinecolor(RGB(72, 62, 48));
    line(px + 48, py, px + pw - 48, py);
    py += 18;
    if (!gameOver) {
        wsprintfW(buf, L"回合: %s", (currentTurn == playerColor) ? L"道友" : (modelDuelMode ? L"GLM-5.1" : L"天机"));
        drawBoxCenterText(px, pw, py, buf, 18, ACCENT_GOLD);
        py += 30;
        int need = getTurnMoveNeed(currentTurn);
        int remain = need - turnPlacedThisRound;
        if (remain < 1) remain = 1;
        wsprintfW(buf, L"本回合: 还落%d子", remain);
        drawBoxCenterText(px, pw, py, buf, 18, TEXT_WHITE);
        py += 30;
    }
    if (gameRunning && !gameOver && timeLimit > 0) {
        DWORD elapsed = (GetTickCount() - stepStartTime) / 1000;
        int remain = timeLimit - (int)elapsed;
        if (remain < 0) remain = 0;
        wsprintfW(buf, L"剩余: %d秒", remain);
        COLORREF tc = (remain <= 5) ? RGB(255, 80, 80) : TEXT_WHITE;
        drawBoxCenterText(px, pw, py, buf, 18, tc);
        py += 30;
    }
    if (!gameOver && showHints) {
        COLORREF sitColor = TEXT_WHITE;
        if (getSituationText(buf, &sitColor)) {
            drawBoxCenterText(px, pw, py, buf, 18, sitColor);
            py += 30;
        }
    }

    int dh = 76;
    int dlgTop = WIN_HEIGHT - 176;
    setfillcolor(RGB(18, 16, 24));
    setlinecolor(PANEL_BORDER);
    fillroundrect(px - 10, dlgTop, px + pw + 10, dlgTop + dh, 12, 12);
    roundrect(px - 6, dlgTop + 4, px + pw + 6, dlgTop + dh - 4, 10, 10);
    HDC hdc2 = GetImageHDC(NULL);
    SetBkMode(hdc2, TRANSPARENT);
    SetTextColor(hdc2, RGB(220, 205, 160));
    settextstyle(15, 0, "SimHei");
    if (aiDialogueText[0]) {
        SIZE sz;
        GetTextExtentPoint32W(hdc2, aiDialogueText, wcslen(aiDialogueText), &sz);
        if (sz.cx <= pw + 8) {
            int tx = px + (pw - sz.cx) / 2;
            TextOutW(hdc2, tx, dlgTop + 26, aiDialogueText, wcslen(aiDialogueText));
        } else {
            int half = (int)wcslen(aiDialogueText) / 2;
            TextOutW(hdc2, px + 10, dlgTop + 12, aiDialogueText, half);
            TextOutW(hdc2, px + 10, dlgTop + 38, aiDialogueText + half, (int)wcslen(aiDialogueText) - half);
        }
    }
}

void drawStatusBar(const wchar_t* extra) {
    HDC hdc = GetImageHDC(NULL);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, TEXT_WHITE);
    settextstyle(18, 0, "SimHei");
    TextOutW(hdc, 20, WIN_HEIGHT - 32, L"", 0);
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

/* ===================== 大模型对局 ===================== */
void setModelDuelStatus(const wchar_t* text) {
    if (!text) return;
    wcsncpy(modelDuelStatus, text, 127);
    modelDuelStatus[127] = 0;
}

void jsonEscape(const char* src, char* dst, int maxLen) {
    if (!src || !dst || maxLen <= 0) return;
    int pos = 0;
    for (int i = 0; src[i] && pos < maxLen - 1; i++) {
        unsigned char ch = (unsigned char)src[i];
        const char* rep = NULL;
        if (ch == '\\') rep = "\\\\";
        else if (ch == '"') rep = "\\\"";
        else if (ch == '\n') rep = "\\n";
        else if (ch == '\r') rep = "\\r";
        else if (ch == '\t') rep = "\\t";
        if (rep) {
            for (int k = 0; rep[k] && pos < maxLen - 1; k++) dst[pos++] = rep[k];
        } else if (ch < 32) {
            if (pos + 6 >= maxLen) break;
            snprintf(dst + pos, maxLen - pos, "\\u%04X", ch);
            pos += 6;
        } else {
            dst[pos++] = src[i];
        }
    }
    dst[pos] = 0;
}

void buildBoardText(char* out, int maxLen) {
    if (!out || maxLen <= 0) return;
    out[0] = 0;
    int pos = 0;
    for (int y = 0; y < BOARD_SIZE && pos < maxLen - 1; y++) {
        int written = snprintf(out + pos, maxLen - pos, "%02d ", y);
        if (written < 0 || written >= maxLen - pos) break;
        pos += written;
        for (int x = 0; x < BOARD_SIZE && pos < maxLen - 1; x++) {
            char ch = '.';
            if (board[y][x] == 1) ch = 'B';
            else if (board[y][x] == 2) ch = 'W';
            out[pos++] = ch;
        }
        if (pos < maxLen - 1) out[pos++] = '\n';
        out[pos] = 0;
    }
}

struct ModelCandidate {
    int id;
    Move moves[2];
    int count;
    int score;
    char note[48];
};

int scoreModelPoint(int x, int y, int color, char* note, int noteMax) {
    if (!isLegalMoveForColor(x, y, color)) return -999999999;
    int opColor = 3 - color;
    int myShape = evaluatePointAdvanced(x, y, color);
    int opShape = evaluatePointAdvanced(x, y, opColor);
    int score = myShape * 3 + opShape * 2;
    const char* reason = "布局";

    board[y][x] = color;
    int myWin = checkWin(x, y, color);
    int leavesOpponentWin = hasImmediateWin(opColor, NULL, NULL);
    board[y][x] = 0;
    if (myWin) {
        score += 500000000;
        reason = "立即取胜";
    } else {
        board[y][x] = opColor;
        int blocksWin = checkWin(x, y, opColor);
        board[y][x] = 0;
        if (blocksWin) {
            score += 450000000;
            reason = "必须防守";
        } else if (leavesOpponentWin) {
            score -= 120000000;
            reason = "防守风险";
        } else if (myShape >= opShape * 2 && myShape >= 10000) {
            reason = "主动进攻";
        } else if (opShape >= myShape && opShape >= 10000) {
            reason = "压制威胁";
        }
    }

    int dist = abs(x - BOARD_SIZE / 2) + abs(y - BOARD_SIZE / 2);
    score -= dist * 5;
    if (note && noteMax > 0) safeCopy(note, reason, noteMax);
    return score;
}

int modelCandidateSame(const ModelCandidate* a, const ModelCandidate* b) {
    if (!a || !b || a->count != b->count) return 0;
    for (int i = 0; i < a->count; i++) {
        if (a->moves[i].x != b->moves[i].x || a->moves[i].y != b->moves[i].y) return 0;
    }
    return 1;
}

void pushModelCandidate(ModelCandidate cands[], int* count, int maxCount, const ModelCandidate* cand) {
    if (!cands || !count || !cand || maxCount <= 0) return;
    for (int i = 0; i < *count; i++) {
        if (modelCandidateSame(&cands[i], cand)) return;
    }
    int pos = *count;
    if (*count < maxCount) {
        (*count)++;
    } else if (cand->score <= cands[*count - 1].score) {
        return;
    } else {
        pos = *count - 1;
    }
    while (pos > 0 && cands[pos - 1].score < cand->score) {
        cands[pos] = cands[pos - 1];
        pos--;
    }
    cands[pos] = *cand;
    for (int i = 0; i < *count; i++) cands[i].id = i + 1;
}

int collectModelSingleCandidates(ModelCandidate cands[], int maxCount, int color) {
    int count = 0;
    int hasStone = 0;
    int nearby[BOARD_SIZE][BOARD_SIZE];
    memset(nearby, 0, sizeof(nearby));
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board[y][x] == 0) continue;
            hasStone = 1;
            for (int dy = -3; dy <= 3; dy++) {
                for (int dx = -3; dx <= 3; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (isInsideBoard(nx, ny)) nearby[ny][nx] = 1;
                }
            }
        }
    }
    if (!hasStone) {
        ModelCandidate cand;
        memset(&cand, 0, sizeof(cand));
        cand.count = 1;
        cand.moves[0].x = BOARD_SIZE / 2;
        cand.moves[0].y = BOARD_SIZE / 2;
        cand.score = 1000000;
        safeCopy(cand.note, "占据天元", 48);
        pushModelCandidate(cands, &count, maxCount, &cand);
        return count;
    }

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (!nearby[y][x]) continue;
            if (!isLegalMoveForColor(x, y, color)) continue;
            ModelCandidate cand;
            memset(&cand, 0, sizeof(cand));
            cand.count = 1;
            cand.moves[0].x = x;
            cand.moves[0].y = y;
            cand.score = scoreModelPoint(x, y, color, cand.note, 48);
            pushModelCandidate(cands, &count, maxCount, &cand);
        }
    }
    return count;
}

int buildModelCandidates(int needMoves, ModelCandidate cands[], int maxCount, int color) {
    if (!cands || maxCount <= 0) return 0;
    if (needMoves <= 1) return collectModelSingleCandidates(cands, maxCount, color);

    ModelCandidate firsts[MODEL_CANDIDATE_MAX];
    int firstCount = collectModelSingleCandidates(firsts, MODEL_CANDIDATE_MAX, color);
    int count = 0;
    int firstLimit = firstCount < 10 ? firstCount : 10;
    for (int i = 0; i < firstLimit; i++) {
        int x1 = firsts[i].moves[0].x;
        int y1 = firsts[i].moves[0].y;
        if (!isLegalMoveForColor(x1, y1, color)) continue;
        board[y1][x1] = color;
        ModelCandidate seconds[MODEL_CANDIDATE_MAX];
        int secondCount = collectModelSingleCandidates(seconds, MODEL_CANDIDATE_MAX, color);
        int secondLimit = secondCount < 8 ? secondCount : 8;
        for (int j = 0; j < secondLimit; j++) {
            int x2 = seconds[j].moves[0].x;
            int y2 = seconds[j].moves[0].y;
            if (!isLegalMoveForColor(x2, y2, color)) continue;
            ModelCandidate cand;
            memset(&cand, 0, sizeof(cand));
            cand.count = 2;
            cand.moves[0] = firsts[i].moves[0];
            cand.moves[1] = seconds[j].moves[0];
            cand.score = firsts[i].score + seconds[j].score;
            if (strcmp(firsts[i].note, "立即取胜") == 0 || strcmp(seconds[j].note, "立即取胜") == 0)
                safeCopy(cand.note, "成阵组合", 48);
            else if (strcmp(firsts[i].note, "必须防守") == 0 || strcmp(seconds[j].note, "必须防守") == 0)
                safeCopy(cand.note, "攻守兼顾", 48);
            else
                safeCopy(cand.note, "双子布局", 48);
            pushModelCandidate(cands, &count, maxCount, &cand);
        }
        board[y1][x1] = 0;
    }
    if (count == 0) return collectModelSingleCandidates(cands, maxCount, color);
    return count;
}

void buildCandidateListText(const ModelCandidate cands[], int count, char* out, int maxLen) {
    if (!out || maxLen <= 0) return;
    out[0] = 0;
    int pos = 0;
    for (int i = 0; i < count && pos < maxLen - 1; i++) {
        char line[160];
        if (cands[i].count >= 2) {
            snprintf(line, sizeof(line), "id=%d moves=[(%d,%d),(%d,%d)] score=%d note=%s\n",
                     cands[i].id, cands[i].moves[0].x, cands[i].moves[0].y,
                     cands[i].moves[1].x, cands[i].moves[1].y,
                     cands[i].score, cands[i].note);
        } else {
            snprintf(line, sizeof(line), "id=%d move=(%d,%d) score=%d note=%s\n",
                     cands[i].id, cands[i].moves[0].x, cands[i].moves[0].y,
                     cands[i].score, cands[i].note);
        }
        int written = snprintf(out + pos, maxLen - pos, "%s", line);
        if (written < 0 || written >= maxLen - pos) break;
        pos += written;
    }
    out[maxLen - 1] = 0;
}

void buildModelPrompt(char* out, int maxLen, int needMoves, const ModelCandidate cands[], int candidateCount) {
    char boardText[2048];
    char candidateText[3072];
    buildBoardText(boardText, sizeof(boardText));
    buildCandidateListText(cands, candidateCount, candidateText, sizeof(candidateText));
    const char* ruleText = "五子棋，连5获胜";
    if (gameMode == MODE_CONNECT6) ruleText = "六子棋，连6获胜，黑方首回合1子，之后每回合2子";
    else if (useForbidden) ruleText = "五子棋，连5获胜，黑棋禁手开启";
    const char* colorText = (aiColor == 1) ? "黑棋B" : "白棋W";
    const char* playerText = (playerColor == 1) ? "黑棋B" : "白棋W";
    snprintf(out, maxLen,
             "棋盘坐标为0到14，x从左到右，y从上到下。空点为.，黑棋为B，白棋为W。\n"
             "规则：%s。\n"
             "你执%s，玩家执%s。本回合你需要落%d子。\n"
             "下面的候选全部由本地引擎验证为合法空点，并按战术分数从强到弱排序。\n"
             "你必须只从候选id中选择一个，不要自造坐标；优先立即取胜，其次必须防守，再其次制造连续威胁。\n"
             "只返回JSON，格式为 {\"id\":1}，不要解释。\n"
             "当前棋盘：\n%s"
             "候选：\n%s",
             ruleText, colorText, playerText, needMoves, boardText, candidateText);
    out[maxLen - 1] = 0;
}

int httpPostGlm(const char* apiKey, const char* payload, char* response, int maxLen, wchar_t* err, int errMax) {
    if (!response || maxLen <= 0) return 0;
    response[0] = 0;
    if (err && errMax > 0) err[0] = 0;
    HINTERNET hSession = WinHttpOpen(L"wuziqi-glm/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        if (err) wcscpy(err, L"WinHTTP 初始化失败");
        return 0;
    }
    WinHttpSetTimeouts(hSession, 10000, 10000, 30000, 90000);
    HINTERNET hConnect = WinHttpConnect(hSession, MODEL_API_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        if (err) wcscpy(err, L"连接智谱接口失败");
        WinHttpCloseHandle(hSession);
        return 0;
    }
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", MODEL_API_PATH, NULL,
                                            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        if (err) wcscpy(err, L"创建模型请求失败");
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 0;
    }

    wchar_t wApiKey[MODEL_API_KEY_MAX];
    if (MultiByteToWideChar(CP_UTF8, 0, apiKey, -1, wApiKey, MODEL_API_KEY_MAX) <= 0) {
        MultiByteToWideChar(CP_ACP, 0, apiKey, -1, wApiKey, MODEL_API_KEY_MAX);
    }
    wchar_t headers[768];
    wsprintfW(headers, L"Content-Type: application/json\r\nAuthorization: Bearer %s\r\nAccept-Language: zh-CN,zh\r\n", wApiKey);
    DWORD payloadLen = (DWORD)strlen(payload);
    BOOL ok = WinHttpSendRequest(hRequest, headers, (DWORD)-1, (LPVOID)payload,
                                 payloadLen, payloadLen, 0);
    if (!ok || !WinHttpReceiveResponse(hRequest, NULL)) {
        if (err) wcscpy(err, L"模型请求发送失败");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 0;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize,
                            WINHTTP_NO_HEADER_INDEX)) {
        if (statusCode < 200 || statusCode >= 300) {
            if (err) wsprintfW(err, L"模型接口返回 HTTP %lu", statusCode);
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return 0;
        }
    }

    DWORD total = 0;
    while (1) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &available) || available == 0) break;
        while (available > 0) {
            char buffer[1024];
            DWORD toRead = available > sizeof(buffer) ? sizeof(buffer) : available;
            DWORD read = 0;
            if (!WinHttpReadData(hRequest, buffer, toRead, &read) || read == 0) break;
            if (total < (DWORD)(maxLen - 1)) {
                DWORD copyLen = read;
                if (copyLen > (DWORD)(maxLen - 1) - total) copyLen = (DWORD)(maxLen - 1) - total;
                memcpy(response + total, buffer, copyLen);
                total += copyLen;
                response[total] = 0;
            }
            if (read >= available) break;
            available -= read;
        }
    }
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return response[0] != 0;
}

int validateModelApiKey() {
    char apiKey[MODEL_API_KEY_MAX];
    if (!loadModelApiKey(apiKey, MODEL_API_KEY_MAX)) return 0;
    char payload[512];
    char response[4096];
    wchar_t err[128] = L"";
    snprintf(payload, sizeof(payload),
             "{\"model\":\"%s\",\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}],\"max_tokens\":2,\"stream\":false}",
             MODEL_NAME_TEXT);
    if (!httpPostGlm(apiKey, payload, response, sizeof(response), err, 128)) return 0;
    if (strstr(response, "\"error\"") != NULL) return 0;
    return strstr(response, "\"content\"") != NULL;
}

struct ValidateKeyJob {
    volatile LONG done;
    int result;
};

DWORD WINAPI validateKeyThreadProc(LPVOID param) {
    ValidateKeyJob* job = (ValidateKeyJob*)param;
    job->result = validateModelApiKey();
    InterlockedExchange(&job->done, 1);
    return 0;
}

int validateModelApiKeyResponsive() {
    ValidateKeyJob job;
    job.done = 0;
    job.result = 0;
    HANDLE thread = CreateThread(NULL, 0, validateKeyThreadProc, &job, 0, NULL);
    if (!thread) return validateModelApiKey();
    while (InterlockedCompareExchange(&job.done, 0, 0) == 0) {
        pumpUiMessages();
        Sleep(30);
    }
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return job.result;
}

int extractAssistantContent(const char* json, char* out, int maxLen) {
    if (!json || !out || maxLen <= 0) return 0;
    out[0] = 0;
    const char* p = strstr(json, "\"content\"");
    if (!p) return 0;
    p = strchr(p, ':');
    if (!p) return 0;
    p++;
    while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
    if (*p != '"') return 0;
    p++;
    int pos = 0;
    while (*p && pos < maxLen - 1) {
        if (*p == '"') break;
        if (*p == '\\') {
            p++;
            if (!*p) break;
            if (*p == '"' || *p == '\\' || *p == '/') out[pos++] = *p;
            else if (*p == 'n') out[pos++] = '\n';
            else if (*p == 'r') out[pos++] = '\r';
            else if (*p == 't') out[pos++] = '\t';
            else if (*p == 'u') {
                p += 4;
            }
        } else {
            out[pos++] = *p;
        }
        p++;
    }
    out[pos] = 0;
    return out[0] != 0;
}

int readIntAfterColon(const char* p, int* value) {
    const char* q = strchr(p, ':');
    if (!q) return 0;
    q++;
    while (*q == ' ' || *q == '\t' || *q == '"' || *q == '[') q++;
    int sign = 1;
    if (*q == '-') { sign = -1; q++; }
    if (!isdigit((unsigned char)*q)) return 0;
    int v = 0;
    while (isdigit((unsigned char)*q)) {
        v = v * 10 + (*q - '0');
        q++;
    }
    *value = v * sign;
    return 1;
}

int normalizeModelCoord(int x, int y, int* outX, int* outY) {
    if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
        *outX = x; *outY = y; return 1;
    }
    if (x >= 1 && x <= BOARD_SIZE && y >= 1 && y <= BOARD_SIZE) {
        *outX = x - 1; *outY = y - 1; return 1;
    }
    return 0;
}

int parseModelMoves(const char* content, Move* moves, int maxMoves) {
    int count = 0;
    const char* p = content;
    while ((p = strstr(p, "\"x\"")) != NULL && count < maxMoves) {
        int x, y, nx, ny;
        if (!readIntAfterColon(p, &x)) { p += 3; continue; }
        const char* q = strstr(p, "\"y\"");
        if (!q || !readIntAfterColon(q, &y)) { p += 3; continue; }
        if (normalizeModelCoord(x, y, &nx, &ny)) {
            moves[count].x = nx;
            moves[count].y = ny;
            count++;
        }
        p = q + 3;
    }
    if (count > 0) return count;

    int nums[16];
    int n = 0;
    for (const char* q = content; *q && n < 16; q++) {
        if ((*q == '-' && isdigit((unsigned char)q[1])) || isdigit((unsigned char)*q)) {
            int sign = 1;
            if (*q == '-') { sign = -1; q++; }
            int v = 0;
            while (isdigit((unsigned char)*q)) {
                v = v * 10 + (*q - '0');
                q++;
            }
            nums[n++] = v * sign;
        }
    }
    for (int i = 0; i + 1 < n && count < maxMoves; i += 2) {
        int nx, ny;
        if (normalizeModelCoord(nums[i], nums[i + 1], &nx, &ny)) {
            moves[count].x = nx;
            moves[count].y = ny;
            count++;
        }
    }
    return count;
}

int parseModelCandidateId(const char* content, const ModelCandidate cands[], int candidateCount) {
    if (!content || !cands || candidateCount <= 0) return -1;
    const char* keys[] = {"\"id\"", "\"choice\"", "\"candidate\"", "\"候选\""};
    for (int k = 0; k < 4; k++) {
        const char* p = strstr(content, keys[k]);
        if (!p) continue;
        int id = -1;
        if (readIntAfterColon(p, &id)) {
            for (int i = 0; i < candidateCount; i++) if (cands[i].id == id) return i;
        }
    }

    int nums[24];
    int n = 0;
    for (const char* q = content; *q && n < 24; q++) {
        if ((*q == '-' && isdigit((unsigned char)q[1])) || isdigit((unsigned char)*q)) {
            int sign = 1;
            if (*q == '-') { sign = -1; q++; }
            int v = 0;
            while (isdigit((unsigned char)*q)) {
                v = v * 10 + (*q - '0');
                q++;
            }
            nums[n++] = v * sign;
        }
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < candidateCount; j++) {
            if (cands[j].id == nums[i]) return j;
        }
    }
    return -1;
}

int isLegalMoveForColor(int x, int y, int color) {
    if (!isInsideBoard(x, y)) return 0;
    if (board[y][x] != 0) return 0;
    if (gameMode == MODE_GOMOKU && useForbidden && color == 1 && isForbiddenMove(x, y, color)) return 0;
    return 1;
}

int requestModelMoves(int needMoves, Move* moves, int* moveCount, wchar_t* err, int errMax) {
    if (moveCount) *moveCount = 0;
    char apiKey[MODEL_API_KEY_MAX];
    if (!loadModelApiKey(apiKey, MODEL_API_KEY_MAX)) {
        if (err) wcscpy(err, L"未找到当前账号的 API Key");
        return 0;
    }

    ModelCandidate candidates[MODEL_CANDIDATE_MAX];
    int candidateCount = buildModelCandidates(needMoves, candidates, MODEL_CANDIDATE_MAX, aiColor);
    if (candidateCount <= 0) {
        if (err) wcscpy(err, L"当前局面没有可用合法候选");
        return 0;
    }
    ModelCandidate promptCandidates[MODEL_CANDIDATE_MAX];
    int promptCount = 0;
    int forcedTactic = candidates[0].score >= 400000000;
    int startIndex = 0;
    int takeCount = candidateCount;
    if (difficulty == 1 && !forcedTactic && candidateCount > 8) {
        startIndex = 5;
        takeCount = candidateCount - startIndex;
        if (takeCount > 8) takeCount = 8;
    } else if (difficulty == 2 && candidateCount > 12) {
        takeCount = 12;
    }
    if (startIndex >= candidateCount) startIndex = 0;
    if (takeCount < 1) takeCount = 1;
    for (int i = 0; i < takeCount && startIndex + i < candidateCount; i++) {
        promptCandidates[promptCount] = candidates[startIndex + i];
        promptCandidates[promptCount].id = promptCount + 1;
        promptCount++;
    }

    char systemPrompt[1024];
    char userPrompt[8192];
    char escSystem[2048];
    char escUser[12000];
    char payload[16000];
    char response[16384];
    char content[4096];
    snprintf(systemPrompt, sizeof(systemPrompt),
             "你是%s五子棋对局引擎。你只能从用户给出的候选id中选择，不能自造坐标。只返回JSON。",
             MODEL_NAME_TEXT);
    buildModelPrompt(userPrompt, sizeof(userPrompt), needMoves, promptCandidates, promptCount);
    jsonEscape(systemPrompt, escSystem, sizeof(escSystem));
    jsonEscape(userPrompt, escUser, sizeof(escUser));
    double temperature = (difficulty == 1) ? 0.35 : (difficulty == 2 ? 0.12 : 0.03);
    snprintf(payload, sizeof(payload),
             "{\"model\":\"%s\",\"messages\":[{\"role\":\"system\",\"content\":\"%s\"},"
             "{\"role\":\"user\",\"content\":\"%s\"}],\"thinking\":{\"type\":\"enabled\"},"
             "\"temperature\":%.2f,\"max_tokens\":128,\"stream\":false}",
             MODEL_NAME_TEXT, escSystem, escUser, temperature);
    payload[sizeof(payload) - 1] = 0;

    int selected = -1;
    if (httpPostGlm(apiKey, payload, response, sizeof(response), err, errMax) &&
        extractAssistantContent(response, content, sizeof(content))) {
        selected = parseModelCandidateId(content, promptCandidates, promptCount);
    }
    if (selected < 0) {
        selected = 0;
        if (err && errMax > 0 && err[0] == 0) wcscpy(err, L"已采用最强候选");
    }

    int count = promptCandidates[selected].count;
    if (count > needMoves) count = needMoves;
    for (int i = 0; i < count; i++) moves[i] = promptCandidates[selected].moves[i];
    if (moveCount) *moveCount = count;
    return 1;
}

int collectModelArrayNumbers(const char* content, const char* key, int nums[], int maxNums) {
    if (!content || !key || !nums || maxNums <= 0) return 0;
    const char* p = strstr(content, key);
    if (!p) return 0;
    p = strchr(p, '[');
    if (!p) return 0;
    int depth = 0;
    int count = 0;
    for (const char* q = p; *q; q++) {
        if (*q == '[') {
            depth++;
            continue;
        }
        if (*q == ']') {
            depth--;
            if (depth <= 0) break;
            continue;
        }
        if (depth > 0 && ((*q == '-' && isdigit((unsigned char)q[1])) || isdigit((unsigned char)*q))) {
            int sign = 1;
            if (*q == '-') { sign = -1; q++; }
            int v = 0;
            while (isdigit((unsigned char)*q)) {
                v = v * 10 + (*q - '0');
                q++;
            }
            if (count < maxNums) nums[count++] = v * sign;
            q--;
        }
    }
    return count;
}

int seedModelChallengeCoords(const int nums[], int numCount, int color, int* stoneCount) {
    if (!nums || !stoneCount || numCount < 2 || (numCount % 2) != 0) return 0;
    for (int i = 0; i + 1 < numCount; i += 2) {
        int x, y;
        if (!normalizeModelCoord(nums[i], nums[i + 1], &x, &y)) return 0;
        if (board[y][x] != 0) return 0;
        seedChallengeStone(x, y, color);
        if (board[y][x] != color) return 0;
        (*stoneCount)++;
    }
    return *stoneCount > 0;
}

int buildModelChallengeFromContent(const char* content, int index) {
    int blackNums[180];
    int whiteNums[180];
    int blackNumCount = collectModelArrayNumbers(content, "\"black\"", blackNums, 180);
    if (blackNumCount <= 0) blackNumCount = collectModelArrayNumbers(content, "black", blackNums, 180);
    int whiteNumCount = collectModelArrayNumbers(content, "\"white\"", whiteNums, 180);
    if (whiteNumCount <= 0) whiteNumCount = collectModelArrayNumbers(content, "white", whiteNums, 180);
    if (blackNumCount <= 0 || whiteNumCount <= 0) return 0;

    initBoard();
    int blackCount = 0;
    int whiteCount = 0;
    if (!seedModelChallengeCoords(blackNums, blackNumCount, 1, &blackCount) ||
        !seedModelChallengeCoords(whiteNums, whiteNumCount, 2, &whiteCount)) {
        initBoard();
        return 0;
    }

    int style = normalizeChallengeIndex(index);
    int minBlack = 8 + style * 2 + (difficulty == 3 ? 2 : 0);
    int minWhite = 6 + style * 2 + (difficulty == 3 ? 2 : 0);
    int total = blackCount + whiteCount;
    if (blackCount < minBlack || whiteCount < minWhite || total > 90 ||
        abs(blackCount - whiteCount) > 10 ||
        boardHasWin(1) || boardHasWin(2) ||
        countImmediateWinsForColor(1) > 0 ||
        countImmediateWinsForColor(2) > 0) {
        initBoard();
        return 0;
    }

    int minBlackScore = (difficulty == 1 ? 1000 : (difficulty == 2 ? 1800 : 4500)) + style * 600;
    int blackBest = bestChallengeScoreForColor(1);
    int whiteBest = bestChallengeScoreForColor(2);
    if (blackBest < minBlackScore || (difficulty == 3 && whiteBest < 1200 + style * 600)) {
        initBoard();
        return 0;
    }

    rebuildChallengeHistory();
    return 1;
}

int tryBuildModelChallengeBoard(int index) {
    char apiKey[MODEL_API_KEY_MAX];
    if (!loadModelApiKey(apiKey, MODEL_API_KEY_MAX)) return 0;

    int style = normalizeChallengeIndex(index);
    const char* styleText[CHALLENGE_COUNT] = {
        "双线做杀，黑方需要找到制造多重威胁的关键手",
        "攻守劫局，白方有反击压力，黑方需要先手化解并进攻",
        "乱战复杂局，双方棋子较多，黑方要在混战中找到强制路线"
    };
    const char* diffText = "中等，关键手不明显但可通过计算找到";
    if (difficulty == 1) diffText = "简单，黑方优势清晰，2到3手内可形成稳定杀势";
    else if (difficulty == 3) diffText = "困难，白方防守和反击都很强，黑方需要连续精确进攻";

    char systemPrompt[1024];
    char userPrompt[8192];
    char escSystem[2048];
    char escUser[12000];
    char payload[16000];
    char response[20000];
    char content[8192];
    wchar_t err[128] = L"";

    snprintf(systemPrompt, sizeof(systemPrompt),
             "你是%s五子棋残局生成器。只输出可解析JSON，不要Markdown，不要解释。", MODEL_NAME_TEXT);
    snprintf(userPrompt, sizeof(userPrompt),
             "生成一个15x15五子棋残局，坐标x,y都使用0到14。黑棋B为玩家，白棋W为防守方，轮到黑棋。"
             "残局类型：%s。难度：%s。限手：%d手。"
             "要求：双方都不能已经连五；双方下一手都不能直接连五；棋子数量适中且集中；"
             "黑棋必须有真实进攻路线，白棋必须有防守压力。"
             "只返回JSON，格式严格为：{\"black\":[[x,y],[x,y]],\"white\":[[x,y],[x,y]]}。",
             styleText[style], diffText, challengeLimits[style]);
    jsonEscape(systemPrompt, escSystem, sizeof(escSystem));
    jsonEscape(userPrompt, escUser, sizeof(escUser));
    double temperature = (difficulty == 1) ? 0.38 : (difficulty == 2 ? 0.28 : 0.18);
    snprintf(payload, sizeof(payload),
             "{\"model\":\"%s\",\"messages\":[{\"role\":\"system\",\"content\":\"%s\"},"
             "{\"role\":\"user\",\"content\":\"%s\"}],\"thinking\":{\"type\":\"enabled\"},"
             "\"temperature\":%.2f,\"max_tokens\":900,\"stream\":false}",
             MODEL_NAME_TEXT, escSystem, escUser, temperature);
    payload[sizeof(payload) - 1] = 0;

    if (!httpPostGlm(apiKey, payload, response, sizeof(response), err, 128)) return 0;
    if (!extractAssistantContent(response, content, sizeof(content))) return 0;
    return buildModelChallengeFromContent(content, style);
}

struct ModelChallengeJob {
    volatile LONG done;
    int index;
    int result;
};

DWORD WINAPI modelChallengeThreadProc(LPVOID param) {
    ModelChallengeJob* job = (ModelChallengeJob*)param;
    job->result = tryBuildModelChallengeBoard(job->index);
    InterlockedExchange(&job->done, 1);
    return 0;
}

int tryBuildModelChallengeBoardResponsive(int index) {
    ModelChallengeJob job;
    job.done = 0;
    job.index = index;
    job.result = 0;
    HANDLE thread = CreateThread(NULL, 0, modelChallengeThreadProc, &job, 0, NULL);
    if (!thread) return tryBuildModelChallengeBoard(index);
    while (InterlockedCompareExchange(&job.done, 0, 0) == 0) {
        pumpUiMessages();
        Sleep(30);
    }
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return job.result;
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

int findFallbackMoveForModel(int* outX, int* outY) {
    AI_move(outX, outY);
    if (isLegalMoveForColor(*outX, *outY, aiColor)) return 1;
    int bestScore = -1;
    if (findBestMoveForColor(aiColor, outX, outY, &bestScore) &&
        isLegalMoveForColor(*outX, *outY, aiColor)) {
        return 1;
    }
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (isLegalMoveForColor(x, y, aiColor)) {
                *outX = x;
                *outY = y;
                return 1;
            }
        }
    }
    return 0;
}

int playModelTurn(int* lastCol, int* lastRow) {
    int need = getTurnMoveNeed(aiColor);
    Move modelMoves[2];
    int modelMoveCount = 0;
    int placed = 0;
    int usedFallback = 0;
    wchar_t err[128] = L"";
    turnPlacedThisRound = 0;
    setModelDuelStatus(L"GLM-5.1 正在落子...");

    int ok = requestModelMoves(need, modelMoves, &modelMoveCount, err, 128);
    if (ok) {
        for (int i = 0; i < modelMoveCount && placed < need; i++) {
            int x = modelMoves[i].x;
            int y = modelMoves[i].y;
            if (!isLegalMoveForColor(x, y, aiColor)) continue;
            if (!placeStone(x, y, aiColor)) continue;
            *lastCol = x;
            *lastRow = y;
            placed++;
            turnPlacedThisRound++;
            playSoundEffect(0);
            if (checkWin(x, y, aiColor)) {
                setModelDuelStatus(L"GLM-5.1 已成阵");
                return 1;
            }
            if (isBoardFull()) {
                setModelDuelStatus(L"棋盘已满");
                return 2;
            }
        }
        if (placed < need) wcscpy(err, L"候选落点异常，已补强");
    }

    while (placed < need) {
        int x = -1, y = -1;
        if (!findFallbackMoveForModel(&x, &y)) return -1;
        if (!placeStone(x, y, aiColor)) return -1;
        usedFallback = 1;
        *lastCol = x;
        *lastRow = y;
        placed++;
        turnPlacedThisRound++;
        playSoundEffect(0);
        if (checkWin(x, y, aiColor)) {
            setModelDuelStatus(usedFallback ? L"模型异常，本地兜底成阵" : L"GLM-5.1 已成阵");
            return 1;
        }
        if (isBoardFull()) {
            setModelDuelStatus(L"棋盘已满");
            return 2;
        }
    }

    turnPlacedThisRound = 0;
    if (usedFallback) {
        setModelDuelStatus(err[0] ? err : L"GLM异常，采用强候选");
    } else if (err[0]) {
        setModelDuelStatus(err);
    } else {
        setModelDuelStatus(L"GLM-5.1 已选强候选");
    }
    return 0;
}

int playAITurn(int* lastCol, int* lastRow) {
    if (!challengeMode && modelDuelMode) return playModelTurn(lastCol, lastRow);
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

struct AiTurnJob {
    volatile LONG done;
    int result;
    int lastCol;
    int lastRow;
};

DWORD WINAPI aiTurnThreadProc(LPVOID param) {
    AiTurnJob* job = (AiTurnJob*)param;
    job->lastCol = -1;
    job->lastRow = -1;
    job->result = playAITurn(&job->lastCol, &job->lastRow);
    InterlockedExchange(&job->done, 1);
    return 0;
}

int playAITurnResponsive(int* lastCol, int* lastRow) {
    if (challengeMode || !modelDuelMode) return playAITurn(lastCol, lastRow);
    AiTurnJob job;
    job.done = 0;
    job.result = -1;
    job.lastCol = -1;
    job.lastRow = -1;
    HANDLE thread = CreateThread(NULL, 0, aiTurnThreadProc, &job, 0, NULL);
    if (!thread) return playAITurn(lastCol, lastRow);
    while (InterlockedCompareExchange(&job.done, 0, 0) == 0) {
        pumpUiMessages();
        Sleep(30);
    }
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    if (job.lastCol >= 0 && job.lastRow >= 0) {
        *lastCol = job.lastCol;
        *lastRow = job.lastRow;
    }
    return job.result;
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
    int limitText = maxLen > 1 ? maxLen - 1 : 0;
    if (limitText > 511) limitText = 511;
    SendMessageW(edit, EM_LIMITTEXT, (WPARAM)limitText, 0);
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
        wchar_t wideText[512] = {0};
        char utf8Text[1024] = {0};
        GetWindowTextW(edit, wideText, 512);
        int n = WideCharToMultiByte(CP_UTF8, 0, wideText, -1, utf8Text, 1024, NULL, NULL);
        if (n > 0 && n <= maxLen) {
            safeCopy(out, utf8Text, maxLen);
        } else {
            out[0] = 0;
            if (n > maxLen) MessageBoxW(GetHWnd(), L"输入内容过长，请缩短后重试。", L"提示", MB_OK);
        }
    } else {
        out[0] = 0;
    }
    DestroyWindow(edit);
    DeleteObject(editFont);
    SetFocus(parent);
}

int promptAndSaveModelApiKey() {
    if (isGuestUser()) {
        MessageBoxW(GetHWnd(), L"游客登录不提供模型对局功能，请先登录或注册账号。", L"模型对局", MB_OK);
        return 0;
    }
    char apiKey[MODEL_API_KEY_MAX] = {0};
    inputScreen(L"请输入 GLM-5.1 API Key", apiKey, MODEL_API_KEY_MAX, 1);
    trimLineEnd(apiKey);
    if (strlen(apiKey) == 0) return 0;
    if (!saveModelApiKey(apiKey)) {
        MessageBoxW(GetHWnd(), L"API Key 保存失败，请确认 exe 所在目录可写。", L"模型对局", MB_OK);
        return 0;
    }
    MessageBoxW(GetHWnd(), L"API Key 已保存，正在验证...", L"模型对局", MB_OK);
    if (validateModelApiKeyResponsive()) {
        MessageBoxW(GetHWnd(), L"API Key 验证通过，模型对局已就绪。", L"模型对局", MB_OK);
    } else {
        MessageBoxW(GetHWnd(), L"API Key 保存成功，但验证未通过。请检查网络或 Key 是否有效。", L"模型对局", MB_OK);
    }
    return 1;
}

int ensureModelApiKeyReady() {
    if (isGuestUser()) {
        modelDuelMode = 0;
        MessageBoxW(GetHWnd(), L"游客登录不提供模型对局功能，请先登录或注册账号。", L"模型对局", MB_OK);
        return 0;
    }
    if (!modelDuelMode || hasModelApiKey()) return 1;
    int ans = MessageBoxW(GetHWnd(), L"模型对局需要先输入 GLM-5.1 API Key，是否现在输入？",
                          L"模型对局", MB_YESNO);
    if (ans != IDYES) return 0;
    return promptAndSaveModelApiKey();
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
                                    enterUserSession(name, u.level, u.exp);
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
                            if (!saveUser(&u)) {
                                MessageBoxW(GetHWnd(), L"注册保存失败，请确认 data 目录可写且账号未重复。", L"错误", MB_OK);
                                needRedraw = 1; continue;
                            }
                            enterUserSession(name, 1, 0);
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
                            if (!loadAdminKey(savedKey, 64)) {
                                MessageBoxW(GetHWnd(), L"管理员密匙尚未配置。请先在 data\\admin_key.txt 中写入密匙。", L"忘记密码", MB_OK);
                                needRedraw = 1; continue;
                            }
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
                            enterUserSession("Guest", 1, 0);
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
            drawCenterTextShadow(126, modelDuelMode ? L"GLM-5.1" : L"天机对局", 34, TEXT_WHITE);
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
    const int MAX_BTN_COUNT = 12;
    int guestMode = isGuestUser();
    if (guestMode && modelDuelMode) clearModelDuelSession();
    const int ACT_DIFFICULTY = 1;
    const int ACT_RULE = 2;
    const int ACT_OPPONENT = 3;
    const int ACT_MODEL = 4;
    const int ACT_API = 5;
    const int ACT_FORBIDDEN = 6;
    const int ACT_COLOR = 7;
    const int ACT_HINT = 8;
    const int ACT_SOUND = 9;
    const int ACT_TIME = 10;
    const int ACT_LOGOUT = 11;
    const int ACT_BACK = 12;
    int actions[MAX_BTN_COUNT];
    int btnCount = 0;
    actions[btnCount++] = ACT_DIFFICULTY;
    actions[btnCount++] = ACT_RULE;
    if (!guestMode) {
        actions[btnCount++] = ACT_OPPONENT;
        actions[btnCount++] = ACT_MODEL;
        actions[btnCount++] = ACT_API;
    }
    actions[btnCount++] = ACT_FORBIDDEN;
    actions[btnCount++] = ACT_COLOR;
    actions[btnCount++] = ACT_HINT;
    actions[btnCount++] = ACT_SOUND;
    actions[btnCount++] = ACT_TIME;
    if (!guestMode) actions[btnCount++] = ACT_LOGOUT;
    actions[btnCount++] = ACT_BACK;
    Button btns[MAX_BTN_COUNT];
    wchar_t btnTexts[MAX_BTN_COUNT][64];
    for (int i = 0; i < btnCount; i++) {
        btns[i].x = (WIN_WIDTH - 360) / 2;
        btns[i].y = 108 + i * 50;
        btns[i].w = 360;
        btns[i].h = 42;
        btns[i].text = btnTexts[i];
    }
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
            for (int i = 0; i < btnCount; i++) {
                switch (actions[i]) {
                    case ACT_DIFFICULTY:
                        wsprintfW(btnTexts[i], L"难度: %s", diffNames[difficulty]);
                        break;
                    case ACT_RULE:
                        wsprintfW(btnTexts[i], L"规则: %s", getModeName());
                        break;
                    case ACT_OPPONENT:
                        wcscpy(btnTexts[i], modelDuelMode ? L"对手: 模型对局" : L"对手: 本地天机");
                        break;
                    case ACT_MODEL:
                        wcscpy(btnTexts[i], L"大模型: GLM-5.1");
                        break;
                    case ACT_API:
                        wcscpy(btnTexts[i], hasModelApiKey() ? L"API Key: 已保存/点击更换" : L"API Key: 未设置/点击输入");
                        break;
                    case ACT_FORBIDDEN:
                        wcscpy(btnTexts[i], gameMode == MODE_CONNECT6 ? L"禁手: 六子棋无禁手" : (useForbidden ? L"禁手: 开启" : L"禁手: 关闭"));
                        break;
                    case ACT_COLOR:
                        wcscpy(btnTexts[i], (playerColor == 1) ? L"执子: 玄黑(先)" : L"执子: 玉白(后)");
                        break;
                    case ACT_HINT:
                        wcscpy(btnTexts[i], showHints ? L"心眼提示: 开启" : L"心眼提示: 关闭");
                        break;
                    case ACT_SOUND:
                        wcscpy(btnTexts[i], soundOn ? L"音效: 开启" : L"音效: 关闭");
                        break;
                    case ACT_TIME:
                        wsprintfW(btnTexts[i], L"步时: %s", timeNames[timeIdx]);
                        break;
                    case ACT_LOGOUT:
                        wcscpy(btnTexts[i], L"注销账号");
                        break;
                    default:
                        wcscpy(btnTexts[i], L"返回");
                        break;
                }
            }
            drawBackground(&imgMenu);
            drawCenterTextShadow(42, L"游戏设置", 34, ACCENT_GOLD);
            const wchar_t* settingsSubtitle;
            if (guestMode) settingsSubtitle = L"游客模式不提供模型对局；登录账号后可配置 GLM-5.1。";
            else if (modelDuelMode) settingsSubtitle = L"模型对局使用当前账号本机保存的 GLM-5.1 API Key。";
            else if (gameMode == MODE_GOMOKU) settingsSubtitle = useForbidden ? L"五子棋: 黑棋先行，连五成阵，禁手已开启。" : L"五子棋: 黑棋先行，连五成阵，无禁手规则。";
            else settingsSubtitle = L"六子棋: 黑先落1子，之后双方每回合落2子。";
            drawCenterText(84, settingsSubtitle, 17, RGB(220, 205, 160));
            for (int i = 0; i < btnCount; i++) drawSmallButton(&btns[i], i == hover, 17);
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
                        int action = actions[i];
                        if (action == ACT_DIFFICULTY) { difficulty++; if (difficulty > 3) difficulty = 1; needRedraw = 1; }
                        else if (action == ACT_RULE) {
                            gameMode = (gameMode == MODE_GOMOKU) ? MODE_CONNECT6 : MODE_GOMOKU;
                            if (gameMode == MODE_CONNECT6) useForbidden = 0;
                            needRedraw = 1;
                        }
                        else if (action == ACT_OPPONENT) {
                            if (!modelDuelMode) {
                                if (!hasModelApiKey() && !promptAndSaveModelApiKey()) {
                                    needRedraw = 1; continue;
                                }
                                if (hasModelApiKey()) {
                                    if (!validateModelApiKeyResponsive()) {
                                        MessageBoxW(GetHWnd(), L"当前保存的 API Key 验证未通过，请检查网络或重新输入。", L"模型对局", MB_OK);
                                        needRedraw = 1; continue;
                                    }
                                }
                                modelDuelMode = 1;
                                resetModelDuelScore();
                            } else {
                                modelDuelMode = 0;
                            }
                            needRedraw = 1;
                        }
                        else if (action == ACT_MODEL) {
                            MessageBoxW(GetHWnd(), L"当前模型固定为 GLM-5.1。后续可在这里扩展更多模型。", L"模型对局", MB_OK);
                            needRedraw = 1;
                        }
                        else if (action == ACT_API) {
                            promptAndSaveModelApiKey();
                            needRedraw = 1;
                        }
                        else if (action == ACT_FORBIDDEN) {
                            if (gameMode == MODE_GOMOKU) useForbidden = !useForbidden;
                            needRedraw = 1;
                        }
                        else if (action == ACT_COLOR) { playerColor = 3 - playerColor; needRedraw = 1; }
                        else if (action == ACT_HINT) { showHints = !showHints; needRedraw = 1; }
                        else if (action == ACT_SOUND) { soundOn = !soundOn; needRedraw = 1; }
                        else if (action == ACT_TIME) { timeIdx = (timeIdx + 1) % 4; timeLimit = timeVals[timeIdx]; needRedraw = 1; }
                        else if (action == ACT_LOGOUT) {
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
                            int ans = MessageBoxW(GetHWnd(), L"确认永久注销当前账号？账号、排行榜记录、该账号存档和 GLM 密钥将被删除。", L"注销账号", MB_YESNO);
                            if (ans == IDYES) {
                                char deletedName[64];
                                safeCopy(deletedName, currentUser, 64);
                                if (deleteUserAccount(deletedName)) {
                                    MessageBoxW(GetHWnd(), L"账号已注销。", L"成功", MB_OK);
                                    enterUserSession("Guest", 1, 0);
                                    loginScreen();
                                    return;
                                }
                                MessageBoxW(GetHWnd(), L"注销失败，请稍后重试。", L"错误", MB_OK);
                            }
                            needRedraw = 1;
                        }
                        else if (action == ACT_BACK) return;
                    }
                }
            }
        }
        Sleep(10);
    }
}

/* ===================== 个人数据 ===================== */
void showPersonalStats() {
    UserInfo accountInfo;
    int hasAccount = (strcmp(currentUser, "Guest") != 0 && findUser(currentUser, &accountInfo));
    wchar_t regText[64] = L"游客模式";
    if (hasAccount) {
        time_t rt = (time_t)accountInfo.regTime;
        struct tm* tmv = localtime(&rt);
        if (tmv) wsprintfW(regText, L"%04d-%02d-%02d", tmv->tm_year + 1900, tmv->tm_mon + 1, tmv->tm_mday);
        else wcscpy(regText, L"未知");
    }
    Button btnTianji, btnModel, btnReset, btnBack;
    int btnY = WIN_HEIGHT - 92;
    btnTianji.x = WIN_WIDTH / 2 - 330; btnTianji.y = btnY; btnTianji.w = 200; btnTianji.h = 46; btnTianji.text = L"天机数据";
    btnModel.x = WIN_WIDTH / 2 - 100; btnModel.y = btnY; btnModel.w = 200; btnModel.h = 46; btnModel.text = L"GLM-5.1";
    btnReset.x = WIN_WIDTH / 2 + 130; btnReset.y = btnY; btnReset.w = 200; btnReset.h = 46; btnReset.text = L"重置本页";
    btnBack.x = WIN_WIDTH / 2 + 360; btnBack.y = btnY; btnBack.w = 200; btnBack.h = 46; btnBack.text = L"返回";
    Button* btns[4] = {&btnTianji, &btnModel, &btnReset, &btnBack};
    int page = 0;
    int hover = -1;
    int needRedraw = 1;
    ExMessage msg;
    while (1) {
        if (needRedraw) {
            StatSummary summary;
            loadUserStatSummary(page, &summary);
            drawBackground(&imgMenu);
            drawCenterTextShadow(48, page ? L"个人数据 · GLM-5.1" : L"个人数据 · 天机", 36, ACCENT_GOLD);
            for (int i = 0; i < 4; i++) drawSmallButton(btns[i], hover == i || (page == i && i < 2), 18);

            int left = 150, top = 185, width = WIN_WIDTH - 300, height = 410;
            setfillcolor(RGB(14, 14, 22));
            setlinecolor(PANEL_BORDER);
            fillroundrect(left, top, left + width, top + height, 14, 14);
            roundrect(left + 8, top + 8, left + width - 8, top + height - 8, 12, 12);
            const wchar_t* diffName[4] = {L"", L"简单", L"中等", L"困难"};
            int y = top + 36;
            wchar_t buf[256];
            wchar_t wuser[64]; asciiToWchar(currentUser, wuser, 64);
            wsprintfW(buf, L"道友: %s    注册: %s", wuser, regText);
            drawBoxCenterText(left, width, y, buf, 22, ACCENT_GOLD); y += 42;
            wsprintfW(buf, L"境界: %s    经验: %d", getXianLevelName(currentUserExp), currentUserExp);
            drawBoxCenterText(left, width, y, buf, 22, TEXT_WHITE); y += 46;

            int col1 = left + 90;
            int col2 = left + width / 2 + 40;
            wsprintfW(buf, L"总对局: %d", summary.totalGames);
            drawLeftText(col1, y, buf, 22, TEXT_WHITE);
            wsprintfW(buf, L"当前连胜: %d    最高连胜: %d", summary.currentStreak, summary.bestStreak);
            drawLeftText(col2, y, buf, 22, ACCENT_GOLD);
            y += 52;

            drawLeftText(col1, y, L"难度战绩", 22, ACCENT_GOLD);
            drawLeftText(col2, y, page ? L"模型规则统计" : L"规则统计", 22, ACCENT_GOLD);
            y += 40;
            for (int d = 1; d <= 3; d++) {
                wsprintfW(buf, L"%s:  胜 %d   负 %d   平 %d", diffName[d], summary.wins[d], summary.losses[d], summary.draws[d]);
                drawLeftText(col1, y, buf, 20, TEXT_WHITE);
                y += 36;
            }
            int ry = top + 230;
            wsprintfW(buf, L"五子棋: %d局 / %d胜", summary.modeGames[0], summary.modeWins[0]);
            drawLeftText(col2, ry, buf, 20, TEXT_WHITE); ry += 38;
            wsprintfW(buf, L"六子棋: %d局 / %d胜", summary.modeGames[1], summary.modeWins[1]);
            drawLeftText(col2, ry, buf, 20, TEXT_WHITE); ry += 46;
            if (summary.totalGames >= 20) wcscpy(buf, page ? L"称号: 模型磨砺者" : L"称号: 棋痴入道");
            else if (summary.bestStreak >= 3) wcscpy(buf, page ? L"称号: 三胜破算" : L"称号: 三胜凝气");
            else if (summary.modeWins[1] > 0) wcscpy(buf, L"称号: 六连初悟");
            else wcscpy(buf, L"称号: 尚待破局");
            drawLeftText(col2, ry, buf, 20, RGB(120, 235, 185));
            FlushBatchDraw();
            needRedraw = 0;
        }
        if (peekmessage(&msg, EX_MOUSE)) {
            int mx = toLogicalX(msg.x);
            int my = toLogicalY(msg.y);
            if (msg.message == WM_MOUSEMOVE) {
                int oldHover = hover;
                hover = -1;
                for (int i = 0; i < 4; i++) if (hitButton(btns[i], mx, my)) { hover = i; break; }
                if (hover != oldHover) needRedraw = 1;
            } else if (msg.message == WM_LBUTTONDOWN) {
                if (hitButton(&btnTianji, mx, my)) { page = 0; needRedraw = 1; }
                else if (hitButton(&btnModel, mx, my)) { page = 1; needRedraw = 1; }
                else if (hitButton(&btnReset, mx, my)) {
                    resetUserStatsByModelFlag(page);
                    MessageBoxW(GetHWnd(), page ? L"GLM-5.1 数据已重置" : L"天机数据已重置", L"提示", MB_OK);
                    needRedraw = 1;
                } else if (hitButton(&btnBack, mx, my)) return;
            }
        }
        Sleep(10);
    }
}

/* ===================== 排行榜 ===================== */
struct PlayerStat { char name[64]; int wins, losses, draws; };

int buildLeaderboardStats(PlayerStat stats[], int maxCount, int modelPage) {
    int count = 0;
    UserInfo users[100];
    int userCount = loadUsers(users, 100);
    for (int i = 0; i < userCount && count < maxCount; i++) {
        safeCopy(stats[count].name, users[i].name, 64);
        stats[count].wins = 0; stats[count].losses = 0; stats[count].draws = 0;
        count++;
    }
    int found = 0;
    for (int i = 0; i < count; i++) if (strcmp(stats[i].name, currentUser) == 0) { found = 1; break; }
    if (!found && count < maxCount && strcmp(currentUser, "Guest") != 0) {
        safeCopy(stats[count].name, currentUser, 64);
        stats[count].wins = 0; stats[count].losses = 0; stats[count].draws = 0;
        count++;
    }
    FILE* fp = fopenRuntimeData(STATS_FILE, "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            StatRecord rec;
            if (!parseStatLine(line, &rec)) continue;
            if (rec.difficulty != 3) continue;
            if (rec.modelFlag != modelPage) continue;
            if (!modelPage && rec.mode != gameMode) continue;
            for (int i = 0; i < count; i++) {
                if (strcmp(stats[i].name, rec.user) == 0) {
                    if (rec.result == 1) stats[i].wins++;
                    else if (rec.result == 0) stats[i].losses++;
                    else stats[i].draws++;
                    break;
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
    return count;
}

void showLeaderboard() {
    PlayerStat stats[100];
    int page = 0;
    Button btnTianji, btnModel, backBtn;
    btnTianji.x = WIN_WIDTH / 2 - 330; btnTianji.y = WIN_HEIGHT - 92; btnTianji.w = 200; btnTianji.h = 46; btnTianji.text = L"天机困难榜";
    btnModel.x = WIN_WIDTH / 2 - 100; btnModel.y = WIN_HEIGHT - 92; btnModel.w = 200; btnModel.h = 46; btnModel.text = L"GLM-5.1榜";
    backBtn.x = WIN_WIDTH / 2 + 130; backBtn.y = WIN_HEIGHT - 92; backBtn.w = 200; backBtn.h = 46; backBtn.text = L"返回";
    Button* btns[3] = {&btnTianji, &btnModel, &backBtn};
    int hover = -1;
    int needRedraw = 1;
    ExMessage msg;
    while (1) {
        if (needRedraw) {
            int count = buildLeaderboardStats(stats, 100, page);
            drawBackground(&imgMenu);
            wchar_t title[80];
            if (page) wcscpy(title, L"GLM-5.1 困难排行榜");
            else wsprintfW(title, L"天机困难排行榜（%s）", getModeName());
            drawCenterTextShadow(34, title, 38, ACCENT_GOLD);

            int left = 120, top = 94, width = WIN_WIDTH - 240, height = WIN_HEIGHT - 215;
            setfillcolor(RGB(14, 14, 22));
            setlinecolor(PANEL_BORDER);
            fillroundrect(left, top, left + width, top + height, 14, 14);
            roundrect(left + 8, top + 8, left + width - 8, top + height - 8, 12, 12);
            drawLeftText(left + 42, top + 32, L"排名", 20, ACCENT_GOLD);
            drawLeftText(left + 150, top + 32, L"道友", 20, ACCENT_GOLD);
            drawLeftText(left + 390, top + 32, L"胜", 20, ACCENT_GOLD);
            drawLeftText(left + 500, top + 32, L"负", 20, ACCENT_GOLD);
            drawLeftText(left + 610, top + 32, L"平", 20, ACCENT_GOLD);
            drawLeftText(left + 730, top + 32, L"胜率", 20, ACCENT_GOLD);
            drawLeftText(left + 875, top + 32, L"境界", 20, ACCENT_GOLD);
            int y = top + 74;
            int showCount = count < 15 ? count : 15;
            for (int i = 0; i < showCount; i++) {
                wchar_t wname[64]; asciiToWchar(stats[i].name, wname, 64);
                wchar_t buf[32];
                int total = stats[i].wins + stats[i].losses + stats[i].draws;
                double rate = total > 0 ? (double)stats[i].wins / total * 100.0 : 0.0;
                UserInfo u; int exp = 0;
                if (findUser(stats[i].name, &u)) exp = u.exp;
                wsprintfW(buf, L"%d", i + 1); drawLeftText(left + 42, y, buf, 18, TEXT_WHITE);
                drawLeftText(left + 150, y, wname, 18, TEXT_WHITE);
                wsprintfW(buf, L"%d", stats[i].wins); drawLeftText(left + 390, y, buf, 18, TEXT_WHITE);
                wsprintfW(buf, L"%d", stats[i].losses); drawLeftText(left + 500, y, buf, 18, TEXT_WHITE);
                wsprintfW(buf, L"%d", stats[i].draws); drawLeftText(left + 610, y, buf, 18, TEXT_WHITE);
                wsprintfW(buf, L"%.1f%%", rate); drawLeftText(left + 730, y, buf, 18, TEXT_WHITE);
                const wchar_t* realm = getXianLevelName(exp);
                drawLeftText(left + 875, y, realm, 18, TEXT_WHITE);
                y += 34;
            }
            for (int i = 0; i < 3; i++) drawSmallButton(btns[i], hover == i || (page == i && i < 2), 18);
            FlushBatchDraw();
            needRedraw = 0;
        }
        if (peekmessage(&msg, EX_MOUSE)) {
            int mx = toLogicalX(msg.x);
            int my = toLogicalY(msg.y);
            if (msg.message == WM_MOUSEMOVE) {
                int oldHover = hover;
                hover = -1;
                for (int i = 0; i < 3; i++) if (hitButton(btns[i], mx, my)) { hover = i; break; }
                if (hover != oldHover) needRedraw = 1;
            } else if (msg.message == WM_LBUTTONDOWN) {
                if (hitButton(&btnTianji, mx, my)) { page = 0; needRedraw = 1; }
                else if (hitButton(&btnModel, mx, my)) { page = 1; needRedraw = 1; }
                else if (hitButton(&backBtn, mx, my)) return;
            }
        }
        Sleep(10);
    }
}


/* ===================== 游戏主循环 ===================== */
void startGame(int isContinue) {
    if (!challengeMode && modelDuelMode && !ensureModelApiKeyReady()) {
        gameRunning = 0;
        return;
    }
    if (!challengeMode && modelDuelMode && !isContinue) resetModelDuelScore();
    aiColor = 3 - playerColor;
    int hoverCol = -1, hoverRow = -1;
    int lastCol = -1, lastRow = -1;
    int gameOver = 0;
    int result = -1;
    int currentTurn = 1;
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
    setAiDialogueBySituation(0);

    Button btnUndo, btnGiveUp, btnDraw, btnSave, btnBack;
    int bw = 124, bh = 42, gap = 19;
    int startX = getCellX(0) - 12;
    int by = WIN_HEIGHT - 82;
    btnUndo.x = startX; btnUndo.y = by; btnUndo.w = bw; btnUndo.h = bh; btnUndo.text = L"悔棋";
    btnGiveUp.x = startX + bw + gap; btnGiveUp.y = by; btnGiveUp.w = bw; btnGiveUp.h = bh; btnGiveUp.text = L"认输";
    btnDraw.x = startX + 2*(bw+gap); btnDraw.y = by; btnDraw.w = bw; btnDraw.h = bh; btnDraw.text = L"心眼提示";
    btnSave.x = startX + 3*(bw+gap); btnSave.y = by; btnSave.w = bw; btnSave.h = bh; btnSave.text = (challengeMode || modelDuelMode) ? L"换局" : L"存档";
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
        int aiResult = playAITurnResponsive(&lastCol, &lastRow);
        if (aiResult == 1) { gameOver = 1; result = 0; saveStats(0); setAiDialogueBySituation(4); }
        else if (aiResult == 2) { gameOver = 1; result = 2; saveStats(2); setAiDialogueBySituation(5); }
        else { setAiDialogueBySituation(2); }
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

        if (!gameOver && gameRunning && timeLimit > 0 && currentTurn == playerColor) {
            DWORD elapsed = (GetTickCount() - stepStartTime) / 1000;
            if ((int)elapsed >= timeLimit) {
                gameOver = 1; result = 0; saveStats(0); playSoundEffect(2);
                setAiDialogueBySituation(4);
                doRedraw = 1; continue;
            }
        }

        ExMessage msg;
        while (peekmessage(&msg, EX_MOUSE)) {
            int mx = toLogicalX(msg.x);
            int my = toLogicalY(msg.y);
            if (msg.message == WM_MOUSEMOVE) {
                int oldCol = hoverCol, oldRow = hoverRow;
                if (!getBoardClickCell(mx, my, &hoverCol, &hoverRow)) {
                    hoverCol = -1;
                    hoverRow = -1;
                }
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
                            currentTurn = playerColor;
                            doRedraw = 1; continue;
                        }
                    } else if (btnClicked == 1) {
                        if (!gameOver) {
                            gameOver = 1; result = 0; saveStats(0); playSoundEffect(2);
                            setAiDialogueBySituation(4);
                            doRedraw = 1; continue;
                        }
                    } else if (btnClicked == 2) {
                        showHints = !showHints;
                        doRedraw = 1; continue;
                    } else if (btnClicked == 3) {
                        if (challengeMode) {
                            loadChallengeBoard(challengeIndex);
                            gameOver = 0; result = -1; lastCol = -1; lastRow = -1;
                            currentTurn = playerColor;
                            doRedraw = 1; continue;
                        }
                        if (modelDuelMode) {
                            int ans = gameOver ? IDYES : MessageBoxW(GetHWnd(), L"换一局？当前局不会计入比分。", L"模型对局", MB_YESNO);
                            if (ans == IDYES) {
                                initBoard();
                                setModelDuelStatus(L"GLM-5.1 已就绪");
                                gameOver = 0; result = -1; lastCol = -1; lastRow = -1;
                                currentTurn = playerColor;
                                if (playerColor == 2) {
                                    aiThinking = 1;
                                    redrawScene(-1, -1, lastCol, lastRow, gameOver, result);
                                    drawInfoPanel(gameOver, result, currentTurn);
                                    for (int i = 0; i < 5; i++) drawSmallButton(bottomBtns[i], -1, 16);
                                    drawStatusBar(L""); FlushBatchDraw();
                                    Sleep(300);
                                    int aiResult = playAITurnResponsive(&lastCol, &lastRow);
                                    if (aiResult == 1) { gameOver = 1; result = 0; saveStats(0); }
                                    else if (aiResult == 2) { gameOver = 1; result = 2; saveStats(2); }
                                    currentTurn = playerColor;
                                    stepStartTime = GetTickCount();
                                    aiThinking = 0;
                                }
                            }
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
                    if (!challengeMode && modelDuelMode) setModelDuelStatus(L"GLM-5.1 已就绪");
                    gameOver = 0; result = -1; lastCol = -1; lastRow = -1;
                    currentTurn = playerColor;
                    setAiDialogueBySituation(0);
                    if (!challengeMode && playerColor == 2) {
                        aiThinking = 1;
                        redrawScene(-1, -1, lastCol, lastRow, gameOver, result);
                        drawInfoPanel(gameOver, result, currentTurn);
                        for (int i = 0; i < 5; i++) drawSmallButton(bottomBtns[i], -1, 16);
                        drawStatusBar(L""); FlushBatchDraw();
                        Sleep(300);
                        int aiResult = playAITurnResponsive(&lastCol, &lastRow);
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
                if (!getBoardClickCell(mx, my, &col, &row)) continue;
                if (board[row][col] != 0) continue;
                if (useForbidden && playerColor == 1 && isForbiddenMove(col, row, 1)) {
                    MessageBoxW(GetHWnd(), L"禁手！此乃天道所不容之棋。", L"违规", MB_OK);
                    continue;
                }
                if (!placeStone(col, row, playerColor)) continue;
                lastCol = col; lastRow = row;
                turnPlacedThisRound++;
                if (challengeMode) challengePlayerMoves = countChallengePlayerMoves();
                setAiDialogueBySituation(1);
                playSoundEffect(0);
                redrawScene(hoverCol, hoverRow, lastCol, lastRow, gameOver, result);
                drawInfoPanel(gameOver, result, currentTurn);
                for (int i = 0; i < 5; i++) drawSmallButton(bottomBtns[i], i == btnHover, 16);
                drawStatusBar(L""); FlushBatchDraw();
                if (checkWin(col, row, playerColor)) {
                    gameOver = 1; result = 1; saveStats(1); playSoundEffect(1);
                    setAiDialogueBySituation(3);
                    doRedraw = 1; continue;
                }
                if (isBoardFull()) {
                    gameOver = 1; result = 2; saveStats(2); playSoundEffect(1);
                    setAiDialogueBySituation(5);
                    doRedraw = 1; continue;
                }
                if (challengeMode) {
                    challengePlayerMoves = countChallengePlayerMoves();
                    if (challengePlayerMoves >= challengeMoveLimit) {
                        gameOver = 1; result = 0; playSoundEffect(2);
                        setAiDialogueBySituation(4);
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
                int aiResult = playAITurnResponsive(&lastCol, &lastRow);
                if (!gameOver) setAiDialogueBySituation(2);
                currentTurn = playerColor; stepStartTime = GetTickCount();
                redrawScene(hoverCol, hoverRow, lastCol, lastRow, gameOver, result);
                drawInfoPanel(gameOver, result, currentTurn);
                for (int i = 0; i < 5; i++) drawSmallButton(bottomBtns[i], i == btnHover, 16);
                drawStatusBar(L""); FlushBatchDraw();
                if (aiResult == 1) {
                    gameOver = 1; result = 0; saveStats(0); playSoundEffect(2);
                    setAiDialogueBySituation(4);
                    aiThinking = 0; doRedraw = 1; continue;
                }
                if (aiResult == 2 || isBoardFull()) {
                    gameOver = 1; result = 2; saveStats(2); playSoundEffect(1);
                    setAiDialogueBySituation(5);
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
    migrateFixedRuntimeFilesToData();
    initgraph(WIN_WIDTH, WIN_HEIGHT);
    lockWindowSize();
    setbkcolor(RGB(20, 20, 20));
    cleardevice();
    char bgLoginPath[RUNTIME_PATH_MAX];
    char bgMenuPath[RUNTIME_PATH_MAX];
    char bgGamePath[RUNTIME_PATH_MAX];
    char bgWinPath[RUNTIME_PATH_MAX];
    buildExeRelativePath(BG_LOGIN_FILE, bgLoginPath, RUNTIME_PATH_MAX);
    buildExeRelativePath(BG_MENU_FILE, bgMenuPath, RUNTIME_PATH_MAX);
    buildExeRelativePath(BG_GAME_FILE, bgGamePath, RUNTIME_PATH_MAX);
    buildExeRelativePath(BG_WIN_FILE, bgWinPath, RUNTIME_PATH_MAX);
    loadimage(&imgLogin, bgLoginPath, WIN_WIDTH, WIN_HEIGHT);
    loadimage(&imgMenu, bgMenuPath, WIN_WIDTH, WIN_HEIGHT);
    loadimage(&imgGame, bgGamePath, WIN_WIDTH, WIN_HEIGHT);
    loadimage(&imgWin, bgWinPath);
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
