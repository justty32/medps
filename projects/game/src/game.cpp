#include "game.h"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "gcore/common/actor.h"
#include "gcore/common/components/location.h"
#include "gcore/common/components/name.h"
#include "gcore/common/components/owner.h"
#include "gcore/common/components/unit.h"
#include "gcore/world/components/position.h"
#include "gcore/world/systems/movement.h"
#include "gcore/world/world_gen.h"
#include "gcore/zone/tile.h"

namespace fs = std::filesystem;

// ============================================================
// 地形渲染資料
// ============================================================

struct TileGlyph { char ch; Fg fg; Bg bg; };

static TileGlyph terrain_glyph(uint32_t terrain) {
    switch (terrain) {
        case TERRAIN_OCEAN:     return {'~', Fg::Blue,      Bg::DarkBlue};
        case TERRAIN_TUNDRA:    return {':', Fg::Gray,      Bg::Black};
        case TERRAIN_GRASSLAND: return {'.', Fg::DarkGreen, Bg::Black};
        case TERRAIN_FOREST:    return {'T', Fg::Green,     Bg::Black};
        default:                return {'?', Fg::DarkRed,   Bg::Black};
    }
}

// ============================================================
// 初始化
// ============================================================

void GameState::init(const std::string& save_dir) {
    // 每次開新遊戲都清掉舊存檔
    fs::remove_all(save_dir);
    fs::create_directories(save_dir);

    zm = std::make_unique<ZoneManager>(save_dir);

    // 建立 World 子 zone
    auto& wz = zm->create_child(ZONE_ROOT, ZoneKind::World);
    world = zone_cast<World>(&wz);
    if (!world) throw std::runtime_error("zone_cast<World> 失敗");

    world->gen = WorldGenParams{MAP_W, MAP_H, 42u, 0.40f, 0.05f, 5};
    world->generate();

    // 在 root 登記 actor 種類（def）
    actor::define_location(zm->root(), KIND_CITY,    "城市");
    actor::define_unit    (zm->root(), KIND_WARRIOR,  "武士");

    setup_game();

    // 初始視口：對準第一個玩家部隊
    auto first = next_player_unit();
    if (first != entt::null) {
        auto& p = world->reg.get<Position>(first);
        cursor_x = p.x; cursor_y = p.y;
        center_view_on(cursor_x, cursor_y);
    }

    message = "選取部隊:Enter  移動:方向鍵  略過:S  結束回合:T  離開:Q";
}

// ============================================================
// 地圖設置
// ============================================================

bool GameState::find_land_tile(int x_min, int x_max, int y_min, int y_max,
                               int& out_x, int& out_y) {
    auto& layer = world->layers[0];
    for (int attempt = 0; attempt < 3000; attempt++) {
        int x = x_min + rand() % (x_max - x_min);
        int y = y_min + rand() % (y_max - y_min);
        auto* tile = layer.getptr(x, y);
        if (!tile || !(tile->flags & TILE_WALKABLE)) continue;

        // 確保與已有實體保持距離
        bool too_close = false;
        for (auto e : world->reg.view<Position>()) {
            auto& p = world->reg.get<Position>(e);
            if (std::abs(p.x - x) + std::abs(p.y - y) < 5) {
                too_close = true; break;
            }
        }
        if (!too_close) { out_x = x; out_y = y; return true; }
    }
    return false;
}

entt::entity GameState::place_city(int x, int y, const std::string& name, uint64_t faction) {
    auto e = actor::spawn_location(world->reg, KIND_CITY, name, faction);
    auto& pos = world->reg.emplace<Position>(e);
    pos.x = x; pos.y = y; pos.z = 0;
    world->reg.emplace<Health>(e, Health{20, 20});
    return e;
}

entt::entity GameState::place_unit(int x, int y, const std::string& name, uint64_t faction) {
    auto e = actor::spawn_unit(world->reg, KIND_WARRIOR, name, faction);
    auto& pos = world->reg.emplace<Position>(e);
    pos.x = x; pos.y = y; pos.z = 0;
    world->reg.emplace<Health>(e, Health{10, 10});
    world->reg.emplace<Moves>(e, Moves{3, 3});
    return e;
}

