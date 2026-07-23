#pragma once
#include <cstdint>

// 部隊種類的「定義」（def）。掛在 root zone 的實體上（root 放全局實體＋規則定義，
// 見 zone.h）。一個帶 UnitKind 的實體＝一種部隊的 def。種類是**開放集合、資料驅動**，
// 不是寫死的 enum：要新增種類就多建一個 def 實體（將來由 Ruleset 載入）。
//
// def 的顯示名走 Name 元件（"軍隊"/"開拓者"…）；玩法欄位（移動力、戰力…）日後長在這。
// id＝零語意穩定序號，供跨 registry 的 actor 參照（比照 zone id / Owner.faction）。
struct UnitKind {
    uint64_t id = 0;

    template <class Archive>
    void serialize(Archive& ar) { ar(id); }
};

// 「部隊基類」的 ECS 對應：actor（住在某個 zone 內）帶此 tag 即為部隊 actor，
// 與地點（Location）並列為 actor 兩大家族。部隊會動，故通常另配 Velocity/移動點
//（移動模型待細修）；佔一格地圖（Position）。
// kind＝它所屬 UnitKind def 的穩定 id（def 實體住在 root）；0＝未指定。
struct Unit {
    uint64_t kind = 0;

    template <class Archive>
    void serialize(Archive& ar) { ar(kind); }
};
