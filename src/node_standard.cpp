#include "node_standard.h"
#include "node_container.h"
#include "node_extra.h"
#include "stx.h"
#include "cppgen.h"
#include "binding_input.h"
#include "binding_eval.h"
#include "ui_message_box.h"
#include "ui_combo_dlg.h"
#include <misc/cpp/imgui_stdlib.h>
#include <nfd.h>
#include <algorithm>
#include <array>

const std::string TMP_LAST_ITEM_VAR = "tmpLastItem";

void toggle(std::vector<UINode*>& c, UINode* val)
{
    auto it = stx::find(c, val);
    if (it == c.end())
        c.push_back(val);
    else
        c.erase(it);
}

// for compatibility with ImRAD 0.7
std::string ParseShortcutOld(const std::string& line)
{
    std::string sh;
    size_t i = -1;
    while (true)
    {
        size_t j1 = line.find("ImGui::IsKeyPressed(ImGuiKey_", i + 1);
        size_t j2 = line.find("ImGuiMod_", i + 1);
        if (j1 == std::string::npos && j2 == std::string::npos)
            break;
        if (j1 < j2) {
            j1 += 29;
            size_t e = line.find_first_of(",)", j1);
            if (e == std::string::npos)
                break;
            sh += "+";
            sh += line.substr(j1, e - j1);
            i = j1;
        }
        else
        {
            j2 += 9;
            sh += "+";
            size_t end = std::find_if(line.begin() + j2, line.end(), [](char c) {
                return !std::isalpha(c);
                }) - line.begin();
                sh += line.substr(j2, end - j2);
                i = j2;
        }
    }
    if (sh.size())
        sh.erase(sh.begin());
    return sh;
}

void TreeNodeProp(const char* name, ImFont* font, const std::string& label, std::function<void()> f)
{
    ImVec2 pad = ImGui::GetStyle().FramePadding;
    ImGui::Unindent();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, pad.y });
    ImGui::PushStyleColor(ImGuiCol_NavCursor, { 0, 0, 0, 0 });
    ImGui::SetNextItemAllowOverlap();
    if (ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_SpanAllColumns)) {
        ImGui::PopStyleVar();
        ImGui::Indent();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { pad.x, 0 }); //row packing
        //ImGui::TableNextColumn();
        //ImGui::Spacing();
        f();
        ImGui::PopStyleVar();
        ImGui::TreePop();
        ImGui::Unindent();
    }
    else
    {
        ImGui::PopStyleVar();
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        ImGui::PushFont(font);
        ImGui::Text("%s", label.c_str());
        ImGui::PopFont();
    }
    ImGui::Indent();
    ImGui::PopStyleColor();
}

//1. limits length of lengthy {} expression (like in case it contains ?:)
//2. formats {{, }} into {, }
//3. errors out on incorrect format string
PreparedString PrepareString(std::string_view s)
{
    const int n = 25;
    PreparedString ps;
    ps.error = false;
    ps.pos = ImGui::GetCursorScreenPos();
    ps.pos.y += ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset;
    ps.label.reserve(s.size() + 10);
    size_t argFrom = 0;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '{') {
            ps.label += "{";
            if (i + 1 < s.size() && s[i + 1] == '{')
                ++i;
            else {
                if (argFrom) {
                    ps.error = true;
                    break;
                }
                argFrom = i + 1;
            }
        }
        else if (s[i] == '}') {
            if (!argFrom) {
                if (i + 1 == s.size() || s[i + 1] != '}') {
                    ps.error = true;
                    break;
                }
                ps.label += "}";
                ++i;
            }
            else {
                if (i == argFrom) {
                    ps.error = true;
                    break;
                }
                if (i - argFrom > n) {
                    ps.label += s.substr(argFrom, n - 3);
                    while (ps.label.back() < 0) //strip incomplete unicode char
                        ps.label.pop_back();
                    ps.label += "...}";
                }
                else {
                    ps.label += s.substr(argFrom, i - argFrom);
                    ps.label += "}";
                }
                ps.fmtArgs.push_back({ argFrom - 1, ps.label.size() });
                argFrom = 0;
            }
        }
        else if (!argFrom) {
            ps.label += s[i];
        }
    }
    if (argFrom)
        ps.error = true;

    if (ps.error) {
        ps.label = "error";
        ps.fmtArgs.clear();
    }
    return ps;
}

ImVec2 IncWrapText(const ImVec2& dpos, const char* s, const char* text_end, float wrap_width, float scale)
{
    ImFont* font = ImGui::GetFont();
    const float line_height = font->FontSize * scale;
    float line_width = dpos.x;
    float text_height = dpos.y;
    const char* word_wrap_eol = NULL;
    const char* real_end = s + strlen(s);
    while (s < text_end)
    {
        // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
        if (!word_wrap_eol)
        {
            word_wrap_eol = font->CalcWordWrapPositionA(scale, s, real_end, wrap_width - line_width);
        }

        if (s >= word_wrap_eol)
        {
            text_height += line_height;
            line_width = 0.0f;
            word_wrap_eol = NULL;
            //Wrapping skips upcoming blanks
            while (s < text_end && ImCharIsBlankA(*s))
                s++;
            if (*s == '\n')
                s++;
            continue;
        }

        // Decode and advance source
        const char* prev_s = s;
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
            s += 1;
        else
            s += ImTextCharFromUtf8(&c, s, text_end);

        if (c < 32)
        {
            if (c == '\n')
            {
                text_height += line_height;
                line_width = 0.0f;
                continue;
            }
            if (c == '\r')
                continue;
        }

        const float char_width = scale * ((int)c < font->IndexAdvanceX.Size ? font->IndexAdvanceX.Data[c] : font->FallbackAdvanceX);
        /*if (line_width + char_width >= max_width)
        {
        s = prev_s;
        break;
        }*/

        line_width += char_width;
    }

    if (s == word_wrap_eol)
    {
        text_height += line_height;
        line_width = 0.0f;
    }

    return { line_width, text_height };
}

void DrawTextArgs(const PreparedString& ps, UIContext& ctx, const ImVec2& offset, const ImVec2& size, const ImVec2& align)
{
    if (ctx.beingResized)
        return;

    ImVec2 pos = ps.pos;
    uint32_t clr = ctx.colors[UIContext::Color::Selected];
    clr = (clr & 0x00ffffff) | 0xb0000000;
    float wrapPos = ImGui::GetCurrentWindow()->DC.TextWrapPos;

    if (wrapPos < 0 || size.x || size.y || offset.x || offset.y)
    {
        if (align.x || align.y) {
            ImVec2 textSize = ImGui::CalcTextSize(ps.label.data(), ps.label.data() + ps.label.size());
            ImVec2 sz = ImGui::CalcItemSize(size, textSize.x, textSize.y);
            ImVec2 dp{ (sz.x - textSize.x) * align.x, (sz.y - textSize.y) * align.y };
            pos += dp;
        }

        pos += offset;
        size_t i = 0;
        for (const auto& arg : ps.fmtArgs) {
            pos.x += ImGui::CalcTextSize(ps.label.data() + i, ps.label.data() + arg.first).x;
            ImGui::GetWindowDrawList()->AddText(pos, clr, "{");
            //ImVec2 sz = ImGui::CalcTextSize(ps.label.data() + arg.first, ps.label.data() + arg.second);
            //ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + sz, 0x50808080);
            ImVec2 sz = ImGui::CalcTextSize(ps.label.data() + arg.first, ps.label.data() + arg.second - 1);
            pos.x += sz.x;
            ImGui::GetWindowDrawList()->AddText(pos, clr, "}");
            i = arg.second - 1;
        }

        if (ps.error)
            ImGui::GetWindowDrawList()->AddText(pos, clr, ps.label.c_str());
    }
    else
    {
        float wrapWidth = ImGui::CalcWrapWidthForPos(ps.pos, wrapPos);
        const char* text = ps.label.data();
        ImVec2 dp{ 0, 0 };
        for (const auto& arg : ps.fmtArgs)
        {
            const char* text_end = ps.label.data() + arg.first;
            dp = IncWrapText(dp, text, text_end, wrapWidth, ctx.zoomFactor);
            ImGui::GetWindowDrawList()->AddText(pos + dp, clr, "{");
            text = text_end;
            text_end = ps.label.data() + arg.second - 1;
            dp = IncWrapText(dp, text, text_end, wrapWidth, ctx.zoomFactor);
            ImGui::GetWindowDrawList()->AddText(pos + dp, clr, "}");
            text = text_end;
        }

        if (ps.error)
            ImGui::GetWindowDrawList()->AddText(pos, clr, ps.label.c_str());
    }
}

//----------------------------------------------------

UINode::child_iterator::iter::iter()
    : children(), idx()
{}

UINode::child_iterator::iter::iter(children_type& ch, bool freePos)
    : children(&ch), freePos(freePos), idx()
{
    while (!end() && !valid())
        ++idx;
}

UINode::child_iterator::iter&
UINode::child_iterator::iter::operator++ () {
    if (end())
        return *this;
    ++idx;
    while (!end() && !valid())
        ++idx;
    return *this;
}

UINode::child_iterator::iter
UINode::child_iterator::iter::operator++ (int) {
    iter it(*this);
    ++(*this);
    return it;
}

bool UINode::child_iterator::iter::operator== (const iter& it) const
{
    if (end() != it.end())
        return false;
    if (!end())
        return idx == it.idx;
    return true;
}

bool UINode::child_iterator::iter::operator!= (const iter& it) const
{
    return !(*this == it);
}

UINode::child_iterator::children_type::value_type&
UINode::child_iterator::iter::operator* ()
{
    static children_type::value_type dummy;
    if (end())
        return dummy;
    return children->at(idx);
}

const UINode::child_iterator::children_type::value_type&
UINode::child_iterator::iter::operator* () const
{
    static children_type::value_type dummy;
    if (end())
        return dummy;
    return children->at(idx);
}

size_t UINode::child_iterator::iter::index() const
{
    return idx;
}

bool UINode::child_iterator::iter::end() const
{
    return !children || idx >= children->size();
}

bool UINode::child_iterator::iter::valid() const
{
    if (end())
        return false;
    bool fp = children->at(idx)->hasPos;
    return freePos == fp;
}

UINode::child_iterator::child_iterator(children_type& children, bool freePos)
    : children(children), freePos(freePos)
{}

UINode::child_iterator::iter
UINode::child_iterator::begin() const
{
    return iter(children, freePos);
}

UINode::child_iterator::iter
UINode::child_iterator::end() const
{
    return iter();
}

UINode::child_iterator::operator bool() const
{
    return begin() != end();
}

//--------------------------------------------------------------------

void UINode::CloneChildrenFrom(const UINode& node, UIContext& ctx)
{
    children.resize(node.children.size());
    for (size_t i = 0; i < node.children.size(); ++i)
        children[i] = node.children[i]->Clone(ctx);
}

void UINode::DrawInteriorRect(UIContext& ctx)
{
    size_t level = ctx.parents.size() - 1;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    int snapCount = UIContext::Color::COUNT - UIContext::Color::Snap1;
    dl->AddRect(cached_pos, cached_pos + cached_size, ctx.colors[UIContext::Snap1 + level % snapCount], 0, 0, 3);
}