void GameState::setup_game() {
    srand(777);  // 確定性配置

    int px, py;

    // ---- 玩家 (左三分之一) ----
    if (find_land_tile(2, MAP_W/3, 2, MAP_H-2, px, py))
        place_city(px, py, "首都", FACTION_PLAYER);
    if (find_land_tile(2, MAP_W/3, 2, MAP_H-2, px, py))
        place_city(px, py, "前哨", FACTION_PLAYER);
    for (int i = 0; i < 3; i++)
        if (find_land_tile(2, MAP_W/3, 2, MAP_H-2, px, py))
            place_unit(px, py, "武士", FACTION_PLAYER);

    // ---- AI (右三分之一) ----
    if (find_land_tile(2*MAP_W/3, MAP_W-2, 2, MAP_H-2, px, py))
        place_city(px, py, "敵都", FACTION_AI);
    if (find_land_tile(2*MAP_W/3, MAP_W-2, 2, MAP_H-2, px, py))
        place_city(px, py, "敵堡", FACTION_AI);
    for (int i = 0; i < 3; i++)
        if (find_land_tile(2*MAP_W/3, MAP_W-2, 2, MAP_H-2, px, py))
            place_unit(px, py, "侵略者", FACTION_AI);
}

// ============================================================
// 渲染
// ============================================================

void GameState::draw_map() {
    // 預先建立 (x,y) → entity 的查找表（城市先，部隊覆蓋城市）
    struct CellInfo { entt::entity ent = entt::null; bool is_unit = false; };
    // 用 MAP_W * MAP_H 的 flat array 加速查找
    static std::vector<CellInfo> cell_table;
    cell_table.assign(MAP_W * MAP_H, CellInfo{});

    for (auto e : world->reg.view<Location, Position>()) {
        auto& p = world->reg.get<Position>(e);
        if (p.z == 0 && p.x >= 0 && p.x < MAP_W && p.y >= 0 && p.y < MAP_H)
            cell_table[p.x * MAP_H + p.y] = {e, false};
    }
    for (auto e : world->reg.view<Unit, Position>()) {
        auto& p = world->reg.get<Position>(e);
        if (p.z == 0 && p.x >= 0 && p.x < MAP_W && p.y >= 0 && p.y < MAP_H)
            cell_table[p.x * MAP_H + p.y] = {e, true};
    }

    // 選取部隊的位置（用於高亮）
    int sel_x = -1, sel_y = -1;
    if (selected != entt::null && world->reg.valid(selected)) {
        auto& sp = world->reg.get<Position>(selected);
        sel_x = sp.x; sel_y = sp.y;
    }

    for (int sy = 0; sy < VIEW_H; sy++) {
        for (int sx = 0; sx < VIEW_W; sx++) {
            int wx = view_x + sx;
            int wy = view_y + sy;

            // 地形底色
            Tile def_tile{};
            Tile tile = (wx >= 0 && wx < MAP_W && wy >= 0 && wy < MAP_H)
                        ? world->layers[0].getval(wx, wy, def_tile)
                        : def_tile;
            auto [ch, fg, bg] = terrain_glyph(tile.terrain);

            // 實體覆蓋
            if (wx >= 0 && wx < MAP_W && wy >= 0 && wy < MAP_H) {
                auto& cell = cell_table[wx * MAP_H + wy];
                if (cell.ent != entt::null) {
                    auto& owner = world->reg.get<Owner>(cell.ent);
                    bool is_player = (owner.faction == FACTION_PLAYER);
                    if (cell.is_unit) {
                        ch = is_player ? '@' : '!';
                        fg = is_player ? Fg::Yellow : Fg::Red;
                        bg = Bg::Black;
                    } else {
                        ch = is_player ? 'C' : 'E';
                        fg = is_player ? Fg::Yellow : Fg::Red;
                        bg = Bg::Black;
                    }
                }
            }

            // 游標高亮
            bool is_cursor   = (wx == cursor_x && wy == cursor_y);
            bool is_selected = (wx == sel_x    && wy == sel_y);

            if (is_cursor)
                bg = Bg::Gray;
            else if (is_selected)
                bg = Bg::DarkGreen;

            printf("\033[%d;%dH\033[%d;%dm%c\033[0m",
                   sy + 2, sx + 1,   // +2: row 1 是狀態列
                   (int)fg, (int)bg, ch);
        }
    }

    // 分隔線
    for (int sy = 0; sy < VIEW_H; sy++)
        printf("\033[%d;%dH\033[37m|\033[0m", sy + 2, SEP_X + 1);
}

