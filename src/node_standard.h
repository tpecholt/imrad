#pragma once
#include <vector>
#include <memory>
#include <optional>
#include <imgui.h>
#include "uicontext.h"
#include "binding_property.h"
#include "imrad.h"
#include "IconsFontAwesome6.h"


#define DRAW_STR(a) cpp::to_draw_str(a.value()).c_str()

extern const color32 FIELD_REF_CLR;

void TreeNodeProp(const char* name, const std::string& label, std::function<void()> f);


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
        SnapGrandparentClip = 0x4,
        NoContextMenu = 0x8,
        NoOverlayPos = 0x10,
        HasSizeX = 0x20,
        HasSizeY = 0x40,
    };

    UINode() {}
    UINode(const UINode&) {} //shallow copy
    virtual ~UINode() {}
    virtual void Draw(UIContext& ctx) = 0;
    virtual void DrawExtra(UIContext& ctx) = 0;
    virtual void TreeUI(UIContext& ctx) = 0;
    virtual auto Properties()->std::vector<Prop> = 0;
    virtual auto Events()->std::vector<Prop> = 0;
    virtual bool PropertyUI(int, UIContext& ctx) = 0;
    virtual bool EventUI(int, UIContext& ctx) = 0;
    virtual void Export(std::ostream&, UIContext& ctx) = 0;
    virtual void Import(cpp::stmt_iterator& sit, UIContext& ctx) = 0;
    virtual int Behavior() = 0;

    void DrawInteriorRect(UIContext& ctx);
    void DrawSnap(UIContext& ctx);
    void RenameFieldVars(const std::string& oldn, const std::string& newn);
    auto FindChild(const UINode*) -> std::optional<std::pair<UINode*, int>>;
    auto FindInRect(const ImRect& r) -> std::vector<UINode*>;
    auto GetAllChildren() -> std::vector<UINode*>;
    void CloneChildrenFrom(const UINode& node, UIContext& ctx);
    void ResetLayout();
    auto GetParentId(UIContext& ctx) -> std::string;
    

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
    };

    direct_val<bool> sameLine = false;
    direct_val<int> nextColumn = 0;
    bindable<dimension> size_x = 0;
    bindable<dimension> size_y = 0;
    bindable<bool> visible = true;
    bindable<bool> disabled = false;
    bindable<std::string> tooltip = "";
    direct_val<std::string> contextMenu = "";
    direct_val<int> cursor = ImGuiMouseCursor_Arrow;
    direct_val<bool> tabStop = true;
    direct_val<bool> hasPos = false;
    direct_val<dimension> pos_x = 0;
    direct_val<dimension> pos_y = 0;
    direct_val<int> indent = 0;
    direct_val<int> spacing = 0;
    direct_val<bool> allowOverlap = false;
    data_loop itemCount;
    bindable<font_name> style_font;
    bindable<color32> style_text;
    bindable<color32> style_frameBg;
    bindable<color32> style_border;
    bindable<color32> style_button;
    bindable<color32> style_buttonHovered;
    bindable<color32> style_buttonActive;
    direct_val<pzdimension> style_frameRounding;
    direct_val<pzdimension2> style_framePadding;
    direct_val<pzdimension> style_frameBorderSize;
    event<> onItemClicked;
    event<> onItemDoubleClicked;
    event<> onItemHovered;
    event<> onItemFocused;
    event<> onItemActivated;
    event<> onItemDeactivated;
    event<> onItemDeactivatedAfterEdit;
    event<> onItemContextMenuClicked;
    std::string userCodeBefore, userCodeAfter;

    static std::unique_ptr<Widget> Create(const std::string& s, UIContext& ctx);

    void Draw(UIContext& ctx);
    void DrawExtra(UIContext& ctx);
    void Export(std::ostream& os, UIContext& ctx);
    void Import(cpp::stmt_iterator& sit, UIContext& ctx);
    auto Properties() -> std::vector<Prop>;
    auto Events() -> std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    void TreeUI(UIContext& ctx);
    bool EventUI(int, UIContext& ctx);
    int Behavior();
    Layout GetLayout(UINode* parent);
    virtual std::unique_ptr<Widget> Clone(UIContext& ctx) = 0;
    virtual ImDrawList* DoDraw(UIContext& ctx) = 0;
    virtual void DoDrawExtra(UIContext& ctx) {}
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
};