void UINode::DrawSnap(UIContext& ctx)
{
    ctx.snapNextColumn = 0;
    ctx.snapSameLine = false;
    ctx.snapUseNextSpacing = false;
    ctx.snapSetNextSameLine = false;

    const float MARGIN = 7;
    assert(ctx.parents.back() == this);
    size_t level = ctx.parents.size() - 1;
    int snapOp = Behavior();
    ImVec2 m = ImGui::GetMousePos();
    ImVec2 d1 = m - cached_pos;
    ImVec2 d2 = cached_pos + cached_size - m;
    float mind = std::min({ d1.x, d1.y, d2.x, d2.y });

    //snap interior (first child)
    if ((snapOp & (SnapInterior | SnapItemInterior)) &&
        mind >= 3 && //allow snapping sides with zero border
        !stx::count_if(children, [](const auto& ch) { return ch->Behavior() & SnapSides; }))
    {
        ctx.snapParent = this;
        child_iterator it(children, true);
        if (it)
            ctx.snapIndex = it.begin().index();
        else
            ctx.snapIndex = children.size();
        DrawInteriorRect(ctx);
        return;
    }

    if (!level || !(snapOp & SnapSides))
        return;

    //snap side
    UINode* parent = ctx.parents[ctx.parents.size() - 2];
    const auto& pchildren = parent->children;
    size_t i = stx::find_if(pchildren, [&](const auto& ch) {
        return ch.get() == this;
        }) - pchildren.begin();
    if (i == pchildren.size())
        return;
    UINode* clip = parent;
    if (clip->Behavior() & SnapGrandparentClip)
        clip = ctx.parents[ctx.parents.size() - 3];
    if (m.x < clip->cached_pos.x ||
        m.y < clip->cached_pos.y ||
        m.x > clip->cached_pos.x + clip->cached_size.x ||
        m.y > clip->cached_pos.y + clip->cached_size.y)
        return;

    int ncols = parent->ColumnCount(ctx);
    int col = 0;
    if (ncols > 1)
    {
        for (size_t j = 0; j <= i; ++j)
            col = (col + pchildren[j]->nextColumn) % ncols;
    }
    bool lastItem = i + 1 == pchildren.size();
    ImGuiDir snapDir = ImGuiDir_None;
    //allow to snap in space between widgets too (don't check all coordinates)
    //works well with parent clip
    if (mind > MARGIN)
        snapDir = ImGuiDir_None;
    else if (d1.x == mind && d1.y >= 0 && d2.y >= 0)
        snapDir = ImGuiDir_Left;
    else if (d2.x == mind && d1.y >= 0 && d2.y >= 0)
        snapDir = ImGuiDir_Right;
    else if (d1.y == mind && d1.x >= 0 && d2.x >= 0)
        snapDir = ImGuiDir_Up;
    else if (d2.y == mind && d1.x >= 0 && d2.x >= 0)
        snapDir = ImGuiDir_Down;
    else if (lastItem && d2.x < 0 && d2.y < 0)
        snapDir = ImGuiDir_Down;

    if (snapDir == ImGuiDir_None)
    {
        if (ImRect(cached_pos, cached_pos + cached_size).Contains(m))
        {
            //end of search with no result (interpreted by TopWindow)
            //children were already snapped so we can safely end search here
            ctx.snapParent = parent;
            ctx.snapIndex = -1;
        }
        return;
    }

    ImVec2 p;
    float w = 0, h = 0;
    //snapRight and snapDown will extend the checked area to the next widget
    switch (snapDir)
    {
    case ImGuiDir_Left:
    {
        p = cached_pos;
        h = cached_size.y;
        auto& ch = pchildren[i];
        //check we are not pointing into widget on left
        const auto* lch = i && (pchildren[i - 1]->Behavior() & SnapSides) ? pchildren[i - 1].get() : nullptr;
        if (ncols > 1 && ch->nextColumn) {
            if (m.x < cached_pos.x)
                return;
        }
        if (ch->sameLine && lch) {
            float xm = (p.x + lch->cached_pos.x + lch->cached_size.x) / 2.f;
            //avg marker
            if (ch->spacing <= 1) {
                p.x = xm;
                h = std::max(h, lch->cached_size.y);
            }
            if (m.x < xm)
                return;
        }
        ctx.snapParent = parent;
        ctx.snapIndex = i;
        ctx.snapNextColumn = pchildren[i]->nextColumn;
        ctx.snapSameLine = pchildren[i]->sameLine;
        ctx.snapUseNextSpacing = true;
        ctx.snapSetNextSameLine = true;
        break;
    }
    case ImGuiDir_Right:
    {
        p = cached_pos + ImVec2(cached_size.x, 0);
        h = cached_size.y;
        //check we are not pointing into widget on right
        const Widget* rch = nullptr;
        if (i + 1 < pchildren.size() && !pchildren[i + 1]->nextColumn && pchildren[i + 1]->sameLine)
            rch = pchildren[i + 1].get();
        if (rch) {
            float xm = (p.x + rch->cached_pos.x) / 2.f;
            //avg marker
            if (rch->spacing <= 1) {
                p.x = xm;
                h = std::max(h, rch->cached_size.y);
            }
            if (m.x > xm)
                return;
        }
        if (!rch && ncols > 1)
        {
            for (size_t j = i + 1; j < pchildren.size(); ++j)
            {
                if (pchildren[j]->nextColumn) {
                    if ((col + pchildren[j]->nextColumn) % ncols > col)
                        rch = pchildren[j].get();
                    break;
                }
            }
            if (rch && m.x >= rch->cached_pos.x)
                return;
        }
        const auto* nch = i + 1 < pchildren.size() ? pchildren[i + 1].get() : nullptr;
        ctx.snapParent = parent;
        ctx.snapIndex = i + 1;
        ctx.snapNextColumn = 0;
        ctx.snapSameLine = true;
        ctx.snapUseNextSpacing = false;
        break;
    }
    case ImGuiDir_Up:
    case ImGuiDir_Down:
    {
        bool down = snapDir == ImGuiDir_Down;
        p = cached_pos;
        if (down)
            p.y += cached_size.y;
        float x2 = p.x + cached_size.x;
        //find range of widgets in the same row and column
        size_t i1 = i, i2 = i;
        for (int j = (int)i - 1; j >= 0; --j)
        {
            if (ncols > 1 && pchildren[j + 1]->nextColumn)
                break;
            if (!pchildren[j + 1]->sameLine)
                break;
            const auto& ch = pchildren[j];
            i1 = j;
            p.x = ch->cached_pos.x;
            if (down)
                p.y = std::max(p.y, ch->cached_pos.y + ch->cached_size.y);
        }
        for (size_t j = i + 1; j < pchildren.size(); ++j)
        {
            const auto& ch = pchildren[j];
            if (ncols > 1 && ch->nextColumn)
                break;
            if (!ch->sameLine)
                break;
            i2 = j;
            x2 = ch->cached_pos.x + ch->cached_size.x;
            if (down)
                p.y = std::max(p.y, ch->cached_pos.y + ch->cached_size.y);
        }
        //find a widget from next/prev column-row
        size_t inr = -1, ipr = -1;
        if (ncols >= 2)
        {
            int nc = ncols - col;
            for (size_t j = i + 1; j < pchildren.size(); ++j)
            {
                nc -= pchildren[j]->nextColumn;
                if (nc <= 0) {
                    inr = j;
                    break;
                }
            }
            nc = ncols;
            for (int j = (int)i; j >= 0; --j)
            {
                if (nc <= 0) {
                    ipr = j;
                    break;
                }
                nc -= pchildren[j]->nextColumn;
            }
        }
        w = x2 - p.x;
        if (down)
        {
            //check m.y not pointing in next column-row
            const Widget* nch = inr < pchildren.size() ? pchildren[inr].get() : nullptr;
            if (nch)
            {
                if (m.y >= nch->cached_pos.y)
                    return;
            }
            //check m.y not pointing in next row
            nch = i2 + 1 < pchildren.size() ? pchildren[i2 + 1].get() : nullptr;
            if (nch && (ncols <= 1 || !nch->nextColumn))
            {
                if (m.y >= (p.y + nch->cached_pos.y) / 2.f)
                    return;
            }
            ctx.snapParent = parent;
            ctx.snapIndex = i2 + 1;
            ctx.snapNextColumn = 0;
            ctx.snapSameLine = false;
            ctx.snapUseNextSpacing = false;
        }
        else
        {
            //check m.y is not pointing in previous column-row
            const Widget* pch = ipr < pchildren.size() ? pchildren[ipr].get() : nullptr;
            if (pch)
            {
                if (m.y <= pch->cached_pos.y + pch->cached_size.y)
                    return;
            }
            //check m.y not pointing in prev row
            pch = i1 > 0 ? pchildren[i1 - 1].get() : nullptr;
            if (pch && (ncols <= 1 || !pchildren[i1]->nextColumn))
            {
                if (m.y <= pch->cached_pos.y + pch->cached_size.y)
                    return;
            }
            ctx.snapParent = parent;
            ctx.snapIndex = i1;
            ctx.snapSameLine = false;
            ctx.snapNextColumn = pchildren[i1]->nextColumn;
            ctx.snapUseNextSpacing = true;
        }
        break;
    }
    default:
        return;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddLine(p, p + ImVec2(w, h), ctx.colors[UIContext::Snap1 + (level - 1)], 3);
}

std::optional<std::pair<UINode*, int>>
UINode::FindChild(const UINode* ch)
{
    if (ch == this)
        return std::pair{ nullptr, 0 };
    for (size_t i = 0; i < children.size(); ++i) {
        const auto& child = children[i];
        if (child.get() == ch)
            return std::pair{ this, (int)i };
        auto tmp = child->FindChild(ch);
        if (tmp)
            return tmp;
    }
    return {};
}

std::vector<UINode*>
UINode::FindInRect(const ImRect& r)
{
    std::vector<UINode*> sel;

    if (cached_size.x && cached_size.y && //skip contextMenu
        cached_pos.x > r.Min.x &&
        cached_pos.y > r.Min.y &&
        cached_pos.x + cached_size.x < r.Max.x &&
        cached_pos.y + cached_size.y < r.Max.y)
        sel.push_back(this);

    for (const auto& child : children) {
        auto chsel = child->FindInRect(r);
        sel.insert(sel.end(), chsel.begin(), chsel.end());
    }
    return sel;
}

std::vector<UINode*>
UINode::GetAllChildren()
{
    std::vector<UINode*> chs;
    chs.reserve(children.size() * 2);
    chs.push_back(this);
    for (const auto& child : children) {
        auto vec = child->GetAllChildren();
        chs.insert(chs.end(), vec.begin(), vec.end());
    }
    return chs;
}

//this must return exact class id
std::string UINode::GetTypeName()
{
    std::string name = typeid(*this).name();
    //erase "struct"
    auto i = name.rfind(' ');
    if (i != std::string::npos)
        name.erase(0, i + 1);
    //erase encoded name length
    auto it = stx::find_if(name, [](char c) { return isalpha(c); });
    if (it != name.end())
        name.erase(0, it - name.begin());
    return name;
}

int GetTotalIndex(UINode* parent, UINode* child)
{
    std::string tname = child->GetTypeName();
    int idx = 0;
    for (const auto& ch : parent->children) {
        if (ch.get() == child)
            return idx;
        idx += GetTotalIndex(ch.get(), child);
        if (ch->GetTypeName() == tname)
            ++idx;
    }
    return idx;
}

void UINode::PushError(UIContext& ctx, const std::string& err)
{
    std::string name = GetTypeName();
    if (this != ctx.root) {
        int idx = GetTotalIndex(ctx.parents[0], this);
        name = name + " #" + std::to_string(idx+1);
    }
    ctx.errors.push_back(name + " : " + err);
}

std::string UINode::GetParentIndexes(UIContext& ctx)
{
    std::string id;
    UINode* node = ctx.parents.back();
    for (auto it = ++ctx.parents.rbegin(); it != ctx.parents.rend(); ++it) {
        size_t i = stx::find_if((*it)->children, [=](const auto& ch) {
            return ch.get() == node;
            }) - (*it)->children.begin();
            id = std::to_string(i) + id;
            node = *it;
    }
    return id;
}

void UINode::ResetLayout()
{
    hbox.clear();
    vbox.clear();
    for (auto& ch : children)
        ch->ResetLayout();
}

std::vector<std::string> UINode::UsedFieldVars()
{
    std::vector<std::string> used;
    auto props = Properties();
    for (auto& p : props) {
        if (!p.property)
            continue;
        auto us = p.property->used_variables();
        used.insert(used.end(), us.begin(), us.end());
    }
    for (auto& child : children) {
        auto us = child->UsedFieldVars();
        used.insert(used.end(), us.begin(), us.end());
    }
    stx::sort(used);
    used.erase(stx::unique(used), used.end());
    return used;
}

void UINode::RenameFieldVars(const std::string& oldn, const std::string& newn)
{
    for (int i = 0; i < 2; ++i)
    {
        auto props = i ? Events() : Properties();
        for (auto& p : props) {
            if (!p.property)
                continue;
            p.property->rename_variable(oldn, newn);
        }
    }
    for (auto& child : children)
        child->RenameFieldVars(oldn, newn);
}

//----------------------------------------------------

std::unique_ptr<Widget>
Widget::Create(const std::string& name, UIContext& ctx)
{
    if (name == "Text")
        return std::make_unique<Text>(ctx);
    else if (name == "Selectable")
        return std::make_unique<Selectable>(ctx);
    else if (name == "Button")
        return std::make_unique<Button>(ctx);
    else if (name == "CheckBox")
        return std::make_unique<CheckBox>(ctx);
    else if (name == "RadioButton")
        return std::make_unique<RadioButton>(ctx);
    else if (name == "Input")
        return std::make_unique<Input>(ctx);
    else if (name == "Combo")
        return std::make_unique<Combo>(ctx);
    else if (name == "Slider")
        return std::make_unique<Slider>(ctx);
    else if (name == "ProgressBar")
        return std::make_unique<ProgressBar>(ctx);
    else if (name == "ColorEdit")
        return std::make_unique<ColorEdit>(ctx);
    else if (name == "Image")
        return std::make_unique<Image>(ctx);
    else if (name == "Spacer")
        return std::make_unique<Spacer>(ctx);
    else if (name == "Separator")
        return std::make_unique<Separator>(ctx);
    else if (name == "CustomWidget")
        return std::make_unique<CustomWidget>(ctx);
    else if (name == "Table")
        return std::make_unique<Table>(ctx);
    else if (name == "Child")
        return std::make_unique<Child>(ctx);
    else if (name == "CollapsingHeader")
        return std::make_unique<CollapsingHeader>(ctx);
    else if (name == "TabBar")
        return std::make_unique<TabBar>(ctx);
    else if (name == "TabItem")
        return std::make_unique<TabItem>(ctx);
    else if (name == "TreeNode")
        return std::make_unique<TreeNode>(ctx);
    else if (name == "MenuBar")
        return std::make_unique<MenuBar>(ctx);
    else if (name == "ContextMenu")
        return std::make_unique<ContextMenu>(ctx);
    else if (name == "MenuIt")
        return std::make_unique<MenuIt>(ctx);
    else if (name == "Splitter")
        return std::make_unique<Splitter>(ctx);
    else if (name == "DockSpace")
        return std::make_unique<DockSpace>(ctx);
    else if (name == "DockNode")
        return std::make_unique<DockNode>(ctx);
    else
        return {};
}

Widget::Widget()
{
    hasPos.add("None", ImRad::AlignNone);
    hasPos.add("TopLeft", ImRad::AlignLeft | ImRad::AlignTop);
    hasPos.add("TopCenter", ImRad::AlignHCenter | ImRad::AlignTop);
    hasPos.add("TopRight", ImRad::AlignRight | ImRad::AlignTop);
    hasPos.add("LeftCenter", ImRad::AlignLeft | ImRad::AlignVCenter);
    hasPos.add("Center", ImRad::AlignHCenter | ImRad::AlignVCenter);
    hasPos.add("RightCenter", ImRad::AlignRight | ImRad::AlignVCenter);
    hasPos.add("BottomLeft", ImRad::AlignLeft | ImRad::AlignBottom);
    hasPos.add("BottomCenter", ImRad::AlignHCenter | ImRad::AlignBottom);
    hasPos.add("BottomRight", ImRad::AlignRight | ImRad::AlignBottom);

    cursor.add$(ImGuiMouseCursor_None);
    cursor.add$(ImGuiMouseCursor_Arrow);
    cursor.add$(ImGuiMouseCursor_TextInput);
    cursor.add$(ImGuiMouseCursor_ResizeAll);
    cursor.add$(ImGuiMouseCursor_ResizeNS);
    cursor.add$(ImGuiMouseCursor_ResizeEW);
    cursor.add$(ImGuiMouseCursor_ResizeNESW);
    cursor.add$(ImGuiMouseCursor_ResizeNWSE);
    cursor.add$(ImGuiMouseCursor_Hand);
    cursor.add$(ImGuiMouseCursor_NotAllowed);
}

int Widget::Behavior()
{
    return hasPos ? 0 : SnapSides;
}

//detects if widget is leftmost/topmost in its row
//colId, rowId - use as index to parent.hbox/vbox
//any usage of stretched dimension triggers HLayout/VLayout for that row/column
Widget::Layout Widget::GetLayout(UINode* parent)
{
    Layout l;
    l.colId = l.rowId = -1;
    l.index = -1;

    if (hasPos || !(Behavior() & SnapSides))
    {
        l.flags |= Layout::Topmost | Layout::Leftmost;
        return l;
    }

    bool firstWidget = true;
    bool leftmost = true;
    bool topmost = true;
    bool bottommost = false;
    bool hlay = false;
    bool vlay = false;
    int colId = 0;
    int rowId = 0;
    for (const auto& child : parent->children)
    {
        if (child->hasPos || !(child->Behavior() & SnapSides)) //ignore MenuBar etc.
            continue;

        if ((!child->sameLine || child->nextColumn) && !firstWidget)
        {
            if (colId == l.colId)
                l.flags |= vlay * Layout::VLayout;
            if (rowId == l.rowId)
                l.flags |= hlay * Layout::HLayout;
            ++rowId;
            topmost = child->nextColumn;
            leftmost = true;
            bottommost = false;
            hlay = false;
            if (child->nextColumn)
            {
                vlay = false;
                colId += child->nextColumn;
            }
        }
        else if (child->sameLine && !child->nextColumn) {
            leftmost = false;
        }

        if (child->size_y.stretched())
            vlay = true;
        if (child->size_x.stretched())
            hlay = true;

        if (child.get() == this) {
            l.index = &child - parent->children.data();
            l.colId = colId;
            l.rowId = rowId;
            l.flags |= (leftmost * Layout::Leftmost) | (topmost * Layout::Topmost);
            bottommost = true;
        }

        firstWidget = false;
    }
    if (bottommost)
        l.flags |= Layout::Bottommost;
    if (colId == l.colId)
        l.flags |= vlay * Layout::VLayout;
    if (rowId == l.rowId)
        l.flags |= hlay * Layout::HLayout;

    return l;
}

void Widget::Draw(UIContext& ctx)
{
    UINode* parent = ctx.parents.back();
    Layout l = GetLayout(parent);
    const int defSpacing = (l.flags & Layout::Topmost) ? 0 : 1;
    ctx.stretchSize = { 0, 0 };

    if (!hasPos && nextColumn) {
        bool inTable = dynamic_cast<Table*>(parent);
        if (inTable)
            ImRad::TableNextColumn(nextColumn);
        else
            ImRad::NextColumn(nextColumn);
    }

    if (hasPos)
    {
        ImRect r = parent->Behavior() & SnapItemInterior ?
            ImRect{ parent->cached_pos, parent->cached_pos + parent->cached_size } :
            ImRad::GetParentInnerRect();
        ImVec2 pos{ pos_x.eval_px(ImGuiAxis_X, ctx), pos_y.eval_px(ImGuiAxis_Y, ctx) };
        if (hasPos & ImRad::AlignLeft)
            pos.x += r.Min.x;
        else if (hasPos & ImRad::AlignRight)
            pos.x += r.Max.x;
        else if (hasPos & ImRad::AlignHCenter)
            pos.x += r.GetCenter().x;
        if (hasPos & ImRad::AlignTop)
            pos.y += r.Min.y;
        else if (hasPos & ImRad::AlignBottom)
            pos.y += r.Max.y;
        else if (hasPos & ImRad::AlignVCenter)
            pos.y += r.GetCenter().y;
        ImGui::SetCursorScreenPos(pos);
    }
    else if (l.flags & (Layout::HLayout | Layout::VLayout))
    {
        ImRad::VBox* vbox = nullptr;
        ImRad::HBox *hbox = nullptr;

        if (l.flags & Layout::VLayout)
        {
            if (l.colId >= parent->vbox.size())
                parent->vbox.resize(l.colId + 1);
            vbox = &parent->vbox[l.colId];
            if ((l.flags & Layout::Topmost) && (l.flags & Layout::Leftmost))
                vbox->BeginLayout();
            /*if (l.flags & Layout::Leftmost)
                ImGui::SetCursorPosY(vbox);*/
        }
        if ((l.flags & Layout::Leftmost) && (spacing - defSpacing))
        {
            ImRad::Spacing(spacing - defSpacing);
        }
        if (l.flags & Layout::HLayout)
        {
            if (l.rowId >= parent->hbox.size())
                parent->hbox.resize(l.rowId + 1);
            hbox = &parent->hbox[l.rowId];
            if (l.flags & Layout::Leftmost)
                hbox->BeginLayout();
            //ImGui::SetCursorPosX(hbox); //currently not needed but may be useful if we upgrade layouts
        }
        if (!(l.flags & Layout::Leftmost))
        {
            //we need to provide correct spacing and don't use SetCursorX
            //becausein that case after hbox.Reset() hbox.GetPos() will return CursorPos with
            //a wrong spacing and it will be used in autosized window contentSize
            ImGui::SameLine(0, spacing * ImGui::GetStyle().ItemSpacing.x);
        }
        //after SameLine was determined
        if (vbox)
            ctx.stretchSize.y = vbox->GetSize(); // !(l.flags & Layout::Leftmost));
        if (hbox)
            ctx.stretchSize.x = hbox->GetSize();
    }
    else
    {
        if (sameLine) {
            ImGui::SameLine(0, spacing * ImGui::GetStyle().ItemSpacing.x);
        }
        else {
            ImRad::Spacing(spacing - defSpacing);
        }
        if (indent)
            ImGui::Indent(indent * ImGui::GetStyle().IndentSpacing / 2);
    }

    ImGui::PushID(this);
    ctx.parents.push_back(this);
    auto lastHovered = ctx.hovered;
    auto p1 = ImGui::GetCursorScreenPos();

    if (style_font.has_value())
        ImGui::PushFont(ImRad::GetFontByName(style_font.eval(ctx)));
    if (!style_text.empty())
        ImGui::PushStyleColor(ImGuiCol_Text, style_text.eval(ImGuiCol_Text, ctx));
    if (!style_border.empty())
        ImGui::PushStyleColor(ImGuiCol_Border, style_border.eval(ImGuiCol_Border, ctx));
    if (!style_frameBg.empty())
        ImGui::PushStyleColor(ImGuiCol_FrameBg, style_frameBg.eval(ImGuiCol_FrameBg, ctx));
    if (!style_button.empty())
        ImGui::PushStyleColor(ImGuiCol_Button, style_button.eval(ImGuiCol_Button, ctx));
    if (!style_buttonHovered.empty())
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style_buttonHovered.eval(ImGuiCol_ButtonHovered, ctx));
    if (!style_buttonActive.empty())
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, style_buttonActive.eval(ImGuiCol_ButtonActive, ctx));
    if (!style_frameBorderSize.empty())
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, style_frameBorderSize);
    if (!style_frameRounding.empty())
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, style_frameRounding);
    if (!style_framePadding.empty())
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, style_framePadding);

    ImGui::BeginDisabled((!disabled.empty() && disabled.eval(ctx)) || (!visible.empty() && !visible.eval(ctx)));
    ImDrawList* drawList = DoDraw(ctx);
    ImGui::EndDisabled();
    CalcSizeEx(p1, ctx);

    if (!style_text.empty())
        ImGui::PopStyleColor();
    if (!style_border.empty())
        ImGui::PopStyleColor();
    if (!style_frameBg.empty())
        ImGui::PopStyleColor();
    if (!style_button.empty())
        ImGui::PopStyleColor();
    if (!style_buttonHovered.empty())
        ImGui::PopStyleColor();
    if (!style_buttonActive.empty())
        ImGui::PopStyleColor();
    if (!style_frameBorderSize.empty())
        ImGui::PopStyleVar();
    if (!style_frameRounding.empty())
        ImGui::PopStyleVar();
    if (!style_framePadding.empty())
        ImGui::PopStyleVar();
    if (style_font.has_value())
        ImGui::PopFont();

    if (!hasPos)
        HashCombineData(ctx.layoutHash, ImGui::GetItemID());
    if (l.flags & Layout::VLayout)
    {
        auto& vbox = parent->vbox[l.colId];
        float sizeY = ImRad::VBox::ItemSize;
        if (Behavior() & HasSizeY) {
            sizeY = size_y.stretched() ? (float)size_y.value() :
                size_y.zero() ? ImRad::VBox::ItemSize :
                size_y.eval_px(ImGuiAxis_Y, ctx);
            HashCombineData(ctx.layoutHash, sizeY);
        }
        if (size_y.stretched()) {
            if (l.flags & Layout::Leftmost)
                vbox.AddSize(spacing, ImRad::VBox::Stretch(sizeY));
            else
                vbox.UpdateSize(0, ImRad::VBox::Stretch(sizeY));
        }
        else {
            if (l.flags & Layout::Leftmost)
                vbox.AddSize(spacing, sizeY);
            else
                vbox.UpdateSize(0, sizeY);
        }
    }
    if (l.flags & Layout::HLayout)
    {
        auto& hbox = parent->hbox[l.rowId];
        float sizeX = ImRad::HBox::ItemSize;
        if (Behavior() & HasSizeX) {
            sizeX = size_x.stretched() ? (float)size_x.value() :
                size_x.zero() ? ImRad::HBox::ItemSize :
                size_x.eval_px(ImGuiAxis_X, ctx);
            HashCombineData(ctx.layoutHash, sizeX);
        }
        int sp = (l.flags & Layout::Leftmost) ? 0 : (int)spacing;
        if (size_x.stretched())
            hbox.AddSize(sp, ImRad::HBox::Stretch(sizeX));
        else
            hbox.AddSize(sp, sizeX);
    }

    //doesn't work for open CollapsingHeader etc:
    //bool hovered1 = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
    bool hovered = ImGui::IsMouseHoveringRect(cached_pos, cached_pos + cached_size) &&
        ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    if ((ctx.mode == UIContext::NormalSelection || ctx.mode == UIContext::SnapInsert) &&
        hovered && !ImGui::GetTopMostAndVisiblePopupModal())
    {
        //prevent changing cursor to e.g. TextInput
        //but don't override child.Draw SetMouseCursor
        if (ctx.hovered == lastHovered)
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
    }
    if (ctx.mode == UIContext::NormalSelection &&
        hovered && !ImGui::GetTopMostAndVisiblePopupModal())
    {
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) //this works even for non-items like TabControl etc.
        {
            if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
            {
                if (!stx::count(ctx.selected, ctx.root))
                    toggle(ctx.selected, this);
            }
            else
                ctx.selected = { this };
            ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left] = false; //eat event so parent won't get selected
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            if (!stx::count(ctx.selected, this))
                ctx.selected = { this };
            ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left] = false; //eat event so parent won't get selected
        }
    }
    bool allowed = !ImGui::GetTopMostAndVisiblePopupModal() &&
        (ctx.activePopups.empty() || stx::count(ctx.activePopups, ImGui::GetCurrentWindow()));
    if (ctx.mode == UIContext::NormalSelection &&
        allowed && hovered)
    {
        if (ctx.hovered == lastHovered) //e.g. child widget was not selected
            ctx.hovered = this;
        //next operations only with single selected widget
        if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) &&
            ctx.selected.size() == 1 && ctx.selected[0] == this)
        {
            int resizeMode = 0;
            ImVec2 mousePos = ImGui::IsMouseDown(ImGuiMouseButton_Left) ?
                ImGui::GetIO().MouseClickedPos[ImGuiMouseButton_Left] :
                ImGui::GetMousePos();
            if ((Behavior() & HasSizeX) && size_x.has_value())
            {
                if (std::abs(mousePos.x - cached_pos.x) < 5)
                    resizeMode |= UIContext::ItemSizingLeft;
                else if (std::abs(mousePos.x - cached_pos.x - cached_size.x) < 5)
                    resizeMode |= UIContext::ItemSizingRight;
            }
            if ((Behavior() & HasSizeY) && size_y.has_value())
            {
                if (std::abs(mousePos.y - cached_pos.y) < 5)
                    resizeMode |= UIContext::ItemSizingTop;
                if (std::abs(mousePos.y - cached_pos.y - cached_size.y) < 5)
                    resizeMode |= UIContext::ItemSizingBottom;
            }
            if (resizeMode)
            {
                if ((resizeMode & UIContext::ItemSizingLeft) && (resizeMode & UIContext::ItemSizingBottom))
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
                else if ((resizeMode & UIContext::ItemSizingRight) && (resizeMode & UIContext::ItemSizingBottom))
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
                else if (resizeMode & (UIContext::ItemSizingLeft | UIContext::ItemSizingRight))
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                else if (resizeMode & (UIContext::ItemSizingTop | UIContext::ItemSizingBottom))
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

                if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    ctx.mode = (UIContext::Mode)resizeMode;
                    ctx.dragged = this;
                    ctx.lastSize = {
                        size_x.eval_px(ImGuiAxis_X, ctx),
                        size_y.eval_px(ImGuiAxis_Y, ctx)
                    };
                    if (ctx.lastSize.x <= 0)
                        ctx.lastSize.x = cached_size.x;
                    if (ctx.lastSize.y <= 0)
                        ctx.lastSize.y = cached_size.y;
                    ctx.lastSize /= ctx.zoomFactor;
                }
            }
            else if (ctx.hovered == this &&
                ((hasPos && pos_x.has_value() && pos_y.has_value()) ||
                (Behavior() & SnapSides)))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) //ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    if (hasPos)
                    {
                        ctx.mode = UIContext::ItemDragging;
                        ctx.dragged = this;
                    }
                    else
                    {
                        ctx.mode = UIContext::SnapMove;
                        ctx.snapParent = nullptr;
                    }
                }
            }
        }
    }
    else if (ctx.mode == UIContext::PickPoint &&
        allowed && hovered &&
        (Behavior() & (SnapInterior | SnapItemInterior)))
    {
        if (ctx.hovered == lastHovered) //e.g. child widget was not selected
        {
            ctx.snapParent = this;
            ctx.snapIndex = children.size();
        }
    }
    else if (ctx.mode == UIContext::SnapMove &&
        allowed)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }
    else if (ctx.mode == UIContext::ItemDragging &&
            allowed && ctx.dragged == this)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImVec2 delta = ImGui::GetMouseDragDelta();
            *ctx.modified = true;
            pos_x = pos_x.value() + delta.x / ctx.zoomFactor;
            pos_y = pos_y.value() + delta.y / ctx.zoomFactor;
            ImGui::ResetMouseDragDelta();
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            ctx.mode = UIContext::NormalSelection;
            ctx.selected = { this };
            ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left] = false; //eat event
        }
    }
    else if ((ctx.mode & UIContext::ItemSizingMask) &&
            allowed && ctx.dragged == this)
    {
        if ((ctx.mode & UIContext::ItemSizingLeft) && (ctx.mode & UIContext::ItemSizingBottom))
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
        else if ((ctx.mode & UIContext::ItemSizingRight) && (ctx.mode & UIContext::ItemSizingBottom))
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        else if (ctx.mode & (UIContext::ItemSizingLeft | UIContext::ItemSizingRight))
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        else if (ctx.mode & (UIContext::ItemSizingTop | UIContext::ItemSizingBottom))
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            *ctx.modified = true;
            ImVec2 delta = ImGui::GetMouseDragDelta() / ctx.zoomFactor;
            ImVec2 sp = ImGui::GetStyle().ItemSpacing;
            if (!sp.x)
                sp.x = 5;
            if (!sp.y)
                sp.y = 5;
            ImGuiWindow* win = ImGui::GetCurrentWindow();
            if (ctx.mode & (UIContext::ItemSizingLeft | UIContext::ItemSizingRight))
            {
                float val = ctx.lastSize.x +
                    ((ctx.mode & UIContext::ItemSizingLeft) ? -delta.x : delta.x);
                val = int(val / sp.x) * sp.x;
                if ((ctx.mode & UIContext::ItemSizingRight) && size_x.value() < 0 && delta.x > 0)
                    val = size_x.value(); //removes flicker
                /*if (ctx.lastSize.x < 0 && !val)
                    val = -1; //snap to the end*/
                if ((ctx.mode & UIContext::ItemSizingRight) &&
                    ctx.lastSize.x > 0 && delta.x > 0 && cached_pos.x + val >= win->WorkRect.Max.x)
                {
                    const ImGuiTable* table = ImGui::GetCurrentTable();
                    const ImGuiTableColumn* column = table ? &table->Columns[table->CurrentColumn] : nullptr;
                    //don't set to stretchable when we are in FixedWidth=0 column
                    //because column width is calculated from widget size
                    if (!table ||
                        (column->Flags & ImGuiTableColumnFlags_WidthStretch) ||
                        column->InitStretchWeightOrWidth)
                        size_x = -1; //set stretchable
                    /*else the effect looks weird
                        size_x = val;*/
                }
                else if (std::signbit(ctx.lastSize.x) == std::signbit(val))
                    size_x = val;

            }
            if (ctx.mode & (UIContext::ItemSizingTop | UIContext::ItemSizingBottom))
            {
                float val = ctx.lastSize.y +
                    ((ctx.mode & UIContext::ItemSizingTop) ? -delta.y : delta.y);
                val = int(val / sp.y) * sp.y;
                if ((ctx.mode & UIContext::ItemSizingBottom) && size_y.value() < 0 && delta.y > 0)
                    val = size_y.value(); //removes flicker
                /*if (ctx.lastSize.y < 0 && !val)
                    val = -1; //snap to the end*/
                if ((ctx.mode & UIContext::ItemSizingBottom) &&
                    ctx.lastSize.y > 0 && delta.y > 0 && cached_pos.y + val >= win->WorkRect.Max.y)
                    size_y = -1; //set auto size
                else if (std::signbit(ctx.lastSize.y) == std::signbit(val))
                    size_y = val;
            }
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            ctx.mode = UIContext::NormalSelection;
            ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left] = false; //eat event
        }
    }

    bool selected = stx::count(ctx.selected, this);
    if (hasPos && !ctx.beingResized) //(selected || ctx.hovered == this))
    {
        ImU32 clr = ctx.colors[UIContext::Selected];
        ImVec2 szx{ cached_size.x, 0 };
        ImVec2 szy{ 0, cached_size.y };
        ImVec2 cx{ cached_size.x / 2, 0 };
        ImVec2 cy{ 0, cached_size.y / 2 };
        float d = 8.f + (selected ? 1 : 0);
        float dd = 0.66f*d;
        if (hasPos == (ImRad::AlignLeft | ImRad::AlignTop))
            drawList->AddTriangleFilled(cached_pos, cached_pos + ImVec2(d, 0), cached_pos + ImVec2(0, d), clr);
        else if (hasPos == (ImRad::AlignHCenter | ImRad::AlignTop))
            drawList->AddTriangleFilled(cached_pos + cx, cached_pos + cx + ImVec2(dd, d), cached_pos + cx + ImVec2(-dd, d), clr);
        else if (hasPos == (ImRad::AlignRight | ImRad::AlignTop))
            drawList->AddTriangleFilled(cached_pos + szx, cached_pos + szx + ImVec2(0, d), cached_pos + szx + ImVec2(-d, 0), clr);
        else if (hasPos == (ImRad::AlignLeft | ImRad::AlignVCenter))
            drawList->AddTriangleFilled(cached_pos + cy, cached_pos + cy + ImVec2(d, -dd), cached_pos + cy + ImVec2(d, dd), clr);
        else if (hasPos == (ImRad::AlignRight | ImRad::AlignVCenter))
            drawList->AddTriangleFilled(cached_pos + szx + cy, cached_pos + cy + szx + ImVec2(-d, -dd), cached_pos + cy + szx + ImVec2(-d, dd), clr);
        else if (hasPos == (ImRad::AlignLeft | ImRad::AlignBottom))
            drawList->AddTriangleFilled(cached_pos + szy + ImVec2(d, 0), cached_pos + szy, cached_pos + szy + ImVec2(0, -d), clr);
        else if (hasPos == (ImRad::AlignHCenter | ImRad::AlignBottom))
            drawList->AddTriangleFilled(cached_pos + cx + szy, cached_pos + cx + szy + ImVec2(-dd, -d), cached_pos + cx + szy + ImVec2(dd, -d), clr);
        else if (hasPos == (ImRad::AlignRight | ImRad::AlignBottom))
            drawList->AddTriangleFilled(cached_pos + cached_size, cached_pos + cached_size + ImVec2(-d, 0), cached_pos + cached_size + ImVec2(0, -d), clr);
        //else if (hasPos == (ImRad::AlignHCenter | ImRad::AlignVCenter))
    }
    /*if (ctx.mode == UIContext::ItemSizingX &&
        ctx.dragged == this)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImRect ir = ImGui::GetCurrentWindow()->InnerRect;
        ImU32 clr = ctx.colors[UIContext::Selected] & 0x4fffffff;
        ImGui::PushClipRect(ir.Min, ir.Max, false);
        dl->AddLine(ImVec2{ cached_pos.x + cached_size.x, ir.Min.y }, ImVec2{ cached_pos.x + cached_size.x, ir.Max.y }, clr);
        ImGui::PopClipRect();
    }*/
    if (selected
        /*ctx.mode >= UIContext::ItemSizingX && ctx.mode <= UIContext::ItemSizingXY &&
        ctx.dragged == this*/)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImRect wr = ImGui::GetCurrentWindow()->WorkRect;
        ImRect ir = ImGui::GetCurrentWindow()->InnerRect;
        ImVec2 pad = ImGui::GetStyle().WindowPadding;
        ImGui::PushClipRect(ir.Min, ir.Max, false);
        ImU32 clr = (ctx.colors[UIContext::Selected] & 0x00ffffff) | 0x60000000;
        if (size_x.eval_px(ImGuiAxis_X, ctx) < 0)
            dl->AddRectFilled(ImVec2{ wr.Max.x, wr.Min.y - pad.y }, ImVec2{ wr.Max.x + pad.x, wr.Max.y + pad.y }, clr);
        if (size_y.eval_px(ImGuiAxis_Y, ctx) < 0)
            dl->AddRectFilled(ImVec2{ wr.Min.x - pad.x, wr.Max.y }, ImVec2{ wr.Max.x + pad.x, wr.Max.y + pad.y }, clr);
        ImGui::PopClipRect();
    }
    if ((selected || ctx.hovered == this) &&
        ctx.mode != UIContext::ItemDragging &&
        (ctx.mode & UIContext::ItemSizingMask) == 0)
    {
        //dl->PushClipRectFullScreen();
        drawList->AddRect(
            cached_pos - ImVec2(1, 1), cached_pos + cached_size,
            selected ? ctx.colors[UIContext::Selected] : ctx.colors[UIContext::Hovered],
            0, 0, selected ? 2.f : 1.f);
        //dl->PopClipRect();
    }
    if (ctx.mode == UIContext::SnapInsert && !ctx.snapParent)
    {
        DrawSnap(ctx);
    }
    else if (ctx.mode == UIContext::SnapMove && !ctx.snapParent)
    {
        if (!ctx.selected[0]->FindChild(parent)) //disallow moving into its child
            DrawSnap(ctx);
    }
    else if (ctx.mode == UIContext::PickPoint && ctx.snapParent == this)
    {
        DrawInteriorRect(ctx);
    }


    ctx.parents.pop_back();
    ImGui::PopID();
}