void GameState::draw_panel() {
    int px = PANEL_X;

    // 清面板
    for (int r = 0; r < VIEW_H; r++) {
        printf("\033[%d;%dH\033[0m%-38s", r + 2, px + 1, "");
    }

    int row = 2;  // screen row（1-indexed: row 1 是狀態列）
    auto line = [&](const std::string& s, Fg fg = Fg::White) {
        printf("\033[%d;%dH\033[%dm%-38s\033[0m", row, px + 1, (int)fg, s.c_str());
        row++;
    };
    auto blank = [&]() { row++; };

    // 回合標題
    std::string turn_label = " 回合 " + std::to_string(turn) + " ";
    std::string faction_label = player_turn ? "  [ 你的回合 ]  " : "  [ AI 回合 ]  ";
    line(turn_label + faction_label, player_turn ? Fg::Green : Fg::Red);
    blank();

    // 統計
    int p_cities = 0, p_units = 0, ai_cities = 0, ai_units = 0;
    for (auto e : world->reg.view<Location, Owner>()) {
        auto& o = world->reg.get<Owner>(e);
        if (o.faction == FACTION_PLAYER) p_cities++;
        else if (o.faction == FACTION_AI) ai_cities++;
    }
    for (auto e : world->reg.view<Unit, Owner>()) {
        auto& o = world->reg.get<Owner>(e);
        if (o.faction == FACTION_PLAYER) p_units++;
        else if (o.faction == FACTION_AI) ai_units++;
    }
    line("你  城市:" + std::to_string(p_cities)  + " 部隊:" + std::to_string(p_units),  Fg::Yellow);
    line("敵  城市:" + std::to_string(ai_cities) + " 部隊:" + std::to_string(ai_units), Fg::Red);
    blank();

    // 選取部隊資訊
    if (selected != entt::null && world->reg.valid(selected)) {
        auto& nm  = world->reg.get<Name>(selected);
        auto& pos = world->reg.get<Position>(selected);
        auto* h   = world->reg.try_get<Health>(selected);
        auto* mv  = world->reg.try_get<Moves>(selected);

        line("── 選取中 ──", Fg::Cyan);
        line(" " + nm.value, Fg::Yellow);
        if (h)  line(" HP " + std::to_string(h->hp)  + "/" + std::to_string(h->max_hp),  Fg::Green);
        if (mv) line(" 移動 " + std::to_string(mv->remaining) + "/" + std::to_string(mv->per_turn), Fg::Cyan);
        line(" 位置 (" + std::to_string(pos.x) + "," + std::to_string(pos.y) + ")", Fg::Gray);
        blank();
    }

    // 游標位置的實體資訊
    auto cu = unit_at(cursor_x, cursor_y);
    auto cc = city_at(cursor_x, cursor_y);
    entt::entity ci = (cu != entt::null) ? cu : cc;
    if (ci != entt::null && ci != selected) {
        auto& nm  = world->reg.get<Name>(ci);
        auto& own = world->reg.get<Owner>(ci);
        auto* h   = world->reg.try_get<Health>(ci);
        bool enemy = (own.faction == FACTION_AI);
        line("── 游標 ──", Fg::DarkCyan);
        line(" " + nm.value, enemy ? Fg::Red : Fg::Yellow);
        if (h) line(" HP " + std::to_string(h->hp) + "/" + std::to_string(h->max_hp), Fg::Gray);
        blank();
    }

    // 操作說明
    while (row < VIEW_H + 2 - 7) blank();
    line("── 操作 ──", Fg::DarkGray);
    line(" 方向鍵/hjkl : 移動",     Fg::Gray);
    line(" Enter : 選取/取消選取",   Fg::Gray);
    line(" S : 略過部隊",            Fg::Gray);
    line(" T : 結束回合",            Fg::Gray);
    line(" Q : 離開",                Fg::Gray);
}

void GameState::draw_status_bar() {
    // 第 1 行（row=1）是全寬狀態列
    std::string bar = " MEDPS Strategy  回合:" + std::to_string(turn);
    bar += player_turn ? "  [你的回合]" : "  [AI回合]";

    // 修剪到螢幕寬度
    if (bar.size() > 99) bar.resize(99);
    Bg bar_bg = player_turn ? Bg::DarkBlue : Bg::DarkRed;
    printf("\033[1;1H\033[%d;%dm%-99s\033[0m", (int)Fg::White, (int)bar_bg, bar.c_str());

    // 最後 1 行（row=24）是訊息列
    // 截斷時確保不切斷 UTF-8 多位元組字元（往前退到合法 byte 邊界）
    std::string msg = message;
    if (msg.size() > 99) {
        msg.resize(99);
        while (!msg.empty() && (msg.back() & 0xC0) == 0x80)
            msg.pop_back();  // 退到 UTF-8 序列起始 byte
    }
    printf("\033[24;1H\033[%dm%-99s\033[0m", (int)Fg::Yellow, msg.c_str());
}

