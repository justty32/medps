#pragma once
#include <memory>
#include <string>
#include <vector>
#include <entt.hpp>
#include "gcore/zone/zone_manager.h"
#include "gcore/world/world.h"
#include "display.h"

// ---- 遊戲專用 component（不進 AllComponents，不序列化）----

struct Health { int hp = 0, max_hp = 0; };
struct Moves  { int remaining = 0, per_turn = 0; };

// ---- 常數 ----

inline constexpr uint64_t FACTION_PLAYER = 1;
inline constexpr uint64_t FACTION_AI     = 2;
inline constexpr uint64_t KIND_CITY      = 1;
inline constexpr uint64_t KIND_WARRIOR   = 1;

// ---- 遊戲狀態 ----

struct GameState {
    // 地圖尺寸（此版用 80×40 以加速 worldgen + 縮短視窗需求）
    static constexpr int MAP_W  = 80;
    static constexpr int MAP_H  = 40;
    // 地圖視口（終端機至少需要 100 列、28 行）
    static constexpr int VIEW_W = 60;   // 視口寬（列）
    static constexpr int VIEW_H = 22;   // 視口高（行）
    // 右側資訊面板
    static constexpr int SEP_X    = 60;  // 分隔線所在列
    static constexpr int PANEL_X  = 61;  // 面板起始列
    static constexpr int PANEL_W  = 38;  // 面板寬

    // Zone 管理（ZoneManager 沒有預設建構式，用 unique_ptr 延遲初始化）
    std::unique_ptr<ZoneManager> zm;
    World* world = nullptr;

    // 視口狀態
    int view_x = 0, view_y = 0;   // 視口左上角在世界座標中的位置
    int cursor_x = 0, cursor_y = 0;
    entt::entity selected = entt::null;  // 目前選取的玩家部隊

    // 遊戲進程
    int  turn = 1;
    bool player_turn  = true;
    bool game_over    = false;
    bool player_won   = false;
    bool player_quit  = false;  // Q/Esc 主動離開，區分勝/敗/放棄
    std::string message;

    // 主要入口
    void init(const std::string& save_dir);
    void run();

private:
    // ---- 渲染 ----
    void render();
    void draw_map();
    void draw_panel();
    void draw_status_bar();

    // ---- 輸入 ----
    void handle_input();

    // ---- 遊戲邏輯 ----
    void end_player_turn();
    void do_ai_turn();
    void check_win();
    void check_win_silent();  // 不設 message，內部呼叫用

    // ---- 實體查詢 ----
    entt::entity unit_at(int x, int y) const;
    entt::entity city_at(int x, int y) const;

    // ---- 動作 ----
    bool try_move_unit(entt::entity e, int dx, int dy);
    void do_combat(entt::entity attacker, entt::entity defender);
    void try_capture_city(entt::entity mover, int x, int y);
    entt::entity next_player_unit() const;

    // ---- 視口 ----
    void center_view_on(int x, int y);
    void clamp_view();

    // ---- 初始化輔助 ----
    void setup_game();
    bool find_land_tile(int x_min, int x_max, int y_min, int y_max,
                        int& out_x, int& out_y);
    entt::entity place_city(int x, int y, const std::string& name, uint64_t faction);
    entt::entity place_unit(int x, int y, const std::string& name, uint64_t faction);
};