void Widget::DrawTools(UIContext& ctx)
{
    ctx.parents.push_back(this);

    if (ctx.selected.size() == 1 && ctx.selected[0] == this)
    {
        ImGui::PushFont(nullptr);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ctx.appStyle->WindowPadding);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ctx.appStyle->ItemSpacing);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ctx.appStyle->Colors[ImGuiCol_WindowBg]);
        ImGui::PushStyleColor(ImGuiCol_Text, ctx.appStyle->Colors[ImGuiCol_Text]);
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, ctx.appStyle->Colors[ImGuiCol_TextDisabled]);
        ImGui::PushStyleColor(ImGuiCol_Button, ctx.appStyle->Colors[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ctx.appStyle->Colors[ImGuiCol_ButtonHovered]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ctx.appStyle->Colors[ImGuiCol_ButtonActive]);

        DoDrawTools(ctx);

        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar(3);
        ImGui::PopFont();
    }

    //for (const auto& child : children) defend against insertions within the loop
    for (size_t i = 0; i < children.size(); ++i)
        children[i]->DrawTools(ctx);

    ctx.parents.pop_back();
}

void Widget::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    cached_pos = p1;
    cached_size = ImGui::GetItemRectSize();
}

void Widget::Export(std::ostream& os, UIContext& ctx)
{
    UINode* parent = ctx.parents.back();
    Layout l = GetLayout(parent);
    ctx.stretchSize = { 0, 0 };
    ctx.stretchSizeExpr = { "", "" };
    const int defSpacing = (l.flags & Layout::Topmost) ? 0 : 1;
    std::string hbName, vbName;

    if (userCodeBefore != "")
        os << userCodeBefore << "\n";

    std::string stype = GetTypeName();
    os << ctx.ind << "/// @begin " << stype << "\n";

    //layout commands first even when !visible
    if (!hasPos && nextColumn)
    {
        bool inTable = dynamic_cast<Table*>(parent);
        if (inTable)
            os << ctx.ind << "ImRad::TableNextColumn(" << nextColumn.to_arg() << ");\n";
        else
            os << ctx.ind << "ImRad::NextColumn(" << nextColumn.to_arg() << ");\n";
    }
    if (!hasPos && (l.flags & Layout::VLayout))
    {
        std::ostringstream osv;
        osv << ctx.codeGen->VBOX_NAME;
        osv << GetParentIndexes(ctx);
        osv << (l.colId + 1);
        vbName = osv.str();
        ctx.codeGen->CreateNamedVar(vbName, "ImRad::VBox", "", CppGen::Var::Impl);

        if ((l.flags & Layout::Topmost) && (l.flags & Layout::Leftmost))
            os << ctx.ind << vbName << ".BeginLayout();\n";
        /*if (l.flags & Layout::Leftmost)
            os << ctx.ind << "ImGui::SetCursorPosY(" << vbName << ");\n";*/
        ctx.stretchSizeExpr[1] = vbName + ".GetSize()";
    }
    if (!hasPos && (l.flags & Layout::HLayout))
    {
        std::ostringstream osv;
        osv << ctx.codeGen->HBOX_NAME;
        osv << GetParentIndexes(ctx);
        osv << (l.rowId + 1);
        hbName = osv.str();
        ctx.codeGen->CreateNamedVar(hbName, "ImRad::HBox", "", CppGen::Var::Impl);

        if (l.flags & Layout::Leftmost)
            os << ctx.ind << hbName << ".BeginLayout();\n";
        //os << ctx.ind << "ImGui::SetCursorPosX(" << hbName << ");\n";
        ctx.stretchSizeExpr[0] = hbName + ".GetSize()";
    }

    if (!visible.empty())
    {
        os << ctx.ind << "if (" << visible.to_arg() << ")\n" << ctx.ind << "{\n";
        ctx.ind_up();
        os << ctx.ind << "//visible\n";
    }

    if (hasPos)
    {
        os << ctx.ind << "ImGui::SetCursorScreenPos({ ";
        std::string parentRect = parent->Behavior() & SnapItemInterior ?
            ctx.parentVarName : "ImRad::GetParentInnerRect()";
        if (hasPos & ImRad::AlignLeft)
            os << parentRect << ".Min.x";
        else if (hasPos & ImRad::AlignRight)
            os << parentRect << ".Max.x";
        else if (hasPos & ImRad::AlignHCenter)
            os << parentRect << ".GetCenter().x";
        os << (!pos_x.has_value() || pos_x.value() >= 0 ? "+" : "")
            << pos_x.to_arg(ctx.unit);
        os << ", ";
        if (hasPos & ImRad::AlignTop)
            os << parentRect << ".Min.y";
        else if (hasPos & ImRad::AlignBottom)
            os << parentRect << ".Max.y";
        else if (hasPos & ImRad::AlignVCenter)
            os << parentRect << ".GetCenter().y";
        os << (!pos_y.has_value() || pos_y.value() >= 0 ? "+" : "")
            << pos_y.to_arg(ctx.unit);
        os << " }); //overlayPos\n";
    }
    else
    {
        if (sameLine)
        {
            os << ctx.ind << "ImGui::SameLine(";
            os << "0, " << spacing << " * ImGui::GetStyle().ItemSpacing.x";
            os << ");\n";
        }
        else if (spacing - defSpacing)
        {
            os << ctx.ind << "ImRad::Spacing(" << (spacing - defSpacing) << ");\n";
        }
        if (indent && !(l.flags & (Layout::HLayout | Layout::VLayout)))
        {
            os << ctx.ind << "ImGui::Indent(" << indent << " * ImGui::GetStyle().IndentSpacing / 2);\n";
        }
    }

    if (allowOverlap)
    {
        os << ctx.ind << "ImGui::SetNextItemAllowOverlap();\n";
    }
    if (!disabled.empty())
    {
        os << ctx.ind << "ImGui::BeginDisabled(" << disabled.to_arg() << ");\n";
    }
    if (!tabStop)
    {
        os << ctx.ind << "ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);\n";
    }
    if (!style_font.empty())
    {
        os << ctx.ind << "ImGui::PushFont(" << style_font.to_arg() << ");\n";
    }
    if (!style_text.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Text, " << style_text.to_arg() << ");\n";
    }
    if (!style_border.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Border, " << style_border.to_arg() << ");\n";
    }
    if (!style_frameBg.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_FrameBg, " << style_frameBg.to_arg() << ");\n";
    }
    if (!style_button.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Button, " << style_button.to_arg() << ");\n";
    }
    if (!style_buttonHovered.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_ButtonHovered, " << style_buttonHovered.to_arg() << ");\n";
    }
    if (!style_buttonActive.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_ButtonActive, " << style_buttonActive.to_arg() << ");\n";
    }
    if (!style_frameBorderSize.empty())
    {
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, " << style_frameBorderSize.to_arg(ctx.unit) << ");\n";
    }
    if (!style_frameRounding.empty())
    {
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, " << style_frameRounding.to_arg(ctx.unit) << ");\n";
    }
    if (!style_framePadding.empty())
    {
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, " << style_framePadding.to_arg(ctx.unit) << ");\n";
    }

    ctx.parents.push_back(this);
    DoExport(os, ctx);
    ctx.parents.pop_back();

    if (l.flags & Layout::VLayout)
    {
        std::string sizeY = "ImRad::VBox::ItemSize";
        if (Behavior() & HasSizeY) {
            std::ostringstream os;
            os << size_y.value();
            sizeY = size_y.stretched() ? "ImRad::VBox::Stretch(" + os.str() + ")" :
                size_y.zero() ? "ImRad::VBox::ItemSize" :
                size_y.to_arg(ctx.unit);
        }
        if (l.flags & Layout::Leftmost)
            os << ctx.ind << vbName << ".AddSize(" << spacing << ", " << sizeY << ");\n";
        else
            os << ctx.ind << vbName << ".UpdateSize(0, " << sizeY << ");\n";
    }
    if (l.flags & Layout::HLayout)
    {
        std::string sizeX = "ImRad::HBox::ItemSize";
        if (Behavior() & HasSizeX) {
            std::ostringstream os;
            os << size_x.value();
            sizeX = size_x.stretched() ? "ImRad::HBox::Stretch(" + os.str() + ")" :
                size_x.zero() ? "ImRad::HBox::ItemSize" :
                size_x.to_arg(ctx.unit);
        }
        int sp = (l.flags & Layout::Leftmost) ? 0 : (int)spacing;
        os << ctx.ind << hbName << ".AddSize(" << sp << ", " << sizeX << ");\n";
    }

    if (!style_frameBorderSize.empty())
    {
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    }
    if (!style_framePadding.empty())
    {
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    }
    if (!style_frameRounding.empty())
    {
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    }
    if (!style_frameBg.empty())
    {
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    }
    if (!style_button.empty())
    {
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    }
    if (!style_buttonHovered.empty())
    {
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    }
    if (!style_buttonActive.empty())
    {
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    }
    if (!style_text.empty())
    {
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    }
    if (!style_border.empty())
    {
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    }
    if (!style_font.empty())
    {
        os << ctx.ind << "ImGui::PopFont();\n";
    }
    if (!tabStop)
    {
        os << ctx.ind << "ImGui::PopItemFlag();\n";
    }
    if (!disabled.empty())
    {
        os << ctx.ind << "ImGui::EndDisabled();\n";
    }

    if (cursor != ImGuiMouseCursor_Arrow)
    {
        os << ctx.ind << "if (ImGui::IsItemHovered())\n";
        ctx.ind_up();
        os << ctx.ind << "ImGui::SetMouseCursor(" << cursor.to_arg() << ");\n";
        ctx.ind_down();
    }
    if (!tooltip.empty())
    {
        os << ctx.ind << "if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))\n";
        ctx.ind_up();
        os << ctx.ind << "ImGui::SetTooltip(" << tooltip.to_arg() << ");\n";
        ctx.ind_down();
    }
    if (!contextMenu.empty())
    {
        os << ctx.ind << "if (ImRad::IsItemContextMenuClicked())\n";
        ctx.ind_up();
        os << ctx.ind << "ImRad::OpenWindowPopup(" << contextMenu.to_arg() << ");\n";
        ctx.ind_down();
    }

    if (initialFocus)
    {
        os << ctx.ind << "if (ImGui::IsWindowAppearing())\n";
        ctx.ind_up();
        os << ctx.ind << "ImGui::SetKeyboardFocusHere(-1);\n";
        ctx.ind_down();
    }
    if (!forceFocus.empty())
    {
        os << ctx.ind << "if (" << forceFocus.to_arg() << ")\n" << ctx.ind << "{\n";
        ctx.ind_up();
        os << ctx.ind << "//forceFocus\n";
        os << ctx.ind << "if (ImGui::IsItemFocused())\n";
        ctx.ind_up();
        os << ctx.ind;
        if (forceFocus.is_reference()) {
            os << forceFocus.to_arg() << " = false;\n";
        }
        else {
            auto vars = forceFocus.used_variables();
            if (vars.empty())
                PushError(ctx, "can't parse variable in forceFocus binding expression");
            else
                os << vars[0] << " = {};\n";
        }
        ctx.ind_down();
        os << ctx.ind << "ImGui::SetKeyboardFocusHere(-1);\n";
        ctx.ind_down();
        os << ctx.ind << "}\n";
    }
    if (!onDragDropSource.empty())
    {
        os << ctx.ind << "if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))\n";
        os << ctx.ind << "{\n";
        ctx.ind_up();
        os << ctx.ind << onDragDropSource.c_str() << "();\n";
        os << ctx.ind << "ImGui::EndDragDropSource();\n";
        ctx.ind_down();
        os << ctx.ind << "}\n";
    }
    if (!onDragDropTarget.empty())
    {
        os << ctx.ind << "if (ImGui::BeginDragDropTarget())\n";
        os << ctx.ind << "{\n";
        ctx.ind_up();
        os << ctx.ind << onDragDropTarget.c_str() << "();\n";
        os << ctx.ind << "ImGui::EndDragDropTarget();\n";
        ctx.ind_down();
        os << ctx.ind << "}\n";
    }
    if (!onItemContextMenuClicked.empty())
    {
        os << ctx.ind << "if (ImRad::IsItemContextMenuClicked())\n";
        ctx.ind_up();
        os << ctx.ind << onItemContextMenuClicked.c_str() << "();\n";
        ctx.ind_down();
    }
    if (!onItemLongPressed.empty())
    {
        os << ctx.ind << "if (ImRad::IsItemLongPressed())\n";
        os << ctx.ind << "{\n";
        ctx.ind_up();
        os << ctx.ind << onItemLongPressed.c_str() << "();\n";
        os << ctx.ind << "ImGui::GetIO().MouseDown[ImGuiMouseButton_Left] = false;\n";
        os << ctx.ind << "ioUserData->longPressID = ImGui::GetItemID();\n";
        ctx.ind_down();
        os << ctx.ind << "}\n";
    }
    if (!onItemHovered.empty())
    {
        os << ctx.ind << "if (ImGui::IsItemHovered())\n";
        ctx.ind_up();
        os << ctx.ind << onItemHovered.c_str() << "();\n";
        ctx.ind_down();
    }
    if (!onItemClicked.empty())
    {
        os << ctx.ind << "if (ImGui::IsItemClicked())\n";
        ctx.ind_up();
        os << ctx.ind << onItemClicked.c_str() << "();\n";
        ctx.ind_down();
    }
    if (!onItemDoubleClicked.empty())
    {
        os << ctx.ind << "if (ImRad::IsItemDoubleClicked())\n";
        ctx.ind_up();
        os << ctx.ind << onItemDoubleClicked.c_str() << "();\n";
        ctx.ind_down();
    }
    if (!onItemFocused.empty())
    {
        os << ctx.ind << "if (ImGui::IsItemFocused())\n";
        ctx.ind_up();
        os << ctx.ind << onItemFocused.c_str() << "();\n";
        ctx.ind_down();
    }
    if (!onItemActivated.empty())
    {
        os << ctx.ind << "if (ImGui::IsItemActivated())\n";
        ctx.ind_up();
        os << ctx.ind << onItemActivated.c_str() << "();\n";
        ctx.ind_down();
    }
    if (!onItemDeactivated.empty())
    {
        os << ctx.ind << "if (ImGui::IsItemDeactivated())\n";
        ctx.ind_up();
        os << ctx.ind << onItemDeactivated.c_str() << "();\n";
        ctx.ind_down();
    }
    if (!onItemDeactivatedAfterEdit.empty())
    {
        os << ctx.ind << "if (ImGui::IsItemDeactivatedAfterEdit())\n";
        ctx.ind_up();
        os << ctx.ind << onItemDeactivatedAfterEdit.c_str() << "();\n";
        ctx.ind_down();
    }

    if (!visible.empty())
    {
        ctx.ind_down();
        os << ctx.ind << "}\n";
        //always submit new line so that next widgets won't get placed to the previous line
        //for Leftmost ensure vb.AddSize so next widget don't update previous line
        const Widget* next = l.index + 1 < parent->children.size() ? parent->children[l.index + 1].get() : nullptr;
        if (!hasPos && (l.flags & Layout::Leftmost) &&
            next && !next->hasPos && !next->nextColumn && next->sameLine)
        {
            os << ctx.ind << "else\n" << ctx.ind << "{\n";
            ctx.ind_up();
            if (spacing - defSpacing)
                os << ctx.ind << "ImRad::Spacing(" << (spacing - defSpacing) << ");\n";
            os << ctx.ind << "ImGui::ItemSize({ 0, 0 });\n";
            if (l.flags & Layout::VLayout)
                os << ctx.ind << vbName << ".AddSize(" << spacing << ", 0);\n";
            ctx.ind_down();
            os << ctx.ind << "}\n";
        }
    }

    os << ctx.ind << "/// @end " << stype << "\n\n";

    if (userCodeAfter != "")
        os << userCodeAfter << "\n";
}

