#pragma once
#include "node_standard.h"

struct DockSpace : Widget
{
    flags_helper flags = 0;
    bindable<color32> style_preview;
    bindable<color32> style_emptyBg;
    
    DockSpace(UIContext& ctx);
    uint32_t CalcHash(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    int Behavior() { return SnapSides | NoOverlayPos | HasSizeX | HasSizeY; }
    ImDrawList* DoDraw(UIContext& ctx);
    void DoDrawTools(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_TABLE; }
    const DockSpace& Defaults() { static DockSpace var; return var; }
private:
    DockSpace() {}
};

struct DockNode : Widget
{
    flags_helper flags = 0;
    direct_val<ImGuiDir> splitDir = ImGuiDir_None;
    bindable<float> splitRatio = 0;
    direct_val<std::string> labels = "";
    ImGuiID nodeId = 0;

    DockNode(UIContext& ctx);
    void CalcHash(uint32_t& hash);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    int Behavior() { return NoOverlayPos; }
    ImGuiID SplitNode(ImGuiID parentId, UIContext& ctx);
    ImDrawList* DoDraw(UIContext& ctx);
    void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    void DoDrawTools(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    auto Events()->std::vector<Prop>;
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    void ExportHelp(std::ostream& os, UIContext& ctx);
    const char* GetIcon() const { return "N"; }
    const DockNode& Defaults() { static DockNode var; return var; }
private:
    DockNode() {}
};
