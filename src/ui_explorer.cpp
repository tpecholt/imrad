#include "ui_explorer.h"
#include "ui_message_box.h"
#include "utils.h"
#include "stx.h"
#include "imrad.h"
#include "cppgen.h"
#include <sstream>
#include <chrono>
#include <IconsFontAwesome6.h>

struct ExplorerEntry
{
    std::string path;
    bool folder;
    bool generated;
    std::string fileName;
    std::time_t last_write_time;
    std::string modified;
};

std::string explorerPath;
int explorerFilter = 0;
ImGuiTableColumnSortSpecs explorerSorting;

static int pathSel = -1;
static std::vector<ExplorerEntry> data;
static std::vector<std::string> suggestions;
static int suggestionSel = -1;
static std::string lastPath;
static bool scrollBack = false;
static bool showSuggestions = false;
static bool autocompleted = false;
enum MoveFocus { None, PathInput, FirstEntry };
static MoveFocus moveFocus = None;

bool IsHeaderFile(const fs::path& p)
{
    auto ext = u8string(p.extension());
    return ext == ".h" || ext == ".hpp" || ext == ".hxx";
}

bool IsCppFile(const fs::path& p)
{
    auto ext = u8string(p.extension());
    return ext == ".c" || ext == ".cpp" || ext == ".cxx";
}

void ReloadExplorer(const CppGen& codeGen)
{
    data.clear();
    auto path = u8path(explorerPath);
    //+= '/' so it works with paths like "c:"
    if (!path.has_root_directory())
        path = u8path(explorerPath + (char)fs::path::preferred_separator);
    path.make_preferred();
    explorerPath = u8string(path);

    if (!fs::is_directory(path)) {
        messageBox.title = "Error";
        messageBox.message = "Can't open \"" + explorerPath + "\"";
        messageBox.buttons = ImRad::Ok;
        messageBox.OpenPopup();
        return;
    }

    scrollBack = true;
    std::ostringstream os;
    auto fsnow = fs::file_time_type::clock::now();
    auto snow = std::chrono::system_clock::now();
    os.imbue(std::locale(""));
    std::error_code ec;
    for (fs::directory_iterator it(path, ec); it != fs::directory_iterator(); ++it)
    {
        if (u8string(it->path().stem())[0] == '.')
            continue;
        if (!it->is_directory() &&
            (!explorerFilter && !IsHeaderFile(it->path())))
            continue;
        ExplorerEntry entry;
        entry.path = u8string(it->path());
        entry.folder = it->is_directory();
        entry.generated = (IsHeaderFile(it->path()) || IsCppFile(it->path())) &&
            codeGen.ReadGenVersion(entry.path);
        entry.fileName = u8string(it->path().filename());
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            it->last_write_time() - fsnow + snow);
        entry.last_write_time = std::chrono::system_clock::to_time_t(sctp);
        auto* tm = std::localtime(&entry.last_write_time);
        os.str("");
        os << std::put_time(tm, "%x %X");
        entry.modified = os.str();
        data.push_back(std::move(entry));
    }
    stx::sort(data, [](const ExplorerEntry& a, const ExplorerEntry& b) {
        if (a.folder != b.folder)
            return a.folder;
        if (!explorerSorting.ColumnIndex) {
            if (explorerSorting.SortDirection == ImGuiSortDirection_Ascending)
                return path_cmp(a.fileName, b.fileName);
            else
                return path_cmp(b.fileName, a.fileName);
        }
        else {
            if (explorerSorting.SortDirection == ImGuiSortDirection_Ascending)
                return a.last_write_time < b.last_write_time;
            else
                return a.last_write_time > b.last_write_time;
        }
        });
    if (!u8path(explorerPath).relative_path().empty()) {
        std::string ppath = u8string(u8path(explorerPath).parent_path());
        ExplorerEntry e;
        e.path = ppath;
        e.folder = true;
        e.fileName = "..";
        data.insert(data.begin(), e);
    }
}

