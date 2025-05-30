#pragma once
#include <vector>
#include <memory>
#include <optional>
#include <imgui.h>
#include "uicontext.h"
#include "binding_property.h"
#include "imrad.h"
#include "IconsFontAwesome6.h"


struct PreparedString
{
    std::string label;
    ImVec2 pos;
    std::vector<std::pair<size_t, size_t>> fmtArgs;
    bool error = false;
};
PreparedString PrepareString(std::string_view s);

#define DRAW_STR(a) PrepareString(a.value()).label.c_str()

void DrawTextArgs(const PreparedString& ps, UIContext& ctx, const ImVec2& offset = { 0, 0 }, const ImVec2& size = { 0, 0 }, const ImVec2& align = { 0, 0 });

void TreeNodeProp(const char* name, ImFont* font, const std::string& label, std::function<void()> f);

//----------------------------------------------------------

struct Widget;

struct UINode
{
    struct Prop {
        std::string_view name;
        property_base* property;
        bool kbdInput = false; //this property accepts keyboard input by default
    };
    enum SnapOptions {
        SnapSides = 0x1,
        SnapInterior = 0x2,
        SnapItemInterior = 0x4,
        SnapGrandparentClip = 0x8,
        NoContextMenu = 0x100,
        NoOverlayPos = 0x200,
        HasSizeX = 0x400,
        HasSizeY = 0x800,
    };

    UINode() {}
    UINode(const UINode&) {} //shallow copy
    virtual ~UINode() {}
    virtual void Draw(UIContext& ctx) = 0;
    virtual void DrawTools(UIContext& ctx) = 0;
    virtual void TreeUI(UIContext& ctx) = 0;
    virtual auto Properties()->std::vector<Prop> = 0;
    virtual auto Events()->std::vector<Prop> = 0;
    virtual bool PropertyUI(int, UIContext& ctx) = 0;
    virtual bool EventUI(int, UIContext& ctx) = 0;
    virtual void Export(std::ostream&, UIContext& ctx) = 0;
    virtual void Import(cpp::stmt_iterator& sit, UIContext& ctx) = 0;
    virtual int Behavior() = 0;
    virtual int ColumnCount(UIContext& ctx) = 0;
    virtual const UINode& Defaults() = 0;

    void DrawInteriorRect(UIContext& ctx);
    void DrawSnap(UIContext& ctx);
    auto UsedFieldVars() -> std::vector<std::string>;
    void RenameFieldVars(const std::string& oldn, const std::string& newn);
    auto FindChild(const UINode*) -> std::optional<std::pair<UINode*, int>>;
    auto FindInRect(const ImRect& r) -> std::vector<UINode*>;
    auto GetAllChildren() -> std::vector<UINode*>;
    void CloneChildrenFrom(const UINode& node, UIContext& ctx);
    void ResetLayout();
    virtual auto GetTypeName()->std::string;
    auto GetParentIndexes(UIContext& ctx)->std::string;
    void PushError(UIContext& ctx, const std::string& err);

    struct child_iterator;

    ImVec2 cached_pos;
    ImVec2 cached_size;
    std::vector<std::unique_ptr<Widget>> children;
    std::vector<ImRad::VBox> vbox;
    std::vector<ImRad::HBox> hbox;
};

//to iterate children matching a filter e.g. free-positioned or not
struct UINode::child_iterator
{
    using children_type = std::vector<std::unique_ptr<Widget>>;

    struct iter
    {
        iter();
        iter(children_type& ch, bool freePos);
        iter& operator++ ();
        iter operator++ (int);
        bool operator== (const iter& it) const;
        bool operator!= (const iter& it) const;
        children_type::value_type& operator* ();
        const children_type::value_type& operator* () const;
        size_t index() const;

    private:
        bool end() const;
        bool valid() const;

        size_t idx;
        children_type* children;
        bool freePos;
    };

    child_iterator(children_type& children, bool freePos);
    iter begin() const;
    iter end() const;
    explicit operator bool() const;

private:
    children_type& children;
    bool freePos;
};

//--------------------------------------------------------------------

struct Widget : UINode
{
    struct Layout
    {
        enum { Topmost = 0x1, Leftmost = 0x2, Bottommost = 0x4, HLayout = 0x10, VLayout = 0x20 };
        int flags = 0;
        int colId = 0;
        int rowId = 0;
        size_t index = 0;
    };

    direct_val<bool> sameLine = false;
    direct_val<int> nextColumn = 0;
    bindable<dimension_t> size_x = 0;
    bindable<dimension_t> size_y = 0;
    bindable<bool> visible;
    bindable<bool> disabled;
    bindable<std::string> tooltip = "";
    direct_val<std::string> contextMenu = "";
    direct_val<ImGuiMouseCursor_> cursor = ImGuiMouseCursor_Arrow;
    direct_val<bool> tabStop = true;
    direct_val<bool> initialFocus = false;
    bindable<bool> forceFocus;
    direct_val<ImRad::Alignment> hasPos = ImRad::AlignNone;
    bindable<dimension_t> pos_x = 0;
    bindable<dimension_t> pos_y = 0;
    direct_val<int> indent = 0;
    direct_val<int> spacing = 0;
    direct_val<bool> allowOverlap = false;

