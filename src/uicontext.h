#pragma once
#include <vector>
#include <array>
#include <string>
#include <imgui.h>

struct UINode;
struct Widget;
class CppGen;
struct property_base;
struct ImGuiWindow;

struct UIContext
{
    //set from outside
    enum Mode {
        NormalSelection, RectSelection, SnapInsert, SnapMove, PickPoint,
        ItemDragging,
        ItemSizingLeft = 0x100, ItemSizingRight = 0x200,
        ItemSizingTop = 0x400, ItemSizingBottom = 0x800,
        ItemSizingMask = ItemSizingLeft | ItemSizingRight | ItemSizingTop | ItemSizingBottom,
    };
    Mode mode = NormalSelection;
    std::vector<UINode*> selected;
    CppGen* codeGen = nullptr;
    ImVec2 designAreaMin, designAreaMax; //ImRect is internal?
    std::string workingDir;
    enum Color { Hovered, Selected, Snap1, Snap2, Snap3, Snap4, Snap5, COUNT };
    std::array<ImU32, Color::COUNT> colors;
    std::vector<std::string> fontNames;
    ImFont* defaultStyleFont = nullptr;
    ImFont* pgFont = nullptr;
    ImFont* pgbFont = nullptr;
    std::string unit; //for dimension export
    bool createVars = true; //create variables etc. during contructor/Clone calls
    ImGuiStyle style;
    const ImGuiStyle* appStyle = nullptr;
    const property_base* setProp = nullptr;
    std::string setPropValue;
    ImTextureID dashTexId = 0;
    bool* modified = nullptr;

    //snap result
    UINode* snapParent = nullptr;
    size_t snapIndex;
    int snapNextColumn;
    bool snapSameLine;
    bool snapUseNextSpacing;
    bool snapSetNextSameLine;

    //recursive info
    int importState = 0; //0 - no import, 1 - within begin/end/separator, 2 - user code import
    UINode* hovered = nullptr;
    UINode* dragged = nullptr;
    ImVec2 lastSize;
    int importVersion;
    int importLevel;
    std::string userCode;
    UINode* root = nullptr;
    ImGuiWindow* rootWin = nullptr;
    bool isAutoSize;
    ImU32 layoutHash = 0, prevLayoutHash = 0;
    ImU32 prevDockspaceHash = 0;
    bool beingResized = false;
    std::vector<ImGuiWindow*> activePopups;
    std::vector<UINode*> parents;
    std::vector<std::string> contextMenus;
    int kind = 0; //TopWindow::Kind
    float zoomFactor = 1; //for dimension value scaling
    ImVec2 selStart, selEnd;
    std::string ind;
    int varCounter;
    std::string parentVarName;
    std::vector<std::string> errors;
    ImVec2 stretchSize;
    std::array<std::string, 2> stretchSizeExpr;

    //convenience
    void ind_up();
    void ind_down();

    static UIContext& Defaults();
};