void GetSuggestions()
{
    suggestions.clear();
    std::error_code ec;
    auto path = u8path(explorerPath);
    //+= '/' so it works with paths like "c:"
    if (!path.has_root_directory())
        path = u8path(explorerPath + (char)fs::path::preferred_separator);
    std::string match = u8string(path.stem());
    for (fs::directory_iterator it(path.parent_path(), ec); it != fs::directory_iterator(); ++it)
    {
        std::string stem = u8string(it->path().stem());
        if (!it->is_directory() || stem[0] == '.')
            continue;
        if (stem.size() > match.size())
            stem.resize(match.size());
        if (path_cmp(stem, match) || path_cmp(match, stem))
            continue;
        std::string fileName = u8string(it->path().filename());
        suggestions.push_back(std::move(fileName));
    }
    stx::sort(suggestions, [](const std::string& a, const std::string& b) {
        return path_cmp(a, b);
        });
}

int ExplorerPathCallback(ImGuiInputTextCallbackData* data)
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
        return DefaultCharFilter(data);
    }
    else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
    {
        if (data->EventKey == ImGuiKey_UpArrow && suggestions.size()) {
            if (suggestionSel < 0) {
                suggestionSel = (int)suggestions.size() - 1;
                lastPath = explorerPath;
            }
            else if (--suggestionSel < 0)
                suggestionSel = -1;
        }
        else if (data->EventKey == ImGuiKey_DownArrow && suggestions.size()) {
            if (suggestionSel < 0) {
                suggestionSel = 0;
                lastPath = explorerPath;
            }
            else if (++suggestionSel == suggestions.size())
                suggestionSel = -1;
        }

        //commit suggestion
        std::string str = lastPath;
        if (suggestionSel >= 0) {
            size_t i = explorerPath.find_last_of("/\\");
            if (i == std::string::npos)
                str = explorerPath + (char)fs::path::preferred_separator;
            else
                str = explorerPath.substr(0, i + 1);
            str += suggestions[suggestionSel];
        }
        if (str.size() + 1 < data->BufSize) { //no reallocation occured
            autocompleted = true;
            data->BufDirty = true;
            strcpy(data->Buf, str.c_str());
            data->BufTextLen = (int)str.size();
            data->SelectionStart = data->SelectionEnd = data->CursorPos = data->BufTextLen;
        }
    }
    return 0;
}

