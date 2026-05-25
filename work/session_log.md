# Session Log — medps

- **起始時間**: 2026-05-25
- **作業系統**: Windows 11 Pro 10.0.26200
- **Agent**: Claude Code (Sonnet 4.6)
- **專案根目錄**: C:\code\mine\medps

---

- [2026-05-25] 初始化工作目錄結構（work/）並完成 Level 1-2 通用分析，識別專案類型為 4X 策略遊戲後端 C++ 函式庫，留檔於 work/architecture/level1.md 與 level2.md，同步生成 CLAUDE.md。
- [2026-05-25] 修復核心框架多個 bug（ComponentManager/MapEntity 的 OBJ_INIT_DEF 錯填、scene.hpp 的 is_base_of_v 參數顛倒、obj_types_list 漏登錄、world.h 重複草稿、新增 virtual GetTypeIDV() 解決 consteval 無法經指標 runtime 呼叫），已 commit/push（1dfa422）。
- [2026-05-25] 確定 ECS 重寫方向：採用 EnTT（clone 至 extern/entt）、地圖維持 grid、混合保留 Obj/Scene 給單例、解耦純 C++ 核心 + 薄 godot::Object facade、ECS 存檔選 EnTT snapshot + BinFSR archive adapter。
- [2026-05-25] 以 subagent 完成 BinFSR 序列化審查，留檔於 work/architecture/binfsr_audit.md（多項高風險：整 struct memcpy 不可攜、size_t 寬度、非 trivial 型別 UB、讀取端零界限檢查）。
- [2026-05-25] 序列化格式定案：cereal（PortableBinaryArchive）全面取代 BinFSR；protobuf 評估後否決（build 依賴、雙重 schema、不相容 EnTT snapshot）。產出分階段實作規劃 work/plan_ecs_rewrite.md（Phase 0 vendoring → 1 cereal 取代 BinFSR → 2 World+registry → 3 汰除舊 Component → 4 systems → 5 Godot facade），待使用者日後逐步執行。
