#pragma once
#include <cstdint>

// 地點種類的「定義」（def）。掛在 root zone 的實體上——root 本就放全局實體
//（陣營/神祇/具名角色，見 zone.h），再加上「規則定義」這一類。一個帶
// LocationKind 的實體＝一種地點的 def。種類是**開放集合、資料驅動**，不是寫死的
// enum：要新增種類就多建一個 def 實體（將來由 Ruleset 載入）。
//
// def 的顯示名走 Name 元件（"城市"/"城堡"…）；玩法欄位（防禦、產出…）日後長在這。
// id＝零語意穩定序號，供跨 registry 的 actor 參照（比照 zone id / Owner.faction）；
// 現階段手動配發，將來由 Ruleset/def 登錄機制發號（比照 ZoneManager 配 zone id）。
struct LocationKind {
    uint64_t id = 0;

    template <class Archive>
    void serialize(Archive& ar) { ar(id); }
};

// 「地點基類」的 ECS 對應：actor（住在某個 zone 內）帶此 tag 即為地點 actor，
// 與部隊（Unit）並列為 actor 兩大家族。地點固定不動，故不配 Velocity；
// 佔一格地圖（Position 由放置流程另掛）。
// kind＝它所屬 LocationKind def 的穩定 id（def 實體住在 root）；0＝未指定。
struct Location {
    uint64_t kind = 0;

    template <class Archive>
    void serialize(Archive& ar) { ar(kind); }
};