void Widget::Import(cpp::stmt_iterator& sit, UIContext& ctx)
{
    ctx.importState = 1;
    ctx.importLevel = -1;
    ctx.parents.push_back(this);
    userCodeBefore = ctx.userCode;
    spacing = -1;
    int ignoreLevel = -1;

    while (sit != cpp::stmt_iterator())
    {
        cpp::stmt_iterator ifBlockIt;
        cpp::stmt_iterator screenPosIt;

        if (sit->level <= ctx.importLevel) {
            ctx.importLevel = -1;
        }
        if (ignoreLevel >= 0)
        {
            if (sit->level >= ignoreLevel) {
                ++sit;
                continue;
            }
            else
                ignoreLevel = -1;
        }

        if (sit->kind == cpp::Comment && !sit->line.compare(0, 11, "/// @begin "))
        {
            ctx.importState = 1;
            sit.enable_parsing(true);
            std::string name = sit->line.substr(11);
            auto w = Widget::Create(name, ctx);
            if (!w) {
                //uknown control
                //create a placeholder not to break parsing and layout
                ctx.errors.push_back("Encountered an unknown control '" + name + "\"");
                auto txt = std::make_unique<Text>(ctx);
                txt->text = "???";
                w = std::move(txt);
            }
            children.push_back(std::move(w));
            children.back()->Import(++sit, ctx); //after insertion
            ctx.importState = 2;
            ctx.userCode = "";
            sit.enable_parsing(false);
        }
        else if (sit->kind == cpp::Comment && !sit->line.compare(0, 9, "/// @end "))
        {
            break; //go back to parent::Import @begin
        }
        else if (sit->kind == cpp::Comment && !sit->line.compare(0, 14, "/// @separator"))
        {
            if (ctx.importState == 1) { //separator at begin
                ctx.importState = 2;
                ctx.userCode = "";
                sit.enable_parsing(false);
            }
            else { //separator at end
                if (!children.empty())
                    children.back()->userCodeAfter = ctx.userCode;
                ctx.importState = 1;
                sit.enable_parsing(true);
            }
        }
        else if (ctx.importState == 2)
        {
            if (ctx.userCode != "")
                ctx.userCode += "\n";
            ctx.userCode += sit->line;
        }
        else if (sit->kind == cpp::IfBlock || sit->kind == cpp::IfCallBlock)
        {
            ifBlockIt = sit; //could be visible or forceFocus block
        }
        else if (sit->kind == cpp::CallExpr &&
            (sit->callee == "ImGui::NextColumn" || sit->callee == "ImGui::TableNextColumn")) //compatibility
        {
            nextColumn = 1;
        }
        else if (sit->kind == cpp::CallExpr &&
            (sit->callee == "ImRad::NextColumn" || sit->callee == "ImRad::TableNextColumn"))
        {
            if (sit->params.size())
                nextColumn.set_from_arg(sit->params[0]);
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetCursorScreenPos")
        {
            screenPosIt = sit;
        }
        else if (sit->kind == cpp::CallExpr &&
            !sit->callee.compare(0, ctx.codeGen->VBOX_NAME.size(), ctx.codeGen->VBOX_NAME) &&
            (sit->callee.find(".AddSize") != std::string::npos ||
             sit->callee.find(".UpdateSize") != std::string::npos))
        {
            if (sit->callee.find(".AddSize") != std::string::npos && sit->params.size())
                spacing.set_from_arg(sit->params[0]);

            if (sit->params.size() >= 2)
            {
                if (sit->params[1] == "ImRad::VBox::ItemSize") {
                    size_y = 0;
                    size_y.stretch(false);
                }
                else if (sit->params[1] == "ImRad::VBox::Stretch") { //compatibility
                    size_y = 1.0f;
                    size_y.stretch(true);
                }
                else if (!sit->params[1].compare(0, 21, "ImRad::VBox::Stretch(")) {
                    size_y.set_from_arg(sit->params[1].substr(21, sit->params[1].size() - 21 - 1));
                    size_y.stretch(true);
                }
                else {
                    size_y.set_from_arg(sit->params[1]);
                    size_y.stretch(false);
                }
            }
        }
        else if (sit->kind == cpp::CallExpr &&
            !sit->callee.compare(0, ctx.codeGen->HBOX_NAME.size(), ctx.codeGen->HBOX_NAME) &&
            sit->callee.find(".AddSize") != std::string::npos)
        {
            if (sit->params.size() && sameLine)
                spacing.set_from_arg(sit->params[0]);

            if (sit->params.size() >= 2)
            {
                if (sit->params[1] == "ImRad::HBox::ItemSize") {
                    size_x = 0;
                    size_x.stretch(false);
                }
                else if (sit->params[1] == "ImRad::HBox::Stretch") { //compatibility
                    size_x = 1.0f;
                    size_x.stretch(true);
                }
                else if (!sit->params[1].compare(0, 21, "ImRad::HBox::Stretch(")) {
                    size_x.set_from_arg(sit->params[1].substr(21, sit->params[1].size() - 21 - 1));
                    size_x.stretch(true);
                }
                else {
                    size_x.set_from_arg(sit->params[1]);
                    size_x.stretch(false);
                }
            }
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SameLine")
        {
            sameLine = true;
            spacing = 1;
            if (sit->params.size() == 2)
                spacing.set_from_arg(sit->params[1]);
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::Spacing") //compatibility
        {
            spacing = 1;
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImRad::Spacing")
        {
            if (sit->params.size()) {
                spacing.set_from_arg(sit->params[0]);
                Layout l = GetLayout(ctx.parents[ctx.parents.size() - 2]);
                int defSpacing = (l.flags & Layout::Topmost) ? 0 : 1; //default ImGui spacing
                spacing += defSpacing;
            }
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::Indent")
        {
            if (sit->params.size())
                indent.set_from_arg(sit->params[0]);
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextItemAllowOverlap")
        {
            allowOverlap = true;
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::BeginDisabled")
        {
            if (sit->params.size())
                disabled.set_from_arg(sit->params[0]);
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushItemFlag")
        {
            if (sit->params.size() >= 2) {
                if (sit->params[0] == "ImGuiItemFlags_NoNav")
                    tabStop.set_from_arg(sit->params[0] != "false" ? "false" : "true");
                else
                    DoImport(sit, ctx);
            }
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushTabStop") //compatibility only
        {
            if (sit->params.size())
                tabStop.set_from_arg(sit->params[0]);
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushFont")
        {
            if (sit->params.size())
                style_font.set_from_arg(sit->params[0]);
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
        {
            if (sit->params.size() == 2 && sit->params[0] == "ImGuiCol_Text")
                style_text.set_from_arg(sit->params[1]);
            else if (sit->params.size() == 2 && sit->params[0] == "ImGuiCol_Border")
                style_border.set_from_arg(sit->params[1]);
            else if (sit->params.size() == 2 && sit->params[0] == "ImGuiCol_FrameBg")
                style_frameBg.set_from_arg(sit->params[1]);
            else if (sit->params.size() == 2 && sit->params[0] == "ImGuiCol_Button")
                style_button.set_from_arg(sit->params[1]);
            else if (sit->params.size() == 2 && sit->params[0] == "ImGuiCol_ButtonHovered")
                style_buttonHovered.set_from_arg(sit->params[1]);
            else if (sit->params.size() == 2 && sit->params[0] == "ImGuiCol_ButtonActive")
                style_buttonActive.set_from_arg(sit->params[1]);
            else
                DoImport(sit, ctx);
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar")
        {
            if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_FrameBorderSize")
                style_frameBorderSize.set_from_arg(sit->params[1]);
            else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_FrameRounding")
                style_frameRounding.set_from_arg(sit->params[1]);
            else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_FramePadding")
                style_framePadding.set_from_arg(sit->params[1]);
            else
                DoImport(sit, ctx);
        }
        else if (sit->kind == cpp::IfCallThenCall &&
            sit->callee == "ImGui::IsWindowAppearing" && sit->callee2 == "ImGui::SetKeyboardFocusHere")
        {
            initialFocus = true;
        }
        else if (sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::IsItemHovered")
        {
            if (sit->callee2 == "ImGui::SetTooltip")
                tooltip.set_from_arg(sit->params2[0]);
            else if (sit->callee2 == "ImGui::SetMouseCursor")
                cursor.set_from_arg(sit->params2[0]);
            else
                onItemHovered.set_from_arg(sit->callee2);
        }
        else if (sit->kind == cpp::IfCallThenCall && sit->callee == "ImRad::IsItemContextMenuClicked")
        {
            if (sit->callee2 == "ImRad::OpenWindowPopup")
                contextMenu.set_from_arg(sit->params2[0]);
            else
                onItemContextMenuClicked.set_from_arg(sit->callee2);
        }
        else if (sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::IsItemClicked")
        {
            onItemClicked.set_from_arg(sit->callee2);
        }
        else if (sit->kind == cpp::IfCallThenCall && sit->callee == "ImRad::IsItemDoubleClicked")
        {
            onItemDoubleClicked.set_from_arg(sit->callee2);
        }
        else if (sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::IsItemFocused")
        {
            onItemFocused.set_from_arg(sit->callee2);
        }
        else if (sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::IsItemActivated")
        {
            onItemActivated.set_from_arg(sit->callee2);
        }
        else if (sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::IsItemDeactivated")
        {
            onItemDeactivated.set_from_arg(sit->callee2);
        }
        else if (sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::IsItemDeactivatedAfterEdit")
        {
            onItemDeactivatedAfterEdit.set_from_arg(sit->callee2);
        }
        else
        {
            DoImport(sit, ctx);
        }

        ++sit;

        if (ifBlockIt != cpp::stmt_iterator() && sit != cpp::stmt_iterator())
        {
            if (sit->kind == cpp::Comment && sit->line == "//visible")
            {
                visible.set_from_arg(ifBlockIt->cond);
            }
            else if (sit->kind == cpp::Comment && sit->line == "//forceFocus")
            {
                forceFocus.set_from_arg(ifBlockIt->cond);
                ignoreLevel = sit->level;
            }
            else if (ifBlockIt->kind == cpp::IfCallBlock && ifBlockIt->callee == "ImGui::BeginDragDropSource")
            {
                if (sit->kind == cpp::CallExpr)
                    onDragDropSource.set_from_arg(sit->callee);
                ignoreLevel = sit->level;
            }
            else if (ifBlockIt->kind == cpp::IfCallBlock && ifBlockIt->callee == "ImGui::BeginDragDropTarget")
            {
                if (sit->kind == cpp::CallExpr)
                    onDragDropTarget.set_from_arg(sit->callee);
                ignoreLevel = sit->level;
            }
            else if (ifBlockIt->kind == cpp::IfCallBlock && ifBlockIt->callee == "ImRad::IsItemLongPressed")
            {
                if (sit->kind == cpp::CallExpr)
                    onItemLongPressed.set_from_arg(sit->callee);
                ignoreLevel = sit->level;
            }
            else
            {
                DoImport(ifBlockIt, ctx);
            }
        }
        if (screenPosIt != cpp::stmt_iterator() && sit != cpp::stmt_iterator())
        {
            if (sit->kind == cpp::Comment && sit->line == "//overlayPos" && sit->params.size())
            {
                hasPos = 0;
                auto pos = cpp::parse_size(sit->params[0]);
                int tmp;
                char buf[101];
                if (!pos.first.compare(0, 33, "ImRad::GetParentInnerRect().Min.x")) {
                    hasPos |= ImRad::AlignLeft;
                    pos_x.set_from_arg(pos.first.substr(33));
                }
                else if (!pos.first.compare(0, 33, "ImRad::GetParentInnerRect().Max.x")) {
                    hasPos |= ImRad::AlignRight;
                    pos_x.set_from_arg(pos.first.substr(33));
                }
                else if (!pos.first.compare(0, 41, "ImRad::GetParentInnerRect().GetCenter().x")) {
                    hasPos |= ImRad::AlignHCenter;
                    pos_x.set_from_arg(pos.first.substr(41));
                }
                else if (std::sscanf(pos.first.c_str(), (TMP_LAST_ITEM_VAR + "%d.Rect.Min.x%100s").c_str(), &tmp, buf) == 2) {
                    hasPos |= ImRad::AlignLeft;
                    pos_x.set_from_arg(buf);
                }
                else if (std::sscanf(pos.first.c_str(), (TMP_LAST_ITEM_VAR + "%d.Rect.Max.x%100s").c_str(), &tmp, buf) == 2) {
                    hasPos |= ImRad::AlignRight;
                    pos_x.set_from_arg(buf);
                }
                else if (std::sscanf(pos.first.c_str(), (TMP_LAST_ITEM_VAR + "%d.Rect.GetCenter().x%100s").c_str(), &tmp, buf) == 2) {
                    hasPos |= ImRad::AlignHCenter;
                    pos_x.set_from_arg(buf);
                }
                else if (!pos.first.compare(0, 42, "ImGui::GetCurrentWindow()->InnerRect.Max.x")) { //compatibility
                    hasPos |= ImRad::AlignRight;
                    pos_x.set_from_arg(pos.first.substr(42));
                }
                else if (!pos.first.compare(0, 47, "0.5f*ImGui::GetCurrentWindow()->InnerRect.Max.x")) { //compatibility
                    hasPos |= ImRad::AlignHCenter;
                    pos_x.set_from_arg(pos.first.substr(47));
                }
                else {
                    hasPos |= ImRad::AlignLeft;
                    pos_x.set_from_arg(pos.first);
                }

                if (!pos.second.compare(0, 33, "ImRad::GetParentInnerRect().Min.y")) {
                    hasPos |= ImRad::AlignTop;
                    pos_y.set_from_arg(pos.second.substr(33));
                }
                else if (!pos.second.compare(0, 33, "ImRad::GetParentInnerRect().Max.y")) {
                    hasPos |= ImRad::AlignBottom;
                    pos_y.set_from_arg(pos.second.substr(33));
                }
                else if (!pos.second.compare(0, 41, "ImRad::GetParentInnerRect().GetCenter().y")) {
                    hasPos |= ImRad::AlignVCenter;
                    pos_y.set_from_arg(pos.second.substr(41));
                }
                else if (std::sscanf(pos.second.c_str(), (TMP_LAST_ITEM_VAR + "%d.Rect.Min.y%100s").c_str(), &tmp, buf) == 2) {
                    hasPos |= ImRad::AlignTop;
                    pos_y.set_from_arg(buf);
                }
                else if (std::sscanf(pos.second.c_str(), (TMP_LAST_ITEM_VAR + "%d.Rect.Max.y%100s").c_str(), &tmp, buf) == 2) {
                    hasPos |= ImRad::AlignBottom;
                    pos_y.set_from_arg(buf);
                }
                else if (std::sscanf(pos.second.c_str(), (TMP_LAST_ITEM_VAR + "%d.Rect.GetCenter().y%100s").c_str(), &tmp, buf) == 2) {
                    hasPos |= ImRad::AlignVCenter;
                    pos_y.set_from_arg(buf);
                }
                else if (!pos.second.compare(0, 42, "ImGui::GetCurrentWindow()->InnerRect.Max.y")) { //compatibility
                    hasPos |= ImRad::AlignBottom;
                    pos_y.set_from_arg(pos.second.substr(42));
                }
                else if (!pos.second.compare(0, 47, "0.5f*ImGui::GetCurrentWindow()->InnerRect.Max.y")) { //compatibility
                    hasPos |= ImRad::AlignVCenter;
                    pos_y.set_from_arg(pos.second.substr(47));
                }
                else {
                    hasPos |= ImRad::AlignTop;
                    pos_y.set_from_arg(pos.second);
                }
            }
            else
            {
                DoImport(screenPosIt, ctx);
            }
        }
    }

    if (spacing < 0)
    {
        Layout l = GetLayout(ctx.parents[ctx.parents.size() - 2]);
        spacing = (l.flags & Layout::Topmost) ? 0 : 1; //default ImGui spacing
    }

    ctx.parents.pop_back();
}

std::vector<UINode::Prop>
Widget::Properties()
{
    //some properties are left up to the subclass
    std::vector<UINode::Prop> props{
        { "common.contextMenu", &contextMenu },
        { "common.cursor", &cursor },
        { "common.disabled", &disabled },
        { "common.forceFocus##1", &forceFocus },
        { "common.initialFocus", &initialFocus },
        { "common.tabStop", &tabStop },
        { "common.tooltip", &tooltip },
        { "common.visible", &visible },
    };
    if ((Behavior() & (HasSizeX | HasSizeY)) == (HasSizeX | HasSizeY))
    {
        props.insert(props.end(), {
            { "layout.size.size", nullptr },
            { "layout.size.size_x", &size_x },
            { "layout.size.size_y", &size_y }
        });
    }
    else
    {
        //here we insert property with same ID as if both HasizeX|Y is set
        //so multiselection edit works
        //it will draw a category arrow but it's not that bad
        if (Behavior() & HasSizeX)
        {
            props.insert(props.end(),
                { "layout.size.size_x", &size_x }
            );
        }
        if (Behavior() & HasSizeY)
        {
            props.insert(props.end(),
                { "layout.size.size_y", &size_y }
            );
        }
    }
    if (!(Behavior() & NoOverlayPos))
    {
        //##1 to disable editing multiple widgets at once (changes hierarchy)
        props.insert(props.end(), {
            { "layout.overlayPos.hasPos##1", &hasPos },
            { "layout.overlayPos.pos_x##1", &pos_x },
            { "layout.overlayPos.pos_y##1", &pos_y },
            });
    }
    if (Behavior() & SnapSides)
    {
        //##1 because indent, sameLine work differently for first in a row and other widgets
        props.insert(props.end(), {
            { "layout.indent##1", &indent },
            { sameLine && !nextColumn ? "layout.spacing_x" : "layout.spacing_y", &spacing },
            { "layout.sameLine##1", &sameLine },
            { "layout.nextColumn##1", &nextColumn },
            { "layout.allowOverlap", &allowOverlap },
            });
    }

    return props;
}

bool Widget::PropertyUI(int i, UIContext& ctx)
{
    bool snapSides = Behavior() & SnapSides;
    bool sizeX = Behavior() & HasSizeX;
    bool sizeY = Behavior() & HasSizeY;
    bool overlayPos = !(Behavior() & NoOverlayPos);
    bool parentItem = ctx.parents[ctx.parents.size() - 2]->Behavior() & SnapItemInterior;
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
    {
        ImGui::BeginDisabled(Behavior() & NoContextMenu);
        ImGui::Text("contextMenu");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectValContextMenu(&contextMenu, ctx);
        ImGui::EndDisabled();
        return changed;
    }
    case 1:
        ImGui::Text("cursor");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = cursor != Defaults().cursor ? InputDirectVal_Modified : 0;
        changed = InputDirectValEnum(&cursor, fl, ctx);
        return changed;
    case 2:
        ImGui::Text("disabled");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = disabled != Defaults().disabled ? InputBindable_Modified : 0;
        changed = InputBindable(&disabled, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("disabled", &disabled, ctx);
        return changed;
    case 3:
        ImGui::Text("forceFocus");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = forceFocus != Defaults().forceFocus ? InputBindable_Modified : 0;
        changed = InputBindable(&forceFocus, fl | InputBindable_ShowVariables | InputBindable_ShowNone, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("forceFocus", &forceFocus, ctx);
        break;
    case 4:
        ImGui::Text("initialFocus");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = initialFocus != Defaults().initialFocus ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&initialFocus, fl, ctx);
        break;
    case 5:
        ImGui::Text("tabStop");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = tabStop != Defaults().tabStop ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&tabStop, fl, ctx);
        return changed;
    case 6:
        ImGui::Text("tooltip");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&tooltip, InputBindable_MultilineEdit, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("tooltip", &tooltip, ctx);
        return changed;
    case 7:
        ImGui::Text("visible");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = visible != Defaults().visible ? InputBindable_Modified : 0;
        changed = InputBindable(&visible, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("visible", &visible, ctx);
        return changed;
    }
    i -= 8;

    if (!i && (sizeX && sizeY))
    {
        ImGui::Text("size");
        ImGui::TableNextColumn();
        bool modified = size_x != Defaults().size_x || size_y != Defaults().size_y;
        ImGui::PushFont(modified ? ctx.pgbFont : ctx.pgFont);
        ImGui::Text("%s", (size_x.to_arg("") + ", " + size_y.to_arg("")).c_str());
        ImGui::PopFont();
        return changed;
    }
    i -= sizeX && sizeY;

    if (!i && sizeX)
    {
        ImGui::Text("size_x");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = size_x != Defaults().size_x ? InputBindable_Modified : 0;
        if (parentItem)
            fl |= InputBindable_StretchButtonDisabled;
        changed = InputBindable(&size_x, fl | InputBindable_StretchButton, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("size_x", &size_x, ctx);
        return changed;
    }
    i -= sizeX;

    if (!i && sizeY)
    {
        ImGui::Text("size_y");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = size_y != Defaults().size_y ? InputBindable_Modified : 0;
        if (parentItem)
            fl |= InputBindable_StretchButtonDisabled;
        changed = InputBindable(&size_y, fl | InputBindable_StretchButton, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("size_y", &size_y, ctx);
        return changed;
    }
    i -= sizeY;

    if (overlayPos)
    {
        switch (i)
        {
        case 0:
            ImGui::BeginDisabled(Behavior() & NoOverlayPos);
            ImGui::Text("overlayPos");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
            fl = hasPos != Defaults().hasPos ? InputDirectVal_Modified : 0;
            changed = InputDirectValEnum(&hasPos, fl, ctx);
            if (changed)
            {
                //prevent widget going out of window
                ImVec2 size{ size_x.eval_px(ImGuiAxis_X, ctx), size_y.eval_px(ImGuiAxis_Y, ctx) };
                if ((hasPos & ImRad::AlignRight) && pos_x.has_value() && pos_x.value() >= 0)
                    pos_x = -20;
                if ((hasPos & ImRad::AlignBottom) && pos_y.has_value() && pos_y.value() >= 0)
                    pos_y = -20;
                if ((hasPos & ImRad::AlignLeft) && pos_x.has_value() && pos_x.value() <= -size.x)
                    pos_x = -size.x + 20;
                if ((hasPos & ImRad::AlignTop) && pos_y.has_value() && pos_y.value() <= -size.y)
                    pos_y = -size.y + 20;

                //adjust parent index
                bool moveChild = false;
                auto pinfo = ctx.root->FindChild(this);
                UINode* parent = pinfo->first;
                int idx = pinfo->second;
                if (hasPos)
                {
                    for (size_t i = idx + 1; i < parent->children.size(); ++i)
                        if (!parent->children[i]->hasPos)
                            moveChild = true;
                }
                else
                {
                    for (int i = idx - 1; i >= 0; --i)
                        if (parent->children[i]->hasPos)
                            moveChild = true;
                }
                if (moveChild)
                {
                    std::unique_ptr<Widget> child = std::move(parent->children[idx]);
                    parent->children.erase(parent->children.begin() + idx);
                    if (hasPos)
                        parent->children.push_back(std::move(child));
                    else
                        parent->children.insert(parent->children.begin(), std::move(child));
                }
            }
            ImGui::EndDisabled();
            return changed;
        case 1:
            ImGui::BeginDisabled(!hasPos);
            ImGui::Text("pos_x");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
            fl = hasPos ? InputBindable_Modified : 0;
            changed = InputBindable(&pos_x, fl, ctx);
            ImGui::SameLine(0, 0);
            changed |= BindingButton("pos_x", &pos_x, ctx);
            ImGui::EndDisabled();
            return changed;
        case 2:
            ImGui::BeginDisabled(!hasPos);
            ImGui::Text("pos_y");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
            fl = hasPos ? InputBindable_Modified : 0;
            changed = InputBindable(&pos_y, fl, ctx);
            ImGui::SameLine(0, 0);
            changed |= BindingButton("pos_y", &pos_y, ctx);
            ImGui::EndDisabled();
            return changed;
        }
        i -= 3;
    }

    switch (i)
    {
    case 0:
        ImGui::BeginDisabled(!snapSides || sameLine);
        ImGui::Text("indent");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = indent != Defaults().indent ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&indent, fl, ctx);
        ImGui::EndDisabled();
        return changed;
    case 1:
        ImGui::BeginDisabled(!snapSides);
        ImGui::Text(sameLine && !nextColumn ? "spacing_x" : "spacing_y");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = spacing != Defaults().spacing ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&spacing, fl, ctx);
        if (ImGui::IsItemDeactivatedAfterEdit() && spacing < 0)
        {
            changed = true;
            spacing = 0;
        }
        ImGui::EndDisabled();
        return changed;
    case 2:
        ImGui::BeginDisabled(!snapSides || nextColumn);
        ImGui::Text("sameLine");
        ImGui::TableNextColumn();
        fl = sameLine != Defaults().sameLine ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&sameLine, fl, ctx);
        if (changed && sameLine)
            indent = 0;
        ImGui::EndDisabled();
        return changed;
    case 3:
    {
        ImGui::BeginDisabled(!snapSides);
        ImGui::Text("nextColumn");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = nextColumn != Defaults().nextColumn ? InputDirectVal_Modified : 0;
        direct_val<int> tmp = nextColumn;
        if (InputDirectVal(&tmp, fl, ctx) && tmp >= 0) {
            changed = true;
            nextColumn = tmp;
            if (nextColumn) {
                sameLine = false;
                spacing = 0;
            }
        }
        ImGui::EndDisabled();
        return changed;
    }
    case 4:
        ImGui::BeginDisabled(!snapSides);
        ImGui::Text("allowOverlap");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = allowOverlap != Defaults().allowOverlap ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&allowOverlap, fl, ctx);
        ImGui::EndDisabled();
        return changed;
    }

    return false;
}

std::vector<UINode::Prop>
Widget::Events()
{
    return {
        { "item.activated", &onItemActivated },
        { "item.clicked", &onItemClicked },
        { "item.contextMenuClicked", &onItemContextMenuClicked },
        { "item.deactivated", &onItemDeactivated },
        { "item.deactivatedAfterEdit", &onItemDeactivatedAfterEdit },
        { "item.doubleClicked", &onItemDoubleClicked },
        { "item.focused", &onItemFocused },
        { "item.hovered", &onItemHovered },
        { "dragDrop.source", &onDragDropSource },
        { "dragDrop.target", &onDragDropTarget },
    };
}

bool Widget::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("Activated");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Activated", &onItemActivated, 0, ctx);
        break;
    case 1:
        ImGui::Text("Clicked");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Clicked", &onItemClicked, 0, ctx);
        break;
    case 2:
        ImGui::BeginDisabled(Behavior() & NoContextMenu);
        ImGui::Text("ContextMenuClicked");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_ContextMenuClicked", &onItemContextMenuClicked, 0, ctx);
        ImGui::EndDisabled();
        break;
    case 3:
        ImGui::Text("Deactivated");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Deactivated", &onItemDeactivated, 0, ctx);
        break;
    case 4:
        ImGui::Text("DeactivatedAfterEdit");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_DeactivatedAfterEdit", &onItemDeactivatedAfterEdit, 0, ctx);
        break;
    case 5:
        ImGui::Text("DoubleClicked");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_DoubleClicked", &onItemDoubleClicked, 0, ctx);
        break;
    case 6:
        ImGui::Text("Focused");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Focused", &onItemFocused, 0, ctx);
        break;
    case 7:
        ImGui::Text("Hovered");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Hovered", &onItemHovered, 0, ctx);
        break;
    case 8:
        ImGui::Text("Source");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_DragDropSource", &onDragDropSource, 0, ctx);
        break;
    case 9:
        ImGui::Text("Target");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_DragDropTarget", &onDragDropTarget, 0, ctx);
        break;
    default:
        return false;
    }
    return changed;
}

void Widget::TreeUI(UIContext& ctx)
{
    std::string label, typeLabel;
    const auto props = Properties();
    for (const auto& p : props) {
        if (p.kbdInput && p.property->c_str()) {
            label = PrepareString(p.property->c_str()).label;
            for (size_t i = 0; i < label.size(); ++i)
                if (label[i] == '\n') {
                    label[i] = ' ';
                }
        }
    }
    if (label.empty())
        typeLabel = GetTypeName();
    std::string icon = ICON_FA_BARS;
    if (GetIcon()) {
        icon = GetIcon();
    }
    icon += "##" + std::to_string((unsigned long long)this);
    std::string suff;
    if (hasPos)
        suff += "P";
    else {
        if (sameLine)
            suff += "L";
        if (nextColumn)
            suff += "C";
    }

    bool selected = stx::count(ctx.selected, this) || ctx.snapParent == this;
    //align icon
    float sp = ImGui::GetFontSize() * 1.4f - ImGui::CalcTextSize(icon.c_str(), 0, true).x;
    ImGui::Dummy({ sp, 0 });
    ImGui::SameLine(0, 0);

    ctx.parents.push_back(this);

    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    //we keep all items open, OpenOnDoubleClick is to block flickering
    int flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (children.empty())
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (ImGui::TreeNodeEx(icon.c_str(), flags))
    {
        if (ImGui::IsItemClicked())
        {
            if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
                toggle(ctx.selected, this);
            else
                ctx.selected = { this };
        }
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[selected ? ImGuiCol_ButtonHovered : ImGuiCol_Text]);
        ImGui::SameLine();
        if (label != "") {
            ImGui::Text("\"");
            ImGui::SameLine(0, 0);
            ImGui::PushFont(!IsAscii(label) ? ctx.defaultStyleFont : ImGui::GetFont());
            ImGui::Text("%s", label.c_str());
            ImGui::PopFont();
            ImGui::SameLine(0, 0);
            ImGui::Text("\"");
        }
        else {
            ImGui::Text("%s", typeLabel.c_str());
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("%s", suff.c_str());

        if (!itemCount.empty())
        {
            std::string icon = ICON_FA_RETWEET; // SHARE_NODES;
            icon += "##IC" + std::to_string((unsigned long long)this);
            std::string label = itemCount.index_name_or(ctx.codeGen->FOR_VAR_NAME);
            label += " = 0.." + itemCount.limit.to_arg();
            float sp = ImGui::GetFontSize() * 1.4f - ImGui::CalcTextSize(icon.c_str(), 0, true).x;
            ImGui::Dummy({ sp, 0 });
            ImGui::SameLine(0, 0);
            bool selected = ctx.mode == UIContext::SnapInsert && ctx.snapParent == this;
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            //we keep all items open, OpenOnDoubleClick is to block flickering
            int flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick;
            ImGui::SetNextItemOpen(true);
            if (ImGui::TreeNodeEx(icon.c_str(), flags))
            {
                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[selected ? ImGuiCol_ButtonHovered : ImGuiCol_TextDisabled]);
                ImGui::SameLine();
                ImGui::Text("%s", label.c_str());
                ImGui::PopStyleColor();
                for (auto& child : child_iterator(children, false))
                    child->TreeUI(ctx);
                ImGui::TreePop();
            }
            else {
                ImGui::PopStyleColor();
            }
            for (auto& child : child_iterator(children, true))
                child->TreeUI(ctx);
        }
        else
        {
            for (auto& child : children)
                child->TreeUI(ctx);
        }
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }

    ctx.parents.pop_back();
}

//----------------------------------------------------

Spacer::Spacer(UIContext& ctx)
{
    size_x = size_y = 20;
}

std::unique_ptr<Widget> Spacer::Clone(UIContext& ctx)
{
    return std::unique_ptr<Widget>(new Spacer(*this));
}

ImDrawList* Spacer::DoDraw(UIContext& ctx)
{
    ImVec2 size { size_x.eval_px(ImGuiAxis_X, ctx), size_y.eval_px(ImGuiAxis_Y, ctx) };
    if (!size.x) //compatibility
        size.x = 20;
    if (!size.y) //compatibility
        size.y = 20;

    ImRad::Dummy(size);

    if (!ctx.beingResized)
    {
        ImVec2 p = ImGui::GetItemRectMin();
        ImVec2 r = ImGui::GetItemRectSize();
        auto* dl = ImGui::GetWindowDrawList();
        ImU32 clr = 0x007f7f7f; //reduces contrast
        clr |= int(0x5f * ImGui::GetStyle().Alpha) << 24;
        //dl->AddRect(p, p + size, clr);

        float th = 2;
        ImVec2 xuv{ std::round(r.x / 5 / ctx.zoomFactor), 0 };
        ImVec2 yuv{ 0, std::round(r.y / 5 / ctx.zoomFactor) };

        dl->PushTextureID(ctx.dashTexId);
        dl->PrimReserve(4*6, 4*4);
        dl->PrimRectUV(p, { p.x + r.x, p.y + th }, { 0, 0 }, xuv, clr);
        dl->PrimRectUV({ p.x, p.y + r.y - th }, p + r, { 0, 0 }, xuv, clr);
        dl->PrimRectUV(p, { p.x + th, p.y + r.y }, { 0, 0 }, yuv, clr);
        dl->PrimRectUV({ p.x + r.x - th, p.y }, p + r, { 0, 0 }, yuv, clr);
        dl->PopTextureID();
    }

    return ImGui::GetWindowDrawList();
}

void Spacer::DoExport(std::ostream& os, UIContext& ctx)
{
    os << ctx.ind << "ImRad::Dummy({ " << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0])
        << ", " << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]) << " });\n";
}

void Spacer::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::Kind::CallExpr && sit->callee == "ImRad::Dummy")
    {
        if (sit->params.size()) {
            auto size = cpp::parse_size(sit->params[0]);
            size_x.set_from_arg(size.first);
            size_y.set_from_arg(size.second);
        }
    }
}

std::vector<UINode::Prop>
Spacer::Properties()
{
    return Widget::Properties();
}

bool Spacer::PropertyUI(int i, UIContext& ctx)
{
    return Widget::PropertyUI(i, ctx);
}

//----------------------------------------------------

Separator::Separator(UIContext& ctx)
{
}

std::unique_ptr<Widget> Separator::Clone(UIContext& ctx)
{
    return std::unique_ptr<Widget>(new Separator(*this));
}

ImDrawList* Separator::DoDraw(UIContext& ctx)
{
    ImRad::IgnoreWindowPaddingData data;
    if (!style_outerPadding && !sameLine)
        ImRad::PushIgnoreWindowPadding(nullptr, &data);

    if (!label.empty())
        ImGui::SeparatorText(DRAW_STR(label));
    else if (style_thickness.has_value())
        ImGui::SeparatorEx(sameLine ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal, style_thickness);
    else
        ImGui::SeparatorEx(sameLine ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal);

    if (!style_outerPadding && !sameLine)
        ImRad::PopIgnoreWindowPadding(data);

    return ImGui::GetWindowDrawList();
}

void Separator::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    const float sp = 3;
    cached_pos = p1;
    cached_size = ImGui::GetItemRectSize();
    if (sameLine) {
        cached_pos.x -= sp;
        cached_size.x += 2 * sp;
    }
    else {
        //cached_pos.x -= ImGui::GetStyle().ItemSpacing.x;
        cached_pos.y -= sp;
        cached_size.y += 2 * sp;
    }
}

void Separator::DoExport(std::ostream& os, UIContext& ctx)
{
    std::string datavar;
    if (!style_outerPadding && !sameLine)
    {
        datavar = "_data" + std::to_string(ctx.varCounter);
        ++ctx.varCounter;
        os << ctx.ind << "ImRad::IgnoreWindowPaddingData " << datavar << ";\n";
        os << ctx.ind << "ImRad::PushIgnoreWindowPadding(nullptr, &" << datavar << ")\n";
    }

    if (label.empty())
    {
        os << ctx.ind << "ImGui::SeparatorEx("
            << (sameLine ? "ImGuiSeparatorFlags_Vertical" : "ImGuiSeparatorFlags_Horizontal");
        if (style_thickness.has_value())
            os << ", " << style_thickness.to_arg(ctx.unit);
        os << ");\n";
    }
    else {
        os << ctx.ind << "ImGui::SeparatorText(" << label.to_arg();
        if (style_thickness.has_value())
            os << ", " << style_thickness.to_arg(ctx.unit);
        os << ");\n";
    }

    if (!style_outerPadding && !sameLine)
    {
        os << ctx.ind << "ImRad::PopIgnoreWindowPadding(" << datavar << ");\n";
    }
}

void Separator::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SeparatorText")
    {
        if (sit->params.size())
            label.set_from_arg(sit->params[0]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SeparatorEx")
    {
        if (sit->params.size() >= 2)
            style_thickness.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::CallExpr &&
        (sit->callee == "ImRad::PushIgnoreWindowPadding" || sit->callee == "ImGui::PushClipRect")) //PushClipRect for compatibility
    {
        style_outerPadding = false;
    }
}

std::vector<UINode::Prop>
Separator::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.thickness", &style_thickness },
        { "appearance.outer_padding", &style_outerPadding },
        { "behavior.label", &label, true }
        });
    return props;
}

bool Separator::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("thickness");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_thickness, ctx);
        break;
    case 1:
        ImGui::Text("outerPadding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_outerPadding, 0, ctx);
        break;
    case 2:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&label, 0, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("label", &label, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 3, ctx);
    }
    return changed;
}

//----------------------------------------------------

Text::Text(UIContext& ctx)
{
}

std::unique_ptr<Widget> Text::Clone(UIContext& ctx)
{
    return std::unique_ptr<Widget>(new Text(*this));
}

ImDrawList* Text::DoDraw(UIContext& ctx)
{
    if (alignToFrame)
        ImGui::AlignTextToFramePadding();

    if (wrap)
    {
        float wrapWidth = 0; //let ImGui calculate it from window.WorkRect
        auto* parent = dynamic_cast<Widget*>(ctx.parents[ctx.parents.size() - 2]);
        if (parent->Behavior() & SnapItemInterior)
            //wrapWidth = parent->cached_pos.x + parent->cached_size.x - pad - ImGui::GetWindowPos().x;
            wrapWidth = ImGui::GetCurrentWindow()->ClipRect.Max.x - ImGui::GetWindowPos().x;
        ImGui::PushTextWrapPos(wrapWidth);
    }

    auto ps = PrepareString(text.value());
    
    if (link)
        ImGui::TextLink(ps.label.c_str());
    else
        ImGui::TextUnformatted(ps.label.c_str());
        
    DrawTextArgs(ps, ctx);

    if (wrap)
        ImGui::PopTextWrapPos();

    return ImGui::GetWindowDrawList();
}

void Text::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    cached_pos = ImGui::GetItemRectMin();
    cached_size = ImGui::GetItemRectSize();
    /*
    cached_pos = p1;
    cached_size = ImGui::GetItemRectSize();
    ImGui::SameLine(0, 0);
    cached_size.x = ImGui::GetCursorScreenPos().x - p1.x;
    ImGui::NewLine();*/
}

void Text::DoExport(std::ostream& os, UIContext& ctx)
{
    if (PrepareString(text.value()).error)
        PushError(ctx, "text is formatted wrongly");

    if (alignToFrame)
        os << ctx.ind << "ImGui::AlignTextToFramePadding();\n";

    if (wrap)
    {
        auto* parent = ctx.parents[ctx.parents.size() - 2];
        if (parent->Behavior() & SnapItemInterior)
        {
            os << ctx.ind << "ImGui::PushTextWrapPos(";
            os << "ImGui::GetCurrentWindow()->ClipRect.x - ImGui::GetWindowPos().x";
            os << ");\n";
        }
        else
        {
            os << ctx.ind << "ImGui::PushTextWrapPos(0);\n";
        }
    }

    if (link) 
    {
        os << ctx.ind;
        if (!onChange.empty())
            os << "if (";
        os << "ImGui::TextLink(" << text.to_arg() << ")";
        if (!onChange.empty()) 
        {
            ctx.ind_up();
            os << ")\n" << ctx.ind << onChange.to_arg() << "();\n";
            ctx.ind_down();
        }
        else
            os << ";\n";
    }
    else
    {
        os << ctx.ind << "ImGui::TextUnformatted(" << text.to_arg() << ");\n";
    }

    if (wrap)
        os << ctx.ind << "ImGui::PopTextWrapPos();\n";
}

void Text::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushTextWrapPos")
    {
        wrap = true;
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::AlignTextToFramePadding")
    {
        alignToFrame = true;
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::TextUnformatted")
    {
        if (sit->params.size() >= 1) {
            text.set_from_arg(sit->params[0]);
            if (text.value() == cpp::INVALID_TEXT)
                PushError(ctx, "unable to parse text");
        }
    }
    else if ((sit->kind == cpp::CallExpr || sit->kind == cpp::IfCallThenCall) && 
        sit->callee == "ImGui::TextLink")
    {
        link = true;
        if (sit->params.size() >= 1) {
            text.set_from_arg(sit->params[0]);
            if (text.value() == cpp::INVALID_TEXT)
                PushError(ctx, "unable to parse text");
        }
        if (sit->kind == cpp::IfCallThenCall)
            onChange.set_from_arg(sit->callee2);
    }
}

std::vector<UINode::Prop>
Text::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.color", &style_text },
        { "appearance.font", &style_font },
        { "appearance.alignToFramePadding", &alignToFrame },
        { "behavior.text", &text, true },
        { "behavior.wrap##text", &wrap },
        { "behavior.link##text", &link }
    });
    return props;
}