void GameState::render() {
    draw_map();
    draw_panel();
    draw_status_bar();
    disp::flush();
}

// ============================================================
// 視口
// ============================================================

void GameState::center_view_on(int x, int y) {
    view_x = x - VIEW_W / 2;
    view_y = y - VIEW_H / 2;
    clamp_view();
}

void GameState::clamp_view() {
    view_x = std::max(0, std::min(MAP_W - VIEW_W, view_x));
    view_y = std::max(0, std::min(MAP_H - VIEW_H, view_y));
}

// ============================================================
// 實體查詢
// ============================================================

entt::entity GameState::unit_at(int x, int y) const {
    for (auto e : world->reg.view<const Unit, const Position>()) {
        auto& p = world->reg.get<const Position>(e);
        if (p.x == x && p.y == y && p.z == 0) return e;
    }
    return entt::null;
}

entt::entity GameState::city_at(int x, int y) const {
    for (auto e : world->reg.view<const Location, const Position>()) {
        auto& p = world->reg.get<const Position>(e);
        if (p.x == x && p.y == y && p.z == 0) return e;
    }
    return entt::null;
}

entt::entity GameState::next_player_unit() const {
    for (auto e : world->reg.view<const Unit, const Owner, const Moves>()) {
        auto& owner = world->reg.get<const Owner>(e);
        auto& mv    = world->reg.get<const Moves>(e);
        if (owner.faction == FACTION_PLAYER && mv.remaining > 0)
            return e;
    }
    return entt::null;
}

// ============================================================
// 戰鬥
// ============================================================

void GameState::do_combat(entt::entity attacker, entt::entity defender) {
    auto* ah = world->reg.try_get<Health>(attacker);
    auto* dh = world->reg.try_get<Health>(defender);
    if (!ah || !dh) return;

    std::string aname = world->reg.get<Name>(attacker).value;
    std::string dname = world->reg.get<Name>(defender).value;

    int atk_dmg = std::max(1, ah->hp / 2);   // 攻擊者傷害
    int def_dmg = std::max(1, dh->hp / 3);   // 防守者反擊（稍弱）

    int new_ahp = ah->hp - def_dmg;
    int new_dhp = dh->hp - atk_dmg;

    message = aname + "⚔" + dname +
              " (-" + std::to_string(atk_dmg) + "/" + "-" + std::to_string(def_dmg) + "HP)";

    if (new_dhp <= 0) {
        world->reg.destroy(defender);
        message += " 消滅敵軍!";
    } else {
        dh->hp = new_dhp;
    }

    // defender 被 destroy 後，EnTT sparse set 可能 swap-and-pop 移動其他
    // component 的記憶體；重新取 attacker 的 Health 指標以確保安全。
    if (new_ahp <= 0) {
        if (selected == attacker) selected = entt::null;
        world->reg.destroy(attacker);
        message += " 我方陣亡!";
    } else if (world->reg.valid(attacker)) {
        auto* ah2 = world->reg.try_get<Health>(attacker);
        if (ah2) ah2->hp = new_ahp;
    }
}

void GameState::try_capture_city(entt::entity mover, int x, int y) {
    auto city = city_at(x, y);
    if (city == entt::null) return;
    auto& city_owner = world->reg.get<Owner>(city);
    auto& mover_owner = world->reg.get<Owner>(mover);
    if (city_owner.faction != mover_owner.faction) {
        auto& cname = world->reg.get<Name>(city).value;
        city_owner.faction = mover_owner.faction;
        message = "佔領 " + cname + "!";
    }
}

// ============================================================
// 移動（玩家與 AI 共用）
// ============================================================

