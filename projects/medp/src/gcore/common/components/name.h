#pragma once
#include <string>
#include <cereal/types/string.hpp>

// actor 的顯示名稱。任何被玩家指涉的實體（地點、部隊、將來的將領/神祇…）都可掛。
// 空字串＝匿名或程序生成尚未命名。actor 兩大家族（Location / Unit）共用的身分元件之一。
struct Name {
    std::string value;

    template <class Archive>
    void serialize(Archive& ar) { ar(value); }
};