bool Text::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("color");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("color", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 2:
        ImGui::Text("alignToFramePadding");
        ImGui::TableNextColumn();
        fl = alignToFrame != Defaults().alignToFrame ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&alignToFrame, fl, ctx);
        break;
    case 3:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&text, InputBindable_MultilineEdit, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("text", &text, ctx);
        break;
    case 4:
        ImGui::Text("wrapped");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = wrap != Defaults().wrap ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&wrap, fl, ctx);
        break;
    case 5:
        ImGui::Text("link");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = link != Defaults().link ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&link, fl, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 6, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
Text::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "text.change", &onChange },
        });
    return props;
}

bool Text::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::BeginDisabled(!link);
        ImGui::Text("Change");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Change", &onChange, 0, ctx);
        ImGui::EndDisabled();
        break;
    default:
        return Widget::EventUI(i - 1, ctx);
    }
    return changed;
}

//----------------------------------------------------

Selectable::Selectable(UIContext& ctx)
{
    flags.add$(ImGuiSelectableFlags_AllowDoubleClick);
    flags.add$(ImGuiSelectableFlags_NoAutoClosePopups);
    flags.add$(ImGuiSelectableFlags_NoPadWithHalfSpacing);
    flags.add$(ImGuiSelectableFlags_SpanAllColumns);

    modalResult.add(" ", ImRad::None);
    modalResult.add$(ImRad::Ok);
    modalResult.add$(ImRad::Cancel);
    modalResult.add$(ImRad::Yes);
    modalResult.add$(ImRad::YesToAll);
    modalResult.add$(ImRad::No);
    modalResult.add$(ImRad::NoToAll);
    modalResult.add$(ImRad::Apply);
    modalResult.add$(ImRad::Discard);
    modalResult.add$(ImRad::Help);
    modalResult.add$(ImRad::Reset);
    modalResult.add$(ImRad::Abort);
    modalResult.add$(ImRad::Retry);
    modalResult.add$(ImRad::Ignore);

    horizAlignment.add("AlignLeft", ImRad::AlignLeft);
    horizAlignment.add("AlignHCenter", ImRad::AlignHCenter);
    horizAlignment.add("AlignRight", ImRad::AlignRight);
    vertAlignment.add("AlignTop", ImRad::AlignTop);
    vertAlignment.add("AlignVCenter", ImRad::AlignVCenter);
    vertAlignment.add("AlignBottom", ImRad::AlignBottom);
}

std::unique_ptr<Widget> Selectable::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<Selectable>(*this);
    if (ctx.createVars && selected.has_single_variable()) {
        sel->selected.set_from_arg(ctx.codeGen->CreateVar("bool", "false", CppGen::Var::Interface));
    }
    return sel;
}

ImDrawList* Selectable::DoDraw(UIContext& ctx)
{
    ImVec2 alignment(0, 0);
    if (horizAlignment == ImRad::AlignHCenter)
        alignment.x = 0.5f;
    else if (horizAlignment == ImRad::AlignRight)
        alignment.x = 1.f;
    if (vertAlignment == ImRad::AlignVCenter)
        alignment.y = 0.5f;
    else if (vertAlignment == ImRad::AlignBottom)
        alignment.y = 1.f;
    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, alignment);

    if (!style_header.empty())
        ImGui::PushStyleColor(ImGuiCol_Header, style_header.eval(ImGuiCol_Header, ctx));

    if (readOnly)
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true); //won't affect text color

    if (alignToFrame)
        ImGui::AlignTextToFramePadding();

    ImVec2 size;
    size.x = size_x.eval_px(ImGuiAxis_X, ctx);
    size.y = size_y.eval_px(ImGuiAxis_Y, ctx);
    auto ps = PrepareString(label.value());
    bool sel = selected.eval(ctx);
    ImRad::Selectable(ps.label.c_str(), sel, flags, size);
    DrawTextArgs(ps, ctx, { 0, 0 }, size, alignment);

    if (readOnly)
        ImGui::PopItemFlag();

    if (!style_header.empty())
        ImGui::PopStyleColor();

    ImGui::PopStyleVar();

    ImVec2 padding = ImGui::GetStyle().FramePadding;
    if (!style_interiorPadding.empty())
        padding = style_interiorPadding.eval_px(ctx);
    auto curData = ImRad::GetCursorData();
    auto lastItem = ImRad::GetLastItemData();
    ImGui::PushClipRect(lastItem.Rect.Min + padding, lastItem.Rect.Max - padding, true);
    ImGui::SetCursorScreenPos(lastItem.Rect.Min + padding);

    for (auto& child : child_iterator(children, false))
    {
        child->Draw(ctx);
    }
    for (auto& child : child_iterator(children, true))
    {
        child->Draw(ctx);
    }
    
    ImGui::PopClipRect();
    ImRad::SetLastItemData(lastItem);
    ImRad::SetCursorData(curData);
    
    return ImGui::GetWindowDrawList();
}

void Selectable::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    //there is a combined effect of AlignTextToFramePadding, NoPadWithHalfSpacing etc.
    cached_pos = ImGui::GetItemRectMin();
    cached_size = ImGui::GetItemRectSize();
}

void Selectable::DoExport(std::ostream& os, UIContext& ctx)
{
    os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { "
        << (horizAlignment == ImRad::AlignLeft ? "0" : horizAlignment == ImRad::AlignHCenter ? "0.5f" : "1.f")
        << ", "
        << (vertAlignment == ImRad::AlignTop ? "0" : vertAlignment == ImRad::AlignVCenter ? "0.5f" : "1.f")
        << " });\n";

    if (!style_header.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Header, " << style_header.to_arg() << ");\n";

    if (readOnly)
        os << ctx.ind << "ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);\n";

    if (alignToFrame)
        os << ctx.ind << "ImGui::AlignTextToFramePadding();\n";

    bool closePopup = ctx.kind == TopWindow::ModalPopup && modalResult != ImRad::None;
    os << ctx.ind;
    if (!onChange.empty() || closePopup)
        os << "if (";

    if (PrepareString(label.value()).error)
        PushError(ctx, "label is formatted wrongly");

    os << "ImRad::Selectable(" << label.to_arg() << ", ";
    if (selected.is_reference())
        os << "&" << selected.to_arg();
    else
        os << selected.to_arg();

    os << ", " << flags.to_arg() << ", { "
        << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", "
        << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1])
        << " })";

    if (!onChange.empty() || closePopup)
    {
        os << ")\n" << ctx.ind << "{\n";
        ctx.ind_up();

        if (!onChange.empty())
            os << ctx.ind << onChange.to_arg() << "();\n";
        if (closePopup)
            os << ctx.ind << "ClosePopup(" << modalResult.to_arg() << ");\n";

        ctx.ind_down();
        os << ctx.ind << "}\n";
    }
    else {
        os << ";\n";
    }

    if (readOnly)
        os << ctx.ind << "ImGui::PopItemFlag();\n";

    if (!style_header.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";

    os << ctx.ind << "ImGui::PopStyleVar();\n";

    if (children.size())
    {
        os << ctx.ind << "auto tmpCursor" << ctx.varCounter 
            << " = ImRad::GetCursorData();\n";
        os << ctx.ind << "auto " << TMP_LAST_ITEM_VAR << ctx.varCounter 
            << " = ImRad::GetLastItemData();\n";
        std::string tmp = ctx.parentVarName;
        ctx.parentVarName = TMP_LAST_ITEM_VAR + std::to_string(ctx.varCounter) + ".Rect";
        std::string padding = "ImGui::GetStyle().FramePadding";
        if (!style_interiorPadding.empty())
            padding = "ImVec2" + style_interiorPadding.to_arg(ctx.unit);
        os << ctx.ind << "ImGui::SetCursorScreenPos(ImGui::GetItemRectMin() + " << padding 
            << ");\n";
        os << ctx.ind << "ImGui::PushClipRect(ImGui::GetItemRectMin() + " << padding 
            << ", ImGui::GetItemRectMax() - " << padding << ", true);\n";
        os << ctx.ind << "/// @separator\n\n";
        
        for (auto& child : child_iterator(children, false))
        {
            child->Export(os, ctx);
        }
        
        for (auto& child : child_iterator(children, true))
        {
            child->Export(os, ctx);
        }

        ctx.parentVarName = tmp;
        os << ctx.ind << "/// @separator\n";
        os << ctx.ind << "ImGui::PopClipRect();\n";
        os << ctx.ind << "ImRad::SetLastItemData(" << TMP_LAST_ITEM_VAR << ctx.varCounter << ");\n";
        os << ctx.ind << "ImRad::SetCursorData(tmpCursor" << ctx.varCounter << ");\n";
        
        ++ctx.varCounter;
    }
}

void Selectable::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if ((sit->kind == cpp::CallExpr && sit->callee == "ImRad::Selectable") ||
        (sit->kind == cpp::IfCallBlock && sit->callee == "ImRad::Selectable") ||
        (sit->kind == cpp::CallExpr && sit->callee == "ImGui::Selectable") || //compatibility
        (sit->kind == cpp::IfCallThenCall && sit->callee == "ImRad::Selectable") || //compatibility
        (sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::Selectable")) //compatibility
    {
        ctx.importLevel = sit->level;

        if (sit->params.size() >= 1) {
            if (!label.set_from_arg(sit->params[0]))
                PushError(ctx, "unable to parse label");
        }
        if (sit->params.size() >= 2) {
            if (!sit->params[1].compare(0, 1, "&"))
                selected.set_from_arg(sit->params[1].substr(1));
            else
                selected.set_from_arg(sit->params[1]);
        }
        if (sit->params.size() >= 3) {
            std::string fl = Replace(sit->params[2], "ImGuiSelectableFlags_DontClosePopups", "ImGuiSelectableFlags_NoAutoClosePopups"); //compatibility
            if (!flags.set_from_arg(fl))
                PushError(ctx, "unrecognized flag in \"" + sit->params[2] + "\"");
        }
        if (sit->params.size() >= 4) {
            auto sz = cpp::parse_size(sit->params[3]);
            size_x.set_from_arg(sz.first);
            if (ctx.importVersion < 9000 && size_x.zero())
                size_x = -1;
            size_y.set_from_arg(sz.second);
        }

        if (sit->kind == cpp::IfCallThenCall)
            onChange.set_from_arg(sit->callee2);
    }
    else if (sit->level == ctx.importLevel + 1)
    {
        if (sit->kind == cpp::CallExpr && sit->callee == "ClosePopup" && ctx.kind == TopWindow::ModalPopup) {
            if (sit->params.size())
                modalResult.set_from_arg(sit->params[0]);
        }
        else if (sit->kind == cpp::CallExpr) {
            onChange.set_from_arg(sit->callee);
        }
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar")
    {
        if (sit->params.size() >= 2 && sit->params[0] == "ImGuiStyleVar_SelectableTextAlign")
        {
            auto a = cpp::parse_fsize(sit->params[1]);
            if (a.x == 0.5f)
                horizAlignment = ImRad::AlignHCenter;
            else if (a.x == 1.f)
                horizAlignment = ImRad::AlignRight;

            if (a.y == 0.5f)
                vertAlignment = ImRad::AlignVCenter;
            else if (a.y == 1.f)
                vertAlignment = ImRad::AlignBottom;
        }
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
    {
        if (sit->params.size() >= 2 && sit->params[0] == "ImGuiCol_Header")
            style_header.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushItemFlag")
    {
        if (sit->params.size() && sit->params[0] == "ImGuiItemFlags_Disabled")
            readOnly = true;
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::AlignTextToFramePadding")
    {
        alignToFrame = true;
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetCursorScreenPos") //other than overlayPos
    {
        if (sit->params.size() && !sit->params[0].compare(0, 30, "ImGui::GetItemRectMin()+ImVec2"))
            style_interiorPadding.set_from_arg(sit->params[0].substr(30));
    }
}

std::vector<UINode::Prop>
Selectable::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.header", &style_header },
        { "appearance.color", &style_text },
        { "appearance.padding", &style_interiorPadding },
        { "appearance.font", &style_font },
        { "appearance.horizAlignment", &horizAlignment },
        { "appearance.vertAlignment", &vertAlignment },
        { "appearance.alignToFrame", &alignToFrame },
        { "behavior.flags##selectable", &flags },
        { "behavior.label", &label, true },
        { "behavior.readOnly", &readOnly },
        { "behavior.modalResult##selectable", &modalResult },
        { "bindings.selected##selectable", &selected },
        });
    return props;
}

bool Selectable::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("header");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_header, ImGuiCol_Header, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("header", &style_header, ctx);
        break;
    case 1:
        ImGui::Text("color");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("color", &style_text, ctx);
        break;
    case 2:
        ImGui::Text("padding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_interiorPadding, ctx);
        break;
    case 3:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 4:
        ImGui::BeginDisabled(size_x.zero());
        ImGui::Text("horizAlignment");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = horizAlignment != Defaults().horizAlignment ? InputDirectVal_Modified : 0;
        changed = InputDirectValEnum(&horizAlignment, fl, ctx);
        ImGui::EndDisabled();
        break;
    case 5:
        ImGui::BeginDisabled(size_y.zero());
        ImGui::Text("vertAlignment");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = vertAlignment != Defaults().vertAlignment ? InputDirectVal_Modified : 0;
        changed = InputDirectValEnum(&vertAlignment, fl, ctx);
        ImGui::EndDisabled();
        break;
    case 6:
        ImGui::Text("alignToFramePadding");
        ImGui::TableNextColumn();
        fl = alignToFrame != Defaults().alignToFrame ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&alignToFrame, fl, ctx);
        break;
    case 7:
        changed = InputDirectValFlags("flags", &flags, Defaults().flags, ctx);
        break;
    case 8:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&label, InputBindable_MultilineEdit, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("label", &label, ctx);
        break;
    case 9:
        ImGui::Text("readOnly");
        ImGui::TableNextColumn();
        fl = readOnly != Defaults().readOnly ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&readOnly, fl, ctx);
        break;
    case 10:
        ImGui::BeginDisabled(ctx.kind != TopWindow::ModalPopup);
        ImGui::Text("modalResult");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = modalResult != Defaults().modalResult ? InputDirectVal_Modified : 0;
        changed = InputDirectValEnum(&modalResult, fl, ctx);
        ImGui::EndDisabled();
        break;
    case 11:
        ImGui::Text("selected");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = selected != Defaults().selected ? InputBindable_Modified : 0;
        changed = InputBindable(&selected, fl | InputBindable_ShowVariables, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("value", &selected, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 12, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
Selectable::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "selectable.change", &onChange },
        { "selectable.longPress", &onItemLongPressed }
        });
    return props;
}