struct Separator : Widget
{
    bindable<std::string> label;
    direct_val<pzdimension> style_thickness;
    direct_val<bool> style_outer_padding = true;

    Separator(UIContext&);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int, UIContext& ctx);
    void DoExport(std::ostream&, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_MINUS; }
};

struct Text : Widget
{
    bindable<std::string> text = "text";
    direct_val<bool> alignToFrame = false;
    direct_val<bool> wrap = false;
    
    Text(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties() ->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    void CalcSizeEx(ImVec2 p1, UIContext& ctx);
    const char* GetIcon() const { return ICON_FA_FONT; }
};

struct Selectable : Widget
{
    bindable<std::string> label = "label";
    flags_helper flags = ImGuiSelectableFlags_DontClosePopups;
    direct_val<ImRad::Alignment> horizAlignment = ImRad::AlignLeft;
    direct_val<ImRad::Alignment> vertAlignment = ImRad::AlignTop;
    direct_val<bool> alignToFrame = false;
    direct_val<bool> readOnly = false;
    field_ref<bool> fieldName;
    bindable<bool> selected = false;
    bindable<color32> style_header;
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
    int Behavior() { return Widget::Behavior() | HasSizeX | HasSizeY; }
    const char* GetIcon() const { return ICON_FA_AUDIO_DESCRIPTION; }
};

struct Button : Widget
{
    bindable<std::string> label = "OK";
    direct_val<ImGuiDir> arrowDir = ImGuiDir_None;
    direct_val<bool> small = false;
    direct_val<ImRad::ModalResult> modalResult = ImRad::None;
    direct_val<shortcut_> shortcut = "";
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
    int Behavior() { return Widget::Behavior() | HasSizeX | HasSizeY; }
    const char* GetIcon() const { return ICON_FA_CIRCLE_PLAY; }
};

struct CheckBox : Widget
{
    bindable<std::string> label = "label";
    field_ref<bool> fieldName;
    bindable<color32> style_check;
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
};

struct RadioButton : Widget
{
    bindable<std::string> label = "label";
    direct_val<int> valueID = 0;
    bindable<color32> style_check;
    field_ref<int> fieldName;
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
};

struct Input : Widget
{
    field_ref<> fieldName;
    direct_val<std::string> label = "";
    direct_val<std::string> type = "std::string";
    bindable<std::string> hint = "";
    direct_val<int> imeType = ImRad::ImeText;
    direct_val<float> step = 1;
    direct_val<std::string> format = "%.3f";
    flags_helper flags = 0;
    direct_val<bool> initialFocus = false;
    field_ref<bool> forceFocus;
    event<> onChange;
    event<> onImeAction;
    event<int(ImGuiInputTextCallbackData&)> onCallback;

    static flags_helper _imeClass, _imeAction;
    
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
};

struct Combo : Widget
{
    direct_val<std::string> label = "";
    field_ref<std::string> fieldName;
    bindable<std::vector<std::string>> items;
    flags_helper flags = ImGuiComboFlags_None;
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
};

struct Slider : Widget
{
    direct_val<std::string> label = "";
    field_ref<> fieldName;
    direct_val<std::string> type = "float";
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
};

struct ProgressBar : Widget
{
    direct_val<bool> indicator = true;
    field_ref<float> fieldName;
    bindable<color32> style_color;

    ProgressBar(UIContext& ctx);
    auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
    ImDrawList* DoDraw(UIContext& ctx);
    auto Properties()->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    void DoExport(std::ostream& os, UIContext& ctx);
    void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
    int Behavior() { return Widget::Behavior() | HasSizeX | HasSizeY; }
    const char* GetIcon() const { return ICON_FA_BATTERY_HALF; }
};

struct ColorEdit : Widget
{
    field_ref<> fieldName;
    direct_val<std::string> label = "";
    direct_val<std::string> type = "color3";
    flags_helper flags = ImGuiColorEditFlags_None;
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
};

struct Image : Widget
{
    bindable<std::string> fileName = "";
    field_ref<ImRad::Texture> fieldName;
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
};
