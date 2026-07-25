#pragma once
#include <cstdio>
#include <string>
#ifdef _WIN32
  #include <windows.h>
  #include <conio.h>
#else
  #include <termios.h>
  #include <unistd.h>
  #include <cerrno>
  #include <cstdlib>
#endif

// 前景色（ANSI 16-color）
enum class Fg : int {
    Black=30, DarkBlue=34, DarkGreen=32, DarkCyan=36,
    DarkRed=31, DarkMagenta=35, DarkYellow=33, Gray=37,
    DarkGray=90, Blue=94, Green=92, Cyan=96,
    Red=91, Magenta=95, Yellow=93, White=97
};
// 背景色
enum class Bg : int {
    Black=40, DarkBlue=44, DarkGreen=42, DarkCyan=46,
    DarkRed=41, DarkMagenta=45, DarkYellow=43, Gray=47,
    DarkGray=100, Blue=104, Green=102, Cyan=106,
    Red=101, Magenta=105, Yellow=103, White=107
};

enum class Key {
    UP, DOWN, LEFT, RIGHT,
    ENTER, SPACE, ESCAPE,
    Q, T, S, N,
    UNKNOWN
};

namespace disp {

#ifndef _WIN32
// POSIX 終端輸入：Windows 靠 conio 的 _getch() 逐鍵讀，Linux/macOS 沒有對應物，
// 得自己把 stdin 切成 raw mode 再用 read() 一個 byte 一個 byte 拿。
namespace detail {

// header-only，用 function-local static 存原始 termios，避免多個 TU 重複定義。
inline termios& saved_termios() { static termios t{}; return t; }
inline bool& raw_active() { static bool active = false; return active; }

// 套用 raw 參數。刻意「只」關 ICANON（逐字元）與 ECHO（不回顯）：
// - OPOST 保留 → printf 的 '\n' 仍會補 CR，排版不會階梯化
// - ISIG 保留 → Ctrl-C 仍能中斷，留一條逃生門
inline void apply_raw(cc_t vmin, cc_t vtime) {
    termios t = saved_termios();
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN]  = vmin;
    t.c_cc[VTIME] = vtime;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

inline void leave_raw() {
    if (!raw_active()) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_termios());
    raw_active() = false;
}

inline void enter_raw() {
    if (raw_active()) return;
    // stdin 不是終端（管線、重導向）就別碰，維持一般 blocking 讀取語意。
    if (tcgetattr(STDIN_FILENO, &saved_termios()) != 0) return;
    raw_active() = true;
    apply_raw(1, 0);   // 阻塞直到至少 1 byte，不設逾時
    // 未經 shutdown() 的退出（例外逸出、abort）也要還原，否則 shell 會失去回顯。
    std::atexit(leave_raw);
}

// 讀一個 byte，回傳 -1 表示沒讀到。
// timed=true 時改成最多等 ~0.1 秒，用來區分「單獨按 ESC」與「方向鍵的 CSI 序列」。
inline int read_byte(bool timed) {
    if (raw_active() && timed) apply_raw(0, 1);
    unsigned char c = 0;
    ssize_t n;
    do { n = read(STDIN_FILENO, &c, 1); } while (n < 0 && errno == EINTR);
    if (raw_active() && timed) apply_raw(1, 0);
    return n == 1 ? (int)c : -1;
}

} // namespace detail
#endif

inline void init() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#else
    detail::enter_raw();
#endif
    setvbuf(stdout, nullptr, _IOFBF, 131072);
#ifdef _WIN32

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    // 把視窗弄大一點
    SMALL_RECT sr{0, 0, 99, 27};
    SetConsoleWindowInfo(hOut, TRUE, &sr);
    COORD sz{100, 28};
    SetConsoleScreenBufferSize(hOut, sz);
#endif
    printf("\033[?25l");   // 隱藏游標
    printf("\033[2J\033[H");
    fflush(stdout);
}

inline void shutdown() {
    printf("\033[?25h\033[0m");  // 還原游標＋重設色彩
    fflush(stdout);
#ifndef _WIN32
    detail::leave_raw();
#endif
}

inline void cls() {
    printf("\033[2J\033[H");
}

inline void flush() {
    fflush(stdout);
}

// 移動終端游標到 (col, row)，0-based
inline void goto_xy(int x, int y) {
    printf("\033[%d;%dH", y + 1, x + 1);
}

// 單字元：位置＋色彩一次輸出
inline void put_char(int x, int y, char c, Fg fg, Bg bg = Bg::Black) {
    printf("\033[%d;%dH\033[%d;%dm%c\033[0m", y+1, x+1, (int)fg, (int)bg, c);
}

// 字串：位置＋色彩一次輸出（不換行）
inline void put_str(int x, int y, const std::string& s, Fg fg, Bg bg = Bg::Black) {
    printf("\033[%d;%dH\033[%d;%dm%-*s\033[0m",
           y+1, x+1, (int)fg, (int)bg,
           (int)s.size(), s.c_str());
}

// 用空白填滿一列，清除舊內容
inline void clear_line(int y, int x_start, int w) {
    printf("\033[%d;%dH\033[0m", y+1, x_start+1);
    for (int i = 0; i < w; i++) putchar(' ');
}

inline Key wait_key() {
#ifdef _WIN32
    int c = _getch();
    if (c == 0 || c == 224) {
        int c2 = _getch();
        switch (c2) {
            case 72: return Key::UP;
            case 80: return Key::DOWN;
            case 75: return Key::LEFT;
            case 77: return Key::RIGHT;
        }
        return Key::UNKNOWN;
    }
    switch (c) {
        case 13: case 10: return Key::ENTER;
        case 32:           return Key::SPACE;
        case 27:           return Key::ESCAPE;
        case 'q': case 'Q': return Key::Q;
        case 't': case 'T': return Key::T;
        case 's': case 'S': return Key::S;
        case 'n': case 'N': return Key::N;
        // vim-style movement
        case 'h': return Key::LEFT;
        case 'j': return Key::DOWN;
        case 'k': return Key::UP;
        case 'l': return Key::RIGHT;
    }
    return Key::UNKNOWN;
#else
    int c = detail::read_byte(false);
    // stdin 關閉或讀取失敗：當成離開。呼叫端是 `while (!game_over)` 迴圈，
    // 這裡若回 UNKNOWN 就會變成不吃輸入的忙轉。
    if (c < 0) return Key::ESCAPE;

    if (c == 0x1b) {
        // ESC 開頭：方向鍵是 CSI 序列（\033[A~D，應用模式為 \033OA~D），
        // 但使用者也可能只是單按 ESC——後續 byte 等不到就是後者。
        int c1 = detail::read_byte(true);
        if (c1 < 0) return Key::ESCAPE;
        if (c1 != '[' && c1 != 'O') return Key::UNKNOWN;
        switch (detail::read_byte(true)) {
            case 'A': return Key::UP;
            case 'B': return Key::DOWN;
            case 'C': return Key::RIGHT;
            case 'D': return Key::LEFT;
        }
        return Key::UNKNOWN;
    }

    switch (c) {
        case 13: case 10: return Key::ENTER;
        case 32:           return Key::SPACE;
        case 'q': case 'Q': return Key::Q;
        case 't': case 'T': return Key::T;
        case 's': case 'S': return Key::S;
        case 'n': case 'N': return Key::N;
        // vim-style movement
        case 'h': return Key::LEFT;
        case 'j': return Key::DOWN;
        case 'k': return Key::UP;
        case 'l': return Key::RIGHT;
    }
    return Key::UNKNOWN;
#endif
}

} // namespace disp