bool GameState::try_move_unit(entt::entity e, int dx, int dy) {
    if (!world->reg.valid(e)) return false;

    auto& pos   = world->reg.get<Position>(e);
    int nx = pos.x + dx;
    int ny = pos.y + dy;

    // 邊界檢查
    if (nx < 0 || nx >= MAP_W || ny < 0 || ny >= MAP_H) return false;

    // 地形檢查
    auto* tile = world->layers[0].getptr(nx, ny);
    if (!tile || !(tile->flags & TILE_WALKABLE)) return false;

    auto& owner = world->reg.get<Owner>(e);

    // 目標格有部隊：友方擋路返回 false，敵方觸發戰鬥
    auto target_unit = unit_at(nx, ny);
    if (target_unit != entt::null) {
        if (world->reg.get<Owner>(target_unit).faction == owner.faction)
            return false;  // 友方擋路
        // 敵方：開戰
        do_combat(e, target_unit);
        if (world->reg.valid(e)) {
            // 攻擊者存活：若敵方死亡則推進並可佔領城市
            if (!world->reg.valid(target_unit)) {
                systems::move_by(*world, e, dx, dy);
                try_capture_city(e, nx, ny);
            }
            world->reg.get<Moves>(e).remaining--;
        }
        return true;
    }

    // 空格：直接移動
    systems::move_by(*world, e, dx, dy);
    if (world->reg.valid(e)) {
        world->reg.get<Moves>(e).remaining--;
        try_capture_city(e, nx, ny);
    }
    return true;
}

// ============================================================
// 回合管理
// ============================================================

void GameState::end_player_turn() {
    selected = entt::null;
    player_turn = false;
    message = "AI 思考中...";
    render();

    // 重設 AI 行動點
    for (auto e : world->reg.view<Unit, Owner, Moves>()) {
        auto& o = world->reg.get<Owner>(e);
        auto& m = world->reg.get<Moves>(e);
        if (o.faction == FACTION_AI) m.remaining = m.per_turn;
    }

    do_ai_turn();
    check_win_silent();
    if (game_over) return;

    // 重設玩家行動點，進入下一回合
    for (auto e : world->reg.view<Unit, Owner, Moves>()) {
        auto& o = world->reg.get<Owner>(e);
        auto& m = world->reg.get<Moves>(e);
        if (o.faction == FACTION_PLAYER) m.remaining = m.per_turn;
    }
    turn++;
    player_turn = true;
    message = "回合 " + std::to_string(turn) + " 開始！";

    // 自動跳到第一個有行動點的部隊
    auto first = next_player_unit();
    if (first != entt::null) {
        auto& p = world->reg.get<Position>(first);
        cursor_x = p.x; cursor_y = p.y;
        center_view_on(cursor_x, cursor_y);
        selected = first;
        message = "回合 " + std::to_string(turn) + " - " +
                  world->reg.get<Name>(first).value + " 可行動";
    }
}

// ---- 簡易 AI：貪婪往最近目標靠近 ----

void GameState::do_ai_turn() {
    // 先收集 AI 部隊列表（避免迭代中刪除造成 UB）
    std::vector<entt::entity> ai_units;
    for (auto e : world->reg.view<Unit, Owner>()) {
        if (world->reg.get<Owner>(e).faction == FACTION_AI)
            ai_units.push_back(e);
    }

    for (auto e : ai_units) {
        if (!world->reg.valid(e)) continue;
        auto* mv = world->reg.try_get<Moves>(e);
        if (!mv || mv->remaining <= 0) continue;

        // 找最近的玩家目標（部隊或城市）
        int best_dist = INT_MAX, tx = -1, ty = -1;
        auto& epos = world->reg.get<Position>(e);

        auto check_target = [&](int gx, int gy) {
            int d = std::abs(gx - epos.x) + std::abs(gy - epos.y);
            if (d > 0 && d < best_dist) { best_dist = d; tx = gx; ty = gy; }
        };

        for (auto t : world->reg.view<Unit, Owner, Position>()) {
            if (world->reg.get<Owner>(t).faction == FACTION_PLAYER) {
                auto& tp = world->reg.get<Position>(t);
                check_target(tp.x, tp.y);
            }
        }
        for (auto t : world->reg.view<Location, Owner, Position>()) {
            if (world->reg.get<Owner>(t).faction == FACTION_PLAYER) {
                auto& tp = world->reg.get<Position>(t);
                check_target(tp.x, tp.y);
            }
        }

        if (tx < 0) continue;  // 沒有目標（玩家全滅）

        // 每個行動點走一步，最多嘗試 20 步以免鬼打牆
        int stuck = 0;
        while (world->reg.valid(e) && mv->remaining > 0 && stuck < 20) {
            auto& cur = world->reg.get<Position>(e);
            int diff_x = tx - cur.x;
            int diff_y = ty - cur.y;
            if (diff_x == 0 && diff_y == 0) break;  // 已到達目標

            // 主方向（較長的軸）
            int dx = 0, dy = 0;
            if (std::abs(diff_x) >= std::abs(diff_y))
                dx = (diff_x > 0) ? 1 : -1;
            else
                dy = (diff_y > 0) ? 1 : -1;

            if (!try_move_unit(e, dx, dy)) {
                // 主方向受阻：往垂直方向試一步（朝向目標的次要軸）
                int dx2 = 0, dy2 = 0;
                if (dx != 0) {
                    // 原本水平移動，改試垂直（朝目標的 y 方向）
                    dy2 = (diff_y > 0) ? 1 : (diff_y < 0) ? -1 : 1;
                } else {
                    // 原本垂直移動，改試水平（朝目標的 x 方向）
                    dx2 = (diff_x > 0) ? 1 : (diff_x < 0) ? -1 : 1;
                }
                if (!try_move_unit(e, dx2, dy2)) { stuck++; break; }
            }
        }
    }
}