bool Selectable::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("Change");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Change", &onChange, 0, ctx);
        break;
    case 1:
        ImGui::Text("LongPressed");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_LongPressed", &onItemLongPressed, 0, ctx);
        break;
    default:
        return Widget::EventUI(i - 2, ctx);
    }
    return changed;
}

//----------------------------------------------------

Button::Button(UIContext& ctx)
{
    modalResult.add(" ", ImRad::None);
    modalResult.add$(ImRad::Ok);
    modalResult.add$(ImRad::Cancel);
    modalResult.add$(ImRad::Yes);
    modalResult.add$(ImRad::YesToAll);
    modalResult.add$(ImRad::No);
    modalResult.add$(ImRad::NoToAll);
    modalResult.add$(ImRad::Apply);
    modalResult.add$(ImRad::Discard);
    modalResult.add$(ImRad::Help);
    modalResult.add$(ImRad::Reset);
    modalResult.add$(ImRad::Abort);
    modalResult.add$(ImRad::Retry);
    modalResult.add$(ImRad::Ignore);

    arrowDir.add$(ImGuiDir_None);
    arrowDir.add$(ImGuiDir_Left);
    arrowDir.add$(ImGuiDir_Right);
    arrowDir.add$(ImGuiDir_Up);
    arrowDir.add$(ImGuiDir_Down);
}

std::unique_ptr<Widget> Button::Clone(UIContext& ctx)
{
    return std::make_unique<Button>(*this);
}

int Button::Behavior()
{
    int fl = Widget::Behavior();
    if (!small)
        fl |= HasSizeX | HasSizeY;
    return fl;
}

ImDrawList* Button::DoDraw(UIContext& ctx)
{
    if (arrowDir != ImGuiDir_None)
        ImGui::ArrowButton("##", arrowDir);
    else if (small)
        ImGui::SmallButton(DRAW_STR(label));
    else
    {
        ImVec2 size;
        size.x = size_x.eval_px(ImGuiAxis_X, ctx);
        size.y = size_y.eval_px(ImGuiAxis_Y, ctx);
        ImGui::Button(DRAW_STR(label), size);

        //if (ctx.modalPopup && text.value() == "OK")
            //ImGui::SetItemDefaultFocus();
    }

    return ImGui::GetWindowDrawList();
}

void Button::DoExport(std::ostream& os, UIContext& ctx)
{
    bool closePopup = ctx.kind == TopWindow::ModalPopup && modalResult != ImRad::None;
    os << ctx.ind;
    if (!onChange.empty() || closePopup || dropDownMenu != "")
        os << "if (";

    if (arrowDir != ImGuiDir_None)
    {
        os << "ImGui::ArrowButton(\"##button" << ctx.varCounter
            << "\", " << arrowDir.to_arg() << ")";
        ++ctx.varCounter;
    }
    else if (small)
    {
        os << "ImGui::SmallButton(" << label.to_arg() << ")";
    }
    else
    {
        os << "ImGui::Button(" << label.to_arg() << ", { "
            << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", "
            << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1])
            << " })";
    }

    if (!shortcut.empty()) {
        os << " ||\n";
        ctx.ind_up();
        os << ctx.ind << "ImGui::Shortcut(" << shortcut.to_arg() << ")";
        ctx.ind_down();
    }

    if (!onChange.empty() || closePopup || dropDownMenu != "")
    {
        os << ")\n" << ctx.ind << "{\n";
        ctx.ind_up();

        if (!onChange.empty())
            os << ctx.ind << onChange.to_arg() << "();\n";
        if (closePopup)
            os << ctx.ind << "ClosePopup(" << modalResult.to_arg() << ");\n";
        if (dropDownMenu != "")
            os << ctx.ind << "ImRad::OpenWindowPopup(" << dropDownMenu.to_arg() << ");\n";

        ctx.ind_down();
        os << ctx.ind << "}\n";
    }
    else
    {
        os << ";\n";
    }
}

void Button::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if ((sit->kind == cpp::CallExpr || sit->kind == cpp::IfCallBlock) &&
        sit->callee == "ImGui::Button")
    {
        ctx.importLevel = sit->level;
        label.set_from_arg(sit->params[0]);

        if (sit->params.size() >= 2) {
            auto size = cpp::parse_size(sit->params[1]);
            size_x.set_from_arg(size.first);
            size_y.set_from_arg(size.second);
        }

        size_t i = sit->line.find("ImGui::Shortcut(");
        if (i != std::string::npos) {
            size_t j = sit->line.find(')', i);
            if (j != std::string::npos)
                shortcut.set_from_arg(sit->line.substr(i + 16, j - i - 16));
        }
        else {
            shortcut = ParseShortcutOld(sit->line);
        }
    }
    else if ((sit->kind == cpp::CallExpr || sit->kind == cpp::IfCallBlock) &&
        sit->callee == "ImGui::SmallButton")
    {
        ctx.importLevel = sit->level;
        label.set_from_arg(sit->params[0]);
    }
    else if ((sit->kind == cpp::CallExpr || sit->kind == cpp::IfCallBlock) &&
        sit->callee == "ImGui::ArrowButton")
    {
        ctx.importLevel = sit->level;
        label = "";
        if (sit->params.size() >= 2)
            arrowDir.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::CallExpr && sit->level == ctx.importLevel + 1)
    {
        if (sit->callee == "ClosePopup" && ctx.kind == TopWindow::ModalPopup) {
            if (sit->params.size())
                modalResult.set_from_arg(sit->params[0]);
        }
        else if (sit->callee == "ImRad::OpenWindowPopup" && sit->params.size())
            dropDownMenu.set_from_arg(sit->params[0]);
        else if (sit->callee == "callback" && sit->params.size()) //compatibility with older version
            modalResult.set_from_arg(sit->params[0]);
        else
            onChange.set_from_arg(sit->callee);
    }
}

std::vector<UINode::Prop>
Button::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.text", &style_text },
        { "appearance.button", &style_button },
        { "appearance.hovered", &style_buttonHovered },
        { "appearance.active", &style_buttonActive },
        { "appearance.border", &style_border },
        { "appearance.padding", &style_framePadding },
        { "appearance.rounding", &style_frameRounding },
        { "appearance.borderSize", &style_frameBorderSize },
        { "appearance.font", &style_font },
        { "appearance.small", &small },
        { "behavior.arrowDir##button", &arrowDir },
        { "behavior.label", &label, true },
        { "behavior.shortcut", &shortcut },
        { "behavior.modalResult##button", &modalResult },
        { "behavior.dropDownMenu", &dropDownMenu },
        });
    return props;
}

bool Button::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("color", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("button");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_button, ImGuiCol_Button, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("bg", &style_button, ctx);
        break;
    case 2:
        ImGui::Text("hovered");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_buttonHovered, ImGuiCol_ButtonHovered, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("hovered", &style_buttonHovered, ctx);
        break;
    case 3:
        ImGui::Text("active");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_buttonActive, ImGuiCol_ButtonActive, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("active", &style_buttonActive, ctx);
        break;
    case 4:
        ImGui::Text("border");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_border, ImGuiCol_Border, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("border", &style_border, ctx);
        break;
    case 5:
        ImGui::Text("padding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_framePadding, ctx);
        break;
    case 6:
        ImGui::Text("rounding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_frameRounding, ctx);
        break;
    case 7:
        ImGui::Text("borderSize");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_frameBorderSize, ctx);
        break;
    case 8:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 9:
        ImGui::BeginDisabled(arrowDir != ImGuiDir_None);
        ImGui::Text("small");
        ImGui::TableNextColumn();
        fl = small != Defaults().small ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&small, fl, ctx);
        ImGui::EndDisabled();
        break;
    case 10:
        ImGui::Text("arrowDir");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = arrowDir != Defaults().arrowDir ? InputDirectVal_Modified : 0;
        changed = InputDirectValEnum(&arrowDir, fl, ctx);
        if (changed && arrowDir != ImGuiDir_None)
            label = "";
        break;
    case 11:
        ImGui::BeginDisabled(arrowDir != ImGuiDir_None);
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&label, 0, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("label", &label, ctx);
        ImGui::EndDisabled();
        break;
    case 12:
        ImGui::Text("shortcut");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&shortcut, InputDirectVal_ShortcutButton, ctx);
        break;
    case 13:
        ImGui::BeginDisabled(ctx.kind != TopWindow::ModalPopup);
        ImGui::Text("modalResult");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = modalResult != Defaults().modalResult ? InputDirectVal_Modified : 0;
        changed = InputDirectValEnum(&modalResult, fl, ctx);
        if (changed && modalResult == ImRad::Cancel)
            shortcut = "Escape";
        ImGui::EndDisabled();
        break;
    case 14:
        ImGui::Text("dropDownMenu");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectValContextMenu(&dropDownMenu, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 15, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
Button::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "button.change", &onChange },
        });
    return props;
}

bool Button::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("Change");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Change", &onChange, 0, ctx);
        break;
    default:
        return Widget::EventUI(i - 1, ctx);
    }
    return changed;
}

//-----------------------------------------------------

CheckBox::CheckBox(UIContext& ctx)
{
    if (ctx.createVars)
        checked.set_from_arg(ctx.codeGen->CreateVar("bool", "false", CppGen::Var::Interface));
}

std::unique_ptr<Widget> CheckBox::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<CheckBox>(*this);
    if (ctx.createVars && checked.has_single_variable()) {
        sel->checked.set_from_arg(ctx.codeGen->CreateVar("bool", "false", CppGen::Var::Interface));
    }
    return sel;
}

ImDrawList* CheckBox::DoDraw(UIContext& ctx)
{
    if (!style_check.empty())
        ImGui::PushStyleColor(ImGuiCol_CheckMark, style_check.eval(ImGuiCol_CheckMark, ctx));

    auto ps = PrepareString(label.value());
    bool val = checked.eval(ctx);
    ImGui::Checkbox(ps.label.c_str(), &val);
    ImVec2 offset{ ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x, ImGui::GetStyle().FramePadding.y };
    DrawTextArgs(ps, ctx, offset);

    if (!style_check.empty())
        ImGui::PopStyleColor();

    return ImGui::GetWindowDrawList();
}

void CheckBox::DoExport(std::ostream& os, UIContext& ctx)
{
    if (!style_check.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_CheckMark, " << style_check.to_arg() << ");\n";

    os << ctx.ind;
    if (!onChange.empty())
        os << "if (";

    if (PrepareString(label.value()).error)
        PushError(ctx, "label is formatted wrongly");
    if (checked.empty())
        PushError(ctx, "checked is unassigned");
    if (!checked.is_reference())
        PushError(ctx, "checked only binds to l-values");

    os << "ImGui::Checkbox("
        << label.to_arg() << ", "
        << "&" << checked.to_arg()
        << ")";

    if (!onChange.empty()) {
        os << ")\n";
        ctx.ind_up();
        os << ctx.ind << onChange.to_arg() << "();\n";
        ctx.ind_down();
    }
    else {
        os << ";\n";
    }

    if (!style_check.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
}

void CheckBox::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
    {
        if (sit->params.size() >= 2)
            style_check.set_from_arg(sit->params[1]);
    }
    else if ((sit->kind == cpp::CallExpr && sit->callee == "ImGui::Checkbox") ||
        (sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::Checkbox"))
    {
        if (sit->params.size())
            label.set_from_arg(sit->params[0]);

        if (sit->params.size() >= 2 && !sit->params[1].compare(0, 1, "&"))
            checked.set_from_arg(sit->params[1].substr(1));

        if (sit->kind == cpp::IfCallThenCall)
            onChange.set_from_arg(sit->callee2);
    }
}

std::vector<UINode::Prop>
CheckBox::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.text", &style_text },
        { "appearance.check", &style_check },
        { "appearance.borderSize", &style_frameBorderSize },
        { "appearance.font", &style_font },
        { "behavior.label", &label, true },
        { "bindings.checked##1", &checked },
        });
    return props;
}

bool CheckBox::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("text", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("check");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_check, ImGuiCol_CheckMark, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("check", &style_check, ctx);
        break;
    case 2:
        ImGui::Text("borderSize");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_frameBorderSize, ctx);
        break;
    case 3:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 4:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&label, InputBindable_MultilineEdit, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("label", &label, ctx);
        break;
    case 5:
        ImGui::Text("checked");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = checked != Defaults().checked ? InputBindable_Modified : 0;
        changed = InputBindable(&checked, fl | InputBindable_ShowVariables, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("checked", &checked, BindingButton_ReferenceOnly, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 6, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
CheckBox::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "checkbox.change", &onChange }
        });
    return props;
}

bool CheckBox::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("Change");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Change", &onChange, 0, ctx);
        break;
    default:
        return Widget::EventUI(i - 1, ctx);
    }
    return changed;
}

//-----------------------------------------------------

RadioButton::RadioButton(UIContext& ctx)
{
    value = "false";
}

std::unique_ptr<Widget> RadioButton::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<RadioButton>(*this);
    //value can be shared
    return sel;
}

ImDrawList* RadioButton::DoDraw(UIContext& ctx)
{
    if (!style_check.empty())
        ImGui::PushStyleColor(ImGuiCol_CheckMark, style_check.eval(ImGuiCol_CheckMark, ctx));

    auto ps = PrepareString(label.value());
    bool checked = valueID == 0;
    if (!value.is_reference())
        checked = value.eval(ctx);
    ImGui::RadioButton(ps.label.c_str(), checked);
    ImVec2 offset{ ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x, ImGui::GetStyle().FramePadding.y };
    DrawTextArgs(ps, ctx, offset);

    if (!style_check.empty())
        ImGui::PopStyleColor();

    return ImGui::GetWindowDrawList();
}

void RadioButton::DoExport(std::ostream& os, UIContext& ctx)
{
    if (!style_check.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_CheckMark, " << style_check.to_arg() << ");\n";

    os << ctx.ind;
    if (!onChange.empty())
        os << "if (";

    if (PrepareString(label.value()).error)
        PushError(ctx, "label is formatted wrongly");
    if (value.empty())
        PushError(ctx, "value is unassigned");

    os << "ImGui::RadioButton("
        << label.to_arg() << ", ";
    if (value.is_reference())
        os << "&" << value.to_arg() << ", " << valueID;
    else
        os << value.to_arg();
    os << ")";

    if (!onChange.empty()) {
        os << ")\n";
        ctx.ind_up();
        os << ctx.ind << onChange.to_arg() << "();\n";
        ctx.ind_down();
    }
    else {
        os << ";\n";
    }

    if (!style_check.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
}

void RadioButton::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
    {
        if (sit->params.size() >= 2)
            style_check.set_from_arg(sit->params[1]);
    }
    else if ((sit->kind == cpp::CallExpr || sit->kind == cpp::IfCallThenCall) &&
        sit->callee == "ImGui::RadioButton")
    {
        if (sit->params.size())
            label.set_from_arg(sit->params[0]);

        if (sit->params.size() >= 2) {
            bool amp = !sit->params[1].compare(0, 1, "&");
            value.set_from_arg(sit->params[1].substr(amp));
        }

        if (sit->params.size() >= 3)
            valueID.set_from_arg(sit->params[2]);

        if (sit->kind == cpp::IfCallThenCall)
            onChange.set_from_arg(sit->callee2);
    }
}

std::vector<UINode::Prop>
RadioButton::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.text", &style_text },
        { "appearance.check", &style_check },
        { "appearance.borderSize", &style_frameBorderSize },
        { "appearance.font", &style_font },
        { "behavior.label", &label, true },
        { "behavior.valueID##1", &valueID },
        { "bindings.value##radio", &value },
    });
    return props;
}

bool RadioButton::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("text", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("check");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_check, ImGuiCol_CheckMark, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("check", &style_check, ctx);
        break;
    case 2:
        ImGui::Text("borderSize");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_frameBorderSize, ctx);
        break;
    case 3:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 4:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&label, InputBindable_MultilineEdit, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("label", &label, ctx);
        break;
    case 5:
        ImGui::Text("valueID");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = valueID != Defaults().valueID ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&valueID, fl, ctx);
        break;
    case 6:
        ImGui::Text("value");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = value != Defaults().value ? InputBindable_Modified : 0;
        changed = InputBindable(&value, fl | InputBindable_ShowVariables, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("value", &value, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 7, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
RadioButton::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "radioButton.change", &onChange }
        });
    return props;
}

bool RadioButton::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("Change");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Change", &onChange, 0, ctx);
        break;
    default:
        return Widget::EventUI(i - 1, ctx);
    }
    return changed;
}

//---------------------------------------------------

direct_val<ImRad::ImeType> Input::_imeClass = 0;
direct_val<ImRad::ImeType> Input::_imeAction = 0;

Input::Input(UIContext& ctx)
{
    size_x = 200;
    size_y = 100;

    flags.add$(ImGuiInputTextFlags_CharsDecimal);
    flags.add$(ImGuiInputTextFlags_CharsHexadecimal);
    flags.add$(ImGuiInputTextFlags_CharsScientific);
    flags.add$(ImGuiInputTextFlags_CharsUppercase);
    flags.add$(ImGuiInputTextFlags_CharsNoBlank);
    flags.separator();
    flags.add$(ImGuiInputTextFlags_AllowTabInput);
    flags.add$(ImGuiInputTextFlags_EnterReturnsTrue);
    flags.add$(ImGuiInputTextFlags_EscapeClearsAll);
    flags.add$(ImGuiInputTextFlags_CtrlEnterForNewLine);
    flags.separator();
    flags.add$(ImGuiInputTextFlags_ReadOnly);
    flags.add$(ImGuiInputTextFlags_Password);
    flags.add$(ImGuiInputTextFlags_AlwaysOverwrite);
    flags.add$(ImGuiInputTextFlags_AutoSelectAll);
    flags.add$(ImGuiInputTextFlags_ParseEmptyRefVal);
    flags.add$(ImGuiInputTextFlags_DisplayEmptyRefVal);
    flags.add$(ImGuiInputTextFlags_NoHorizontalScroll);
    flags.add$(ImGuiInputTextFlags_NoUndoRedo);
    flags.add$(ImGuiInputTextFlags_ElideLeft);
    flags.separator();
    flags.add$(ImGuiInputTextFlags_CallbackCompletion);
    flags.add$(ImGuiInputTextFlags_CallbackHistory);
    flags.add$(ImGuiInputTextFlags_CallbackAlways);
    flags.add$(ImGuiInputTextFlags_CallbackCharFilter);
    flags.separator();
    flags.add$(ImGuiInputTextFlags_Multiline);

    type.add("std::string", 0);
    type.add("ImGuiTextFilter", 1);
    type.add("int", 2);
    type.add("int2", 3);
    type.add("int3", 4);
    type.add("int4", 5);
    type.add("float", 6);
    type.add("float2", 7);
    type.add("float3", 8);
    type.add("float4", 9);
    type.add("double", 10);

    if (_imeClass.get_ids().empty())
    {
        _imeClass.add("Default", 0);
        _imeClass.add$(ImRad::ImeText);
        _imeClass.add$(ImRad::ImeNumber);
        _imeClass.add$(ImRad::ImeDecimal);
        _imeClass.add$(ImRad::ImeEmail);
        _imeClass.add$(ImRad::ImePhone);
        _imeAction.add("ImeActionNone", 0);
        _imeAction.add$(ImRad::ImeActionDone);
        _imeAction.add$(ImRad::ImeActionGo);
        _imeAction.add$(ImRad::ImeActionNext);
        _imeAction.add$(ImRad::ImeActionPrevious);
        _imeAction.add$(ImRad::ImeActionSearch);
        _imeAction.add$(ImRad::ImeActionSend);
    }

    if (ctx.createVars)
        value.set_from_arg(ctx.codeGen->CreateVar(type.get_id(), "", CppGen::Var::Interface));
}

std::unique_ptr<Widget> Input::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<Input>(*this);
    if (ctx.createVars && value.has_single_variable()) {
        sel->value.set_from_arg(ctx.codeGen->CreateVar(type.get_id(), "", CppGen::Var::Interface));
    }
    return sel;
}

int Input::Behavior()
{
    int b = Widget::Behavior() | HasSizeX;
    if (flags & ImGuiInputTextFlags_Multiline)
        b |= HasSizeY;
    return b;
}

ImDrawList* Input::DoDraw(UIContext& ctx)
{
    float ftmp[4] = {};
    int itmp[4] = {};
    double dtmp[4] = {};
    std::string stmp;
    static ImGuiTextFilter filter;

    std::string id = label;
    if (id.empty())
        id = "##" + *value.access();
    std::string tid = type.get_id();

    if (flags & ImGuiInputTextFlags_Multiline)
    {
        ImVec2 size;
        size.x = size_x.eval_px(ImGuiAxis_X, ctx);
        size.y = size_y.eval_px(ImGuiAxis_Y, ctx);
        ImGui::InputTextMultiline(id.c_str(), &stmp, size, flags);
    }
    else if (tid == "std::string" || tid == "ImGuiTextFilter")
    {
        float w = size_x.eval_px(ImGuiAxis_X, ctx);
        if (w)
            ImGui::SetNextItemWidth(w);
        if (!hint.empty())
            ImGui::InputTextWithHint(id.c_str(), DRAW_STR(hint), &stmp, flags);
        else
            ImGui::InputText(id.c_str(), &stmp, flags);
    }
    else
    {
        float w = size_x.eval_px(ImGuiAxis_X, ctx);
        if (w)
            ImGui::SetNextItemWidth(w);
        if (tid == "int")
            ImGui::InputInt(id.c_str(), itmp, (int)step);
        else if (tid == "int2")
            ImGui::InputInt2(id.c_str(), itmp);
        else if (tid == "int3")
            ImGui::InputInt3(id.c_str(), itmp);
        else if (tid == "int4")
            ImGui::InputInt4(id.c_str(), itmp);
        else if (tid == "float")
            ImGui::InputFloat(id.c_str(), ftmp, step, 0.f, format.c_str());
        else if (tid == "float2")
            ImGui::InputFloat2(id.c_str(), ftmp, format.c_str());
        else if (tid == "float3")
            ImGui::InputFloat3(id.c_str(), ftmp, format.c_str());
        else if (tid == "float4")
            ImGui::InputFloat4(id.c_str(), ftmp, format.c_str());
        else if (tid == "double")
            ImGui::InputDouble(id.c_str(), dtmp, step, 0, format.c_str());
    }

    return ImGui::GetWindowDrawList();
}

