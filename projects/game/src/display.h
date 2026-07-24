#pragma once
#include <cstdio>
#include <string>
#ifdef _WIN32
  #include <windows.h>
  #include <conio.h>
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

inline void init() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
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
    return Key::UNKNOWN;
#endif
}

} // namespace disp