// ============================================================
// 勝負判定
// ============================================================

void GameState::check_win_silent() {
    int p_cities = 0, ai_cities = 0, p_units = 0, ai_units = 0;
    for (auto e : world->reg.view<Location, Owner>()) {
        auto f = world->reg.get<Owner>(e).faction;
        if (f == FACTION_PLAYER) p_cities++;
        else if (f == FACTION_AI) ai_cities++;
    }
    for (auto e : world->reg.view<Unit, Owner>()) {
        auto f = world->reg.get<Owner>(e).faction;
        if (f == FACTION_PLAYER) p_units++;
        else if (f == FACTION_AI) ai_units++;
    }

    if (ai_cities == 0 || (ai_cities == 0 && ai_units == 0)) {
        game_over = true; player_won = true;
        message = "★ 勝利！所有敵方城市已佔領！ ★";
    } else if (p_cities == 0 && p_units == 0) {
        game_over = true; player_won = false;
        message = "× 失敗！所有部隊與城市已淪陷！ ×";
    } else if (p_cities == 0) {
        game_over = true; player_won = false;
        message = "× 失敗！首都淪陷！ ×";
    }
}

void GameState::check_win() {
    check_win_silent();
}

// ============================================================
// 輸入處理
// ============================================================

void GameState::handle_input() {
    Key key = disp::wait_key();

    switch (key) {

    case Key::Q:
    case Key::ESCAPE:
        game_over = true;
        player_won = false;
        player_quit = true;
        message = "已離開遊戲";
        return;

    case Key::T:
        if (player_turn) end_player_turn();
        return;

    case Key::ENTER: {
        if (selected != entt::null && world->reg.valid(selected)) {
            // 取消選取
            selected = entt::null;
            message = "取消選取";
        } else {
            // 嘗試選取游標上的玩家部隊
            auto e = unit_at(cursor_x, cursor_y);
            if (e != entt::null && world->reg.get<Owner>(e).faction == FACTION_PLAYER) {
                auto* mv = world->reg.try_get<Moves>(e);
                if (mv && mv->remaining > 0) {
                    selected = e;
                    message = "選取 " + world->reg.get<Name>(e).value + "，方向鍵移動";
                } else {
                    selected = e;
                    message = "此部隊本回合已無行動點（S略過，T結束回合）";
                }
            } else if (e != entt::null) {
                message = "那是敵方部隊！";
            } else {
                message = "此處沒有部隊（移動游標後 Enter 選取）";
            }
        }
        break;
    }

    case Key::S: {
        // 略過選取中的部隊
        entt::entity to_skip = selected;
        if (to_skip == entt::null) to_skip = unit_at(cursor_x, cursor_y);
        if (to_skip != entt::null && world->reg.valid(to_skip)) {
            auto& o = world->reg.get<Owner>(to_skip);
            if (o.faction == FACTION_PLAYER) {
                auto* mv = world->reg.try_get<Moves>(to_skip);
                if (mv) mv->remaining = 0;
                selected = entt::null;

                auto next = next_player_unit();
                if (next != entt::null) {
                    selected = next;
                    auto& p = world->reg.get<Position>(next);
                    cursor_x = p.x; cursor_y = p.y;
                    center_view_on(cursor_x, cursor_y);
                    message = "略過，自動選取下一部隊";
                } else {
                    message = "所有部隊已行動，按 T 結束回合";
                }
            }
        }
        break;
    }

    case Key::UP:
    case Key::DOWN:
    case Key::LEFT:
    case Key::RIGHT: {
        int dx = 0, dy = 0;
        if (key == Key::UP)    dy = -1;
        if (key == Key::DOWN)  dy =  1;
        if (key == Key::LEFT)  dx = -1;
        if (key == Key::RIGHT) dx =  1;

        if (selected != entt::null && world->reg.valid(selected)) {
            // 選取狀態：移動部隊
            auto* mv = world->reg.try_get<Moves>(selected);
            if (!mv || mv->remaining <= 0) {
                message = "行動點已耗盡，按 S 略過或 T 結束回合";
                break;
            }
            if (try_move_unit(selected, dx, dy)) {
                if (world->reg.valid(selected)) {
                    auto& p = world->reg.get<Position>(selected);
                    cursor_x = p.x; cursor_y = p.y;
                    center_view_on(cursor_x, cursor_y);
                    auto* mv2 = world->reg.try_get<Moves>(selected);
                    if (mv2 && mv2->remaining == 0) {
                        message = world->reg.get<Name>(selected).value + " 行動點耗盡";
                    }
                } else {
                    // 部隊在戰鬥中陣亡
                    selected = entt::null;
                }
                check_win();
            } else {
                message = "無法移動（地形阻擋或越界）";
            }
        } else {
            // 未選取：移動游標
            int nx = std::max(0, std::min(MAP_W - 1, cursor_x + dx));
            int ny = std::max(0, std::min(MAP_H - 1, cursor_y + dy));
            cursor_x = nx; cursor_y = ny;

            // 視口跟隨
            if (cursor_x < view_x)             view_x = cursor_x;
            if (cursor_x >= view_x + VIEW_W)   view_x = cursor_x - VIEW_W + 1;
            if (cursor_y < view_y)             view_y = cursor_y;
            if (cursor_y >= view_y + VIEW_H)   view_y = cursor_y - VIEW_H + 1;
            clamp_view();

            // 顯示游標下的地形資訊
            auto* tile = world->layers[0].getptr(cursor_x, cursor_y);
            if (tile) {
                const char* tname = "未知";
                if (tile->terrain == TERRAIN_OCEAN)     tname = "海洋";
                else if (tile->terrain == TERRAIN_TUNDRA)    tname = "苔原";
                else if (tile->terrain == TERRAIN_GRASSLAND) tname = "草原";
                else if (tile->terrain == TERRAIN_FOREST)    tname = "森林";
                message = std::string("地形：") + tname +
                          " (" + std::to_string(cursor_x) + "," + std::to_string(cursor_y) + ")";
            }
        }
        break;
    }

    default: break;
    }
}

