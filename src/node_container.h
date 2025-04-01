#pragma once
#include "node_standard.h"

struct Table : Widget
{
    struct ColumnData 
    {
        direct_val<std::string> label = "";
        bindable<bool> visible;
        direct_val<ImGuiTableColumnFlags_> sizingPolicy = 0;
        direct_val<float> width = 0;
        direct_val<ImGuiTableColumnFlags_> flags;

        ColumnData();
        ColumnData(const std::string& l, ImGuiTableColumnFlags_ sizingPolicy, float w = 0);
        const ColumnData& Defaults() const { static ColumnData cd; return cd; }
        auto Properties()->std::vector<Prop>;
        bool PropertyUI(int i, UIContext& ctx);
    };

    direct_val<ImGuiTableFlags_> flags = ImGuiTableFlags_Borders;
    std::vector<ColumnData> columnData;
    direct_val<bool> header = true;
    bindable<bool> rowFilter;
    bindable<dimension_t> rowHeight = 0;
    direct_val<int> scrollFreeze_x = 0;
    direct_val<int> scrollFreeze_y = 0;
    direct_val<bool> scrollWhenDragging = false;
    direct_val<pzdimension2_t> style_cellPadding;
    bindable<color_t> style_headerBg;
    bindable<color_t> style_rowBg;
    bindable<color_t> style_rowBgAlt;
    bindable<color_t> style_childBg;
    event<> onBeginRow;
    event<> onEndRow;
    event<> onSetup;

    Table(UIContext&);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    int Behavior() { return Widget::Behavior() | SnapInterior | HasSizeX | HasSizeY; }
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties() ->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    auto Events()->std::vector<Prop>;
    bool EventUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_TABLE_CELLS_LARGE; }
    const Table& Defaults() { static Table var(UIContext::Defaults()); return var; }
    int ColumnCount(UIContext& ctx) { return (int)columnData.size(); }
};

struct Child : Widget
{
    direct_val<ImGuiChildFlags_> flags = ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened;
    direct_val<ImGuiWindowFlags_> wflags = ImGuiWindowFlags_NoSavedSettings;
    bindable<int> columnCount = 1;
    direct_val<bool> columnBorder = true;
    direct_val<bool> scrollWhenDragging = false;
    direct_val<pzdimension2_t> style_padding;
    direct_val<pzdimension2_t> style_spacing;
    direct_val<bool> style_outerPadding = true;
    direct_val<pzdimension_t> style_rounding;
    direct_val<pzdimension_t> style_borderSize;
    bindable<color_t> style_bg;
    
    Child(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    int Behavior();
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties() ->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_SQUARE_FULL; }
    void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    const Child& Defaults() { static Child var(UIContext::Defaults()); return var; }
    int ColumnCount(UIContext& ctx) { return columnCount.eval(ctx); }
};

struct CollapsingHeader : Widget
{
    bindable<std::string> label = "label";
    direct_val<ImGuiTreeNodeFlags_> flags;
    bindable<bool> open;
    bindable<color_t> style_header;
    bindable<color_t> style_hovered;
    bindable<color_t> style_active;

    CollapsingHeader(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    int Behavior() { return SnapSides | SnapInterior; }
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_ARROW_DOWN_WIDE_SHORT; }
    const CollapsingHeader& Defaults() { static CollapsingHeader var(UIContext::Defaults()); return var; }
};

struct TabBar : Widget
{
    direct_val<ImGuiTabBarFlags_> flags;
    field_ref<int> activeTab;
    bindable<color_t> style_tab;
    bindable<color_t> style_tabDimmed;
    bindable<color_t> style_hovered;
    bindable<color_t> style_selected;
    bindable<color_t> style_dimmedSelected;
    bindable<color_t> style_overline;
    direct_val<bool> style_regularWidth = false;

    TabBar(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    int Behavior() { return NoOverlayPos | SnapSides; }
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_FOLDER_CLOSED; }
    const TabBar& Defaults() { static TabBar var(UIContext::Defaults()); return var; }
};

struct TabItem : Widget
{
    bindable<std::string> label = "TabItem";
    direct_val<bool> closeButton = false;
    event<> onClose;

    TabItem(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    int Behavior() { return SnapInterior | SnapGrandparentClip | NoOverlayPos; }
    ImDrawList* DoDraw(UIContext& ctx);
    void DoDrawTools(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    auto Events()->std::vector<Prop>;
    bool EventUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_FOLDER; }
    const TabItem& Defaults() { static TabItem var(UIContext::Defaults()); return var; }
};

struct TreeNode : Widget
{
    direct_val<ImGuiTreeNodeFlags_> flags;
    bindable<std::string> label = "Node";
    bindable<bool> open;
    bool lastOpen;

    TreeNode(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    int Behavior() { return Widget::Behavior() | SnapInterior; }
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_SITEMAP; }
    const TreeNode& Defaults() { static TreeNode var(UIContext::Defaults()); return var; }
};

struct MenuBar : Widget
{
    MenuBar(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    int Behavior() { return NoOverlayPos; }
    ImDrawList* DoDraw(UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_ELLIPSIS; }
    const MenuBar& Defaults() { static MenuBar var(UIContext::Defaults()); return var; }
};

struct ContextMenu : Widget
{
    direct_val<std::string> label = "Item";
    direct_val<pzdimension2_t> style_padding;
    direct_val<pzdimension2_t> style_spacing;
    direct_val<pzdimension_t> style_rounding;
    
    ContextMenu(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    int Behavior() { return NoOverlayPos | NoContextMenu; }
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void ExportAllShortcuts(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_MESSAGE; }
    const ContextMenu& Defaults() { static ContextMenu var(UIContext::Defaults()); return var; }
};

struct MenuIt : Widget
{
    direct_val<bool> ownerDraw = false;
    direct_val<std::string> label = "Item";
    direct_val<shortcut_t> shortcut = "";
    direct_val<bool> separator = false;
    field_ref<bool> checked;
    event<> onChange;

    MenuIt(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    int Behavior() { return NoOverlayPos | NoContextMenu; }
    ImDrawList* DoDraw(UIContext& ctx);
    void DoDrawTools(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    auto Events()->std::vector<Prop>;
    bool EventUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void ExportShortcut(std::ostream& os, UIContext& ctx);
    void ExportAllShortcuts(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_BARS; }
    const MenuIt& Defaults() { static MenuIt var(UIContext::Defaults()); return var; }
};

struct Splitter : Widget
{
    direct_val<dimension_t> min_size1 = 10;
    direct_val<dimension_t> min_size2 = 10;
    field_ref<dimension_t> position;
    bindable<color_t> style_active;
    bindable<color_t> style_bg;
    
    Splitter(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    int Behavior() { return Widget::Behavior() | SnapInterior | HasSizeX | HasSizeY | NoOverlayPos; }
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    //void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_TABLE_COLUMNS; }
    const Splitter& Defaults() { static Splitter var(UIContext::Defaults()); return var; }
};