void ExplorerUI(const CppGen& codeGen, std::function<void(const std::string& fpath)> openFileFunc)
{
    explorerPath.reserve(1024); //ImGuiInputText_HistoryCallback doesn't allow buffer reallocation??
    if (explorerPath == "")
        explorerPath = u8string(fs::current_path());

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 4, 4 });
    ImGui::Begin("Explorer");
    if (ImGui::IsWindowAppearing()) {
        ReloadExplorer(codeGen);
        showSuggestions = false;
        autocompleted = false;
        moveFocus = FirstEntry;
    }

    //clickable path box
    const int sp = 4;
    const std::string ELIDE_STR = "...";
    std::string inputId = "##explorerCwd";
    if (ImGui::GetFocusID() != ImGui::GetID(inputId.c_str()))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImGui::GetStyle().FramePadding);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, ImGui::GetStyle().FrameRounding);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xffd0d0d0);
        ImGui::PushStyleColor(ImGuiCol_Border, 0xffb0b0b0);
        if (ImGui::BeginChild("cwdChild", { -ImGui::GetFrameHeight() - sp, ImGui::GetFrameHeight() }, ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_NoScrollbar))
        {
            fs::path path = u8path(explorerPath);
            char separator = (char)fs::path::preferred_separator;
            int elideStart = (int)path.has_root_name() + (int)path.has_root_directory();
            int elideN = 0;
            ImVec2 wpad = ImGui::GetStyle().WindowPadding;
            int n = 0;
            for (const auto& p : path)
                ++n;
            for (; elideStart + elideN < n; ++elideN) {
                std::string tmp;
                int i = 0;
                for (const auto& p : path) {
                    if (i < elideStart)
                        tmp += u8string(p);
                    else if (i == elideStart && elideN)
                        tmp += ELIDE_STR + separator;
                    else if (i >= elideStart + elideN)
                        tmp += u8string(p) + separator;
                    ++i;
                }
                if (tmp.back() == separator)
                    tmp.pop_back();
                if (elideStart + elideN + 1 == n ||
                    ImGui::CalcTextSize(tmp.c_str()).x + 2 * wpad.x < ImGui::GetWindowWidth())
                    break;
            }
            int i = 0;
            int rootDirIdx = path.has_root_directory() ? (int)path.has_root_name() : -1;
            int lastPathSel = pathSel;
            pathSel = -1;
            bool sepSel = false;
            fs::path subpath;
            for (const auto& p : path) {
                subpath /= p;
                if (i >= elideStart && i < elideStart + elideN) {
                    ++i;
                    continue;
                }
                if (p.has_root_directory()) {
                    ImGui::SameLine(0, 0);
                    ImGui::Text("%s", u8string(p).c_str());
                    if (ImGui::IsItemHovered())
                        sepSel = true;
                    ++i;
                    continue;
                }
                if (i && i != rootDirIdx + 1) {
                    ImGui::SameLine(0, 0);
                    std::string sep = elideN && i == elideStart + elideN ? ELIDE_STR : "";
                    sep += separator;
                    ImGui::Text("%s", sep.c_str());
                    if (ImGui::IsItemHovered())
                        sepSel = true;
                }
                ImGui::SameLine(0, 0);
                ImGui::PushStyleColor(ImGuiCol_Text, i == lastPathSel ? 0xff0000ff : 0xff000000);
                ImGui::Text("%s", u8string(p).c_str());
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered() && i + 1 != n)
                    pathSel = i;
                if (ImGui::IsItemClicked() && i + 1 != n) {
                    explorerPath = u8string(subpath);
                    ReloadExplorer(codeGen);
                    moveFocus = FirstEntry;
                }
                ++i;
            }
            if (ImGui::IsWindowHovered() && !sepSel && pathSel < 0)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    moveFocus = PathInput;
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    ImGui::SameLine(0, 0);
    if (ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() - sp <= 0)
        ImGui::SetNextItemWidth(0);
    else
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() - sp);
    //ImGui::PushStyleColor(ImGuiCol_FrameBg, 0xffd0d0d0);
    //ImGui::PushStyleColor(ImGuiCol_Border, 0xffb0b0b0);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
    int fl = ImGuiInputTextFlags_ElideLeft | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCharFilter;
    if (ImGui::InputText(inputId.c_str(), &explorerPath, fl, ExplorerPathCallback)) {
        if (autocompleted)
            autocompleted = false;
        else {
            GetSuggestions();
            showSuggestions = true;
            suggestionSel = -1;
        }
    }
    //ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
    if (moveFocus == PathInput)
    {
        ImGui::SetKeyboardFocusHere(-1);
        auto* state = ImGui::GetInputTextState(ImGui::GetItemID());
        if (state)
            state->ClearSelection();
        if (ImGui::IsItemFocused())
            moveFocus = None;
    }
    if (ImGui::IsItemDeactivated()) { //AfterEdit())
        ReloadExplorer(codeGen);
        showSuggestions = false;
        moveFocus = FirstEntry;
    }
    if (showSuggestions)
    {
        ImGui::PushFont(ImRad::GetFontByName("imrad.explorer"));
        ImGui::SetNextWindowPos({ ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y });
        float h = ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemInnerSpacing.y;
        float pad = ImGui::GetStyle().WindowPadding.y;
        const int N = 7;
        ImGui::SetNextWindowSize({ ImGui::GetItemRectSize().x, std::min(N, (int)suggestions.size()) * h + 2 * pad });
        float selY = std::max(suggestionSel, 0) * h + pad;
        if (ImGui::BeginTooltip()) //Tooltip won't steal the focus as BeginPopup does
        {
            if (selY + h > ImGui::GetScrollY() + ImGui::GetWindowSize().y)
                ImGui::SetScrollY(selY - (N - 1) * h - pad);
            if (selY < ImGui::GetScrollY())
                ImGui::SetScrollY(selY - pad);
            for (size_t i = 0; i < suggestions.size(); ++i) {
                /*ImGui::PushStyleColor(ImGuiCol_Text, 0xff40b0b0);
                ImGui::Selectable(ICON_FA_FOLDER, i == suggestionSel, ImGuiSelectableFlags_SpanAvailWidth);
                ImGui::PopStyleColor();
                ImGui::SameLine(0, 4);*/
                ImGui::Selectable(suggestions[i].c_str(), i == suggestionSel, 0);
            }
            ImGui::EndTooltip();
        }
        ImGui::PopFont();
    }

    ImGui::SameLine(0, sp);
    ImGui::PushStyleColor(ImGuiCol_Text, 0xff404040);
    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
    if (ImGui::Button(ICON_FA_ROTATE_RIGHT "##ego")) {
        ReloadExplorer(codeGen);
        moveFocus = FirstEntry;
    }
    ImGui::PopItemFlag();
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_TableRowBg, 0xffffffff);
    ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, 0xffffffff);
    if (scrollBack) {
        scrollBack = false;
        ImGui::SetNextWindowScroll({ 0, 0 });
    }
    float h = -ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.y;
    if (ImGui::BeginTable("files", 2, ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, { -1, h }))
    {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthFixed, 140);
        ImGui::TableSetupScrollFreeze(0, 1);
        auto* spec = ImGui::TableGetSortSpecs();
        if (spec && spec->SpecsDirty) {
            spec->SpecsDirty = false;
            explorerSorting = *spec->Specs;
            ReloadExplorer(codeGen);
        }
        ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
        ImGui::TableHeadersRow();
        ImGui::PopItemFlag();
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        ImGui::PushFont(ImRad::GetFontByName("imrad.explorer"));
        int n = 0;
        for (const auto& entry : data)
        {
            ImGui::PushID(n++);
            bool source = IsHeaderFile(entry.path) || IsCppFile(entry.path);
            ImGui::PushStyleColor(ImGuiCol_Text,
                entry.folder ? 0xff40b0b0 :
                entry.generated ? 0xffe0a060 :
                source ? 0xff60a0d0 :
                0xffa0a0a0
            );
            ImGui::Selectable(entry.folder ? ICON_FA_FOLDER :
                entry.generated ? ICON_FA_DICE_D6 :
                source ? ICON_FA_FILE_CODE :
                ICON_FA_FILE,
                false, ImGuiSelectableFlags_SpanAllColumns);
            ImGui::PopStyleColor();
            if (n == 1 && moveFocus == FirstEntry) {
                if (ImGui::IsItemFocused())
                    moveFocus = None;
                ImGui::SetKeyboardFocusHere(-1);
            }
            if (ImRad::IsItemDoubleClicked() ||
                (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
            {
                if (entry.folder) {
                    explorerPath = entry.path;
                    ReloadExplorer(codeGen);
                    ImGui::PopID();
                    break;
                }
                else if (!codeGen.ReadGenVersion(entry.path)) {
                    ShellExec(entry.path);
                }
                else {
                    openFileFunc(entry.path);
                }
            }
            ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
            ImGui::SameLine(0, 4);
            ImGui::Selectable(entry.fileName.c_str(), false);
            ImGui::TableNextColumn();
            ImGui::Selectable(entry.modified.c_str(), false, ImGuiSelectableFlags_Disabled);
            ImGui::TableNextColumn();
            ImGui::PopItemFlag();
            ImGui::PopID();
        }
        ImGui::PopFont();
        ImGui::EndTable();
    }
    ImGui::PopStyleColor(2);

    ImGui::PushStyleColor(ImGuiCol_FrameBg, 0xffd0d0d0);
    ImGui::PushStyleColor(ImGuiCol_Border, 0xffb0b0b0);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##filter", &explorerFilter, "H Files (*.h,*.hpp,*.hxx)\0All Files (*.*)\0"))
        ReloadExplorer(codeGen);
    ImGui::PopStyleColor(2);

    ImGui::End();
    ImGui::PopStyleVar();
}