// ============================================================
// 主迴圈
// ============================================================

void GameState::run() {
    disp::cls();
    message = "回合 1 開始！Enter 選取部隊，方向鍵移動，T 結束回合";

    while (!game_over) {
        render();
        handle_input();
    }

    // 遊戲結束畫面（主動離開則跳過）
    if (!player_quit) {
        render();
        int msg_row = VIEW_H / 2 + 2;
        int msg_col = (VIEW_W - 32) / 2;
        if (player_won) {
            disp::put_str(msg_col, msg_row,   "==================================", Fg::Yellow, Bg::DarkBlue);
            disp::put_str(msg_col, msg_row+1, "      ★  恭喜，勝利！  ★        ", Fg::Yellow, Bg::DarkBlue);
            disp::put_str(msg_col, msg_row+2, "==================================", Fg::Yellow, Bg::DarkBlue);
        } else {
            disp::put_str(msg_col, msg_row,   "==================================", Fg::Red, Bg::Black);
            disp::put_str(msg_col, msg_row+1, "      ×  遊戲結束：失敗  ×       ", Fg::Red, Bg::Black);
            disp::put_str(msg_col, msg_row+2, "==================================", Fg::Red, Bg::Black);
        }
        disp::put_str(msg_col + 4, msg_row + 4, "按任意鍵退出...", Fg::Gray);
        disp::flush();
        disp::wait_key();
    }
}
