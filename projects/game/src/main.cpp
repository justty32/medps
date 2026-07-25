#include <cstdio>
#include <stdexcept>
#include "display.h"
#include "game.h"

int main() {
    disp::init();

    try {
        GameState game;
        game.init("./game_save");
        game.run();
    } catch (const std::exception& ex) {
        // 先等鍵再 shutdown：此時終端仍在 raw mode，wait_key() 才是真的「按任意鍵」。
        printf("\033[0m\n\n[錯誤] %s\n", ex.what());
        printf("Press any key to exit...\n");
        fflush(stdout);
        disp::wait_key();
        disp::shutdown();
        return 1;
    } catch (...) {
        printf("\033[0m\n\n[未知錯誤] 遊戲崩潰\n");
        printf("Press any key to exit...\n");
        fflush(stdout);
        disp::wait_key();
        disp::shutdown();
        return 1;
    }

    disp::shutdown();
    printf("\n感謝遊玩 MEDPS Strategy！\n");
    fflush(stdout);
    return 0;
}