    //not shown by default
    data_loop itemCount;
    bindable<font_name_t> style_font;
    bindable<color_t> style_text;
    bindable<color_t> style_frameBg;
    bindable<color_t> style_border;
    bindable<color_t> style_button;
    bindable<color_t> style_buttonHovered;
    bindable<color_t> style_buttonActive;
    direct_val<pzdimension_t> style_frameRounding;
    direct_val<pzdimension2_t> style_framePadding;
    direct_val<pzdimension_t> style_frameBorderSize;
    direct_val<pzdimension2_t> style_interiorPadding;

    event<> onItemClicked;
    event<> onItemDoubleClicked;
    event<> onItemHovered;
    event<> onItemFocused;
    event<> onItemActivated;
    event<> onItemDeactivated;
    event<> onItemDeactivatedAfterEdit;
    event<> onItemContextMenuClicked;
    event<> onItemLongPressed; //not shown by default
    event<> onDragDropSource;
    event<> onDragDropTarget;

    std::string userCodeBefore, userCodeAfter;

    static std::unique_ptr<Widget> Create(const std::string& s, UIContext& ctx);

    Widget();
    void Draw(UIContext& ctx);
    void DrawTools(UIContext& ctx);
    void Export(std::ostream& os, UIContext& ctx);
    void Import(cpp::stmt_iterator& sit, UIContext& ctx);
    auto Properties() -> std::vector<Prop>;
    auto Events() -> std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    void TreeUI(UIContext& ctx);
    bool EventUI(int, UIContext& ctx);
    int Behavior();
    int ColumnCount(UIContext& ctx) { return 0; }
    const Widget& Defaults() = 0;
    Layout GetLayout(UINode* parent);
    virtual std::unique_ptr<Widget> Clone(UIContext& ctx) = 0;
    virtual ImDrawList* DoDraw(UIContext& ctx) = 0;
    virtual void DoDrawTools(UIContext& ctx) {}
    virtual void DoExport(std::ostream& os, UIContext& ctx) = 0;
    virtual void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx) = 0;
    virtual void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    virtual const char* GetIcon() const { return ""; }
};

struct Spacer : Widget
{
    Spacer(UIContext&);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int, UIContext& ctx);
    void DoExport(std::ostream&, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    int Behavior() { return Widget::Behavior() | HasSizeX | HasSizeY; }
    const char* GetIcon() const { return ICON_FA_LEFT_RIGHT; }
    const Spacer& Defaults() { static Spacer var(UIContext::Defaults()); return var; }
};

struct Separator : Widget
{
    bindable<std::string> label;
    direct_val<pzdimension_t> style_thickness;
    direct_val<bool> style_outerPadding = true;

    Separator(UIContext&);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int, UIContext& ctx);
    void DoExport(std::ostream&, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_MINUS; }
    const Separator& Defaults() { static Separator var(UIContext::Defaults()); return var; }
};

struct Text : Widget
{
    bindable<std::string> text = "text";
    direct_val<bool> alignToFrame = false;
    direct_val<bool> wrap = false;
    direct_val<bool> link = false;
    event<> onChange;

    Text(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties() ->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    auto Events()->std::vector<Prop>;
    bool EventUI(int, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_FONT; }
    const Text& Defaults() { static Text var(UIContext::Defaults()); return var; }
};

struct Selectable : Widget
{
    bindable<std::string> label = "label";
    direct_val<ImGuiSelectableFlags_> flags = ImGuiSelectableFlags_NoAutoClosePopups;
    direct_val<ImRad::Alignment> horizAlignment = ImRad::AlignLeft;
    direct_val<ImRad::Alignment> vertAlignment = ImRad::AlignTop;
    direct_val<bool> alignToFrame = false;
    direct_val<bool> readOnly = false;
    direct_val<ImRad::ModalResult> modalResult = ImRad::None;
    bindable<bool> selected = false;
    bindable<color_t> style_header;
    event<> onChange;