void Input::DoExport(std::ostream& os, UIContext& ctx)
{
    if (!value.is_reference())
        PushError(ctx, "value only supports l-values");

    if (!(flags & ImGuiInputTextFlags_Multiline))
        os << ctx.ind << "ImGui::SetNextItemWidth(" << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ");\n";

    std::string tid = type.get_id();
    os << ctx.ind;
    if (tid == "ImGuiTextFilter" || !onChange.empty())
        os << "if (";

    std::string cap = (char)std::toupper(tid[0]) + tid.substr(1);
    std::string id = label.to_arg();
    std::string defIme = "ImRad::ImeText";
    if (label.empty())
        id = "\"##" + *value.access() + "\"";

    if (tid == "int")
    {
        defIme = "ImRad::ImeNumber";
        os << "ImGui::InputInt(" << id << ", &"
            << value.to_arg() << ", " << (int)step << ")";
    }
    else if (tid == "float")
    {
        defIme = "ImRad::ImeDecimal";
        os << "ImGui::InputFloat(" << id << ", &"
            << value.to_arg() << ", " << step.to_arg() << ", 0.f, "
            << format.to_arg() << ")";
    }
    else if (tid == "double")
    {
        defIme = "ImRad::ImeDecimal";
        os << "ImGui::InputDouble(" << id << ", &"
            << value.to_arg() << ", " << step.to_arg() << ", 0.0, "
            << format.to_arg() << ")";
    }
    else if (!tid.compare(0, 3, "int"))
    {
        defIme = "ImRad::ImeNumber";
        os << "ImGui::Input"<< cap << "(" << id << ", &"
            << value.to_arg() << ")";
    }
    else if (!tid.compare(0, 5, "float"))
    {
        defIme = "ImRad::ImeDecimal";
        os << "ImGui::Input" << cap << "(" << id << ", &"
            << value.to_arg() << ", " << format.to_arg() << ")";
    }
    else if (flags & ImGuiInputTextFlags_Multiline)
    {
        os << "ImGui::InputTextMultiline(" << id << ", &"
            << value.to_arg() << ", { "
            << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", "
            << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]) << " }, "
            << flags.to_arg();
        if (!onCallback.empty())
            os << ", IMRAD_INPUTTEXT_EVENT(" << ctx.codeGen->GetName() << ", " << onCallback.to_arg() << ")";
        os << ")";
    }
    else if (tid == "std::string")
    {
        if (!hint.empty())
            os << "ImGui::InputTextWithHint(" << id << ", " << hint.to_arg() << ", ";
        else
            os << "ImGui::InputText(" << id << ", ";
        os << "&" << value.to_arg() << ", " << flags.to_arg();
        if (!onCallback.empty())
            os << ", IMRAD_INPUTTEXT_EVENT(" << ctx.codeGen->GetName() << ", " << onCallback.to_arg() << ")";
        os << ")";
    }
    else if (tid == "ImGuiTextFilter")
    {
        //.Draw function is deprecated see https://github.com/ocornut/imgui/issues/6395
        if (!hint.empty())
            os << "ImGui::InputTextWithHint(" << id << ", " << hint.to_arg() << ", ";
        else
            os << "ImGui::InputText(" << id << ", ";
        os << value.to_arg() << ".InputBuf, "
            << "IM_ARRAYSIZE(" << value.to_arg() << ".InputBuf), "
            << flags.to_arg();
        if (!onCallback.empty())
            os << ", IMRAD_INPUTTEXT_EVENT(" << ctx.codeGen->GetName() << ", " << onCallback.to_arg() << ")";
        os << ")";
    }

    if (tid == "ImGuiTextFilter") {
        os << ")\n" << ctx.ind << "{\n";
        ctx.ind_up();
        os << ctx.ind << value.to_arg() << ".Build();\n";
        if (!onChange.empty())
            os << ctx.ind << onChange.to_arg() << "();\n";
        ctx.ind_down();
        os << ctx.ind << "}\n";

    }
    else if (!onChange.empty()) {
        os << ")\n";
        ctx.ind_up();
        os << ctx.ind << onChange.to_arg() << "();\n";
        ctx.ind_down();
    }
    else {
        os << ";\n";
    }

    os << ctx.ind << "if (ImGui::IsItemActive())\n";
    ctx.ind_up();
    int cls = imeType & 0xff;
    int action = imeType & (~0xff);
    os << ctx.ind << "ioUserData->imeType = "
        << (cls ? _imeClass.find_id(cls)->first : defIme);
    if (action)
        os << " | " << _imeAction.find_id(action)->first;
    os << ";\n";
    ctx.ind_down();

    if (!onImeAction.empty()) {
        os << ctx.ind << "if (ImRad::IsItemImeAction())\n";
        ctx.ind_up();
        //os << ctx.ind << "ioUserData->imeActionPressed = false;\n";
        os << ctx.ind << onImeAction.to_arg() << "();\n";
        ctx.ind_down();
    }
}

void Input::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if ((sit->kind == cpp::CallExpr && sit->callee == "ImGui::InputTextMultiline") ||
        (sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::InputTextMultiline"))
    {
        type.set_id("std::string");

        if (sit->params.size()) {
            label.set_from_arg(sit->params[0]);
            if (!label.access()->compare(0, 2, "##"))
                label = "";
        }

        if (sit->params.size() >= 2 && !sit->params[1].compare(0, 1, "&")) {
            value.set_from_arg(sit->params[1].substr(1));
        }

        if (sit->params.size() >= 3) {
            auto size = cpp::parse_size(sit->params[2]);
            size_x.set_from_arg(size.first);
            size_y.set_from_arg(size.second);
        }

        if (sit->params.size() >= 4) {
            if (!flags.set_from_arg(sit->params[3]))
                PushError(ctx, "unrecognized flag in \"" + sit->params[3] + "\"");
        }

        if (sit->params.size() >= 5 &&
            !sit->params[4].compare(0, 22, "IMRAD_INPUTTEXT_EVENT("))
        {
            const std::string& arg = sit->params[4];
            size_t j = arg.find(',');
            onCallback.set_from_arg(arg.substr(j + 1, arg.size() - j - 2));
        }

        if (sit->kind == cpp::IfCallThenCall)
            onChange.set_from_arg(sit->callee2);
    }
    else if ((sit->kind == cpp::CallExpr && !sit->callee.compare(0, 16, "ImGui::InputText")) ||
        (sit->kind == cpp::IfCallThenCall && !sit->callee.compare(0, 16, "ImGui::InputText")) ||
        (sit->kind == cpp::IfCallBlock && !sit->callee.compare(0, 16, "ImGui::InputText")))
    {
        if (sit->params.size()) {
            label.set_from_arg(sit->params[0]);
            if (!label.access()->compare(0, 2, "##"))
                label = "";
        }

        size_t i = 0;
        if (sit->callee == "ImGui::InputTextWithHint") {
            hint = cpp::parse_str_arg(sit->params[1]);
            ++i;
        }

        if (sit->params.size() > 1 + i)
        {
            std::string expr = sit->params[1 + i];
            if (!expr.compare(0, 1, "&")) {
                type.set_id("std::string");
                value.set_from_arg(sit->params[1 + i].substr(1));
            }
            else if (!expr.compare(expr.size() - 9, 9, ".InputBuf")) {
                type.set_id("ImGuiTextFilter");
                value.set_from_arg(expr.substr(0, expr.size() - 9));
                ++i;
            }
        }

        if (sit->params.size() > 2 + i)
        {
            if (!flags.set_from_arg(sit->params[2 + i]))
                PushError(ctx, "unrecognized flag in \"" + sit->params[2 + i] + "\"");
        }

        if (sit->params.size() > 3 + i &&
            !sit->params[3 + i].compare(0, 22, "IMRAD_INPUTTEXT_EVENT("))
        {
            const std::string& arg = sit->params[3 + i];
            size_t j = arg.find(',');
            onCallback.set_from_arg(arg.substr(j + 1, arg.size() - j - 2));
        }

        if (sit->kind == cpp::IfCallThenCall)
            onChange.set_from_arg(sit->callee2);
        else if (sit->kind == cpp::IfCallBlock)
            ctx.importLevel = sit->level;
    }
    else if (sit->kind == cpp::CallExpr && sit->level == ctx.importLevel + 1)
    {
        if (sit->callee != *value.access() + ".Build") {
            onChange.set_from_arg(sit->callee);
            ctx.importLevel = -1;
        }
    }
    else if ((sit->kind == cpp::CallExpr && !sit->callee.compare(0, 12, "ImGui::Input")) ||
            (sit->kind == cpp::IfCallThenCall && !sit->callee.compare(0, 12, "ImGui::Input")))
    {
        std::string tid = sit->callee.substr(12);
        tid[0] = std::tolower(tid[0]);
        type.set_id(tid);

        if (sit->params.size()) {
            label.set_from_arg(sit->params[0]);
            if (!label.access()->compare(0, 2, "##"))
                label = "";
        }

        if (sit->params.size() >= 2 && !sit->params[1].compare(0, 1, "&")) {
            value.set_from_arg(sit->params[1].substr(1));
        }

        if (tid == "int")
        {
            if (sit->params.size() >= 3)
                step.set_from_arg(sit->params[2]);
        }
        if (tid == "float")
        {
            if (sit->params.size() >= 3)
                step.set_from_arg(sit->params[2]);
            if (sit->params.size() >= 5)
                format.set_from_arg(sit->params[4]);
        }
        else if (!tid.compare(0, 5, "float"))
        {
            if (sit->params.size() >= 3)
                format.set_from_arg(sit->params[2]);
        }

        if (sit->kind == cpp::IfCallThenCall)
            onChange.set_from_arg(sit->callee2);
    }
    else if (sit->kind == cpp::CallExpr && //for compatibility only
        (!sit->callee.compare(sit->callee.size() - 5, 5, ".Draw") ||
        !sit->callee.compare(sit->callee.size() - 13, 13, ".DrawWithHint")))
    {
        type.set_id("ImGuiTextFilter");
        size_t i = sit->callee.rfind('.');
        value.set_from_arg(sit->callee.substr(0, i));

        if (sit->params.size()) {
            label.set_from_arg(sit->params[0]);
            if (!label.access()->compare(0, 2, "##"))
                label = "";
        }

        if (sit->params.size() >= 2)
            hint.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextItemWidth")
    {
        if (sit->params.size())
            size_x.set_from_arg(sit->params[0]);
    }
    else if (sit->kind == cpp::IfCallStmt && sit->callee == "ImGui::IsItemActive")
    {
        auto pos = sit->line.find('=');
        if (pos != std::string::npos) {
            std::string fl = sit->line.substr(pos + 1);
            _imeClass.set_from_arg(fl);
            _imeAction.set_from_arg(fl);
            //set imeClass to default
            std::string tid = type.get_id();
            if (!tid.compare(0, 3, "int") && _imeClass == ImRad::ImeText)
                _imeClass = 0;
            else if (!tid.compare(0, 5, "float") && _imeClass == ImRad::ImeDecimal)
                _imeClass = 0;
            else if (!tid.compare(0, 5, "double") && _imeClass == ImRad::ImeDecimal)
                _imeClass = 0;
            else if ((tid == "std::string" || tid == "ImGuiTextFilter") && _imeClass == ImRad::ImeText)
                _imeClass = 0;

            imeType = _imeClass | _imeAction;
        }
    }
    else if (sit->kind == cpp::IfCallThenCall && sit->callee == "IsItemImeAction")
    {
        onImeAction.set_from_arg(sit->callee);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetKeyboardFocusHere") //compatibility
    {
        PushError(ctx, "forceFocus uses different protocol in " + VER_STR + ". Please set it again");
    }
}

std::vector<UINode::Prop>
Input::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.text", &style_text },
        { "appearance.frameBg", &style_frameBg },
        { "appearance.border", &style_border },
        { "appearance.borderSize", &style_frameBorderSize },
        { "appearance.font", &style_font },
        { "behavior.flags##input", &flags },
        { "behavior.label", &label, true },
        { "behavior.type##input", &type },
        { "behavior.hint", &hint },
        { "behavior.imeType##input", &imeType },
        { "behavior.step##input", &step },
        { "behavior.format##input", &format },
        { "bindings.value##1", &value },
    });
    return props;
}

bool Input::PropertyUI(int i, UIContext& ctx)
{
    std::string tid = type.get_id();
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("text", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("frameBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_frameBg, ImGuiCol_FrameBg, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("frameBg", &style_frameBg, ctx);
        break;
    case 2:
        ImGui::Text("border");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_border, ImGuiCol_Border, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("border", &style_border, ctx);
        break;
    case 3:
        ImGui::Text("borderSize");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_frameBorderSize, ctx);
        break;
    case 4:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 5:
        changed = InputDirectValFlags("flags", &flags, Defaults().flags, ctx);
        break;
    case 6:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&label, InputDirectVal_Modified, ctx);
        break;
    case 7:
        ImGui::Text("type");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = type != Defaults().type ? InputDirectVal_Modified : 0;
        changed = InputDirectValEnum(&type, fl, ctx);
        if (changed)
        {
            ctx.codeGen->ChangeVar(value.c_str(), type.get_id(), "");
        }
        break;
    case 8:
        ImGui::BeginDisabled(
            (tid != "std::string" && tid != "ImGuiTextFilter") ||
            (flags & ImGuiInputTextFlags_Multiline)
        );
        ImGui::Text("hint");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&hint, InputBindable_MultilineEdit, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("hint", &hint, ctx);
        ImGui::EndDisabled();
        break;
    case 9:
    {
        int type = imeType & 0xff;
        int action = imeType & (~0xff);
        std::string val = _imeClass.find_id(type)->first.substr(7);
        if (action)
            val += " | " + _imeAction.find_id(action)->first.substr(7);
        TreeNodeProp("imeType", ctx.pgbFont, val, [&] {
            ImGui::TableNextColumn();
            ImGui::Spacing();
            for (const auto& id : _imeClass.get_ids()) {
                int pre = !id.first.compare(0, 7, "ImRad::") ? 7 : 0;
                changed |= ImGui::RadioButton(id.first.substr(pre).c_str(), &type, id.second);
            }
            ImGui::Separator();
            for (const auto& id : _imeAction.get_ids()) {
                int pre = !id.first.compare(0, 7, "ImRad::") ? 7 : 0;
                changed |= ImGui::RadioButton(id.first.substr(pre).c_str(), &action, id.second);
            }
            imeType = type | action;
            });
        break;
    }
    case 10:
    {
        ImGui::BeginDisabled(tid != "int" && tid != "float");
        ImGui::Text("step");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        ImGui::PushFont(step != Defaults().step ? ctx.pgbFont : ctx.pgFont);
        if (tid == "int")
        {
            int st = (int)*step.access();
            changed = ImGui::InputInt("##step", &st);
            *step.access() = (float)st;
        }
        else {
            changed = ImGui::InputFloat("##step", step.access());
        }
        ImGui::PopFont();
        ImGui::EndDisabled();
        break;
    }
    case 11:
        ImGui::BeginDisabled(tid.compare(0, 5, "float"));
        ImGui::Text("format");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = format != Defaults().format ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&format, fl, ctx);
        ImGui::EndDisabled();
        break;
    case 12:
        ImGui::Text("value");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = value != Defaults().value ? InputBindable_Modified : 0;
        changed = InputBindable(&value, tid, fl | InputBindable_ShowVariables, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("value", &value, tid, BindingButton_ReferenceOnly, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 13, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
Input::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "input.callback", &onCallback },
        { "input.change", &onChange },
        { "input.imeAction", &onImeAction },
        });
    return props;
}

bool Input::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("Callback");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Callback", &onCallback, 0, ctx);
        break;
    case 1:
        ImGui::Text("Change");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Change", &onChange, 0, ctx);
        break;
    case 2:
        ImGui::Text("ImeAction");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_ImeAction", &onImeAction, 0, ctx);
        break;
    default:
        return Widget::EventUI(i - 3, ctx);
    }
    return changed;
}

//---------------------------------------------------

Combo::Combo(UIContext& ctx)
{
    size_x = 200;

    if (ctx.createVars)
        value.set_from_arg(ctx.codeGen->CreateVar("std::string", "", CppGen::Var::Interface));

    flags.add$(ImGuiComboFlags_HeightLarge);
    flags.add$(ImGuiComboFlags_HeightLargest);
    flags.add$(ImGuiComboFlags_HeightRegular);
    flags.add$(ImGuiComboFlags_HeightSmall);
    flags.add$(ImGuiComboFlags_NoArrowButton);
    flags.add$(ImGuiComboFlags_NoPreview);
    flags.add$(ImGuiComboFlags_PopupAlignLeft);
}

std::unique_ptr<Widget> Combo::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<Combo>(*this);
    if (ctx.createVars && !value.has_single_variable()) {
        sel->value.set_from_arg(ctx.codeGen->CreateVar("std::string", "", CppGen::Var::Interface));
    }
    return sel;
}

ImDrawList* Combo::DoDraw(UIContext& ctx)
{
    float w = size_x.eval_px(ImGuiAxis_X, ctx);
    if (w)
        ImGui::SetNextItemWidth(w);
    std::string id = label;
    if (id.empty())
        id = std::string("##") + value.c_str();

    auto vars = items.used_variables();
    if (vars.empty())
    {
        std::string tmp = items.c_str();
        ImRad::Combo(id.c_str(), &tmp, items.c_str(), flags);
    }
    else
    {
        std::string tmp = '{' + vars[0] + "}\0";
        ImRad::Combo(id.c_str(), &tmp, "\0", flags);
    }

    return ImGui::GetWindowDrawList();
}

void Combo::DoExport(std::ostream& os, UIContext& ctx)
{
    if (!size_x.empty())
        os << ctx.ind << "ImGui::SetNextItemWidth(" << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ");\n";

    os << ctx.ind;
    if (!onChange.empty())
        os << "if (";

    std::string id = label.to_arg();
    if (label.empty())
        id = std::string("\"##") + value.c_str() + "\"";

    os << "ImRad::Combo(" << id << ", &" << value.to_arg()
        << ", " << items.to_arg() << ", " << flags.to_arg() << ")";

    if (!onChange.empty()) {
        os << ")\n";
        ctx.ind_up();
        os << ctx.ind << onChange.to_arg() << "();\n";
        ctx.ind_down();
    }
    else {
        os << ";\n";
    }
}

void Combo::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextItemWidth")
    {
        if (sit->params.size()) {
            size_x.set_from_arg(sit->params[0]);
        }
    }
    else if ((sit->kind == cpp::CallExpr && (sit->callee == "ImGui::Combo" || sit->callee == "ImRad::Combo")) ||
        (sit->kind == cpp::IfCallThenCall && (sit->callee == "ImGui::Combo" || sit->callee == "ImRad::Combo")))
    {
        if (sit->params.size()) {
            label.set_from_arg(sit->params[0]);
            if (!label.access()->compare(0, 2, "##"))
                label = "";
        }

        if (sit->params.size() >= 2 && !sit->params[1].compare(0, 1, "&")) {
            value.set_from_arg(sit->params[1].substr(1));
        }

        if (sit->params.size() >= 3) {
            items.set_from_arg(sit->params[2]);
        }

        if (sit->params.size() >= 4) {
            if (!flags.set_from_arg(sit->params[3]))
                PushError(ctx, "unrecognized flag in \"" + sit->params[3] + "\"");
        }

        if (sit->kind == cpp::IfCallThenCall)
            onChange.set_from_arg(sit->callee2);
    }
}

std::vector<UINode::Prop>
Combo::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.text", &style_text },
        { "appearance.button", &style_button },
        { "appearance.hovered", &style_buttonHovered },
        { "appearance.active", &style_buttonActive },
        { "appearance.borderSize", &style_frameBorderSize },
        { "appearance.font", &style_font },
        { "behavior.flags##combo", &flags },
        { "behavior.label", &label, true },
        { "behavior.items##1", &items },
        { "bindings.value##1", &value },
        });
    return props;
}

bool Combo::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("text", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("button");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_button, ImGuiCol_Button, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("bg", &style_button, ctx);
        break;
    case 2:
        ImGui::Text("hovered");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_buttonHovered, ImGuiCol_ButtonHovered, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("hovered", &style_buttonHovered, ctx);
        break;
    case 3:
        ImGui::Text("active");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_buttonActive, ImGuiCol_ButtonActive, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("active", &style_buttonActive, ctx);
        break;
    case 4:
        ImGui::Text("borderSize");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_frameBorderSize, ctx);
        break;
    case 5:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 6:
        changed = InputDirectValFlags("flags", &flags, Defaults().flags, ctx);
        break;
    case 7:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&label, InputDirectVal_Modified, ctx);
        break;
    case 8:
        ImGui::Text("items");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&items, ctx);
        break;
    case 9:
        ImGui::Text("value");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = value != Defaults().value ? InputBindable_Modified : 0;
        changed = InputBindable(&value, "std::string", fl | InputBindable_ShowVariables, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("value", &value, "std::string", BindingButton_ReferenceOnly, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 10, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
Combo::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "combo.change", &onChange }
        });
    return props;
}

bool Combo::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("Change");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Change", &onChange, 0, ctx);
        break;
    default:
        return Widget::EventUI(i - 1, ctx);
    }
    return changed;
}

//---------------------------------------------------

Slider::Slider(UIContext& ctx)
{
    size_x = 200;

    flags.add$(ImGuiSliderFlags_ClampOnInput);
    flags.add$(ImGuiSliderFlags_ClampZeroRange);
    flags.add$(ImGuiSliderFlags_Logarithmic);
    flags.add$(ImGuiSliderFlags_NoInput);
    flags.add$(ImGuiSliderFlags_NoRoundToFormat);
    //flags.add$(ImGuiSliderFlags_WrapAround); only works with DragXXX

    type.add("int", 0);
    type.add("int2", 1);
    type.add("int3", 2);
    type.add("int4", 3);
    type.add("float", 4);
    type.add("float2", 5);
    type.add("float3", 6);
    type.add("float4", 7);
    type.add("angle", 8);

    if (ctx.createVars)
        value.set_from_arg(ctx.codeGen->CreateVar(type.get_id(), "", CppGen::Var::Interface));
}

std::unique_ptr<Widget> Slider::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<Slider>(*this);
    if (ctx.createVars && value.has_single_variable()) {
        sel->value.set_from_arg(ctx.codeGen->CreateVar(type.get_id(), "", CppGen::Var::Interface));
    }
    return sel;
}

ImDrawList* Slider::DoDraw(UIContext& ctx)
{
    float ftmp[4] = {};
    int itmp[4] = {};

    float w = size_x.eval_px(ImGuiAxis_X, ctx);
    if (w)
        ImGui::SetNextItemWidth(w);

    const char* fmt = nullptr;
    if (!format.empty())
        fmt = format.c_str();
    std::string id = label;
    if (label.empty())
        id = "##" + *value.access();
    std::string tid = type.get_id();
    int fl = flags | ImGuiSliderFlags_NoInput;

    if (tid == "int")
        ImGui::SliderInt(id.c_str(), itmp, (int)min, (int)max, fmt, fl);
    else if (tid == "int2")
        ImGui::SliderInt2(id.c_str(), itmp, (int)min, (int)max, fmt, fl);
    else if (tid == "int3")
        ImGui::SliderInt3(id.c_str(), itmp, (int)min, (int)max, fmt, fl);
    else if (tid == "int4")
        ImGui::SliderInt4(id.c_str(), itmp, (int)min, (int)max, fmt, fl);
    else if (tid == "float")
        ImGui::SliderFloat(id.c_str(), ftmp, min, max, fmt, fl);
    else if (tid == "float2")
        ImGui::SliderFloat2(id.c_str(), ftmp, min, max, fmt, fl);
    else if (tid == "float3")
        ImGui::SliderFloat3(id.c_str(), ftmp, min, max, fmt, fl);
    else if (tid == "float4")
        ImGui::SliderFloat4(id.c_str(), ftmp, min, max, fmt, fl);
    else if (tid == "angle")
        ImGui::SliderAngle(id.c_str(), ftmp, min, max, fmt, fl);

    return ImGui::GetWindowDrawList();
}

void Slider::DoExport(std::ostream& os, UIContext& ctx)
{
    os << ctx.ind << "ImGui::SetNextItemWidth(" << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ");\n";

    os << ctx.ind;
    if (!onChange.empty())
        os << "if (";

    std::string fmt = "nullptr";
    if (!format.empty())
        fmt = format.to_arg();

    std::string cap = type.get_id();
    cap[0] = std::toupper(cap[0]);
    std::string id = label.to_arg();
    if (label.empty())
        id = "\"##" + *value.access() + "\"";

    os << "ImGui::Slider" << cap << "(" << id << ", &"
        << value.to_arg() << ", " << min.to_arg() << ", " << max.to_arg()
        << ", " << fmt << ", " << flags.to_arg() << ")";

    if (!onChange.empty()) {
        os << ")\n";
        ctx.ind_up();
        os << ctx.ind << onChange.to_arg() << "();\n";
        ctx.ind_down();
    }
    else {
        os << ";\n";
    }
}

