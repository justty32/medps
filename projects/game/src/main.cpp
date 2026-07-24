#include <cstdio>
#include <stdexcept>
#ifdef _WIN32
  #include <conio.h>
#endif
#include "display.h"
#include "game.h"

static void wait_any_key() {
#ifdef _WIN32
    _getch();
#else
    getchar();
#endif
}

int main() {
    disp::init();

    try {
        GameState game;
        game.init("./game_save");
        game.run();
    } catch (const std::exception& ex) {
        disp::shutdown();
        printf("\n\n[錯誤] %s\n", ex.what());
        printf("Press any key to exit...\n");
        fflush(stdout);
        wait_any_key();
        return 1;
    } catch (...) {
        disp::shutdown();
        printf("\n\n[未知錯誤] 遊戲崩潰\n");
        printf("Press any key to exit...\n");
        fflush(stdout);
        wait_any_key();
        return 1;
    }

    disp::shutdown();
    printf("\n感謝遊玩 MEDPS Strategy！\n");
    fflush(stdout);
    return 0;
}