    Selectable(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties() ->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    auto Events()->std::vector<Prop>;
    bool EventUI(int, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    int Behavior() { return Widget::Behavior() | HasSizeX | HasSizeY | SnapItemInterior; }
    const char* GetIcon() const { return ICON_FA_AUDIO_DESCRIPTION; }
    const Selectable& Defaults() { static Selectable var(UIContext::Defaults()); return var; }
};

struct Button : Widget
{
    bindable<std::string> label = "OK";
    direct_val<ImGuiDir> arrowDir = ImGuiDir_None;
    direct_val<bool> small = false;
    direct_val<ImRad::ModalResult> modalResult = ImRad::None;
    direct_val<shortcut_t> shortcut = "";
    direct_val<std::string> dropDownMenu = "";
    event<> onChange;

    Button(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties() ->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    auto Events() ->std::vector<Prop>;
    bool EventUI(int, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    int Behavior();
    const char* GetIcon() const { return ICON_FA_CIRCLE_PLAY; }
    const Button& Defaults() { static Button var(UIContext::Defaults()); return var; }
};

struct CheckBox : Widget
{
    bindable<std::string> label = "label";
    bindable<bool> checked;
    bindable<color_t> style_check;
    event<> onChange;

    CheckBox(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties() ->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    auto Events()->std::vector<Prop>;
    bool EventUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_SQUARE_CHECK; }
    const CheckBox& Defaults() { static CheckBox var(UIContext::Defaults()); return var; }
};

struct RadioButton : Widget
{
    bindable<std::string> label = "label";
    direct_val<int> valueID = 0;
    bindable<color_t> style_check;
    bindable<int> value;
    event<> onChange;

    RadioButton(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties() ->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    auto Events()->std::vector<Prop>;
    bool EventUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_CIRCLE_DOT; }
    const RadioButton& Defaults() { static RadioButton var(UIContext::Defaults()); return var; }
};

struct Input : Widget
{
    bindable<> value;
    direct_val<std::string> label = "";
    direct_val<int, true> type = 0;
    bindable<std::string> hint = "";
    direct_val<int> imeType = ImRad::ImeText;
    direct_val<float> step = 1;
    direct_val<std::string> format = "%.3f";
    direct_val<ImGuiInputTextFlags_> flags;
    event<> onChange;
    event<> onImeAction;
    event<int(ImGuiInputTextCallbackData&)> onCallback;

    static direct_val<ImRad::ImeType> _imeClass;
    static direct_val<ImRad::ImeType> _imeAction;

    Input(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties() ->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    auto Events()->std::vector<Prop>;
    bool EventUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    int Behavior();
    const char* GetIcon() const { return "|a_|"; }
    const Input& Defaults() { static Input var(UIContext::Defaults()); return var; }
};

struct Combo : Widget
{
    direct_val<std::string> label = "";
    bindable<> value; //don't use bindable<string> that would force format style edit
    bindable<std::vector<std::string>> items;
    direct_val<ImGuiComboFlags_> flags;
    event<> onChange;

    Combo(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties() ->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    auto Events()->std::vector<Prop>;
    bool EventUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    int Behavior() { return Widget::Behavior() | HasSizeX; }
    const char* GetIcon() const { return ICON_FA_SQUARE_CARET_DOWN; }
    const Combo& Defaults() { static Combo var(UIContext::Defaults()); return var; }
};

struct Slider : Widget
{
    direct_val<ImGuiSliderFlags_> flags;
    direct_val<std::string> label = "";
    bindable<> value;
    direct_val<int, true> type = 0;
    direct_val<float> min = 0;
    direct_val<float> max = 1;
    direct_val<std::string> format = "";
    event<> onChange;

    Slider(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    auto Events()->std::vector<Prop>;
    bool EventUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    int Behavior() { return Widget::Behavior() | HasSizeX; }
    const char* GetIcon() const { return ICON_FA_SLIDERS; }
    const Slider& Defaults() { static Slider var(UIContext::Defaults()); return var; }
};

struct ProgressBar : Widget
{
    direct_val<bool> indicator = true;
    bindable<float> value;
    bindable<color_t> style_color;

    ProgressBar(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    int Behavior() { return Widget::Behavior() | HasSizeX | HasSizeY; }
    const char* GetIcon() const { return ICON_FA_BATTERY_HALF; }
    const ProgressBar& Defaults() { static ProgressBar var(UIContext::Defaults()); return var; }
};

struct ColorEdit : Widget
{
    bindable<> value;
    direct_val<std::string> label = "";
    direct_val<std::string> type = "color3";
    direct_val<ImGuiColorEditFlags_> flags;
    event<> onChange;

    ColorEdit(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    auto Events()->std::vector<Prop>;
    bool EventUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    int Behavior() { return Widget::Behavior() | HasSizeX; }
    const char* GetIcon() const { return ICON_FA_CIRCLE_HALF_STROKE; }
    const ColorEdit& Defaults() { static ColorEdit var(UIContext::Defaults()); return var; }
};

struct Image : Widget
{
    bindable<std::string> fileName = "";
    bindable<ImRad::Texture> texture;
    enum StretchPolicy { None, Scale, FitIn, FitOut };
    direct_val<StretchPolicy> stretchPolicy = Scale;

    ImRad::Texture tex;

    Image(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    void RefreshTexture(UIContext& ctx);
    bool PickFileName(UIContext& ctx);
    int Behavior() { return Widget::Behavior() | HasSizeX | HasSizeY; }
    const char* GetIcon() const { return ICON_FA_IMAGE; }
    const Image& Defaults() { static Image var(UIContext::Defaults()); return var; }
};

struct CustomWidget : Widget
{
    event<void(const ImRad::CustomWidgetArgs&)> onDraw;

    CustomWidget(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    auto Events()->std::vector<Prop>;
    bool EventUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    int Behavior() { return Widget::Behavior() | HasSizeX | HasSizeY; }
    const char* GetIcon() const { return ICON_FA_EXPAND; }
    const CustomWidget& Defaults() { static CustomWidget var(UIContext::Defaults()); return var; }
};