void Slider::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if ((sit->kind == cpp::CallExpr && !sit->callee.compare(0, 13, "ImGui::Slider")) ||
        (sit->kind == cpp::IfCallThenCall && !sit->callee.compare(0, 13, "ImGui::Slider")))
    {
        std::string tid = sit->callee.substr(13);
        tid[0] = std::tolower(tid[0]);
        type.set_id(tid);

        if (sit->params.size()) {
            label.set_from_arg(sit->params[0]);
            if (!label.access()->compare(0, 2, "##"))
                label = "";
        }

        if (sit->params.size() > 1 && !sit->params[1].compare(0, 1, "&"))
            value.set_from_arg(sit->params[1].substr(1));

        if (sit->params.size() > 2)
            min.set_from_arg(sit->params[2]);

        if (sit->params.size() > 3)
            max.set_from_arg(sit->params[3]);

        if (sit->params.size() > 4)
            format.set_from_arg(sit->params[4]);

        if (sit->params.size() > 5) {
            if (!flags.set_from_arg(sit->params[5]))
                PushError(ctx, "unrecognized flag in \"" + sit->params[5] + "\"");
        }

        if (sit->kind == cpp::IfCallThenCall)
            onChange.set_from_arg(sit->callee2);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextItemWidth")
    {
        if (sit->params.size()) {
            size_x.set_from_arg(sit->params[0]);
        }
    }
}

std::vector<UINode::Prop>
Slider::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.text", &style_text },
        { "appearance.frameBg", &style_frameBg },
        { "appearance.border", &style_border },
        { "appearance.borderSize", &style_frameBorderSize },
        { "appearance.font", &style_font },
        { "behavior.flags##slider", &flags },
        { "behavior.label", &label, true },
        { "behavior.type##slider", &type },
        { "behavior.min##slider", &min },
        { "behavior.max##slider", &max },
        { "behavior.format##slider", &format },
        { "bindings.value##1", &value },
        });
    return props;
}

bool Slider::PropertyUI(int i, UIContext& ctx)
{
    static const char* TYPES[]{
        "int",
        "int2",
        "int3",
        "int4",
        "float",
        "float2",
        "float3",
        "float4",
        "angle"
    };

    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("text", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("frameBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_frameBg, ImGuiCol_FrameBg, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("frameBg", &style_frameBg, ctx);
        break;
    case 2:
        ImGui::Text("border");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_border, ImGuiCol_Border, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("border", &style_border, ctx);
        break;
    case 3:
        ImGui::Text("borderSize");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_frameBorderSize, ctx);
        break;
    case 4:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 5:
        changed = InputDirectValFlags("flags", &flags, Defaults().flags, ctx);
        break;
    case 6:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&label, InputDirectVal_Modified, ctx);
        break;
    case 7:
        ImGui::Text("type");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = type != Defaults().type ? InputDirectVal_Modified : 0;
        if (InputDirectValEnum(&type, fl, ctx))
        {
            changed = true;
            std::string tid = type.get_id();
            ctx.codeGen->ChangeVar(value.c_str(), tid == "angle" ? "float" : tid, "");
        }
        break;
    case 8:
        ImGui::Text("min");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        ImGui::PushFont(min != Defaults().min ? ctx.pgbFont : ctx.pgFont);
        if (type.get_id() == "int") {
            int val = (int)min;
            changed = ImGui::InputInt("##min", &val);
            min = (float)val;
        }
        else {
            changed = ImGui::InputFloat("##min", min.access());
        }
        ImGui::PopFont();
        break;
    case 9:
        ImGui::Text("max");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        ImGui::PushFont(max != Defaults().max ? ctx.pgbFont : ctx.pgFont);
        if (type.get_id() == "int") {
            int val = (int)max;
            changed = ImGui::InputInt("##max", &val);
            max = (float)val;
        }
        else {
            changed = ImGui::InputFloat("##max", max.access());
        }
        ImGui::PopFont();
        break;
    case 10:
        ImGui::Text("format");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = format != Defaults().format ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&format, fl, ctx);
        break;
    case 11:
        ImGui::Text("value");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = value != Defaults().value ? InputBindable_Modified : 0;
        changed = InputBindable(&value, type.get_id(), fl | InputBindable_ShowVariables, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("value", &value, type.get_id(), BindingButton_ReferenceOnly, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 12, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
Slider::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "slider.change", &onChange }
        });
    return props;
}

bool Slider::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("Change");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Change", &onChange, 0, ctx);
        break;
    default:
        return Widget::EventUI(i - 1, ctx);
    }
    return changed;
}

//---------------------------------------------------

ProgressBar::ProgressBar(UIContext& ctx)
{
    size_x = 200;

    if (ctx.createVars)
        value.set_from_arg(ctx.codeGen->CreateVar("float", "0", CppGen::Var::Interface));
}

std::unique_ptr<Widget> ProgressBar::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<ProgressBar>(*this);
    if (ctx.createVars && value.has_single_variable()) {
        sel->value.set_from_arg(ctx.codeGen->CreateVar("float", "0", CppGen::Var::Interface));
    }
    return sel;
}

ImDrawList* ProgressBar::DoDraw(UIContext& ctx)
{
    if (!style_color.empty())
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, style_color.eval(ImGuiCol_PlotHistogram, ctx));

    float w = size_x.eval_px(ImGuiAxis_X, ctx);
    float h = size_y.eval_px(ImGuiAxis_Y, ctx);

    float pc = 0.5f;
    if (!value.empty())
        pc = value.eval(ctx);
    ImGui::ProgressBar(pc, { w, h }, indicator ? nullptr : "");

    if (!style_color.empty())
        ImGui::PopStyleColor();

    return ImGui::GetWindowDrawList();
}

void ProgressBar::DoExport(std::ostream& os, UIContext& ctx)
{
    if (!style_color.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_PlotHistogram, "
            << style_color.to_arg() << ");\n";

    std::string fmt = indicator ? "nullptr" : "\"\"";

    os << ctx.ind << "ImGui::ProgressBar("
        << value.to_arg() << ", { "
        << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", "
        << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]) << " }, "
        << fmt << ");\n";

    if (!style_color.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
}

void ProgressBar::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::ProgressBar")
    {
        if (sit->params.size() >= 1)
            value.set_from_arg(sit->params[0]);

        if (sit->params.size() >= 2) {
            auto siz = cpp::parse_size(sit->params[1]);
            size_x.set_from_arg(siz.first);
            size_y.set_from_arg(siz.second);
        }

        if (sit->params.size() >= 3)
            indicator = sit->params[2] == "nullptr";
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
    {
        if (sit->params.size() >= 2 && sit->params[0] == "ImGuiCol_PlotHistogram")
            style_color.set_from_arg(sit->params[1]);
    }
}

std::vector<UINode::Prop>
ProgressBar::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.text", &style_text },
        { "appearance.frameBg", &style_frameBg },
        { "appearance.color", &style_color },
        { "appearance.border", &style_border },
        { "appearance.borderSize", &style_frameBorderSize },
        { "appearance.font", &style_font },
        { "appearance.indicator##progress", &indicator },
        { "bindings.value##1", &value },
        });
    return props;
}

bool ProgressBar::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("text", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("frameBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_frameBg, ImGuiCol_FrameBg, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("frameBg", &style_frameBg, ctx);
        break;
    case 2:
        ImGui::Text("color");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_color, ImGuiCol_PlotHistogram, ctx);
        break;
    case 3:
        ImGui::Text("border");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_border, ImGuiCol_Border, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("border", &style_border, ctx);
        break;
    case 4:
        ImGui::Text("borderSize");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_frameBorderSize, ctx);
        break;
    case 5:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 6:
        ImGui::Text("indicator");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&indicator, 0, ctx);
        break;
    case 7:
        ImGui::Text("value");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&value, InputBindable_ShowVariables, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("value", &value, BindingButton_ReferenceOnly, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 8, ctx);
    }
    return changed;
}

//---------------------------------------------------

ColorEdit::ColorEdit(UIContext& ctx)
{
    size_x = 200;

    flags.add$(ImGuiColorEditFlags_NoAlpha);
    flags.add$(ImGuiColorEditFlags_NoBorder);
    flags.add$(ImGuiColorEditFlags_NoDragDrop);
    flags.add$(ImGuiColorEditFlags_NoInputs);
    flags.add$(ImGuiColorEditFlags_NoLabel);
    flags.add$(ImGuiColorEditFlags_NoOptions);
    flags.add$(ImGuiColorEditFlags_NoPicker);
    flags.add$(ImGuiColorEditFlags_NoSidePreview);
    flags.add$(ImGuiColorEditFlags_NoSmallPreview);
    flags.add$(ImGuiColorEditFlags_NoTooltip);

    if (ctx.createVars)
        value.set_from_arg(ctx.codeGen->CreateVar(type, "", CppGen::Var::Interface));
}

std::unique_ptr<Widget> ColorEdit::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<ColorEdit>(*this);
    if (ctx.createVars && value.has_single_variable()) {
        sel->value.set_from_arg(ctx.codeGen->CreateVar(type, "", CppGen::Var::Interface));
    }
    return sel;
}

ImDrawList* ColorEdit::DoDraw(UIContext& ctx)
{
    ImVec4 ftmp = {};

    std::string id = label;
    if (id.empty())
        id = "##" + *value.access();
    float w = size_x.eval_px(ImGuiAxis_X, ctx);
    if (w)
        ImGui::SetNextItemWidth(w);

    if (type == "color3")
        ImGui::ColorEdit3(id.c_str(), (float*)&ftmp, flags);
    else if (type == "color4")
        ImGui::ColorEdit4(id.c_str(), (float*)&ftmp, flags);

    return ImGui::GetWindowDrawList();
}

void ColorEdit::DoExport(std::ostream& os, UIContext& ctx)
{
    if (!size_x.empty())
        os << ctx.ind << "ImGui::SetNextItemWidth(" << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ");\n";

    os << ctx.ind;
    if (!onChange.empty())
        os << "if (";

    std::string id = label.to_arg();
    if (label.empty())
        id = "\"##" + *value.access() + "\"";

    if (type == "color3")
    {
        os << "ImGui::ColorEdit3(" << id << ", (float*)&"
            << value.to_arg() << ", " << flags.to_arg() << ")";
    }
    else if (type == "color4")
    {
        os << "ImGui::ColorEdit4(" << id << ", (float*)&"
            << value.to_arg() << ", " << flags.to_arg() << ")";
    }

    if (!onChange.empty()) {
        os << ")\n";
        ctx.ind_up();
        os << ctx.ind << onChange.to_arg() << "();\n";
        ctx.ind_down();
    }
    else {
        os << ";\n";
    }
}

void ColorEdit::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if ((sit->kind == cpp::CallExpr && !sit->callee.compare(0, 16, "ImGui::ColorEdit")) ||
        (sit->kind == cpp::IfCallThenCall && !sit->callee.compare(0, 16, "ImGui::ColorEdit")))
    {
        type = sit->callee.substr(16, 1) == "3" ? "color3" : "color4";

        if (sit->params.size()) {
            label.set_from_arg(sit->params[0]);
            if (!label.access()->compare(0, 2, "##"))
                label = "";
        }

        if (sit->params.size() >= 2 && !sit->params[1].compare(0, 9, "(float*)&")) {
            value.set_from_arg(sit->params[1].substr(9));
        }

        if (sit->params.size() >= 3) {
            if (!flags.set_from_arg(sit->params[2]))
                PushError(ctx, "unrecognized flag in \"" + sit->params[2] + "\"");
        }

        if (sit->kind == cpp::IfCallThenCall)
            onChange.set_from_arg(sit->callee2);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextItemWidth")
    {
        if (sit->params.size())
            size_x.set_from_arg(sit->params[0]);
    }
}

std::vector<UINode::Prop>
ColorEdit::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.text", &style_text },
        { "appearance.frameBg", &style_frameBg },
        { "appearance.border", &style_border },
        { "appearance.borderSize", &style_frameBorderSize },
        { "appearance.font", &style_font },
        { "behavior.flags##color", &flags },
        { "behavior.label", &label, true },
        { "behavior.type##color", &type },
        { "bindings.value##1", &value },
        });
    return props;
}

bool ColorEdit::PropertyUI(int i, UIContext& ctx)
{
    static const char* TYPES[]{
        "color3",
        "color4",
    };

    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("text", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("frameBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_frameBg, ImGuiCol_FrameBg, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("frameBg", &style_frameBg, ctx);
        break;
    case 2:
        ImGui::Text("border");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_border, ImGuiCol_Border, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("border", &style_border, ctx);
        break;
    case 3:
        ImGui::Text("borderSize");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_frameBorderSize, ctx);
        break;
    case 4:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 5:
        changed = InputDirectValFlags("flags", &flags, Defaults().flags, ctx);
        break;
    case 6:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&label, InputDirectVal_Modified, ctx);
        break;
    case 7:
        ImGui::Text("type");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        ImGui::PushFont(type != Defaults().type ? ctx.pgbFont : ctx.pgFont);
        if (ImGui::BeginCombo("##type", type.c_str()))
        {
            ImGui::PopFont();
            ImGui::PushFont(ImGui::GetFont());
            for (const auto& tp : TYPES)
            {
                if (ImGui::Selectable(tp, type == tp)) {
                    changed = true;
                    type = tp;
                    ctx.codeGen->ChangeVar(value.c_str(), type, "");
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopFont();
        break;
    case 8:
        ImGui::Text("value");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = value != Defaults().value ? InputBindable_Modified : 0;
        changed = InputBindable(&value, type, fl | InputBindable_ShowVariables, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("value", &value, type, BindingButton_ReferenceOnly, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 9, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
ColorEdit::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "colorEdit.change", &onChange }
        });
    return props;
}

bool ColorEdit::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("Change");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Change", &onChange, 0, ctx);
        break;
    default:
        return Widget::EventUI(i - 1, ctx);
    }
    return changed;
}

//----------------------------------------------------

Image::Image(UIContext& ctx)
{
    stretchPolicy.add("None", StretchPolicy::None);
    stretchPolicy.add("Scale", StretchPolicy::Scale);
    stretchPolicy.add("FitIn", StretchPolicy::FitIn);
    stretchPolicy.add("FitOut", StretchPolicy::FitOut);

    if (ctx.createVars)
        *texture.access() = ctx.codeGen->CreateVar("ImRad::Texture", "", CppGen::Var::Impl);
}

std::unique_ptr<Widget> Image::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<Image>(*this);
    if (ctx.createVars && texture.has_single_variable()) {
        sel->texture.set_from_arg(ctx.codeGen->CreateVar("ImRad::Texture", "", CppGen::Var::Impl));
    }
    return sel;
}

ImDrawList* Image::DoDraw(UIContext& ctx)
{
    float w = 20, h = 20;
    if (!size_x.zero())
        w = size_x.eval_px(ImGuiAxis_X, ctx);
    else if (tex.id)
        w = (float)tex.w;
    if (!size_y.zero())
        h = size_y.eval_px(ImGuiAxis_Y, ctx);
    else if (tex.id)
        h = (float)tex.h;

    ImVec2 uv0(0, 0);
    ImVec2 uv1(1, 1);
    if (stretchPolicy != Scale && tex.id)
    {
        float wrel = w / tex.w;
        float hrel = h / tex.h;
        float scale = 1.f;
        if (stretchPolicy == FitIn)
            scale = std::min(wrel, hrel);
        else if (stretchPolicy == FitOut)
            scale = std::max(wrel, hrel);
        wrel /= scale;
        hrel /= scale;
        uv0 = { -(wrel - 1) / 2, -(hrel - 1) / 2 };
        uv1 = { 1 + (wrel - 1) / 2, 1 + (hrel - 1) / 2 };
    }
    ImGui::Image(tex.id, { w, h }, uv0, uv1);
    return ImGui::GetWindowDrawList();
}

void Image::DoExport(std::ostream& os, UIContext& ctx)
{
    if (texture.empty())
        PushError(ctx, "texture field empty");
    if (fileName.empty())
        PushError(ctx, "fileName empty");

    os << ctx.ind << "if (!" << texture.to_arg() << ")\n";
    ctx.ind_up();
    os << ctx.ind << texture.to_arg() << " = ImRad::LoadTextureFromFile(" << fileName.to_arg() << ");\n";
    ctx.ind_down();

    os << ctx.ind << "ImGui::Image(" << texture.to_arg() << ".id, { ";

    if (size_x.zero())
        os << "(float)" << texture.to_arg() << ".w";
    else
        os << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]);

    os << ", ";

    if (size_y.zero())
        os << "(float)" << texture.to_arg() << ".h";
    else
        os << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]);
    os << " }, ";

    ImVec2 uv0(0, 0);
    ImVec2 uv1(1, 1);
    if (stretchPolicy != Scale && tex.id && size_x.has_value() && size_y.has_value())
    {
        //todo: binded dimensions
        float wrel = size_x.zero() ? 1.f : size_x.eval_px(ImGuiAxis_X, ctx) / tex.w;
        float hrel = size_y.zero() ? 1.f : size_y.eval_px(ImGuiAxis_Y, ctx) / tex.h;
        float scale = 1.f;
        if (stretchPolicy == FitIn)
            scale = std::min(wrel, hrel);
        else if (stretchPolicy == FitOut)
            scale = std::max(wrel, hrel);
        wrel /= scale;
        hrel /= scale;
        uv0 = { -(wrel - 1) / 2, -(hrel - 1) / 2 };
        uv1 = { 1 + (wrel - 1) / 2, 1 + (hrel - 1) / 2 };
    }
    os << "{ " << uv0.x << ", " << uv0.y << " }, { " << uv1.x << ", " << uv1.y << " }";

    std::string sp = stretchPolicy == None ? "StretchPolicy::None" :
        stretchPolicy == Scale ? "StretchPolicy::Scale" :
        stretchPolicy == FitIn ? "StretchPolicy::FitIn" :
        stretchPolicy == FitOut ? "StretchPolicy::FitOut" : "";
    os << "); //" << sp << "\n";
}

void Image::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::IfStmt)
    {
        auto i = sit->line.find("ImRad::LoadTextureFromFile(");
        if (i != std::string::npos)
            fileName.set_from_arg(sit->line.substr(i + 27, sit->line.size() - 1 - i - 27));
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::Image")
    {
        if (sit->params.size() >= 1 && !sit->params[0].compare(sit->params[0].size() - 3, 3, ".id"))
            texture.set_from_arg(sit->params[0].substr(0, sit->params[0].size() - 3));

        if (sit->params.size() >= 2) {
            auto size = cpp::parse_size(sit->params[1]);
            if (size.first == "(float)" + *texture.access() + ".w")
                size_x.set_from_arg("0");
            else
                size_x.set_from_arg(size.first);

            if (size.second == "(float)" + *texture.access() + ".h")
                size_y.set_from_arg("0");
            else
                size_y.set_from_arg(size.second);
        }

        RefreshTexture(ctx);
    }
    else if (sit->kind == cpp::Comment && sit->line == "//StretchPolicy::None")
    {
        stretchPolicy = None;
    }
    else if (sit->kind == cpp::Comment && sit->line == "//StretchPolicy::Scale")
    {
        stretchPolicy = Scale;
    }
    else if (sit->kind == cpp::Comment && sit->line == "//StretchPolicy::FitIn")
    {
        stretchPolicy = FitIn;
    }
    else if (sit->kind == cpp::Comment && sit->line == "//StretchPolicy::FitOut")
    {
        stretchPolicy = FitOut;
    }
}

std::vector<UINode::Prop>
Image::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "behavior.file_name#1", &fileName, true },
        { "behavior.stretchPolicy", &stretchPolicy },
        { "bindings.texture#1", &texture },
        });
    return props;
}

bool Image::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("fileName");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&fileName, InputBindable_CustomButton, ctx, [this,&ctx] { 
            if (PickFileName(ctx))
                RefreshTexture(ctx);
            });
        if (changed)
            RefreshTexture(ctx);
        break;
    case 1:
        ImGui::Text("stretchPolicy");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = stretchPolicy != Defaults().stretchPolicy ? InputDirectVal_Modified : 0;
        changed = InputDirectValEnum(&stretchPolicy, fl, ctx);
        break;
    case 2:
        ImGui::Text("texture");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = texture != Defaults().texture ? InputBindable_Modified : 0;
        changed = InputBindable(&texture, fl | InputBindable_ShowVariables, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("texture", &texture, BindingButton_ReferenceOnly, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 3, ctx);
    }
    return changed;
}

bool Image::PickFileName(UIContext& ctx)
{
    if (ctx.workingDir.empty()) {
        messageBox.title = "Warning";
        messageBox.message = "Please save the file first so relative paths can work";
        messageBox.buttons = ImRad::Ok;
        messageBox.OpenPopup();
        return false;
    }

    nfdchar_t *outPath = NULL;
    nfdfilteritem_t filterItem[1] = { { "All Images", "bmp,gif,jpg,jpeg,png,tga" } };
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);
    if (result != NFD_OKAY)
        return false;

    fileName = generic_u8string(fs::relative(u8path(outPath), u8path(ctx.workingDir)));
    NFD_FreePath(outPath);
    return true;
}

void Image::RefreshTexture(UIContext& ctx)
{
    tex.id = 0;

    if (fileName.empty() ||
        !fileName.used_variables().empty())
        return;

    std::string fname = fileName.value();
    if (u8path(fname).is_relative())
    {
        if (ctx.workingDir.empty() && !ctx.importState) {
            messageBox.title = "Warning";
            messageBox.message = "Please save the file first so relative paths can work";
            messageBox.buttons = ImRad::Ok;
            messageBox.OpenPopup();
            return;
        }
        fname = u8string(u8path(ctx.workingDir) / u8path(fileName.value()));
    }

    tex = ImRad::LoadTextureFromFile(fname);
    if (!tex && ctx.importState)
        PushError(ctx, "can't read \"" + fname + "\"");
}

//----------------------------------------------------

CustomWidget::CustomWidget(UIContext& ctx)
{
}

std::unique_ptr<Widget> CustomWidget::Clone(UIContext& ctx)
{
    return std::make_unique<CustomWidget>(*this);
}

ImDrawList* CustomWidget::DoDraw(UIContext& ctx)
{
    ImVec2 size;
    size.x = size_x.eval_px(ImGuiAxis_X, ctx);
    if (!size.x)
        size.x = 20;
    size.y = size_y.eval_px(ImGuiAxis_Y, ctx);
    if (!size.y)
        size.y = 20;

    std::string id = std::to_string((uintptr_t)this);
    ImGui::BeginChild(id.c_str(), size, ImGuiChildFlags_Border);
    ImGui::EndChild();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    auto clr = ImGui::GetStyle().Colors[ImGuiCol_Border];
    dl->AddLine(cached_pos, cached_pos + cached_size, ImGui::ColorConvertFloat4ToU32(clr));
    dl->AddLine(cached_pos + ImVec2(0, cached_size.y), cached_pos + ImVec2(cached_size.x, 0), ImGui::ColorConvertFloat4ToU32(clr));

    return ImGui::GetWindowDrawList();
}

void CustomWidget::DoExport(std::ostream& os, UIContext& ctx)
{
    if (onDraw.empty()) {
        PushError(ctx, "Draw event not set");
        return;
    }
    os << ctx.ind << onDraw.to_arg() << "({ "
        << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", "
        << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1])
        << " });\n";
}

void CustomWidget::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr)
    {
        onDraw.set_from_arg(sit->callee);

        if (sit->params.size()) {
            auto size = cpp::parse_size(sit->params[0]);
            size_x.set_from_arg(size.first);
            size_y.set_from_arg(size.second);
        }
    }
}

std::vector<UINode::Prop>
CustomWidget::Properties()
{
    return Widget::Properties();
}

bool CustomWidget::PropertyUI(int i, UIContext& ctx)
{
    return Widget::PropertyUI(i, ctx);
}

std::vector<UINode::Prop>
CustomWidget::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "customWidget.draw", &onDraw },
        });
    return props;
}

bool CustomWidget::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("Draw");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Draw", &onDraw, 0, ctx);
        break;
    default:
        return Widget::EventUI(i - 1, ctx);
    }
    return changed;
}
