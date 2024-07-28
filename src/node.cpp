#include "node.h"
#include "stx.h"
#include "imrad.h"
#include "cppgen.h"
#include "binding_input.h"
#include "binding_field.h"
#include "ui_table_columns.h"
#include "ui_message_box.h"
#include "ui_combo_dlg.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <nfd.h>
#include <IconsFontAwesome6.h>
#include <algorithm>
#include <array>

const color32 FIELD_NAME_CLR = IM_COL32(222, 222, 255, 255);

const std::string HBOX_NAME = "hb";
const std::string VBOX_NAME = "vb";

#define DRAW_STR(a) cpp::to_draw_str(a.value()).c_str()


void toggle(std::vector<UINode*>& c, UINode* val)
{
	auto it = stx::find(c, val);
	if (it == c.end())
		c.push_back(val);
	else
		c.erase(it);
}

template <class F>
void TreeNodeProp(const char* name, const std::string& label, F&& f)
{
	ImVec2 pad = ImGui::GetStyle().FramePadding;
	ImGui::Unindent();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, pad.y });
	if (ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_SpanAvailWidth)) {
		ImGui::PopStyleVar();
		ImGui::Indent();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { pad.x, 0 }); //row packing
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
		ImRad::Selectable(label.c_str(), false, ImGuiSelectableFlags_Disabled, { -ImGui::GetFrameHeight(), 0 });
	}
	ImGui::Indent();
}

bool DataLoopProp(const char* name, data_loop* val, UIContext& ctx)
{
	bool changed = false;
	ImVec2 pad = ImGui::GetStyle().FramePadding;
	ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
	ImGui::Unindent();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, pad.y });
	bool open = ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_SpanAvailWidth);
	ImGui::PopStyleVar();
	ImGui::TableNextColumn();
	ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
	changed |= InputDataSize("##size", &val->limit, true, ctx);
	//changed = ImGui::InputText("##size", val->limit.access()); //allow empty
	ImGui::SameLine(0, 0);
	changed |= BindingButton(name, &val->limit, ctx);
	if (open)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Indent();
		ImGui::AlignTextToFramePadding();
		ImGui::BeginDisabled(val->empty());
		ImGui::Text("index");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed |= InputFieldRef("##index", &val->index, true, ctx);
		ImGui::EndDisabled();
		ImGui::TreePop();
	}
	else
	{
		ImGui::Indent();
	}
	return changed;
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

//----------------------------------------------------

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
	ctx.snapSameLine = false;
	ctx.snapNextColumn = 0;
	ctx.snapSetNextSameLine = false;
	ctx.snapClearNextNextColumn = false;

	const float MARGIN = 7;
	assert(ctx.parents.back() == this);
	size_t level = ctx.parents.size() - 1;
	int snapOp = Behavior();
	ImVec2 m = ImGui::GetMousePos();
	ImVec2 d1 = m - cached_pos;
	ImVec2 d2 = cached_pos + cached_size - m;
	float mind = std::min({ d1.x, d1.y, d2.x, d2.y });

	//snap interior (first child)
	if ((snapOp & SnapInterior) && 
		mind >= 2 && //allow snapping sides with zero border
		!stx::count_if(children, [](const auto& ch) { return ch->Behavior() & SnapSides; }))
	{
		ctx.snapParent = this;
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
		return;
	
	ImVec2 p;
	float w = 0, h = 0;
	const auto& pch = pchildren[i];
	//snapRight and snapDown will extend the area to the next widget
	//snapLeft and snapTop only when we are first widget
	switch (snapDir)
	{
	case ImGuiDir_Left:
	{
		p = cached_pos;
		h = cached_size.y;
		bool leftmost = !i || !pch->sameLine || pch->nextColumn;
		if (!leftmost) //handled by right(i-1)
			return;
		if (pch->nextColumn && m.x < pch->cached_pos.x)
			return;
		ctx.snapParent = parent;
		ctx.snapIndex = i;
		ctx.snapSameLine = pchildren[i]->sameLine;
		ctx.snapNextColumn = pchildren[i]->nextColumn;
		ctx.snapSetNextSameLine = true;
		break;
	}
	case ImGuiDir_Right:
	{
		p = cached_pos + ImVec2(cached_size.x, 0);
		h = cached_size.y;
		auto* nch = i + 1 < pchildren.size() ? pchildren[i + 1].get() : nullptr;
		if (nch && !nch->sameLine)
		{
			for (int j = (int)i + 1; j < (int)pchildren.size(); ++j)
				if (pchildren[j]->nextColumn) {
					nch = pchildren[j].get();
					break;
				}
		}
		if (nch && (nch->sameLine || nch->nextColumn))
		{
			if (m.x >= nch->cached_pos.x) //no margin, left(i+1) can differ
				return;
		}
		nch = i + 1 < pchildren.size() ? pchildren[i + 1].get() : nullptr;
		if (nch && nch->sameLine && !nch->nextColumn)
		{
			//same effect for insering right(i) or left(i+1) so avg marker
			auto p2 = nch->cached_pos;
			auto h2 = nch->cached_size.y;
			ImVec2 q{ (p.x + p2.x) / 2, std::min(p.y, p2.y) };
			h = std::max(p.y + h, p2.y + h2) - q.y;
			p = q;
		}
		ctx.snapParent = parent;
		ctx.snapIndex = i + 1;
		ctx.snapSameLine = true;
		ctx.snapNextColumn = 0;
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
		bool topmost = true;
		for (int j = (int)i; j >= 0; --j)
		{
			if (pchildren[j]->nextColumn)
				break;
			if (j && 
				(pchildren[j - 1]->Behavior() & SnapSides) && //ignore preceeding MenuBar etc.
				!pchildren[j]->sameLine && 
				!pchildren[j]->nextColumn)
				topmost = false;
		}
		//find range of widgets in the same row and column
		size_t i1 = i, i2 = i;
		for (int j = (int)i - 1; j >= 0; --j)
		{
			if (!pchildren[j + 1]->sameLine || pchildren[j + 1]->nextColumn) 
				break;
			i1 = j;
			const auto& ch = pchildren[j];
			p.x = ch->cached_pos.x;
			if (down)
				p.y = std::max(p.y, ch->cached_pos.y + ch->cached_size.y);
			else
				p.y = std::min(p.y, ch->cached_pos.y);
		}
		for (int j = (int)i + 1; j < (int)pchildren.size(); ++j)
		{
			if (!pchildren[j]->sameLine || pchildren[j]->nextColumn)
				break;
			i2 = j;
			const auto& ch = pchildren[j];
			x2 = ch->cached_pos.x + ch->cached_size.x;
			if (down)
				p.y = std::max(p.y, ch->cached_pos.y + ch->cached_size.y);
			else
				p.y = std::min(p.y, ch->cached_pos.y);
		}
		w = x2 - p.x;
		if (down)
		{
			const auto* nch = i2 + 1 < pchildren.size() ? pchildren[i2 + 1].get() : nullptr;
			//check m.y not pointing in next row
			//we assume all columns belong to the same row and there is no more rows so
			//we don't check
			if (nch && !nch->nextColumn)
			{
				if (m.y >= nch->cached_pos.y + MARGIN)
					return;
				p.y = (p.y + nch->cached_pos.y) / 2;
			}
			ctx.snapParent = parent;
			ctx.snapIndex = i2 + 1;
			ctx.snapSameLine = false;
			ctx.snapNextColumn = 0;
		}
		else
		{
			if (!topmost) //up(i) is handled by down(i-1)
				return;
			ctx.snapParent = parent;
			ctx.snapIndex = i1;
			ctx.snapSameLine = false;
			ctx.snapNextColumn = pchildren[i1]->nextColumn;
			ctx.snapClearNextNextColumn = true;
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
	
	if (cached_pos.x > r.Min.x &&
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

std::string UINode::GetParentId(UIContext& ctx)
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

TopWindow::TopWindow(UIContext& ctx)
	: kind(Kind(ctx.kind))
{
	if (kind == Activity) {
		title = "MyActivity";
		size_x = 400;
		size_y = 700;
	}
	else {
		size_x = 640;
		size_y = 480;
	}

	flags.prefix("ImGuiWindowFlags_");
	flags.add$(ImGuiWindowFlags_NoTitleBar);
	flags.add$(ImGuiWindowFlags_NoResize);
	flags.add$(ImGuiWindowFlags_NoMove);
	flags.add$(ImGuiWindowFlags_NoScrollbar);
	flags.add$(ImGuiWindowFlags_NoScrollWithMouse);
	flags.add$(ImGuiWindowFlags_NoCollapse);
	flags.add$(ImGuiWindowFlags_AlwaysAutoResize);
	flags.add$(ImGuiWindowFlags_NoBackground);
	flags.add$(ImGuiWindowFlags_NoSavedSettings);
	flags.add$(ImGuiWindowFlags_MenuBar);
	flags.add$(ImGuiWindowFlags_AlwaysHorizontalScrollbar);
	flags.add$(ImGuiWindowFlags_AlwaysVerticalScrollbar);
	flags.add$(ImGuiWindowFlags_NoNavInputs);
	flags.add$(ImGuiWindowFlags_NoNavFocus);
	flags.add$(ImGuiWindowFlags_NoDocking);

	placement.add(" ", None);
	placement.add$(Left);
	placement.add$(Right);
	placement.add$(Top);
	placement.add$(Bottom);
	placement.add$(Center);
	placement.add$(Maximize);
}

void TopWindow::Draw(UIContext& ctx)
{
	ctx.unit = ctx.unit == "px" ? "" : ctx.unit;
	ctx.root = this;
	ctx.isAutoSize = flags & ImGuiWindowFlags_AlwaysAutoResize;
	ctx.prevLayoutHash = ctx.layoutHash;
	ctx.layoutHash = ctx.isAutoSize;
	bool dimAll = ctx.activePopups.size();
	ctx.activePopups.clear();
	ctx.parents = { this };
	ctx.hovered = nullptr;
	ctx.snapParent = nullptr;
	ctx.kind = kind;
	ctx.contextMenus.clear();
	
	std::string cap = title.value();
	cap += "###TopWindow"; //don't clash 
	int fl = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings;
	if (kind == MainWindow)
		fl |= ImGuiWindowFlags_NoCollapse;
	else if (kind == Popup)
		fl |= ImGuiWindowFlags_NoTitleBar;
	else if (kind == Activity) //only allow resizing here to test layout behavior
		fl |= ImGuiWindowFlags_NoTitleBar /*| ImGuiWindowFlags_NoResize*/ | ImGuiWindowFlags_NoCollapse;
	fl |= flags;

	if (style_font.has_value())
		ImGui::PushFont(ImRad::GetFontByName(style_font.eval(ctx)));
	if (style_padding.has_value())
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style_padding.eval_px(ctx));
	if (style_spacing.has_value())
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style_spacing.eval_px(ctx));
	if (style_borderSize.has_value())
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, style_borderSize.eval_px(ctx));
	if (style_rounding.has_value())
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, style_rounding.eval_px(ctx));
	if (style_scrollbarSize.has_value())
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, style_scrollbarSize.eval_px(ctx));
	if (!style_bg.empty())
		ImGui::PushStyleColor(ImGuiCol_WindowBg, style_bg.eval(ctx));
	if (!style_menuBg.empty())
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, style_menuBg.eval(ctx));

	ImGui::SetNextWindowScroll({ 0, 0 }); //click on a child causes scrolling which doesn't go back
	ImGui::SetNextWindowPos(ctx.designAreaMin + ImVec2{ 20, 20 }); // , ImGuiCond_Always, { 0.5, 0.5 });
	
	if (kind == MainWindow && placement == Maximize) 
	{
		ImGui::SetNextWindowSize(ctx.designAreaMax - ctx.designAreaMin - ImVec2{ 40, 40 });
	}
	else if (fl & ImGuiWindowFlags_AlwaysAutoResize)
	{
		//prevent autosized window to collapse 
		ImGui::SetNextWindowSizeConstraints({ 30, 30 }, { FLT_MAX, FLT_MAX });
	}
	else
	{
		float w = size_x.eval_px(ImGuiAxis_X, ctx);
		if (!w)
			w = 640;
		float h = size_y.eval_px(ImGuiAxis_Y, ctx);
		if (!h)
			h = 480;
		ImGui::SetNextWindowSize({ w, h });
	}
	
	if (style_titlePadding.has_value())
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, style_titlePadding.eval_px(ctx));
	
	bool tmp;
	ImGui::Begin(cap.c_str(), &tmp, fl);
	
	if (style_titlePadding.has_value())
		ImGui::PopStyleVar();
	
	ImGui::SetWindowFontScale(ctx.zoomFactor);

	ctx.rootWin = ImGui::FindWindowByName(cap.c_str());
	assert(ctx.rootWin);
	for (int i = 0; i < 4; ++i) 
	{
		if (ImGui::GetActiveID() == ImGui::GetWindowResizeCornerID(ctx.rootWin, i) ||
			ImGui::GetActiveID() == ImGui::GetWindowResizeBorderID(ctx.rootWin, (ImGuiDir)i))
			ctx.beingResized = true;
	}
	if (ctx.beingResized && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		ctx.beingResized = false;
		ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left] = false; //eat event so random widget won't get selected
	}
	if (ctx.beingResized)
		ctx.selected = { this }; //reset selection so there is no visual noise

	if (placement == Left)
		ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCornersLeft);
	else if (placement == Right)
		ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCornersRight);
	else if (placement == Top)
		ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCornersTop);
	else if (placement == Bottom)
		ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCornersBottom);

	ImGui::GetCurrentContext()->NavDisableMouseHover = true;
	ImGui::GetCurrentContext()->NavDisableHighlight = true;
	if (dimAll)
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.3f);

	for (size_t i = 0; i < children.size(); ++i)
		children[i]->Draw(ctx);
	
	if (dimAll)
		ImGui::PopStyleVar();
	ImGui::GetCurrentContext()->NavDisableHighlight = false;
	ImGui::GetCurrentContext()->NavDisableMouseHover = false;
	
	//use all client area to allow snapping close to the border
	auto pad = ImGui::GetStyle().WindowPadding - ImVec2(3+1, 3);
	auto mi = ImGui::GetWindowContentRegionMin() - pad;
	auto ma = ImGui::GetWindowContentRegionMax() + pad;
	cached_pos = ImGui::GetWindowPos() + mi;
	cached_size = ma - mi;

	if (!ImGui::GetTopMostAndVisiblePopupModal() && ctx.activePopups.size() &&
		ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		ctx.selected = { ctx.parents[0] };
	}
	bool allowed = !ImGui::GetTopMostAndVisiblePopupModal() && ctx.activePopups.empty();
	if (allowed && ctx.mode == UIContext::NormalSelection)
	{
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			ctx.selStart = ctx.selEnd = ImGui::GetMousePos();
		}
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
			ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
			Norm(ImGui::GetMousePos() - ctx.selStart) > 5)
		{
			ctx.mode = UIContext::RectSelection;
			ctx.selEnd = ImGui::GetMousePos();
		}
	}
	if (ctx.mode == UIContext::NormalSelection &&
		ImGui::IsWindowHovered() &&
		ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
			; //don't participate in group selection
		else
			ctx.selected = { this };
		ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left] = false; //eat event
		ImGui::SetKeyboardFocusHere(); //for DEL hotkey reaction
	}
	if (ctx.mode == UIContext::RectSelection)
	{
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			ctx.selEnd = ImGui::GetMousePos();
		else {
			ImVec2 a{ std::min(ctx.selStart.x, ctx.selEnd.x), std::min(ctx.selStart.y, ctx.selEnd.y) };
			ImVec2 b{ std::max(ctx.selStart.x, ctx.selEnd.x), std::max(ctx.selStart.y, ctx.selEnd.y) };
			auto sel = FindInRect(ImRect(a, b));
			if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
				for (auto s : sel)
					if (!stx::count(ctx.selected, s))
						ctx.selected.push_back(s);
			}
			else
				ctx.selected = sel;
			ctx.mode = UIContext::NormalSelection; //todo
		}
	}
	if (allowed && ctx.mode == UIContext::PickPoint &&
		!ctx.snapParent &&
		ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
	{
		ctx.snapParent = this;
		ctx.snapIndex = children.size();
	}
	
	if (ctx.mode == UIContext::Snap && !ctx.snapParent && ImGui::IsWindowHovered())
	{
		DrawSnap(ctx);
	}
	if (ctx.mode == UIContext::PickPoint && ctx.snapParent == this)
	{
		DrawInteriorRect(ctx);
	}
	if (ctx.mode == UIContext::RectSelection)
	{
		ImDrawList* dl = ImGui::GetForegroundDrawList();
		ImVec2 a{ std::min(ctx.selStart.x, ctx.selEnd.x), std::min(ctx.selStart.y, ctx.selEnd.y) };
		ImVec2 b{ std::max(ctx.selStart.x, ctx.selEnd.x), std::max(ctx.selStart.y, ctx.selEnd.y) };
		dl->AddRectFilled(a, b, ctx.colors[UIContext::Hovered]);
	}

	ImGui::End();

	if (!style_bg.empty())
		ImGui::PopStyleColor();
	if (!style_menuBg.empty())
		ImGui::PopStyleColor();
	if (style_scrollbarSize.has_value())
		ImGui::PopStyleVar();
	if (style_rounding.has_value())
		ImGui::PopStyleVar();
	if (style_borderSize.has_value())
		ImGui::PopStyleVar();
	if (style_spacing.has_value())
		ImGui::PopStyleVar();
	if (style_padding.has_value())
		ImGui::PopStyleVar();
	if (style_font.has_value())
		ImGui::PopFont();
}

void TopWindow::Export(std::ostream& os, UIContext& ctx)
{
	ctx.varCounter = 1;
	ctx.parents = { this };
	ctx.kind = kind;
	ctx.errors.clear();
	ctx.unit = ctx.unit == "px" ? "" : ctx.unit;
	
	ctx.codeGen->RemovePrefixedVars(HBOX_NAME);
	ctx.codeGen->RemovePrefixedVars(VBOX_NAME);

	//todo: put before ///@ params
	if (userCodeBefore != "")
		os << userCodeBefore << "\n";

	//provide stable ID when title changes
	bindable<std::string> titleId = title;
	*titleId.access() += "###" + ctx.codeGen->GetName();
	std::string tit = titleId.to_arg();
	bool hasMB = children.size() && dynamic_cast<MenuBar*>(children[0].get());
	bool autoSize = flags & ImGuiWindowFlags_AlwaysAutoResize;

	os << ctx.ind << "/// @begin TopWindow\n";
	os << ctx.ind << "auto* ioUserData = (ImRad::IOUserData*)ImGui::GetIO().UserData;\n";

	if (ctx.unit == "dp")
	{
		os << ctx.ind << "const float dp = ioUserData->dpiScale;\n";
	}
	
	if (kind == Popup || kind == ModalPopup)
	{
		os << ctx.ind << "ID = ImGui::GetID(\"###" << ctx.codeGen->GetName() << "\");\n";
	}
	else if (kind == Activity)
	{
		os << ctx.ind << "if (ioUserData->activeActivity != \"" << ctx.codeGen->GetName() << "\")\n";
		ctx.ind_up();
		os << ctx.ind << "return;\n";
		ctx.ind_down();
	}

	if (!style_font.empty())
	{
		os << ctx.ind << "ImGui::PushFont(" << style_font.to_arg() << ");\n";
	}
	if (!style_bg.empty())
	{
		os << ctx.ind << "ImGui::PushStyleColor(" <<
			((kind == Popup || kind == ModalPopup) ? "ImGuiCol_PopupBg" : "ImGuiCol_WindowBg") <<
			", " << style_bg.to_arg() << ");\n";
	}
	if (!style_menuBg.empty())
	{
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_MenuBarBg, " << style_menuBg.to_arg() << ");\n";
	}
	if (style_padding.has_value())
	{
		os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, "
			<< style_padding.to_arg(ctx.unit) << ");\n";
	}
	if (style_spacing.has_value())
	{
		os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, "
			<< style_spacing.to_arg(ctx.unit) << ");\n";
	}
	if (style_rounding.has_value())
	{
		os << ctx.ind << "ImGui::PushStyleVar(";
		//ModalPopup uses WindowRounding
		os << (kind == Popup ? "ImGuiStyleVar_PopupRounding" : "ImGuiStyleVar_WindowRounding");
		os << ", " << style_rounding.to_arg(ctx.unit) << ");\n";
	}
	if (style_borderSize.has_value() && kind != Activity && kind != MainWindow)
	{
		os << ctx.ind << "ImGui::PushStyleVar(";
		os << ((kind == Popup || kind == ModalPopup) ? "ImGuiStyleVar_PopupBorderSize" : "ImGuiStyleVar_WindowBorderSize");
		os << ", " << style_borderSize.to_arg(ctx.unit) << ");\n";
	}
	if (style_scrollbarSize.has_value())
	{
		os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, "
			<< style_scrollbarSize.to_arg(ctx.unit) << ");\n";
	}

	if (kind == MainWindow)
	{
		os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);\n";
		os << ctx.ind << "glfwSetWindowTitle(window, " << title.to_arg() << ");\n";
		os << ctx.ind << "ImGui::SetNextWindowPos({ 0, 0 });\n";
		if (!autoSize) 
		{
			os << ctx.ind << "int tmpWidth, tmpHeight;\n";
			os << ctx.ind << "glfwGetWindowSize(window, &tmpWidth, &tmpHeight);\n";
			os << ctx.ind << "ImGui::SetNextWindowSize({ (float)tmpWidth, (float)tmpHeight });\n";
		}
		
		flags_helper fl = flags;
		fl |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings;
		os << ctx.ind << "bool tmpOpen;\n";
		os << ctx.ind << "if (ImGui::Begin(\"###" << ctx.codeGen->GetName() << "\", &tmpOpen, " << fl.to_arg() << "))\n";
		os << ctx.ind << "{\n";
		ctx.ind_up();
		
		if (autoSize)
		{
			os << ctx.ind << "if (ImGui::IsWindowAppearing())\n" << ctx.ind << "{\n";
			ctx.ind_up();
			//to prevent edge at right/bottom border. Not sure why ImGui needs it
			os << ctx.ind << "ImGui::GetStyle().DisplaySafeAreaPadding = { 0, 0 };\n";
			os << ctx.ind << "glfwSetWindowAttrib(window, GLFW_RESIZABLE, false);\n";
			os << ctx.ind << "glfwSetWindowAttrib(window, GLFW_DECORATED, "
				<< std::boolalpha << !(flags & ImGuiWindowFlags_NoTitleBar) << ");\n";
			ctx.ind_down();
			os << ctx.ind << "}\n";
			//glfwSetWindowSize only when SizeFull is determined
			//alternatively calculate it from ContentSize + padding
			os << ctx.ind << "else\n" << ctx.ind << "{\n";
			ctx.ind_up();
			os << ctx.ind << "ImVec2 size = ImGui::GetCurrentWindow()->SizeFull;\n";
			os << ctx.ind << "glfwSetWindowSize(window, (int)size.x, (int)size.y);\n";
			ctx.ind_down();
			os << ctx.ind << "}\n";
		}
		else
		{
			os << ctx.ind << "if (ImGui::IsWindowAppearing())\n" << ctx.ind << "{\n";
			ctx.ind_up();
			if (placement == Maximize)
				os << ctx.ind << "glfwMaximizeWindow(window);\n";
			else
				os << ctx.ind << "glfwSetWindowSize(window, " << size_x.to_arg(ctx.unit)
				<< ", " << size_y.to_arg(ctx.unit) << ");\n";
			
			os << ctx.ind << "glfwSetWindowAttrib(window, GLFW_RESIZABLE, "
				<< std::boolalpha << !(flags & ImGuiWindowFlags_NoResize) << ");\n";
			os << ctx.ind << "glfwSetWindowAttrib(window, GLFW_DECORATED, "
				<< std::boolalpha << !(flags & ImGuiWindowFlags_NoTitleBar) << ");\n";
			ctx.ind_down();
			os << ctx.ind << "}\n";
		}
	}
	else if (kind == Activity)
	{
		os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);\n";
		os << ctx.ind << "ImGui::SetNextWindowPos(ioUserData->WorkRect().Min);\n";
		os << ctx.ind << "ImGui::SetNextWindowSize(ioUserData->WorkRect().GetSize());";
		//signal designed size
		os << " //{ " << size_x.to_arg(ctx.unit) << ", " << size_y.to_arg(ctx.unit) << " }\n";
		flags_helper fl = flags;
		fl |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
		os << ctx.ind << "bool tmpOpen;\n";
		os << ctx.ind << "if (ImGui::Begin(\"###" << ctx.codeGen->GetName() << "\", &tmpOpen, " << fl.to_arg() << "))\n";
		os << ctx.ind << "{\n";
		ctx.ind_up();

		if (!onBackButton.empty()) {
			os << ctx.ind << "if (ImGui::IsKeyPressed(ImGuiKey_AppBack))\n";
			ctx.ind_up();
			os << ctx.ind << onBackButton.to_arg() << "();\n";
			ctx.ind_down();
		}
	}
	else if (kind == Window)
	{
		if (!autoSize)
		{
			os << ctx.ind << "ImGui::SetNextWindowSize({ "
				<< size_x.to_arg(ctx.unit) << ", " << size_y.to_arg(ctx.unit) << " }, "
				<< "ImGuiCond_FirstUseEver);\n";
		}
		if (style_titlePadding.has_value())
		{
			os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, " <<
				style_titlePadding.to_arg(ctx.unit) << ");\n";
		}
		os << ctx.ind << "if (isOpen && ImGui::Begin(" << tit << ", &isOpen, " << flags.to_arg() << "))\n";
		os << ctx.ind << "{\n";
		ctx.ind_up();
		if (style_titlePadding.has_value())
		{
			os << ctx.ind << "ImGui::PopStyleVar();\n";
			os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImGui::GetStyle().FramePadding);\n";
		}
	}
	else if (kind == Popup || kind == ModalPopup)
	{
		if (placement == Left || placement == Top)
		{
			os << ctx.ind << "ImGui::SetNextWindowPos(";
			if (animate)
				os << "{ ioUserData->WorkRect().Min.x + animPos.x, ioUserData->WorkRect().Min.y + animPos.y }";
			else
				os << "ioUserData->WorkRect().Min";
			os << "); " << (placement == Left ? " //Left\n" : " //Top\n");
		}
		else if (placement == Right || placement == Bottom)
		{
			os << ctx.ind << "ImGui::SetNextWindowPos(";
			if (animate)
				os << "{ ioUserData->WorkRect().Max.x - animPos.x, ioUserData->WorkRect().Max.y - animPos.y }, 0, { 1, 1 }";
			else
				os << "ioUserData->WorkRect().Max, 0, { 1, 1 }";
			os << "); " << (placement == Right ? " //Right\n" : " //Bottom\n");
		}
		else if (placement == Center)
		{
			os << ctx.ind << "ImGui::SetNextWindowPos(";
			if (animate)
				os << "{ ioUserData->WorkRect().GetCenter().x, ioUserData->WorkRect().GetCenter().y + animPos.y }, 0, { 0.5f, 0.5f }";
			else
				os << "ioUserData->WorkRect().GetCenter(), 0, { 0.5f, 0.5f }";
			os << "); //Center\n";
		}

		//size
		os << ctx.ind << "ImGui::SetNextWindowSize({ ";

		if (placement == Top || placement == Bottom)
			os << "ioUserData->WorkRect().GetWidth()";
		else if (autoSize)
			os << "0";
		else
			os << size_x.to_arg(ctx.unit);

		os << ", ";

		if (placement == Left || placement == Right)
			os << "ioUserData->WorkRect().GetHeight()";
		else if (autoSize)
			os << "0";
		else
			os << size_y.to_arg(ctx.unit);

		os << " }";
		if (!autoSize && placement != Left && placement != Right && placement != Top && placement != Bottom)
			os << ", ImGuiCond_FirstUseEver";
		os << ");";
		//signal designed size
		os << " //{ " << size_x.to_arg(ctx.unit) << ", " << size_y.to_arg(ctx.unit) << " }\n";

		//begin
		if (kind == ModalPopup)
		{
			if (style_titlePadding.has_value())
			{
				os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, " <<
					style_titlePadding.to_arg(ctx.unit) << ");\n";
			}
			os << ctx.ind << "bool tmpOpen = true;\n";
			os << ctx.ind << "if (ImGui::BeginPopupModal(" << tit << ", &tmpOpen, " << flags.to_arg() << "))\n";
			os << ctx.ind << "{\n";
			ctx.ind_up();
			if (style_titlePadding.has_value())
			{
				os << ctx.ind << "ImGui::PopStyleVar();\n";
				os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImGui::GetStyle().FramePadding);\n";
			}
		}
		else
		{
			os << ctx.ind << "if (ImGui::BeginPopup(" << tit << ", " << flags.to_arg() << "))\n";
			os << ctx.ind << "{\n";
			ctx.ind_up();
		}
		
		//animation
		if (animate)
		{
			os << ctx.ind << "animator.Tick();\n";
			if (placement == Left || placement == Right || placement == Top || placement == Bottom)
			{
				os << ctx.ind << "if (!ImRad::MoveWhenDragging(";
				if (placement == Left)
					os << "ImGuiDir_Left";
				else if (placement == Right)
					os << "ImGuiDir_Right";
				else if (placement == Top)
					os << "ImGuiDir_Up";
				else if (placement == Bottom)
					os << "ImGuiDir_Down";
				os << ", animPos, ioUserData->dimBgRatio))\n";
				ctx.ind_up();
				os << ctx.ind << "ClosePopup();\n";
				ctx.ind_down();
			}
		}

		//back button
		if (!onBackButton.empty())
		{
			os << ctx.ind << "if (ImGui::IsKeyPressed(ImGuiKey_AppBack))\n";
			ctx.ind_up();
			os << ctx.ind << onBackButton.to_arg() << "();\n";
			ctx.ind_down();
		}

		//rendering
		if (kind == ModalPopup)
		{
			os << ctx.ind << "if (ioUserData->activeActivity != \"\")\n";
			ctx.ind_up();
			os << ctx.ind << "ImRad::RenderDimmedBackground(ioUserData->WorkRect(), ioUserData->dimBgRatio);\n";
			ctx.ind_down();
		}
		if (style_rounding &&
			(placement == Left || placement == Right || placement == Top || placement == Bottom))
		{
			os << ctx.ind << "ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCorners";
			if (placement == Left)
				os << "Left";
			else if (placement == Right)
				os << "Right";
			else if (placement == Top)
				os << "Top";
			else if (placement == Bottom)
				os << "Bottom";
			os << ");\n";
		}

		//CloseCurrentPopup
		os << ctx.ind << "if (modalResult != ImRad::None";
		if (animate)
			os << " && animator.IsDone()";
		os << ")\n";
		os << ctx.ind << "{\n";
		ctx.ind_up();
		os << ctx.ind << "ImGui::CloseCurrentPopup();\n";
		//callback is called after CloseCurrentPopup so it is able to open another dialog
		//without calling Draw from it
		if (kind == ModalPopup)
		{
			os << ctx.ind << "if (modalResult != ImRad::Cancel)\n";
			ctx.ind_up();
			os << ctx.ind << "callback(modalResult);\n";
			ctx.ind_down();
		}
		ctx.ind_down();
		os << ctx.ind << "}\n";

		if (closeOnEscape)
		{
			os << ctx.ind << "if (ImGui::Shortcut(ImGuiKey_Escape))\n";
			ctx.ind_up();
			os << ctx.ind << "ClosePopup();\n";
			ctx.ind_down();
		}
	}
	
	if (!onWindowAppearing.empty())
	{
		os << ctx.ind << "if (ImGui::IsWindowAppearing())\n";
		ctx.ind_up();
		os << ctx.ind << onWindowAppearing.to_arg() << "();\n";
		ctx.ind_down();
	}

	os << ctx.ind << "/// @separator\n\n";
	
	//at next import comment becomes userCode
	/*if (userCodeMid != "")
		os << userCodeMid << "\n";*/
	if (children.empty() || children[0]->userCodeBefore.empty())
		os << ctx.ind << "// TODO: Add Draw calls of dependent popup windows here\n\n";

	for (const auto& ch : children)
		ch->Export(os, ctx);
		
	os << ctx.ind << "/// @separator\n";
	
	if (kind == Popup || kind == ModalPopup)
	{
		os << ctx.ind << "ImGui::EndPopup();\n";
		ctx.ind_down();
		os << ctx.ind << "}\n";
	}
	else
	{
		os << ctx.ind << "ImGui::End();\n";
		ctx.ind_down();
		os << ctx.ind << "}\n";
	}

	if (style_titlePadding.has_value() && (kind == Window || kind == ModalPopup))
		os << ctx.ind << "ImGui::PopStyleVar();\n";
	if (style_scrollbarSize.has_value())
		os << ctx.ind << "ImGui::PopStyleVar();\n";
	if (style_borderSize.has_value() || kind == Activity || kind == MainWindow)
		os << ctx.ind << "ImGui::PopStyleVar();\n";
	if (style_rounding.has_value())
		os << ctx.ind << "ImGui::PopStyleVar();\n";
	if (style_spacing.has_value())
		os << ctx.ind << "ImGui::PopStyleVar();\n";
	if (style_padding.has_value())
		os << ctx.ind << "ImGui::PopStyleVar();\n";
	if (!style_bg.empty())
		os << ctx.ind << "ImGui::PopStyleColor();\n";
	if (!style_menuBg.empty())
		os << ctx.ind << "ImGui::PopStyleColor();\n";
	if (!style_font.empty())
		os << ctx.ind << "ImGui::PopFont();\n";

	os << ctx.ind << "/// @end TopWindow\n";

	if (userCodeAfter != "")
		os << userCodeAfter << "\n";
}

void TopWindow::Import(cpp::stmt_iterator& sit, UIContext& ctx)
{
	ctx.errors.clear();
	bool tmpCreateDeps = ctx.createVars;
	ctx.createVars = false;
	ctx.importState = 2; //1 - inside TopWindow block, 2 - before TopWindow or outside @begin/end/separator, 3 - after @end TopWindow
	ctx.userCode = "";
	ctx.root = this;
	ctx.parents = { this };
	ctx.kind = kind = Window;
	bool hasGlfw = false;
	bool windowAppearingBlock = false;
	
	while (sit != cpp::stmt_iterator())
	{
		bool parseCommentSize = false;
		bool parseCommentPos = false;
		
		if (sit->kind == cpp::Comment && !sit->line.compare(0, 20, "/// @begin TopWindow"))
		{
			ctx.importState = 1;
			sit.enable_parsing(true);
			userCodeBefore = ctx.userCode;
			ctx.userCode = "";
		}
		else if (sit->kind == cpp::Comment && !sit->line.compare(0, 11, "/// @begin "))
		{
			ctx.importState = 1;
			sit.enable_parsing(true);
			std::string type = sit->line.substr(11);
			if (auto node = Widget::Create(type, ctx))
			{
				children.push_back(std::move(node));
				children.back()->Import(++sit, ctx); //after insertion
				ctx.importState = 2;
				ctx.userCode = "";
				sit.enable_parsing(false);
			}
		}
		else if (sit->kind == cpp::Comment && !sit->line.compare(0, 18, "/// @end TopWindow"))
		{
			ctx.importState = 3;
			ctx.userCode = "";
			sit.enable_parsing(false);
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
				else
					userCodeMid = ctx.userCode;
				ctx.userCode = "";
				ctx.importState = 1;
				sit.enable_parsing(true);
			}
		}
		else if ((ctx.importState == 2 || ctx.importState == 3) && 
				(sit->kind != cpp::Comment || sit->line.compare(0, 5, "/// @")))
		{
			if (ctx.importState == 3 && sit->line.size() && sit->line.back() == '}') { //reached end of Draw
				sit.base().stream().putback('}');
				sit.enable_parsing(true);
				break;
			}
			if (ctx.userCode != "")
				ctx.userCode += "\n";
			ctx.userCode += sit->line;
		}
		else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::IsWindowAppearing")
		{
			windowAppearingBlock = true;
		}
		else if (sit->kind == cpp::Other && sit->line == "else" && 
			sit->level == ctx.importLevel + 1)
		{
			windowAppearingBlock = false;
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushFont")
		{
			if (sit->params.size())
				style_font.set_from_arg(sit->params[0]);
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
		{
			if (sit->params[0] == "ImGuiCol_WindowBg" || sit->params[0] == "ImGuiCol_PopupBg")
				style_bg.set_from_arg(sit->params[1]);
			else if (sit->params[0] == "ImGuiCol_MenuBarBg")
				style_menuBg.set_from_arg(sit->params[1]);
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar" && sit->params.size() == 2)
		{
			if (sit->params[0] == "ImGuiStyleVar_WindowPadding")
				style_padding.set_from_arg(sit->params[1]);
			else if (sit->params[0] == "ImGuiStyleVar_ItemSpacing")
				style_spacing.set_from_arg(sit->params[1]);
			else if (sit->params[0] == "ImGuiStyleVar_WindowRounding" || sit->params[0] == "ImGuiStyleVar_PopupRounding")
				style_rounding.set_from_arg(sit->params[1]);
			else if (sit->params[0] == "ImGuiStyleVar_WindowBorderSize" || sit->params[0] == "ImGuiStyleVar_PopupBorderSize")
				style_borderSize.set_from_arg(sit->params[1]);
			else if (sit->params[0] == "ImGuiStyleVar_ScrollbarSize")
				style_scrollbarSize.set_from_arg(sit->params[1]);
			else if (sit->params[0] == "ImGuiStyleVar_FramePadding" && style_titlePadding.empty())
				style_titlePadding.set_from_arg(sit->params[1]);
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextWindowPos")
		{
			parseCommentPos = true;
			animate = sit->line.find("animPos") != std::string::npos;
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextWindowSize")
		{
			parseCommentSize = true;
			if (sit->params.size())
			{
				auto size = cpp::parse_size(sit->params[0]);
				size_x.set_from_arg(size.first);
				size_y.set_from_arg(size.second);
			}
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "glfwSetWindowTitle")
		{
			hasGlfw = true;
			if (sit->params.size() >= 2)
				title.set_from_arg(sit->params[1]);
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "glfwSetWindowSize" && 
			windowAppearingBlock)
		{
			hasGlfw = true;
			if (sit->params.size() >= 3) {
				size_x.set_from_arg(sit->params[1]);
				size_y.set_from_arg(sit->params[2]);
			}
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "glfwMaximizeWindow")
		{
			hasGlfw = true;
			placement = Maximize;
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "glfwSetWindowAttrib"
			&& sit->params.size() >= 3)
		{
			hasGlfw = true;
			if (sit->params[1] == "GLFW_RESIZABLE") {
				bool resizable = sit->params[2] != "false";
				if (!resizable && !(flags & ImGuiWindowFlags_AlwaysAutoResize))
					flags |= ImGuiWindowFlags_NoResize;
			}
			else if (sit->params[1] == "GLFW_DECORATED") {
				bool decorated = sit->params[2] != "false";
				if (!decorated)
					flags |= ImGuiWindowFlags_NoTitleBar;
			}
		}
		else if (sit->kind == cpp::IfCallThenCall && sit->cond == "ImGui::IsKeyPressed(ImGuiKey_AppBack)")
		{
			onBackButton.set_from_arg(sit->callee2);
		}
		else if (sit->kind == cpp::IfCallThenCall && sit->cond == "ImGui::Shortcut(ImGuiKey_Escape)" &&
			sit->callee2 == "ClosePopup")
		{
			closeOnEscape = true;
		}
		else if (sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::IsWindowAppearing")
		{
			if (sit->params.empty() && sit->params2.empty())
				onWindowAppearing.set_from_arg(sit->callee2);
		}
		else if ((sit->kind == cpp::IfCallBlock || sit->kind == cpp::CallExpr) &&
			sit->callee == "ImGui::BeginPopupModal")
		{
			ctx.kind = kind = ModalPopup;
			title.set_from_arg(sit->params[0]);
			size_t i = title.access()->rfind("###");
			if (i != std::string::npos)
				title.access()->resize(i);

			if (sit->params.size() >= 3)
				flags.set_from_arg(sit->params[2]);
		}
		else if ((sit->kind == cpp::IfCallBlock || sit->kind == cpp::CallExpr) &&
			sit->callee == "ImGui::BeginPopup")
		{
			ctx.kind = kind = Popup;
			title.set_from_arg(sit->params[0]);
			size_t i = title.access()->rfind("###");
			if (i != std::string::npos)
				title.access()->resize(i);

			if (sit->params.size() >= 2)
				flags.set_from_arg(sit->params[1]);
		}
		else if (sit->kind == cpp::IfCallBlock && sit->callee == "isOpen&&ImGui::Begin")
		{
			ctx.kind = kind = Window;
			title.set_from_arg(sit->params[0]);
			size_t i = title.access()->rfind("###");
			if (i != std::string::npos)
				title.access()->resize(i);

			if (sit->params.size() >= 3)
				flags.set_from_arg(sit->params[2]);
		}
		else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::Begin")
		{
			ctx.importLevel = sit->level;
			if (sit->params.size() >= 3)
				flags.set_from_arg(sit->params[2]);
			
			if (hasGlfw) {
				ctx.kind = kind = MainWindow;
				//reset sizes from earlier SetNextWindowSize
				TopWindow def(ctx);
				size_x = def.size_x;
				size_y = def.size_y;
				flags &= ~(ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
			}
			else {
				ctx.kind = kind = Activity;
				placement = None;
				flags &= ~(ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
			}
		}
		
		++sit;

		if (sit->kind == cpp::Comment && parseCommentSize)
		{
			auto size = cpp::parse_size(sit->line.substr(2));
			size_x.set_from_arg(size.first);
			size_y.set_from_arg(size.second);
		}
		else if (sit->kind == cpp::Comment && parseCommentPos)
		{
			if (sit->line == "//Left")
				placement = Left;
			else if (sit->line == "//Right")
				placement = Right;
			else if (sit->line == "//Top")
				placement = Top;
			else if (sit->line == "//Bottom")
				placement = Bottom;
			else if (sit->line == "//Center")
				placement = Center;
			else if (sit->line == "//Maximize")
				placement = Maximize;
		}
	}

	userCodeAfter = ctx.userCode;
	ctx.createVars = tmpCreateDeps;
}

void TopWindow::TreeUI(UIContext& ctx)
{
	static const char* NAMES[]{ "MainWindow", "Window", "Popup", "ModalPopup", "Activity" };

	ctx.parents = { this };
	std::string str = ctx.codeGen->GetName();
	bool selected = stx::count(ctx.selected, this) || ctx.snapParent == this;
	if (selected)
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
	ImGui::SetNextItemOpen(true, ImGuiCond_Always);
	if (ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_SpanAllColumns))
	{
		if (selected)
			ImGui::PopStyleColor();
		bool activated = ImGui::IsItemClicked(); //todo || ImGui::IsItemActivated();
		ImGui::SameLine(0, 0);
		ImGui::TextDisabled(" : %s", NAMES[kind]);
		if (activated)
		{
			if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
				; // don't participate in group selection toggle(ctx.selected, this);
			else
				ctx.selected = { this };
		}
		for (const auto& ch : children)
			ch->TreeUI(ctx);

		ImGui::TreePop();
	}
	else {
		if (selected)
			ImGui::PopStyleColor();
		ImGui::SameLine(0, 0);
		ImGui::TextDisabled(" : %s", NAMES[kind]);
	}
}

std::vector<UINode::Prop>
TopWindow::Properties()
{
	return {
		{ "top.kind", nullptr },
		{ "top.@style.bg", &style_bg },
		{ "top.@style.menuBg", &style_menuBg },
		{ "top.@style.padding", &style_padding },
		{ "top.@style.titlePadding", &style_titlePadding },
		{ "top.@style.rounding", &style_rounding },
		{ "top.@style.borderSize", &style_borderSize },
		{ "top.@style.scrollbarSize", &style_scrollbarSize },
		{ "top.@style.spacing", &style_spacing },
		{ "top.@style.font", &style_font },
		{ "top.flags", nullptr },
		{ "title", &title, true },
		{ "closeOnEscape", &closeOnEscape },
		{ "size_x", &size_x },
		{ "size_y", &size_y },
		{ "placement", &placement },
		{ "animate", &animate },
	};
}

bool TopWindow::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
	{
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(164, 164, 164, 255));
		ImGui::Text("kind");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		int tmp = (int)kind;
		if (ImGui::Combo("##kind", &tmp, "Main Window (GLFW)\0Window\0Popup\0Modal Popup\0Activity (Android)\0"))
		{
			changed = true;
			kind = (Kind)tmp;
		}
		break;
	}
	case 1:
	{
		ImGui::Text("bg");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		int clr = (kind == Popup || kind == ModalPopup) ? ImGuiCol_PopupBg : ImGuiCol_WindowBg;
		changed = InputBindable("##bg", &style_bg, clr, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("bg", &style_bg, ctx);
		break;
	}
	case 2:
		ImGui::Text("menuBarBg");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##menubg", &style_menuBg, ImGuiCol_MenuBarBg, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("menubg", &style_bg, ctx);
		break;
	case 3:
	{
		ImGui::Text("padding");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##style_padding", &style_padding, ctx);
		break;
	}
	case 4:
	{
		ImGui::Text("titlePadding");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##style_titlePadding", &style_titlePadding, ctx);
		break;
	}
	case 5:
	{
		ImGui::Text("rounding");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##style_rounding", &style_rounding, ctx);
		break;
	}
	case 6:
	{
		ImGui::Text("borderSize");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##style_border", &style_borderSize, ctx);
		break;
	}
	case 7:
	{
		ImGui::Text("scrollbarSize");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##style_scrbs", &style_scrollbarSize, ctx);
		break;
	}
	case 8:
	{
		ImGui::Text("spacing");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##style_spacing", &style_spacing, ctx);
		break;
	}
	case 9:
		ImGui::Text("font");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##font", &style_font, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("font", &style_font, ctx);
		break;
	case 10:
		TreeNodeProp("flags", "...", [&] {
			ImGui::TableNextColumn();
			ImGui::Spacing();
			bool hasAutoResize = flags & ImGuiWindowFlags_AlwaysAutoResize;
			bool hasMB = children.size() && dynamic_cast<MenuBar*>(children[0].get());
			changed = CheckBoxFlags(&flags);
			bool flagsMB = flags & ImGuiWindowFlags_MenuBar;
			if (flagsMB && !hasMB)
				children.insert(children.begin(), std::make_unique<MenuBar>(ctx));
			else if (!flagsMB && hasMB)
				children.erase(children.begin());
			});
		break;
	case 11:
		ImGui::Text("title");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##title", &title, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("title", &title, ctx);
		break;
	case 12:
		ImGui::BeginDisabled(kind != Popup && kind != ModalPopup);
		ImGui::Text("closeOnEscape");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##cesc", &closeOnEscape, ctx);
		ImGui::EndDisabled();
		break;
	case 13:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::BeginDisabled((flags & ImGuiWindowFlags_AlwaysAutoResize) || (kind == MainWindow && placement == Maximize));
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, {}, 0, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_x", &size_x, ctx);
		ImGui::EndDisabled();
		break;
	case 14:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::BeginDisabled((flags & ImGuiWindowFlags_AlwaysAutoResize) || (kind == MainWindow && placement == Maximize));
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, {}, 0, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_y", &size_y, ctx);
		ImGui::EndDisabled();
		break;
	case 15:
	{
		ImGui::Text("placement");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		auto tmp = placement;
		bool isPopup = kind == Popup || kind == ModalPopup;
		if (ImGui::BeginCombo("##placement", placement.get_id().c_str()))
		{
			if (ImGui::Selectable("None", placement == None))
				placement = None;
			if (isPopup && ImGui::Selectable("Left", placement == Left))
				placement = Left;
			if (isPopup && ImGui::Selectable("Right", placement == Right))
				placement = Right;
			if (isPopup && ImGui::Selectable("Top", placement == Top))
				placement = Top;
			if (isPopup && ImGui::Selectable("Bottom", placement == Bottom))
				placement = Bottom;
			if (isPopup && ImGui::Selectable("Center", placement == Center))
				placement = Center;
			if (kind == MainWindow && ImGui::Selectable("Maximize", placement == Maximize))
				placement = Maximize;

			ImGui::EndCombo();
		}
		changed = placement != tmp;
		break;
	}
	case 16:
		ImGui::BeginDisabled(kind != Popup && kind != ModalPopup);
		ImGui::Text("animate");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##animate", &animate, ctx);
		ImGui::EndDisabled();
		break;
	default:
		return false;
	}
	return changed;
}

std::vector<UINode::Prop>
TopWindow::Events()
{
	return {
		{ "OnWindowAppearing", &onWindowAppearing },
		{ "OnBackButton", &onBackButton },
	};
}

bool TopWindow::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	int sat = (i & 1) ? 202 : 164;
	ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(255, 255, sat, 255));
	switch (i)
	{
	case 0:
		ImGui::Text("OnWindowAppearing");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##OnWindowAppearing", &onWindowAppearing, ctx);
		break;
	case 1:
		ImGui::Text("OnBackButton");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##OnBackButton", &onBackButton, ctx);
		break;
	}
	return changed;
}

//-------------------------------------------------

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
	else if (name == "MenuIt")
		return std::make_unique<MenuIt>(ctx);
	else if (name == "Splitter")
		return std::make_unique<Splitter>(ctx);
	else
		return {};
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
		bool inTable = dynamic_cast<Table*>(ctx.parents.back());
		if (inTable)
			ImRad::TableNextColumn(nextColumn);
		else
			ImRad::NextColumn(nextColumn);
	}

	if (hasPos) 
	{
		ImRect r = ImGui::GetCurrentWindow()->InnerRect;
		ImVec2 pos{ pos_x.eval_px(ctx), pos_y.eval_px(ctx) };
		if (pos.x < 0)
			pos.x += r.GetWidth();
		if (pos.y < 0)
			pos.y += r.GetHeight();
		ImGui::SetCursorScreenPos(r.Min + pos);
	}
	else if (l.flags & (Layout::HLayout | Layout::VLayout)) 
	{
		if (!(l.flags & Layout::Leftmost)) {
			//we need to provide correct spacing even though SetCursorX is called later
			//because after hbox.Reset() hbox.GetPos() will return CursorPos with a wrong
			//spacing and it will be used in autosized window contentSize
			ImGui::SameLine(0, spacing * ImGui::GetStyle().ItemSpacing.x);
		}

		if (l.flags & Layout::VLayout)
		{
			if (l.colId >= parent->vbox.size())
				parent->vbox.resize(l.colId + 1);
			auto& vbox = parent->vbox[l.colId];
			
			if ((l.flags & Layout::Topmost) && (l.flags & Layout::Leftmost))
				vbox.BeginLayout();
			if (l.flags & Layout::Leftmost)
				ImGui::SetCursorPosY(vbox);
			ctx.stretchSize.y = vbox.GetSize(!(l.flags & Layout::Leftmost));
		}
		else if (l.flags & Layout::Leftmost)
		{
			ImRad::Spacing(spacing - defSpacing);
		}

		if (l.flags & Layout::HLayout)
		{
			if (l.rowId >= parent->hbox.size())
				parent->hbox.resize(l.rowId + 1);
			auto& hbox = parent->hbox[l.rowId];
			
			if (l.flags & Layout::Leftmost)
				hbox.BeginLayout();
			ImGui::SetCursorPosX(hbox); //currently not needed but may be useful if we upgrade layouts
			ctx.stretchSize.x = hbox.GetSize();
		}
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
	cached_pos = ImGui::GetCursorScreenPos();
	auto p1 = ImGui::GetCursorPos();

	if (style_font.has_value()) 
		ImGui::PushFont(ImRad::GetFontByName(style_font.eval(ctx)));
	if (!style_text.empty())
		ImGui::PushStyleColor(ImGuiCol_Text, style_text.eval(ctx));
	if (!style_border.empty())
		ImGui::PushStyleColor(ImGuiCol_Border, style_border.eval(ctx)); 
	if (!style_frameBg.empty())
		ImGui::PushStyleColor(ImGuiCol_FrameBg, style_frameBg.eval(ctx));
	if (!style_frameBorderSize.empty())
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, style_frameBorderSize);
	if (!style_frameRounding.empty())
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, style_frameRounding);
	if (!style_framePadding.empty())
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, style_framePadding);

	ImGui::BeginDisabled((disabled.has_value() && disabled.value()) || (visible.has_value() && !visible.value()));
	ImDrawList* drawList = DoDraw(ctx);
	ImGui::EndDisabled();
	CalcSizeEx(p1, ctx);
	
	if (!style_text.empty())
		ImGui::PopStyleColor();
	if (!style_border.empty())
		ImGui::PopStyleColor();
	if (!style_frameBg.empty())
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
		ImRad::HashCombine(ctx.layoutHash, ImGui::GetItemID());
	if (l.flags & Layout::VLayout) 
	{
		auto& vbox = parent->vbox[l.colId];
		float sizeY = ImRad::VBox::ItemSize;
		if (Behavior() & HasSizeY) {
			sizeY = size_y.stretched() ? ImRad::VBox::Stretch :
				size_y.zero() ? ImRad::VBox::ItemSize :
				size_y.eval_px(ImGuiAxis_Y, ctx);
			ImRad::HashCombine(ctx.layoutHash, sizeY);
		}
		if (l.flags & Layout::Leftmost)
			vbox.AddSize(spacing, sizeY);
		else
			vbox.UpdateSize(0, sizeY);
	}
	if (l.flags & Layout::HLayout)
	{
		auto& hbox = parent->hbox[l.rowId];
		float sizeX = ImRad::HBox::ItemSize;
		if (Behavior() & HasSizeX) {
			sizeX = size_x.stretched() ? ImRad::HBox::Stretch :
				size_x.zero() ? ImRad::HBox::ItemSize :
				size_x.eval_px(ImGuiAxis_X, ctx);
			ImRad::HashCombine(ctx.layoutHash, sizeX);
		}
		int sp = (l.flags & Layout::Leftmost) ? 0 : (int)spacing;
		hbox.AddSize(sp, sizeX);
	}

	//doesn't work for open CollapsingHeader etc:
	//bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
	bool allowed = !ImGui::GetTopMostAndVisiblePopupModal() && 
		(ctx.activePopups.empty() || stx::count(ctx.activePopups, ImGui::GetCurrentWindow()));
	bool hovered = ImGui::IsMouseHoveringRect(cached_pos, cached_pos + cached_size);
	if (ctx.mode == UIContext::NormalSelection &&
		allowed && hovered)
	{
		if (ctx.hovered == lastHovered) //e.g. child widget was not selected
		{
			ctx.hovered = this;
		}
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) //this works even for non-items like TabControl etc.  
		{
			if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
				toggle(ctx.selected, this);
			else
				ctx.selected = { this };
			ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left] = false; //eat event
		}
		bool hoverSizeX = 
			(Behavior() & HasSizeX) &&
			ctx.selected.size() == 1 && ctx.selected[0] == this &&
			std::abs(ImGui::GetMousePos().x - cached_pos.x - cached_size.x) < 5 &&
			size_x.has_value();
		bool hoverSizeY =
			(Behavior() & HasSizeY) &&
			ctx.selected.size() == 1 && ctx.selected[0] == this &&
			std::abs(ImGui::GetMousePos().y - cached_pos.y - cached_size.y) < 5 &&
			size_y.has_value();
		if (hoverSizeX || hoverSizeY)
		{
			ImGui::SetMouseCursor((hoverSizeX && hoverSizeY) ? ImGuiMouseCursor_ResizeNWSE :
				hoverSizeX ? ImGuiMouseCursor_ResizeEW :
				ImGuiMouseCursor_ResizeNS
			);
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				ctx.mode = (hoverSizeX && hoverSizeY) ? UIContext::ItemSizingXY :
					hoverSizeX ? UIContext::ItemSizingX : 
					UIContext::ItemSizingY;
				ctx.dragged = this;
				ctx.lastSize = { size_x.eval_px(ImGuiAxis_X, ctx), size_y.eval_px(ImGuiAxis_Y, ctx) };
				if (!ctx.lastSize.x)
					ctx.lastSize.x = cached_size.x;
				if (!ctx.lastSize.y)
					ctx.lastSize.y = cached_size.y;
			}
		}
		else if (hasPos && ctx.hovered == this && !ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				ctx.mode = UIContext::ItemDragging;
				ctx.dragged = this;
			}
		}
	}
	else if (ctx.mode == UIContext::PickPoint &&
		allowed && hovered &&
		(Behavior() & SnapInterior))
	{
		if (ctx.hovered == lastHovered) //e.g. child widget was not selected
		{
			ctx.snapParent = this;
			ctx.snapIndex = children.size();
		}
	}
	else if (ctx.mode == UIContext::ItemDragging && 
			ctx.dragged == this)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			ImVec2 delta = ImGui::GetMouseDragDelta();
			if (std::signbit(pos_x + delta.x) == std::signbit(float(pos_x)) &&
				std::signbit(pos_y + delta.y) == std::signbit(float(pos_y)))
			{
				pos_x += delta.x;
				pos_y += delta.y;
				ImGui::ResetMouseDragDelta();
			}
		}
		else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			ctx.mode = UIContext::NormalSelection;
			ctx.selected = { this };
			ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left] = false; //eat event
		}
	}
	else if (ctx.mode >= UIContext::ItemSizingX && ctx.mode <= UIContext::ItemSizingXY &&
			ctx.dragged == this)
	{
		ImGui::SetMouseCursor(
			ctx.mode == UIContext::ItemSizingX ? ImGuiMouseCursor_ResizeEW :
			ctx.mode == UIContext::ItemSizingY ? ImGuiMouseCursor_ResizeNS :
			ImGuiMouseCursor_ResizeNWSE
		);
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			ImVec2 delta = ImGui::GetMouseDragDelta();
			ImVec2 sp = ImGui::GetStyle().ItemSpacing;
			if (!sp.x)
				sp.x = 5;
			if (!sp.y)
				sp.y = 5;
			ImGuiWindow* win = ImGui::GetCurrentWindow();
			if (ctx.mode != UIContext::ItemSizingY)
			{
				float val = ctx.lastSize.x + delta.x;
				val = int(val / sp.x) * sp.x;
				if (ctx.lastSize.x < 0 && !val)
					val = -1; //snap to the end
				if (ctx.lastSize.x > 0 && cached_pos.x + val >= win->WorkRect.Max.x)
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
			if (ctx.mode != UIContext::ItemSizingX)
			{
				float val = ctx.lastSize.y + delta.y;
				val = int(val / sp.y) * sp.y;
				if (ctx.lastSize.y < 0 && !val)
					val = -1; //snap to the end
				if (ctx.lastSize.y > 0 && cached_pos.y + val >= win->WorkRect.Max.y)
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
	
	if (hasPos)
	{
		ImU32 clr = ctx.colors[UIContext::Selected];
		float d = 10;
		if (pos_x >= 0 && pos_y >= 0)
			drawList->AddTriangleFilled(cached_pos, cached_pos + ImVec2(d, 0), cached_pos + ImVec2(0, d), clr);
		else if (pos_x < 0 && pos_y >= 0)
			drawList->AddTriangleFilled(cached_pos + ImVec2(cached_size.x, 0), cached_pos + ImVec2(cached_size.x, d), cached_pos + ImVec2(cached_size.x - d, 0), clr);
		else if (pos_x >= 0 && pos_y < 0)
			drawList->AddTriangleFilled(cached_pos + ImVec2(d, cached_size.y), cached_pos + ImVec2(0, cached_size.y), cached_pos + ImVec2(0, cached_size.y - d), clr);
		else if (pos_x < 0 && pos_y < 0)
			drawList->AddTriangleFilled(cached_pos + cached_size, cached_pos + ImVec2(cached_size.x - d, cached_size.y), cached_pos + ImVec2(cached_size.x, cached_size.y - d), clr);
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
	bool selected = stx::count(ctx.selected, this);
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
		(ctx.mode < UIContext::ItemSizingX || ctx.mode > UIContext::ItemSizingXY))
	{
		//dl->PushClipRectFullScreen();
		drawList->AddRect(
			cached_pos - ImVec2(1, 1), cached_pos + cached_size,
			selected ? ctx.colors[UIContext::Selected] : ctx.colors[UIContext::Hovered],
			0, 0, selected ? 2.f : 1.f);
		//dl->PopClipRect();
	}
	if (selected)
	{
		DrawExtra(ctx);
	}
	if (ctx.mode == UIContext::Snap && !ctx.snapParent)
	{
		DrawSnap(ctx);
	}
	if (ctx.mode == UIContext::PickPoint && ctx.snapParent == this)
	{
		DrawInteriorRect(ctx);
	}

	
	ctx.parents.pop_back();
	ImGui::PopID();
}

void Widget::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
	cached_size = ImGui::GetItemRectSize();
}

void Widget::Export(std::ostream& os, UIContext& ctx)
{
	Layout l = GetLayout(ctx.parents.back());
	ctx.stretchSize = { 0, 0 };
	ctx.stretchSizeExpr = { "", "" };
	const int defSpacing = (l.flags & Layout::Topmost) ? 0 : 1;
	std::string hbName, vbName;

	if (userCodeBefore != "")
		os << userCodeBefore << "\n";
	
	std::string stype = typeid(*this).name();
	auto i = stype.find(' ');
	if (i != std::string::npos)
		stype.erase(0, i + 1);
	auto it = stx::find_if(stype, [](char c) { return isalpha(c);});
	if (it != stype.end())
		stype.erase(0, it - stype.begin());
	os << ctx.ind << "/// @begin " << stype << "\n";

	if (!hasPos && nextColumn)
	{
		bool inTable = dynamic_cast<Table*>(ctx.parents.back());
		if (inTable)
			os << ctx.ind << "ImRad::TableNextColumn(" << nextColumn.to_arg() << ");\n";
		else
			os << ctx.ind << "ImRad::NextColumn(" << nextColumn.to_arg() << ");\n";
	}
	if (!visible.has_value() || !visible.value())
	{
		os << ctx.ind << "if (" << visible.c_str() << ")\n" << ctx.ind << "{\n";
		ctx.ind_up();
		os << ctx.ind << "//visible\n";
	}

	if (hasPos)
	{
		os << ctx.ind << "ImGui::SetCursorScreenPos({ ";
		if (pos_x < 0)
			os << "ImGui::GetCurrentWindow()->InnerRect.Max.x" << pos_x.to_arg(ctx.unit);
		else
			os << pos_x.to_arg(ctx.unit);
		os << ", ";
		if (pos_y < 0)
			os << "ImGui::GetCurrentWindow()->InnerRect.Max.y" << pos_y.to_arg(ctx.unit);
		else
			os << pos_y.to_arg(ctx.unit);
		os << " }); //overlayPos\n";
	}
	else if (l.flags & (Layout::HLayout | Layout::VLayout))
	{
		if (!(l.flags & Layout::Leftmost))
			os << ctx.ind << "ImGui::SameLine(0, " << spacing << " * ImGui::GetStyle().ItemSpacing.x);\n";
		if (l.flags & Layout::VLayout)
		{
			std::ostringstream osv;
			osv << VBOX_NAME;
			osv << GetParentId(ctx);
			osv << (l.colId + 1);
			vbName = osv.str();
			ctx.codeGen->CreateNamedVar(vbName, "ImRad::VBox", "", CppGen::Var::Impl);
			
			if ((l.flags & Layout::Topmost) && (l.flags & Layout::Leftmost))
				os << ctx.ind << vbName << ".BeginLayout();\n";
			if (l.flags & Layout::Leftmost)
				os << ctx.ind << "ImGui::SetCursorPosY(" << vbName << ");\n";
			ctx.stretchSizeExpr[1] = vbName + ".GetSize(" +
				((l.flags & Layout::Leftmost) ? "false)" : "true)");
		}
		else if ((l.flags & Layout::Leftmost) && (spacing - defSpacing))
		{
			os << ctx.ind << "ImRad::Spacing(" << (spacing - defSpacing) << ");\n";
		}

		if (l.flags & Layout::HLayout)
		{
			std::ostringstream osv;
			osv << HBOX_NAME;
			osv << GetParentId(ctx);
			osv << (l.rowId + 1);
			hbName = osv.str();
			ctx.codeGen->CreateNamedVar(hbName, "ImRad::HBox", "", CppGen::Var::Impl);

			if (l.flags & Layout::Leftmost)
				os << ctx.ind << hbName << ".BeginLayout();\n";
			os << ctx.ind << "ImGui::SetCursorPosX(" << hbName << ");\n";
			ctx.stretchSizeExpr[0] = hbName + ".GetSize()";
		}
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
		if (indent)
		{
			os << ctx.ind << "ImGui::Indent(" << indent << " * ImGui::GetStyle().IndentSpacing / 2);\n";
		}
	}

	if (allowOverlap)
	{
		os << ctx.ind << "ImGui::SetNextItemAllowOverlap();\n";
	}
	if (!disabled.has_value() || disabled.value())
	{
		os << ctx.ind << "ImGui::BeginDisabled(" << disabled.c_str() << ");\n";
	}
	if (!tabStop)
	{
		os << ctx.ind << "ImGui::PushTabStop(false);\n";
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
			sizeY = size_y.stretched() ? "ImRad::VBox::Stretch" :
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
			sizeX = size_x.stretched() ? "ImRad::HBox::Stretch" :
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
		os << ctx.ind << "ImGui::PopTabStop();\n";
	}
	if (!disabled.has_value() || disabled.value())
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
		os << ctx.ind << "if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))\n";
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
	
	if (!onItemContextMenuClicked.empty())
	{
		os << ctx.ind << "if (ImRad::IsItemContextMenuClicked())\n";
		ctx.ind_up();
		os << ctx.ind << onItemContextMenuClicked.c_str() << "();\n";
		ctx.ind_down();
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

	if (!visible.has_value() || !visible.value())
	{
		ctx.ind_down();
		os << ctx.ind << "}\n";
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

	while (sit != cpp::stmt_iterator())
	{
		cpp::stmt_iterator ifBlockIt;
		cpp::stmt_iterator screenPosIt;

		if (sit->kind == cpp::Comment && !sit->line.compare(0, 11, "/// @begin "))
		{
			ctx.importState = 1;
			sit.enable_parsing(true);
			std::string name = sit->line.substr(11);
			auto w = Widget::Create(name, ctx);
			if (!w) {
				//uknown control 
				//create a placeholder not to break parsing and layout
				ctx.errors.push_back("Encountered an unknown control '" + name + "'");
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
			ifBlockIt = sit; //could be visible block
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
		else if (sit->kind == cpp::CallExpr && !sit->callee.compare(0, VBOX_NAME.size(), VBOX_NAME) &&
			(sit->callee.find(".AddSize") != std::string::npos ||
			 sit->callee.find(".UpdateSize") != std::string::npos))
		{
			if (sit->callee.find(".AddSize") != std::string::npos && sit->params.size())
				spacing.set_from_arg(sit->params[0]);

			if (sit->params.size() >= 2) 
			{
				if (sit->params[1] == "ImRad::VBox::ItemSize")
					size_y = 0;
				else if (sit->params[1] == "ImRad::VBox::Stretch")
					size_y = dimension::GROW;
				else
					size_y.set_from_arg(sit->params[1]);
			}
		}
		else if (sit->kind == cpp::CallExpr && !sit->callee.compare(0, HBOX_NAME.size(), HBOX_NAME) &&
			sit->callee.find(".AddSize") != std::string::npos)
		{
			if (sit->params.size() && sameLine)
				spacing.set_from_arg(sit->params[0]);

			if (sit->params.size() >= 2)
			{
				if (sit->params[1] == "ImRad::HBox::ItemSize")
					size_x = 0;
				else if (sit->params[1] == "ImRad::HBox::Stretch")
					size_x = dimension::GROW;
				else
					size_x.set_from_arg(sit->params[1]);
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
		else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushTabStop")
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
			else
			{
				DoImport(ifBlockIt, ctx);
			}
		}
		if (screenPosIt != cpp::stmt_iterator() && sit != cpp::stmt_iterator())
		{
			if (sit->kind == cpp::Comment && sit->line == "//overlayPos" && sit->params.size())
			{
				hasPos = true;
				auto size = cpp::parse_size(sit->params[0]);
				if (!size.first.compare(0, 42, "ImGui::GetCurrentWindow()->InnerRect.Max.x"))
					pos_x.set_from_arg(size.first.substr(42));
				else
					pos_x.set_from_arg(size.first);
				if (!size.second.compare(0, 42, "ImGui::GetCurrentWindow()->InnerRect.Max.y"))
					pos_y.set_from_arg(size.second.substr(42));
				else
					pos_y.set_from_arg(size.second);
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
		{ "visible", &visible },
		{ "tabStop", &tabStop },
		{ "tooltip", &tooltip },
		{ "contextMenu", &contextMenu },
		{ "cursor", &cursor },
		{ "disabled", &disabled },
	};
	if (Behavior() & SnapSides) //only last section is optional
	{
		props.insert(props.end(), {
			{ "@overlayPos.hasPos", &hasPos },
			{ "@overlayPos.pos_x", &pos_x },
			{ "@overlayPos.pos_y", &pos_y },
			{ "indent", &indent },
			{ "spacing", &spacing },
			{ "sameLine", &sameLine },
			{ "nextColumn", &nextColumn },
			{ "allowOverlap", &allowOverlap },
			});
	}
	else if (!(Behavior() & NoOverlayPos))
	{
		props.insert(props.end(), {
			{ "@overlayPos.hasPos", &hasPos },
			{ "@overlayPos.pos_x", &pos_x },
			{ "@overlayPos.pos_y", &pos_y }
			});
	}

	return props;
}

bool Widget::PropertyUI(int i, UIContext& ctx)
{
	int sat = (i & 1) ? 202 : 164;
	if (i <= 5)
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(sat, 255, sat, 255));
	else
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(255, 255, sat, 255));

	bool snapSides = Behavior() & SnapSides;
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("visible");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##visible", &visible, true, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("visible", &visible, ctx);
		break;
	case 1:
		ImGui::Text("tabStop");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##tabStop", &tabStop, ctx);
		break;
	case 2:
		ImGui::Text("tooltip");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##tooltip", &tooltip, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("tooltip", &tooltip, ctx);
		break;
	case 3:
	{
		ImGui::BeginDisabled(Behavior() & NoContextMenu);
		ImGui::Text("contextMenu");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (ImGui::BeginCombo("##ctxm", contextMenu.c_str()))
		{
			if (ImGui::Selectable(" ", contextMenu.empty()))
			{
				changed = true;
				contextMenu = "";
			}
			for (const std::string& cm : ctx.contextMenus)
			{
				if (ImGui::Selectable(cm.c_str(), cm == contextMenu.c_str()))
				{
					changed = true;
					contextMenu = cm;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::EndDisabled();
		break;
	}
	case 4:
		ImGui::Text("cursor");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::Combo("##cursor", cursor.access(), "Arrow\0TextInput\0ResizeAll\0ResizeNS\0ResizeEW\0ResizeNESW\0ResizeNWSE\0Hand\0NotAllowed\0\0");
		break;
	case 5:
		ImGui::Text("disabled");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##disabled", &disabled, false, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("disabled", &disabled, ctx);
		break;
	case 6:
		ImGui::BeginDisabled(Behavior() & NoOverlayPos);
		ImGui::Text("hasPos");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::Checkbox("##hasPos", hasPos.access());
		ImGui::EndDisabled();
		break;
	case 7:
		ImGui::BeginDisabled(!hasPos);
		ImGui::Text("pos_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputFloat("##pos_x", pos_x.access(), 0, 0, "%.0f");
		ImGui::EndDisabled();
		break;
	case 8:
		ImGui::BeginDisabled(!hasPos);
		ImGui::Text("pos_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputFloat("##pos_y", pos_y.access(), 0, 0, "%.0f");
		ImGui::EndDisabled();
		break;
	case 9: 
		ImGui::BeginDisabled(!snapSides || sameLine);
		ImGui::Text("indent");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputInt("##indent", indent.access());
		/* negative indent is useful
		if (ImGui::IsItemDeactivatedAfterEdit() && indent < 0)
		{
			changed = true;
			indent = 0;
		}*/
		ImGui::EndDisabled();
		break;
	case 10:
		ImGui::BeginDisabled(!snapSides);
		ImGui::Text(sameLine && !nextColumn ? "spacing_x" : "spacing_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputInt("##spacing", spacing.access());
		if (ImGui::IsItemDeactivatedAfterEdit() && spacing < 0)
		{
			changed = true;
			spacing = 0;
		}
		ImGui::EndDisabled();
		break;
	case 11:
		ImGui::BeginDisabled(!snapSides || nextColumn);
		ImGui::Text("sameLine");
		ImGui::TableNextColumn();
		if (ImGui::Checkbox("##sameLine", sameLine.access())) {
			changed = true;
			if (sameLine)
				indent = 0;
		}
		ImGui::EndDisabled();
		break;
	case 12:
		ImGui::BeginDisabled(!snapSides);
		ImGui::Text("nextColumn");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (ImGui::InputInt("##nextColumn", nextColumn.access())) {
			changed = true;
			if (nextColumn) {
				sameLine = false;
				spacing = 0;
			}
		}
		ImGui::EndDisabled();
		break;
	case 13:
		ImGui::BeginDisabled(!snapSides);
		ImGui::Text("allowOverlap");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::Checkbox("##allowOverlap", allowOverlap.access());
		ImGui::EndDisabled();
		break;
	default:
		return false;
	}
	return changed;
}

std::vector<UINode::Prop>
Widget::Events()
{
	return {
		{ "IsItemContextMenu", &onItemContextMenuClicked },
		{ "IsItemHovered", &onItemHovered },
		{ "IsItemClicked", &onItemClicked },
		{ "IsItemDoubleClicked", &onItemDoubleClicked },
		{ "IsItemFocused", &onItemFocused },
		{ "IsItemActivated", &onItemActivated },
		{ "IsItemDeactivated", &onItemDeactivated },
		{ "IsItemDeactivatedAfterEdit", &onItemDeactivatedAfterEdit },
	};
}

bool Widget::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	int sat = (i & 1) ? 202 : 164;
	ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(255, 255, sat, 255));
	switch (i)
	{
	case 0:
		ImGui::BeginDisabled(Behavior() & NoContextMenu);
		ImGui::Text("IsItemContextMenuClicked");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##IsContextMenu", &onItemContextMenuClicked, ctx);
		ImGui::EndDisabled();
		break;
	case 1:
		ImGui::Text("IsItemHovered");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##IsItemHovered", &onItemHovered, ctx);
		break;
	case 2:
		ImGui::Text("IsItemClicked");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##itemclicked", &onItemClicked, ctx);
		break;
	case 3:
		ImGui::Text("IsItemDoubleClicked");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##itemdblclicked", &onItemDoubleClicked, ctx);
		break;
	case 4:
		ImGui::Text("IsItemFocused");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##IsItemFocused", &onItemFocused, ctx);
		break;
	case 5:
		ImGui::Text("IsItemActivated");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##IsItemActivated", &onItemActivated, ctx);
		break;
	case 6:
		ImGui::Text("IsItemDeactivated");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##IsItemDeactivated", &onItemDeactivated, ctx);
		break;
	case 7:
		ImGui::Text("IsItemDeactivatedAfterEdit");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##IsItemDeactivatedAfterEdit", &onItemDeactivatedAfterEdit, ctx);
		break;
	default:
		return false;
	}
	return changed;
}

void Widget::TreeUI(UIContext& ctx)
{
	std::string label;
	const auto props = Properties();
	for (const auto& p : props) {
		if (p.kbdInput && p.property->c_str()) {
			label = cpp::to_draw_str(p.property->c_str());
		}
	}
	if (label.empty()) {
		label = typeid(*this).name();
		auto i = label.find(' ');
		if (i != std::string::npos)
			label.erase(0, i + 1);
		auto it = stx::find_if(label, [](char c) { return isalpha(c); });
		if (it != label.end())
			label.erase(0, it - label.begin());
	}
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

	/*float cx = ImGui::GetCursorPosX();
	cx += 2 * ImGui::GetStyle().FramePadding.x + ImGui::GetFontSize(); //indentation used by TreeNodeEx
	cx += ImGui::GetFontSize() * 1.3f;*/
	bool selected = stx::count(ctx.selected, this) || ctx.snapParent == this;
	float sp = ImGui::GetFontSize() * 1.4f - ImGui::CalcTextSize(icon.c_str(), 0, true).x;
	ImGui::Dummy({ sp, 0 });
	ImGui::SameLine(0, 0);

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
		ImGui::PushFont(ctx.defaultFont);
		ImGui::Text("%s", label.c_str());
		ImGui::PopFont();
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::TextDisabled("%s", suff.c_str());
		ctx.parents.push_back(this);
		for (const auto& ch : children)
			ch->TreeUI(ctx);
		ctx.parents.pop_back();
		ImGui::TreePop();
	}
	else {
		ImGui::PopStyleColor();
	}
}

//----------------------------------------------------

Spacer::Spacer(UIContext& ctx)
{
}

std::unique_ptr<Widget> Spacer::Clone(UIContext& ctx)
{
	return std::unique_ptr<Widget>(new Spacer(*this));
}

ImDrawList* Spacer::DoDraw(UIContext& ctx)
{
	ImVec2 size { size_x.eval_px(ImGuiAxis_X, ctx),	size_y.eval_px(ImGuiAxis_Y, ctx) };
	/*if (!size.x)
		size.x = 20;
	if (!size.y)
		size.y = 20;*/
	ImVec2 r = ImGui::CalcItemSize(size, 20, 20);
	
	if (!ctx.beingResized) 
	{
		auto* dl = ImGui::GetWindowDrawList();
		ImVec2 p = ImGui::GetCursorScreenPos();
		ImU32 clr = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Border));
		//dl->AddRect(p, p + size, clr);
		
		float th = 2;
		ImVec2 xuv{ std::round(r.x / 5), 0 };
		ImVec2 yuv{ 0, std::round(r.y / 5) };
		
		dl->PushTextureID(ctx.dashTexId);
		dl->PrimReserve(4*6, 4*4);
		dl->PrimRectUV(p, { p.x + r.x, p.y + th }, { 0, 0 }, xuv, clr);
		dl->PrimRectUV({ p.x, p.y + r.y - th }, p + r, { 0, 0 }, xuv, clr);
		dl->PrimRectUV(p, { p.x + th, p.y + r.y }, { 0, 0 }, yuv, clr);
		dl->PrimRectUV({ p.x + r.x - th, p.y }, p + r, { 0, 0 }, yuv, clr);
		dl->PopTextureID();

		/*if (!size.y) {
			dl->PushTextureID(ctx.dashTexId);
			dl->PrimReserve(6, 4);
			dl->PrimRectUV({ p.x, p.y + (r.y - th) / 2 }, { p.x + r.x, p.y + (r.y - th) / 2 + th }, { 0, 0 }, xuv, clr);
			dl->PopTextureID(); 
			dl->AddLine(p, { p.x, p.y + r.y }, clr, th);
			dl->AddLine({ p.x + r.x, p.y }, p + r, clr, th);
		}
		else if (!size.x) {
			dl->PushTextureID(ctx.dashTexId);
			dl->PrimReserve(6, 4);
			dl->PrimRectUV({ p.x + (r.x - th) / 2, p.y }, { p.x + (r.x - th) / 2 + th, p.y + r.y }, { 0, 0 }, yuv, clr);
			dl->PopTextureID(); 
			dl->AddLine(p, { p.x + r.x, p.y }, clr, th);
			dl->AddLine({ p.x, p.y + r.y }, p + r, clr, th);
		}*/
		
	}
	ImRad::Dummy(r);
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
			auto size = cpp::parse_fsize(sit->params[0]);
			size_x = size.x;
			size_y = size.y;
		}
	}
}

std::vector<UINode::Prop>
Spacer::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "size_x", &size_x },
		{ "size_y", &size_y },
		});
	return props;
}

bool Spacer::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_x", &size_x, ctx);
		break;
	case 1:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, {}, true, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_y", &size_y, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 2, ctx);
	}
	return changed;
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
	if (!style_outer_padding && !sameLine)
		ImRad::PushIgnoreWindowPadding(nullptr, &data);

	if (!label.empty())
		ImGui::SeparatorText(DRAW_STR(label));
	else if (style_thickness.has_value())
		ImGui::SeparatorEx(sameLine ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal, style_thickness);
	else 
		ImGui::SeparatorEx(sameLine ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal);

	if (!style_outer_padding && !sameLine)
		ImRad::PopIgnoreWindowPadding(data);

	return ImGui::GetWindowDrawList();
}

void Separator::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
	const float sp = 3;
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
	if (!style_outer_padding && !sameLine) 
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

	if (!style_outer_padding && !sameLine)
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
		style_outer_padding = false;
	}
}

std::vector<UINode::Prop>
Separator::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "@style.thickness", &style_thickness },
		{ "@style.outer_padding", &style_outer_padding },
		{ "label", &label, true }
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
		changed = InputDirectVal("##th", &style_thickness, ctx);
		break;
	case 1:
		ImGui::Text("outerPadding");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##op", &style_outer_padding, ctx);
		break;
	case 2:
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##label", &label, ctx);
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
		//float x1 = ImGui::GetCursorPosX();
		//float w = wrap_x.value();
		//if (w < 0) w += ImGui::GetContentRegionAvail().x;
		//ImGui::PushTextWrapPos(x1 + w);
		ImGui::PushTextWrapPos(0);
		ImGui::TextUnformatted(DRAW_STR(text));
		ImGui::PopTextWrapPos();
		//ImGui::SameLine(x1 + w);
		//ImGui::NewLine();
	}
	else 
	{
		ImGui::TextUnformatted(DRAW_STR(text));
	}

	return ImGui::GetWindowDrawList();
}

void Text::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
	cached_size = ImGui::GetItemRectSize();
	ImGui::SameLine(0, 0);
	cached_size.x = ImGui::GetCursorPosX() - p1.x;
	ImGui::NewLine();
}

void Text::DoExport(std::ostream& os, UIContext& ctx)
{
	if (alignToFrame)
		os << ctx.ind << "ImGui::AlignTextToFramePadding();\n";

	if (wrap)
	{
		os << ctx.ind << "ImGui::PushTextWrapPos(0);\n";
		os << ctx.ind << "ImGui::TextUnformatted(" << text.to_arg() << ");\n";
		os << ctx.ind << "ImGui::PopTextWrapPos();\n";
	}
	else
	{
		os << ctx.ind << "ImGui::TextUnformatted(" << text.to_arg() << ");\n";
	}
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
				ctx.errors.push_back("Text: unable to parse text");
		}
	}
}

std::vector<UINode::Prop>
Text::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "@style.color", &style_text },
		{ "@style.font", &style_font },
		{ "text", &text, true },
		{ "alignToFramePadding", &alignToFrame },
		{ "text.wrap", &wrap },
	});
	return props;
}

bool Text::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("color");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##color", &style_text, ImGuiCol_Text, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("color", &style_text, ctx);
		break;
	case 1:
		ImGui::Text("font");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##font", &style_font, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("font", &style_font, ctx);
		break;
	case 2:
		ImGui::Text("text");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##text", &text, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("text", &text, ctx);
		break;
	case 3:
		ImGui::Text("alignToFramePadding");
		ImGui::TableNextColumn();
		changed = ImGui::Checkbox("##alignToFrame", alignToFrame.access());
		break;
	case 4:
		ImGui::Text("wrap");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::Checkbox("##wrap", wrap.access());
		break;
	default:
		return Widget::PropertyUI(i - 5, ctx);
	}
	return changed;
}

//----------------------------------------------------

Selectable::Selectable(UIContext& ctx)
{
	flags.prefix("ImGuiSelectableFlags_");
	flags.add$(ImGuiSelectableFlags_DontClosePopups);
	flags.add$(ImGuiSelectableFlags_SpanAllColumns);
	flags.add$(ImGuiSelectableFlags_NoPadWithHalfSpacing);
	
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
	if (!fieldName.empty() && ctx.createVars) {
		sel->fieldName.set_from_arg(ctx.codeGen->CreateVar("bool", "false", CppGen::Var::Interface));
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
		ImGui::PushStyleColor(ImGuiCol_Header, style_header.eval(ctx));
	
	if (readOnly)
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true); //won't affect text color

	if (alignToFrame)
		ImGui::AlignTextToFramePadding();

	ImVec2 size;
	size.x = size_x.eval_px(ImGuiAxis_X, ctx);
	size.y = size_y.eval_px(ImGuiAxis_Y, ctx);
	bool sel = fieldName.empty() ? selected.eval(ctx) : fieldName.eval(ctx);
	ImRad::Selectable(DRAW_STR(label), sel, flags, size);

	if (readOnly)
		ImGui::PopItemFlag();

	if (!style_header.empty())
		ImGui::PopStyleColor();

	ImGui::PopStyleVar();
	return ImGui::GetWindowDrawList();
}

void Selectable::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
	cached_size = ImGui::GetItemRectSize();

	assert(ctx.parents.back() == this);
	const auto* parent = ctx.parents[ctx.parents.size() - 2];
	size_t idx = stx::find_if(parent->children, [this](const auto& ch) { 
		return ch.get() == this; 
		}) - parent->children.begin();
	bool explicitWidth = size_x.has_value() && !size_x.zero();
	if (!(flags & ImGuiSelectableFlags_SpanAllColumns) &&
		!explicitWidth &&
		idx + 1 < parent->children.size() && 
		parent->children[idx + 1]->sameLine) 
	{
		//when size.x=0 ItemRect spans to the right edge even anoter widget 
		//is on the same line. Fix it here so next widget can be selected
		cached_size.x = ImGui::CalcTextSize(label.c_str(), nullptr, true).x;
		cached_size.x += ImGui::GetStyle().ItemSpacing.x;
	}
	
	if (!(flags & ImGuiSelectableFlags_NoPadWithHalfSpacing)) {
		//ItemRect has ItemSpacing padding by default. Adjust pos to make
		//the rectangle look centered
		cached_pos.x -= ImGui::GetStyle().ItemSpacing.x / 2; 
		//cached_pos.y -= ImGui::GetStyle().ItemSpacing.y / 2;
	}
	else {
		cached_pos.y += ImGui::GetStyle().ItemSpacing.y / 2;
	}
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

	os << ctx.ind;
	if (!onChange.empty())
		os << "if (";
	
	os << "ImRad::Selectable(" << label.to_arg() << ", ";
	if (!fieldName.empty())
		os << "&" << fieldName.to_arg();
	else
		os << selected.to_arg();
	
	os << ", " << flags.to_arg() << ", { "
		<< size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", " 
		<< size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]) 
		<< " })";
	
	if (!onChange.empty()) {
		os << ")\n";
		ctx.ind_up();
		os << ctx.ind << onChange.to_arg() << "();\n";
		ctx.ind_down();
	}
	else {
		os << ";\n";
	}

	if (readOnly)
		os << ctx.ind << "ImGui::PopItemFlag();\n";

	if (!style_header.empty())
		os << ctx.ind << "ImGui::PopStyleColor();\n";

	os << ctx.ind << "ImGui::PopStyleVar();\n";
}

void Selectable::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if ((sit->kind == cpp::CallExpr && sit->callee == "ImRad::Selectable") ||
		(sit->kind == cpp::CallExpr && sit->callee == "ImGui::Selectable") || //compatibility
		(sit->kind == cpp::IfCallThenCall && sit->callee == "ImRad::Selectable") ||
		(sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::Selectable")) //compatibility
	{
		if (sit->params.size() >= 1) {
			label.set_from_arg(sit->params[0]);
			if (label.value() == cpp::INVALID_TEXT)
				ctx.errors.push_back("Selectable: unable to parse label");
		}
		if (sit->params.size() >= 2) {
			if (!sit->params[1].compare(0, 1, "&"))
				fieldName.set_from_arg(sit->params[1].substr(1));
			else
				selected.set_from_arg(sit->params[1]);
		}
		if (sit->params.size() >= 3)
			flags.set_from_arg(sit->params[2]);
		if (sit->params.size() >= 4) {
			auto sz = cpp::parse_size(sit->params[3]);
			size_x.set_from_arg(sz.first);
			size_y.set_from_arg(sz.second);
		}

		if (sit->kind == cpp::IfCallThenCall)
			onChange.set_from_arg(sit->callee2);
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
}

std::vector<UINode::Prop>
Selectable::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "@style.header", &style_header },
		{ "@style.color", &style_text },
		{ "@style.font", &style_font },
		{ "selectable.flags", &flags },
		{ "label", &label, true },
		{ "readOnly", &readOnly },
		{ "horizAlignment", &horizAlignment },
		{ "vertAlignment", &vertAlignment },
		{ "alignToFrame", &alignToFrame },
		{ "selectable.fieldName", &fieldName },
		{ "selectable.selected", &selected },
		{ "size_x", &size_x },
		{ "size_y", &size_y }
		});
	return props;
}

bool Selectable::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("header");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##header", &style_header, ImGuiCol_Header, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("header", &style_header, ctx);
		break;
	case 1:
		ImGui::Text("color");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##color", &style_text, ImGuiCol_Text, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("color", &style_text, ctx);
		break;
	case 2:
		ImGui::Text("font");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##font", &style_font, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("font", &style_font, ctx);
		break;
	case 3:
		TreeNodeProp("flags", "...", [&] {
			ImGui::TableNextColumn();
			ImGui::Spacing();
			changed = CheckBoxFlags(&flags);
			});
		break;
	case 4:
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##label", &label, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("label", &label, ctx);
		break;
	case 5:
		ImGui::Text("readOnly");
		ImGui::TableNextColumn();
		changed = InputDirectVal("##ronly", &readOnly, ctx);
		break;
	case 6:
		ImGui::Text("horizAlignment");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (ImGui::BeginCombo("##horizAlignment", horizAlignment.get_id().c_str()))
		{
			for (const auto& item : horizAlignment.get_ids())
				if (ImGui::Selectable(item.first.c_str(), horizAlignment == item.second)) {
					changed = true;
					horizAlignment = item.second;
				}
			ImGui::EndCombo();
		}
		break;
	case 7:
		ImGui::BeginDisabled(size_y.has_value() && !size_y.eval_px(ImGuiAxis_Y, ctx));
		ImGui::Text("vertAlignment");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (ImGui::BeginCombo("##vertAlignment", vertAlignment.get_id().c_str()))
		{
			for (const auto& item : vertAlignment.get_ids())
				if (ImGui::Selectable(item.first.c_str(), vertAlignment == item.second)) {
					changed = true;
					vertAlignment = item.second;
				}
			ImGui::EndCombo();
		}
		ImGui::EndDisabled();
		break;
	case 8:
		ImGui::Text("alignToFramePadding");
		ImGui::TableNextColumn();
		changed = ImGui::Checkbox("##alignToFrame", alignToFrame.access());
		break;
	case 9:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##fieldName", &fieldName, true, ctx);
		break;
	case 10:
		ImGui::Text("selected");
		ImGui::TableNextColumn();
		ImGui::BeginDisabled(!fieldName.empty());
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##value", &selected, false, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("value", &selected, ctx);
		ImGui::EndDisabled();
		break;
	case 11:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_x", &size_x, ctx);
		break;
	case 12:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, {}, true, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_y", &size_y, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 13, ctx);
	}
	return changed;
}

std::vector<UINode::Prop>
Selectable ::Events()
{
	auto props = Widget::Events();
	props.insert(props.begin(), {
		{ "onChange", &onChange },
		});
	return props;
}

bool Selectable::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("OnChange");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##onChange", &onChange, ctx);
		break;
	default:
		return Widget::EventUI(i - 1, ctx);
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
	modalResult.add$(ImRad::No);
	modalResult.add$(ImRad::Abort);
	modalResult.add$(ImRad::Retry);
	modalResult.add$(ImRad::Ignore);
	modalResult.add$(ImRad::All);
	
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

ImDrawList* Button::DoDraw(UIContext& ctx)
{
	if (!style_button.empty())
		ImGui::PushStyleColor(ImGuiCol_Button, style_button.eval(ctx));
	if (!style_hovered.empty())
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style_hovered.eval(ctx));

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
	
	if (!style_hovered.empty())
		ImGui::PopStyleColor();
	if (!style_button.empty())
		ImGui::PopStyleColor();

	return ImGui::GetWindowDrawList();
}

void Button::DoExport(std::ostream& os, UIContext& ctx)
{
	if (!style_button.empty())
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Button, " << style_button.to_arg() << ");\n";
	if (!style_hovered.empty())
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_ButtonHovered, " << style_hovered.to_arg() << ");\n";
	if (!style_active.empty())
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_ButtonActive, " << style_active.to_arg() << ");\n";

	bool closePopup = ctx.kind == TopWindow::ModalPopup && modalResult != ImRad::None;
	os << ctx.ind;
	if (!onChange.empty() || closePopup || dropDownMenu != "")
		os << "if (";
	
	if (arrowDir != ImGuiDir_None)
	{
		os << "ImGui::ArrowButton(\"##" << label.c_str() << "\", " << arrowDir.to_arg() << ")";
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

	if (!style_button.empty())
		os << ctx.ind << "ImGui::PopStyleColor();\n";
	if (!style_hovered.empty())
		os << ctx.ind << "ImGui::PopStyleColor();\n";
	if (!style_active.empty())
		os << ctx.ind << "ImGui::PopStyleColor();\n";
}

void Button::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
	{
		if (sit->params.size() >= 2 && sit->params[0] == "ImGuiCol_Button")
			style_button.set_from_arg(sit->params[1]);
		if (sit->params.size() >= 2 && sit->params[0] == "ImGuiCol_ButtonHovered")
			style_hovered.set_from_arg(sit->params[1]);
		if (sit->params.size() >= 2 && sit->params[0] == "ImGuiCol_ButtonActive")
			style_active.set_from_arg(sit->params[1]);
	}
	else if ((sit->kind == cpp::CallExpr || sit->kind == cpp::IfCallBlock) &&
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
		{ "@style.text", &style_text },
		{ "@style.button", &style_button },
		{ "@style.hovered", &style_hovered },
		{ "@style.active", &style_active },
		{ "@style.border", &style_border },
		{ "@style.padding", &style_framePadding },
		{ "@style.rounding", &style_frameRounding },
		{ "@style.borderSize", &style_frameBorderSize },
		{ "@style.font", &style_font },
		{ "button.arrowDir", &arrowDir },
		{ "label", &label, true },
		{ "shortcut", &shortcut },
		{ "button.modalResult", &modalResult },
		{ "button.dropDownMenu", &dropDownMenu },
		{ "button.small", &small },
		{ "size_x", &size_x },
		{ "size_y", &size_y },
		});
	return props;
}

bool Button::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("text");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##color", &style_text, ImGuiCol_Text, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("color", &style_text, ctx);
		break;
	case 1:
		ImGui::Text("button");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##bg", &style_button, ImGuiCol_Button, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("bg", &style_button, ctx);
		break;
	case 2:
		ImGui::Text("hovered");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##hovered", &style_hovered, ImGuiCol_ButtonHovered, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("hovered", &style_hovered, ctx);
		break;
	case 3:
		ImGui::Text("active");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##active", &style_active, ImGuiCol_ButtonActive, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("active", &style_active, ctx);
		break;
	case 4:
		ImGui::Text("border");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##border", &style_border, ImGuiCol_Border, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("border", &style_border, ctx);
		break;
	case 5:
		ImGui::Text("padding");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##padding", &style_framePadding, ctx);
		break;
	case 6:
		ImGui::Text("rounding");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##rounding", &style_frameRounding, ctx);
		break;
	case 7:
		ImGui::Text("borderSize");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##borderSize", &style_frameBorderSize, ctx);
		break;
	case 8:
		ImGui::Text("font");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##font", &style_font, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("font", &style_font, ctx);
		break;
	case 9:
	{
		ImGui::Text("arrowDir");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (ImGui::BeginCombo("##arrowDir", arrowDir.get_id().c_str()))
		{
			changed = true;
			for (const auto& item : arrowDir.get_ids())
			{
				if (ImGui::Selectable(item.first.c_str(), arrowDir == item.second))
					arrowDir = item.second;
			}
			ImGui::EndCombo();
		}
		break;
	}
	case 10:
		ImGui::BeginDisabled(arrowDir != ImGuiDir_None);
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##label", &label, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("label", &label, ctx);
		ImGui::EndDisabled();
		break;
	case 11:
		ImGui::Text("shortcut");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##shortcut", &shortcut, true, ctx);
		break;
	case 12:
	{
		ImGui::BeginDisabled(ctx.kind != TopWindow::ModalPopup);
		ImGui::Text("modalResult");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (ImGui::BeginCombo("##modalResult", modalResult.get_id().c_str()))
		{
			changed = true;
			for (const auto& item : modalResult.get_ids())
			{
				if (ImGui::Selectable(item.first.c_str(), modalResult == item.second)) {
					modalResult = item.second;
					if (modalResult == ImRad::Cancel)
						shortcut = "Escape";
				}
			}
			ImGui::EndCombo();
		}
		ImGui::EndDisabled();
		break;
	}
	case 13:
		ImGui::Text("dropDownMenu");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (ImGui::BeginCombo("##ddm", dropDownMenu.c_str()))
		{
			if (ImGui::Selectable(" ", dropDownMenu.empty()))
			{
				changed = true;
				dropDownMenu = "";
			}
			for (const std::string& cm : ctx.contextMenus)
			{
				if (ImGui::Selectable(cm.c_str(), cm == dropDownMenu.c_str()))
				{
					changed = true;
					dropDownMenu = cm;
				}
			}
			ImGui::EndCombo();
		}
		break;
	case 14:
		ImGui::BeginDisabled(arrowDir != ImGuiDir_None);
		ImGui::Text("small");
		ImGui::TableNextColumn();
		changed = InputDirectVal("##small", &small, ctx);
		ImGui::EndDisabled();
		break;
	case 15:
		ImGui::BeginDisabled(small || arrowDir != ImGuiDir_None);
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_x", &size_x, ctx);
		ImGui::EndDisabled();
		break;
	case 16:
		ImGui::BeginDisabled(small || arrowDir != ImGuiDir_None);
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, {}, true, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_y", &size_y, ctx);
		ImGui::EndDisabled();
		break;
	default:
		return Widget::PropertyUI(i - 17, ctx);
	}
	return changed;
}

std::vector<UINode::Prop>
Button::Events()
{
	auto props = Widget::Events();
	props.insert(props.begin(), {
		{ "onChange", &onChange },
		});
	return props;
}

bool Button::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("OnChange");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##onChange", &onChange, ctx);
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
		fieldName.set_from_arg(ctx.codeGen->CreateVar("bool", "false", CppGen::Var::Interface));
}

std::unique_ptr<Widget> CheckBox::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<CheckBox>(*this);
	if (!fieldName.empty() && ctx.createVars) {
		sel->fieldName.set_from_arg(ctx.codeGen->CreateVar("bool", "false", CppGen::Var::Interface));
	}
	return sel;
}

ImDrawList* CheckBox::DoDraw(UIContext& ctx)
{
	if (!style_check.empty())
		ImGui::PushStyleColor(ImGuiCol_CheckMark, style_check.eval(ctx));

	bool val = fieldName.eval(ctx);
	ImGui::Checkbox(DRAW_STR(label), &val);

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

	os << "ImGui::Checkbox("
		<< label.to_arg() << ", "
		<< "&" << fieldName.c_str()
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
		{
			fieldName.set_from_arg(sit->params[1].substr(1));
			std::string fn = fieldName.c_str();
			const auto* var = ctx.codeGen->GetVar(fn);
			if (!var)
				ctx.errors.push_back("CheckBox: field_name variable '" + fn + "' doesn't exist");
		}

		if (sit->kind == cpp::IfCallThenCall)
			onChange.set_from_arg(sit->callee2);
	}
}

std::vector<UINode::Prop>
CheckBox::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "@style.text", &style_text },
		{ "@style.check", &style_check }, 
		{ "@style.borderSize", &style_frameBorderSize },
		{ "@style.font", &style_font },
		{ "label", &label, true },
		{ "check.field_name", &fieldName },
		});
	return props;
}

bool CheckBox::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("text");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##text", &style_text, ImGuiCol_Text, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("text", &style_text, ctx);
		break;
	case 1:
		ImGui::Text("check");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##check", &style_check, ImGuiCol_CheckMark, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("check", &style_check, ctx);
		break;
	case 2:
		ImGui::Text("borderSize");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##borderSize", &style_frameBorderSize, ctx);
		break;
	case 3:
		ImGui::Text("font");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##font", &style_font, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("font", &style_font, ctx);
		break;
	case 4:
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##label", &label, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("label", &label, ctx);
		break;
	case 5:
	{
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##field", &fieldName, false, ctx);
		break;
	}
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
		{ "onChange", &onChange }
		});
	return props;
}

bool CheckBox::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("OnChange");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##onChange", &onChange, ctx);
		break;
	default:
		return Widget::EventUI(i - 1, ctx);
	}
	return changed;
}

//-----------------------------------------------------

RadioButton::RadioButton(UIContext& ctx)
{
	//variable is shared among buttons so don't generate new here
}

std::unique_ptr<Widget> RadioButton::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<RadioButton>(*this);
	//fieldName can be shared
	return sel;
}

ImDrawList* RadioButton::DoDraw(UIContext& ctx)
{
	if (!style_check.empty())
		ImGui::PushStyleColor(ImGuiCol_CheckMark, style_check.eval(ctx));
	
	ImGui::RadioButton(DRAW_STR(label), valueID==0);

	if (!style_check.empty())
		ImGui::PopStyleColor();

	return ImGui::GetWindowDrawList();
}

void RadioButton::DoExport(std::ostream& os, UIContext& ctx)
{
	if (!ctx.codeGen->GetVar(fieldName.c_str()))
		ctx.errors.push_back("RadioButon: field_name variable doesn't exist");

	if (!style_check.empty())
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_CheckMark, " << style_check.to_arg() << ");\n";
	
	os << ctx.ind;
	if (!onChange.empty())
		os << "if (";

	os << "ImGui::RadioButton("
		<< label.to_arg() << ", "
		<< "&" << fieldName.c_str() << ", "
		<< valueID << ")";

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

		if (sit->params.size() >= 2 && !sit->params[1].compare(0, 1, "&"))
		{
			fieldName.set_from_arg(sit->params[1].substr(1));
		}

		if (sit->kind == cpp::IfCallThenCall)
			onChange.set_from_arg(sit->callee2);
	}
}

std::vector<UINode::Prop>
RadioButton::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "@style.text", &style_text },
		{ "@style.check", &style_check },
		{ "@style.borderSize", &style_frameBorderSize },
		{ "@style.font", &style_font },
		{ "label", &label, true },
		{ "radio.valueID", &valueID },
		{ "field_name", &fieldName },
	});
	return props;
}

bool RadioButton::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("text");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##text", &style_text, ImGuiCol_Text, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("text", &style_text, ctx);
		break;
	case 1:
		ImGui::Text("check");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##check", &style_check, ImGuiCol_CheckMark, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("check", &style_check, ctx);
		break;
	case 2:
		ImGui::Text("borderSize");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##borderSize", &style_frameBorderSize, ctx);
		break;
	case 3:
		ImGui::Text("font");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##font", &style_font, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("font", &style_font, ctx);
		break;
	case 4:
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##label", &label, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("label", &label, ctx);
		break;
	case 5:
		ImGui::Text("valueID");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputInt("##valueID", valueID.access());
		break;
	case 6:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##fieldName", &fieldName, false, ctx);
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
		{ "onChange", &onChange }
		});
	return props;
}

bool RadioButton::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("OnChange");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##onChange", &onChange, ctx);
		break;
	default:
		return Widget::EventUI(i - 1, ctx);
	}
	return changed;
}

//---------------------------------------------------

flags_helper Input::_imeClass = 0;
flags_helper Input::_imeAction = 0;

Input::Input(UIContext& ctx)
{
	size_x = 200;
	size_y = 100;

	flags.prefix("ImGuiInputTextFlags_");
	flags.add$(ImGuiInputTextFlags_CharsDecimal);
	flags.add$(ImGuiInputTextFlags_CharsHexadecimal);
	flags.add$(ImGuiInputTextFlags_CharsScientific);
	flags.add$(ImGuiInputTextFlags_CharsUppercase);
	flags.add$(ImGuiInputTextFlags_CharsNoBlank);
	flags.add$(ImGuiInputTextFlags_AutoSelectAll);
	/* TODO: make events
	flags.add$(ImGuiInputTextFlags_CallbackCompletion);
	flags.add$(ImGuiInputTextFlags_CallbackHistory);
	flags.add$(ImGuiInputTextFlags_CallbackAlways);
	flags.add$(ImGuiInputTextFlags_CallbackCharFilter);*/
	flags.add$(ImGuiInputTextFlags_CtrlEnterForNewLine);
	flags.add$(ImGuiInputTextFlags_NoHorizontalScroll);
	flags.add$(ImGuiInputTextFlags_ReadOnly);
	flags.add$(ImGuiInputTextFlags_Password);
	flags.add$(ImGuiInputTextFlags_Multiline);
	flags.add$(ImGuiInputTextFlags_EscapeClearsAll);

	if (_imeClass.get_ids().empty()) 
	{
		_imeClass.prefix("ImRad::");
		_imeClass.add("Default", 0);
		_imeClass.add$(ImRad::ImeText);
		_imeClass.add$(ImRad::ImeNumber);
		_imeClass.add$(ImRad::ImeDecimal);
		_imeClass.add$(ImRad::ImeEmail);
		_imeClass.add$(ImRad::ImePhone);
		_imeAction.prefix("ImRad::");
		_imeAction.add("ImeActionNone", 0);
		_imeAction.add$(ImRad::ImeActionDone);
		_imeAction.add$(ImRad::ImeActionGo);
		_imeAction.add$(ImRad::ImeActionNext);
		_imeAction.add$(ImRad::ImeActionPrevious);
		_imeAction.add$(ImRad::ImeActionSearch);
		_imeAction.add$(ImRad::ImeActionSend);
	}

	if (ctx.createVars)
		fieldName.set_from_arg(ctx.codeGen->CreateVar(type, "", CppGen::Var::Interface));
}

std::unique_ptr<Widget> Input::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<Input>(*this);
	if (!fieldName.empty() && ctx.createVars) {
		sel->fieldName.set_from_arg(ctx.codeGen->CreateVar(type, "", CppGen::Var::Interface));
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
		id = "##" + fieldName.value();
	if (flags & ImGuiInputTextFlags_Multiline)
	{
		ImVec2 size;
		size.x = size_x.eval_px(ImGuiAxis_X, ctx);
		size.y = size_y.eval_px(ImGuiAxis_Y, ctx);
		ImGui::InputTextMultiline(id.c_str(), &stmp, size, flags);
	}
	else if (type == "std::string" || type == "ImGuiTextFilter")
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
		if (type == "int")
			ImGui::InputInt(id.c_str(), itmp, (int)step);
		else if (type == "int2")
			ImGui::InputInt2(id.c_str(), itmp);
		else if (type == "int3")
			ImGui::InputInt3(id.c_str(), itmp);
		else if (type == "int4")
			ImGui::InputInt4(id.c_str(), itmp);
		else if (type == "float")
			ImGui::InputFloat(id.c_str(), ftmp, step, 0.f, format.c_str());
		else if (type == "float2")
			ImGui::InputFloat2(id.c_str(), ftmp, format.c_str());
		else if (type == "float3")
			ImGui::InputFloat3(id.c_str(), ftmp, format.c_str());
		else if (type == "float4")
			ImGui::InputFloat4(id.c_str(), ftmp, format.c_str());
		else if (type == "double")
			ImGui::InputDouble(id.c_str(), dtmp, step, 0, format.c_str());
	}

	return ImGui::GetWindowDrawList();
}

void Input::DoExport(std::ostream& os, UIContext& ctx)
{
	if (initialFocus)
	{
		os << ctx.ind << "if (ImGui::IsWindowAppearing())\n";
		ctx.ind_up();
		os << ctx.ind << "ImGui::SetKeyboardFocusHere();\n";
		ctx.ind_down();
	}
	if (!forceFocus.empty())
	{
		os << ctx.ind << "if (" << forceFocus.to_arg() << ")\n" << ctx.ind << "{\n";
		ctx.ind_up();
		os << ctx.ind << forceFocus.to_arg() << " = false;\n";
		os << ctx.ind << "ImGui::SetKeyboardFocusHere();\n";
		ctx.ind_down();
		os << ctx.ind << "}\n";
	}

	if (!(flags & ImGuiInputTextFlags_Multiline))
		os << ctx.ind << "ImGui::SetNextItemWidth(" << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ");\n";
	
	os << ctx.ind;
	if (type == "ImGuiTextFilter" || !onChange.empty())
		os << "if (";

	std::string cap = type;
	cap[0] = std::toupper(cap[0]);
	std::string id = label.to_arg();
	std::string defIme = "ImRad::ImeText";
	if (label.empty())
		id = "\"##" + fieldName.value() + "\"";
	
	if (type == "int")
	{
		defIme = "ImRad::ImeNumber";
		os << "ImGui::InputInt(" << id << ", &"
			<< fieldName.to_arg() << ", " << (int)step << ")";
	}
	else if (type == "float")
	{
		defIme = "ImRad::ImeDecimal";
		os << "ImGui::InputFloat(" << id << ", &"
			<< fieldName.to_arg() << ", " << step.to_arg() << ", 0.f, "
			<< format.to_arg() << ")";
	}
	else if (type == "double")
	{
		defIme = "ImRad::ImeDecimal";
		os << "ImGui::InputDouble(" << id << ", &"
			<< fieldName.to_arg() << ", " << step.to_arg() << ", 0.0, "
			<< format.to_arg() << ")";
	}
	else if (!type.access()->compare(0, 3, "int"))
	{
		defIme = "ImRad::ImeNumber";
		os << "ImGui::Input"<< cap << "(" << id << ", &"
			<< fieldName.to_arg() << ")";
	}
	else if (!type.access()->compare(0, 5, "float"))
	{
		defIme = "ImRad::ImeDecimal";
		os << "ImGui::Input" << cap << "(" << id << ", &"
			<< fieldName.to_arg() << ", " << format.to_arg() << ")";
	}
	else if (flags & ImGuiInputTextFlags_Multiline)
	{
		os << "ImGui::InputTextMultiline(" << id << ", &" 
			<< fieldName.to_arg() << ", { " 
			<< size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", " 
			<< size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]) << " }, "
			<< flags.to_arg() << ")";
	}
	else if (type == "std::string")
	{
		if (!hint.empty())
			os << "ImGui::InputTextWithHint(" << id << ", " << hint.to_arg() << ", ";
		else
			os << "ImGui::InputText(" << id << ", ";
		os << "&" << fieldName.to_arg() << ", " << flags.to_arg() << ")";
	}
	else if (type == "ImGuiTextFilter")
	{
		//.Draw function is deprecated see https://github.com/ocornut/imgui/issues/6395
		if (!hint.empty())
			os << "ImGui::InputTextWithHint(" << id << ", " << hint.to_arg() << ", ";
		else
			os << "ImGui::InputText(" << id << ", ";
		os << fieldName.to_arg() << ".InputBuf, " 
			<< "IM_ARRAYSIZE(" << fieldName.to_arg() << ".InputBuf), " 
			<< flags.to_arg() << ")";
	}

	if (type == "ImGuiTextFilter") {
		os << ")\n" << ctx.ind << "{\n";
		ctx.ind_up();
		os << ctx.ind << fieldName.to_arg() << ".Build();\n";
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
		<< (cls ? _imeClass.get_name(cls) : defIme);
	if (action)
		os << " | " << _imeAction.get_name(action);
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
		type = "std::string";

		if (sit->params.size()) {
			label.set_from_arg(sit->params[0]);
			if (!label.access()->compare(0, 2, "##"))
				label = "";
		}
		
		if (sit->params.size() >= 2 && !sit->params[1].compare(0, 1, "&")) {
			fieldName.set_from_arg(sit->params[1].substr(1));
			std::string fn = fieldName.c_str();
			if (!ctx.codeGen->GetVar(fn))
				ctx.errors.push_back("Input: field_name variable '" + fn + "' doesn't exist");
		}

		if (sit->params.size() >= 3) {
			auto size = cpp::parse_size(sit->params[2]);
			size_x.set_from_arg(size.first);
			size_y.set_from_arg(size.second);
		}

		if (sit->params.size() >= 4)
			flags.set_from_arg(sit->params[3]);

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
				type = "std::string";
				fieldName.set_from_arg(sit->params[1 + i].substr(1));
			}
			else if (!expr.compare(expr.size() - 9, 9, ".InputBuf")) {
				type = "ImGuiTextFilter";
				fieldName.set_from_arg(expr.substr(0, expr.size() - 9));
				++i;
			}
			if (!ctx.codeGen->GetVar(fieldName.value()))
				ctx.errors.push_back("Input: field_name variable '" + fieldName.value() + "' doesn't exist");
		}

		if (sit->params.size() > 2 + i)
			flags.set_from_arg(sit->params[2 + i]);

		if (sit->kind == cpp::IfCallThenCall)
			onChange.set_from_arg(sit->callee2);
		else if (sit->kind == cpp::IfCallBlock)
			ctx.importLevel = sit->level;
	}
	else if (sit->kind == cpp::CallExpr && sit->level == ctx.importLevel + 1)
	{
		if (sit->callee != fieldName.value() + ".Build") {
			onChange.set_from_arg(sit->callee);
			ctx.importLevel = -1;
		}
	}
	else if ((sit->kind == cpp::CallExpr && !sit->callee.compare(0, 12, "ImGui::Input")) ||
			(sit->kind == cpp::IfCallThenCall && !sit->callee.compare(0, 12, "ImGui::Input")))
	{
		type = sit->callee.substr(12);
		type.access()->front() = std::tolower(std::string(type)[0]);

		if (sit->params.size()) {
			label.set_from_arg(sit->params[0]);
			if (!label.access()->compare(0, 2, "##"))
				label = "";
		}

		if (sit->params.size() >= 2 && !sit->params[1].compare(0, 1, "&")) {
			fieldName.set_from_arg(sit->params[1].substr(1));
			std::string fn = fieldName.c_str();
			if (!ctx.codeGen->GetVar(fn))
				ctx.errors.push_back("Input: field_name variable '" + fn + "' doesn't exist");
		}

		if (type == "int") 
		{
			if (sit->params.size() >= 3)
				step.set_from_arg(sit->params[2]);
		}
		if (type == "float") 
		{
			if (sit->params.size() >= 3)
				step.set_from_arg(sit->params[2]);
			if (sit->params.size() >= 5)
				format.set_from_arg(sit->params[4]);
		}
		else if (!type.access()->compare(0, 5, "float")) 
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
		type = "ImGuiTextFilter";
		size_t i = sit->callee.rfind('.');
		fieldName.set_from_arg(sit->callee.substr(0, i));
		std::string fn = fieldName.c_str();
		if (!ctx.codeGen->GetVar(fn))
			ctx.errors.push_back("Input: field_name variable '" + fn + "' doesn't exist");

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
			
			if (!type.access()->compare(0, 3, "int") && _imeClass == ImRad::ImeText)
				_imeClass = 0;
			else if (!type.access()->compare(0, 5, "float") && _imeClass == ImRad::ImeDecimal)
				_imeClass = 0;
			else if (!type.access()->compare(0, 5, "double") && _imeClass == ImRad::ImeDecimal)
				_imeClass = 0;
			else if ((type == "std::string" || type == "ImGuiTextFilter") && _imeClass == ImRad::ImeText)
				_imeClass = 0;

			imeType = _imeClass | _imeAction;
		}
	}
	else if (sit->kind == cpp::IfCallThenCall &&
		sit->callee == "IsItemImeAction")
	{
		onImeAction.set_from_arg(sit->callee);
	}
	else if (sit->kind == cpp::IfCallThenCall &&
			sit->callee == "ImGui::IsWindowAppearing" && sit->callee2 == "ImGui::SetKeyboardFocusHere")
	{
		initialFocus = true;
	}
	else if (sit->kind == cpp::IfBlock) //todo: weak condition
	{
		forceFocus.set_from_arg(sit->cond);
	}
}

std::vector<UINode::Prop>
Input::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "@style.text", &style_text },
		{ "@style.frameBg", &style_frameBg },
		{ "@style.border", &style_border },
		{ "@style.borderSize", &style_frameBorderSize },
		{ "@style.font", &style_font },
		{ "input.flags", &flags },
		{ "label", &label, true },
		{ "input.type", &type },
		{ "input.field_name", &fieldName },
		{ "input.hint", &hint },
		{ "input.imeType", &imeType },
		{ "input.step", &step },
		{ "input.format", &format },
		{ "initial_focus", &initialFocus },
		{ "force_focus", &forceFocus },
		{ "size_x", &size_x },
		{ "size_y", &size_y }, 
	});
	return props;
}

bool Input::PropertyUI(int i, UIContext& ctx)
{
	static const char* TYPES[] {
		"std::string",
		"ImGuiTextFilter",
		"int",
		"int2",
		"int3",
		"int4",
		"float",
		"float2",
		"float3",
		"float4",
		"double",
	};

	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("text");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##text", &style_text, ImGuiCol_Text, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("text", &style_text, ctx);
		break;
	case 1:
		ImGui::Text("frameBg");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##frameBg", &style_frameBg, ImGuiCol_FrameBg, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("frameBg", &style_frameBg, ctx);
		break;
	case 2:
		ImGui::Text("border");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##border", &style_border, ImGuiCol_Border, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("border", &style_border, ctx);
		break;
	case 3:
		ImGui::Text("borderSize");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##borderSize", &style_frameBorderSize, ctx);
		break;
	case 4:
		ImGui::Text("font");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##font", &style_font, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("font", &style_font, ctx);
		break;
	case 5:
		TreeNodeProp("flags", "...", [&] {
			ImGui::TableNextColumn();
			ImGui::Spacing();
			changed = CheckBoxFlags(&flags);
			});
		break;
	case 6:
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##label", &label, ctx);
		break;
	case 7:
		ImGui::Text("type");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (ImGui::BeginCombo("##type", type.c_str()))
		{
			for (const auto& tp : TYPES)
			{
				if (ImGui::Selectable(tp, type == tp)) {
					changed = true;
					type = tp;
					ctx.codeGen->ChangeVar(fieldName.c_str(), type, "");
				}
			}
			ImGui::EndCombo();
		}
		break;
	case 8:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##fieldName", &fieldName, type, false, ctx);
		break;
	case 9:
		ImGui::BeginDisabled(type != "std::string" && type != "ImGuiTextFilter");
		ImGui::Text("hint");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##hint", &hint, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("hint", &hint, ctx);
		ImGui::EndDisabled();
		break;
	case 10:
	{
		int type = imeType & 0xff;
		int action = imeType & (~0xff);
		std::string val = _imeClass.get_name(type, false);
		if (action)
			val += " | " + _imeAction.get_name(action, false);
		TreeNodeProp("imeType", val, [&] {
			ImGui::TableNextColumn();
			ImGui::Spacing();
			for (const auto& id : _imeClass.get_ids()) {
				changed |= ImGui::RadioButton(id.first.c_str(), &type, id.second);
			}
			ImGui::Separator();
			for (const auto& id : _imeAction.get_ids()) {
				changed |= ImGui::RadioButton(id.first.c_str(), &action, id.second);
			}
			imeType = type | action;
			});
		break;
	}
	case 11:
		ImGui::BeginDisabled(type != "int" && type != "float");
		ImGui::Text("step");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (type == "int") {
			int st = (int)*step.access();
			changed = ImGui::InputInt("##step", &st);
			*step.access() = (float)st;
		}
		else {
			changed = ImGui::InputFloat("##step", step.access());
		}
		ImGui::EndDisabled();
		break;
	case 12:
		ImGui::BeginDisabled(type.access()->compare(0, 5, "float"));
		ImGui::Text("format");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputText("##format", format.access());
		ImGui::EndDisabled();
		break;
	case 13:
		ImGui::Text("initialFocus");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::Checkbox("##kbf", initialFocus.access());
		break;
	case 14:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("forceFocus");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##forceFocus", &forceFocus, true, ctx);
		break;
	case 15:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_x", &size_x, ctx);
		break;
	case 16:
		ImGui::BeginDisabled(type != "std::string" || !(flags & ImGuiInputTextFlags_Multiline));
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, {}, true, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_y", &size_y, ctx);
		ImGui::EndDisabled();
		break;
	default:
		return Widget::PropertyUI(i - 17, ctx);
	}
	return changed;
}

std::vector<UINode::Prop>
Input::Events()
{
	auto props = Widget::Events();
	props.insert(props.begin(), {
		{ "onChange", &onChange },
		{ "onImeAction", &onImeAction },
		});
	return props;
}

bool Input::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("OnChange");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##onChange", &onChange, ctx);
		break;
	case 1:
		ImGui::Text("OnImeAction");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##onImeAction", &onImeAction, ctx);
		break;
	default:
		return Widget::EventUI(i - 2, ctx);
	}
	return changed;
}

//---------------------------------------------------

Combo::Combo(UIContext& ctx)
{
	size_x = 200;

	if (ctx.createVars)
		fieldName.set_from_arg(ctx.codeGen->CreateVar("std::string", "", CppGen::Var::Interface));

	flags.prefix("ImGuiComboFlags_");
	flags.add$(ImGuiComboFlags_PopupAlignLeft);
	flags.add$(ImGuiComboFlags_HeightSmall);
	flags.add$(ImGuiComboFlags_HeightRegular);
	flags.add$(ImGuiComboFlags_HeightLarge);
	flags.add$(ImGuiComboFlags_HeightLargest);
	flags.add$(ImGuiComboFlags_NoArrowButton);
	flags.add$(ImGuiComboFlags_NoPreview);
}

std::unique_ptr<Widget> Combo::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<Combo>(*this);
	if (!fieldName.empty() && ctx.createVars) {
		sel->fieldName.set_from_arg(ctx.codeGen->CreateVar("std::string", "", CppGen::Var::Interface));
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
		id = std::string("##") + fieldName.c_str();
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
		id = std::string("\"##") + fieldName.c_str() + "\"";
	
	os << "ImRad::Combo(" << id << ", &" << fieldName.to_arg() 
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
	if ((sit->kind == cpp::CallExpr && (sit->callee == "ImGui::Combo" || sit->callee == "ImRad::Combo")) ||
		(sit->kind == cpp::IfCallThenCall && (sit->callee == "ImGui::Combo" || sit->callee == "ImRad::Combo")))
	{
		if (sit->params.size()) {
			label.set_from_arg(sit->params[0]);
			if (!label.access()->compare(0, 2, "##"))
				label = "";
		}

		if (sit->params.size() >= 2 && !sit->params[1].compare(0, 1, "&")) {
			fieldName.set_from_arg(sit->params[1].substr(1));
			std::string fn = fieldName.c_str();
			if (!ctx.codeGen->GetVar(fn))
				ctx.errors.push_back("Combo: field_name variable '" + fn + "' doesn't exist");
		}

		if (sit->params.size() >= 3) {
			items.set_from_arg(sit->params[2]);
		}

		if (sit->params.size() >= 4) {
			flags.set_from_arg(sit->params[3]);
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
Combo::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "@style.text", &style_text },
		{ "@style.borderSize", &style_frameBorderSize },
		{ "@style.font", &style_font },
		{ "combo.flags", &flags },
		{ "label", &label, true },
		{ "combo.field_name", &fieldName },
		{ "combo.items", &items },
		{ "size_x", &size_x },
		});
	return props;
}

bool Combo::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("text");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##text", &style_text, ImGuiCol_Text, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("text", &style_text, ctx);
		break;
	case 1:
		ImGui::Text("borderSize");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##borderSize", &style_frameBorderSize, ctx);
		break;
	case 2:
		ImGui::Text("font");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##font", &style_font, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("font", &style_font, ctx);
		break;
	case 3:
		TreeNodeProp("flags", "...", [&] {
			ImGui::TableNextColumn();
			ImGui::Spacing();
			changed = CheckBoxFlags(&flags);
			});
		break;
	case 4:
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##label", &label, ctx);
		break;
	case 5:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##fieldName", &fieldName, false, ctx);
		break;
	case 6:
		ImGui::Text("items");
		ImGui::TableNextColumn();
		//ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (ImGui::Selectable(ICON_FA_PEN_TO_SQUARE, false, 0, { ImGui::GetContentRegionAvail().x-ImGui::GetFrameHeight(), ImGui::GetFrameHeight() }))
		{
			changed = true;
			std::string tmp = *items.access(); //preserve embeded nulls
			stx::replace(tmp, '\0', '\n');
			comboDlg.value = tmp;
			comboDlg.defaultFont = ctx.defaultFont;
			comboDlg.OpenPopup([this](ImRad::ModalResult) {
				std::string tmp = comboDlg.value;
				if (!tmp.empty() && tmp.back() != '\n')
					tmp.push_back('\n');
				stx::replace(tmp, '\n', '\0');
				*items.access() = tmp;
				});
		}
		ImGui::SameLine(0, 0);
		changed |= BindingButton("items", &items, ctx);
		break;
	case 7:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_x", &size_x, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 8, ctx);
	}
	return changed;
}

std::vector<UINode::Prop>
Combo::Events()
{
	auto props = Widget::Events();
	props.insert(props.begin(), {
		{ "onChange", &onChange }
		});
	return props;
}

bool Combo::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("OnChange");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##onChange", &onChange, ctx);
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

	if (ctx.createVars)
		fieldName.set_from_arg(ctx.codeGen->CreateVar(type, "", CppGen::Var::Interface));
}

std::unique_ptr<Widget> Slider::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<Slider>(*this);
	if (!fieldName.empty() && ctx.createVars) {
		sel->fieldName.set_from_arg(ctx.codeGen->CreateVar(type, "", CppGen::Var::Interface));
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
		id = "##" + fieldName.value();

	if (type == "int")
		ImGui::SliderInt(id.c_str(), itmp, (int)min, (int)max, fmt, ImGuiSliderFlags_NoInput);
	else if (type == "int2")
		ImGui::SliderInt2(id.c_str(), itmp, (int)min, (int)max, fmt, ImGuiSliderFlags_NoInput);
	else if (type == "int3")
		ImGui::SliderInt3(id.c_str(), itmp, (int)min, (int)max, fmt, ImGuiSliderFlags_NoInput);
	else if (type == "int4")
		ImGui::SliderInt4(id.c_str(), itmp, (int)min, (int)max, fmt, ImGuiSliderFlags_NoInput);
	else if (type == "float")
		ImGui::SliderFloat(id.c_str(), ftmp, min, max, fmt, ImGuiSliderFlags_NoInput);
	else if (type == "float2")
		ImGui::SliderFloat2(id.c_str(), ftmp, min, max, fmt, ImGuiSliderFlags_NoInput);
	else if (type == "float3")
		ImGui::SliderFloat3(id.c_str(), ftmp, min, max, fmt, ImGuiSliderFlags_NoInput);
	else if (type == "float4")
		ImGui::SliderFloat4(id.c_str(), ftmp, min, max, fmt, ImGuiSliderFlags_NoInput);
	else if (type == "angle")
		ImGui::SliderAngle(id.c_str(), ftmp, min, max, fmt, ImGuiSliderFlags_NoInput);

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

	std::string cap = type;
	cap[0] = std::toupper(cap[0]);
	std::string id = label.to_arg();
	if (label.empty())
		id = "\"##" + fieldName.value() + "\"";

	os << "ImGui::Slider" << cap << "(" << id << ", &"
		<< fieldName.to_arg() << ", " << min.to_arg() << ", " << max.to_arg() 
		<< ", " << fmt << ")";
	
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
		type = sit->callee.substr(13);
		type.access()->front() = std::tolower(type.c_str()[0]);

		if (sit->params.size()) {
			label.set_from_arg(sit->params[0]);
			if (!label.access()->compare(0, 2, "##"))
				label = "";
		}

		if (sit->params.size() > 1 && !sit->params[1].compare(0, 1, "&"))
			fieldName.set_from_arg(sit->params[1].substr(1));

		if (sit->params.size() > 2)
			min.set_from_arg(sit->params[2]);

		if (sit->params.size() > 3)
			max.set_from_arg(sit->params[3]);
		
		if (sit->params.size() > 4)
			format.set_from_arg(sit->params[4]);

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
		{ "label", &label, true },
		{ "slider.type", &type },
		{ "slider.field_name", &fieldName },
		{ "slider.min", &min },
		{ "slider.max", &max },
		{ "slider.format", &format },
		{ "size_x", &size_x }
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
	switch (i)
	{
	case 0:
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##label", &label, ctx);
		break;
	case 1:
		ImGui::Text("type");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (ImGui::BeginCombo("##type", type.c_str()))
		{
			for (const auto& tp : TYPES)
			{
				if (ImGui::Selectable(tp, type == tp)) {
					changed = true;
					type = tp;
					ctx.codeGen->ChangeVar(fieldName.c_str(), type == "angle" ? "float" : type, "");
				}
			}
			ImGui::EndCombo();
		}
		break;
	case 2:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##fieldName", &fieldName, type, false, ctx);
		break;
	case 3:
		ImGui::Text("min");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (type == "int") {
			int val = (int)min;
			changed = ImGui::InputInt("##min", &val);
			min = (float)val;
		}
		else {
			changed = ImGui::InputFloat("##min", min.access());
		}
		break;
	case 4:
		ImGui::Text("max");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (type == "int") {
			int val = (int)max;
			changed = ImGui::InputInt("##max", &val);
			max = (float)val;
		}
		else {
			changed = ImGui::InputFloat("##max", max.access());
		}
		break;
	case 5:
		ImGui::Text("format");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputText("##format", format.access());
		break;
	case 6:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_x", &size_x, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 7, ctx);
	}
	return changed;
}

std::vector<UINode::Prop>
Slider::Events()
{
	auto props = Widget::Events();
	props.insert(props.begin(), {
		{ "onChange", &onChange }
		});
	return props;
}

bool Slider::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("OnChange");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##onChange", &onChange, ctx);
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
		fieldName.set_from_arg(ctx.codeGen->CreateVar("float", "0", CppGen::Var::Interface));
}

std::unique_ptr<Widget> ProgressBar::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<ProgressBar>(*this);
	if (!fieldName.empty() && ctx.createVars) {
		sel->fieldName.set_from_arg(ctx.codeGen->CreateVar("float", "0", CppGen::Var::Interface));
	}
	return sel;
}

ImDrawList* ProgressBar::DoDraw(UIContext& ctx)
{
	if (!style_color.empty())
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, style_color.eval(ctx));

	float w = size_x.eval_px(ImGuiAxis_X, ctx);
	float h = size_y.eval_px(ImGuiAxis_Y, ctx);

	float pc = 0.5f;
	if (!fieldName.empty())
		pc = fieldName.eval(ctx);
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
		<< fieldName.to_arg() << ", { "
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
			fieldName.set_from_arg(sit->params[0]);

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
		{ "@style.color", &style_color },
		{ "@style.borderSize", &style_frameBorderSize },
		{ "progress.field_name", &fieldName },
		{ "progress.indicator", &indicator },
		{ "size_x", &size_x },
		{ "size_y", &size_y },
		});
	return props;
}

bool ProgressBar::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("color");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##color", &style_color, ImGuiCol_PlotHistogram, ctx);
		break;
	case 1:
		ImGui::Text("borderSize");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##borderSize", &style_frameBorderSize, ctx);
		break;
	case 2:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##fieldName", &fieldName, "float", false, ctx);
		break;
	case 3:
	{
		ImGui::Text("indicator");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		bool ind = indicator;
		if (ImGui::Checkbox("##indicator", &ind))
			indicator = ind;
		break;
	}
	case 4:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_x", &size_x, ctx);
		break;
	case 5:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, {}, true, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_y", &size_y, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 6, ctx);
	}
	return changed;
}

//---------------------------------------------------

ColorEdit::ColorEdit(UIContext& ctx)
{
	size_x = 200;

	flags.prefix("ImGuiColorEditFlags_");
	flags.add$(ImGuiColorEditFlags_NoAlpha);
	flags.add$(ImGuiColorEditFlags_NoPicker);
	flags.add$(ImGuiColorEditFlags_NoOptions);
	flags.add$(ImGuiColorEditFlags_NoSmallPreview);
	flags.add$(ImGuiColorEditFlags_NoInputs);
	flags.add$(ImGuiColorEditFlags_NoTooltip);
	flags.add$(ImGuiColorEditFlags_NoLabel);
	flags.add$(ImGuiColorEditFlags_NoSidePreview);
	flags.add$(ImGuiColorEditFlags_NoDragDrop);
	flags.add$(ImGuiColorEditFlags_NoBorder);

	if (ctx.createVars)
		fieldName.set_from_arg(ctx.codeGen->CreateVar(type, "", CppGen::Var::Interface));
}

std::unique_ptr<Widget> ColorEdit::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<ColorEdit>(*this);
	if (!fieldName.empty() && ctx.createVars) {
		sel->fieldName.set_from_arg(ctx.codeGen->CreateVar(type, "", CppGen::Var::Interface));
	}
	return sel;
}

ImDrawList* ColorEdit::DoDraw(UIContext& ctx)
{
	ImVec4 ftmp = {};
	
	std::string id = label;
	if (id.empty())
		id = "##" + fieldName.value();
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
		id = "\"##" + fieldName.value() + "\"";
	
	if (type == "color3")
	{
		os << "ImGui::ColorEdit3(" << id << ", (float*)&"
			<< fieldName.to_arg() << ", " << flags.to_arg() << ")";
	}
	else if (type == "color4")
	{
		os << "ImGui::ColorEdit4(" << id << ", (float*)&"
			<< fieldName.to_arg() << ", " << flags.to_arg() << ")";
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
			fieldName.set_from_arg(sit->params[1].substr(9));
			std::string fn = fieldName.c_str();
			if (!ctx.codeGen->GetVar(fn))
				ctx.errors.push_back("Input: field_name variable '" + fn + "' doesn't exist");
		}

		if (sit->params.size() >= 3)
			flags.set_from_arg(sit->params[2]);

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
		{ "@style.borderSize", &style_frameBorderSize },
		{ "color.flags", &flags },
		{ "label", &label, true },
		{ "color.type", &type },
		{ "color.field_name", &fieldName },
		{ "size_x", &size_x },
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
	switch (i)
	{
	case 0:
		ImGui::Text("borderSize");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##borderSize", &style_frameBorderSize, ctx);
		break;
	case 1:
		TreeNodeProp("flags", "...", [&] {
			ImGui::TableNextColumn();
			ImGui::Spacing();
			changed = CheckBoxFlags(&flags);
			});
		break;
	case 2:
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##label", &label, ctx);
		break;
	case 3:
		ImGui::Text("type");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (ImGui::BeginCombo("##type", type.c_str()))
		{
			for (const auto& tp : TYPES)
			{
				if (ImGui::Selectable(tp, type == tp)) {
					changed = true;
					type = tp;
					ctx.codeGen->ChangeVar(fieldName.c_str(), type, "");
				}
			}
			ImGui::EndCombo();
		}
		break;
	case 4:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##fieldName", &fieldName, type, false, ctx);
		break;
	case 5:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_x", &size_x, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 6, ctx);
	}
	return changed;
}

std::vector<UINode::Prop>
ColorEdit::Events()
{
	auto props = Widget::Events();
	props.insert(props.begin(), {
		{ "onChange", &onChange }
		});
	return props;
}

bool ColorEdit::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("OnChange");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##onChange", &onChange, ctx);
		break;
	default:
		return Widget::EventUI(i - 1, ctx);
	}
	return changed;
}

//----------------------------------------------------

Image::Image(UIContext& ctx)
{
	if (ctx.createVars)
		*fieldName.access() = ctx.codeGen->CreateVar("ImRad::Texture", "", CppGen::Var::Impl);
}

std::unique_ptr<Widget> Image::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<Image>(*this);
	if (!fieldName.empty() && ctx.createVars) {
		sel->fieldName.set_from_arg(ctx.codeGen->CreateVar("ImRad::Texture", "", CppGen::Var::Impl));
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
	if (!stretch && tex.id) 
	{
		float wrel = w / tex.w;
		float hrel = h / tex.h;
		uv0 = { -(wrel - 1) / 2, -(hrel - 1) / 2 };
		uv1 = { 1 + (wrel - 1) / 2, 1 + (hrel - 1) / 2 };
	}
	ImGui::Image(tex.id, { w, h }, uv0, uv1);
	return ImGui::GetWindowDrawList();
}

void Image::DoExport(std::ostream& os, UIContext& ctx)
{
	if (fieldName.empty())
		ctx.errors.push_back("Image: field_name doesn't exist");
	if (fileName.empty())
		ctx.errors.push_back("Image: file_name doesn't exist");

	os << ctx.ind << "if (!" << fieldName.to_arg() << ")\n";
	ctx.ind_up();
	os << ctx.ind << fieldName.to_arg() << " = ImRad::LoadTextureFromFile(" << fileName.to_arg() << ");\n";
	ctx.ind_down();

	os << ctx.ind << "ImGui::Image(" << fieldName.to_arg() << ".id, { ";
	
	if (size_x.zero())
		os << "(float)" << fieldName.to_arg() << ".w";
	else
		os << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]);
	
	os << ", ";
	
	if (size_y.zero())
		os << "(float)" << fieldName.to_arg() << ".h";
	else
		os << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]);
	os << " }, ";

	ImVec2 uv0(0, 0);
	ImVec2 uv1(1, 1);
	//todo: center with binded dimensions
	if (!stretch && !size_x.zero() && size_x.has_value()) 
	{
		float wrel = size_x.eval_px(ImGuiAxis_X, ctx) / tex.w;
		uv0.x = -(wrel - 1) / 2;
		uv1.x = 1 + (wrel - 1) / 2;
	}
	if (!stretch && !size_y.zero() && size_y.has_value()) 
	{
		float hrel = size_y.eval_px(ImGuiAxis_Y, ctx) / tex.h;
		uv0.y = -(hrel - 1) / 2;
		uv1.y = 1 + (hrel - 1) / 2;
	}
	os << "{ " << uv0.x << ", " << uv0.y << " }, { " << uv1.x << ", " << uv1.y << " }";

	os << ");\n";
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
			fieldName.set_from_arg(sit->params[0].substr(0, sit->params[0].size() - 3));

		if (sit->params.size() >= 2) {
			auto size = cpp::parse_size(sit->params[1]);
			if (size.first == "(float)" + fieldName.value() + ".w")
				size_x.set_from_arg("0");
			else
				size_x.set_from_arg(size.first);
			
			if (size.second == "(float)" + fieldName.value() + ".h")
				size_y.set_from_arg("0");
			else
				size_y.set_from_arg(size.second);
		}

		if (sit->params.size() >= 4) {
			ImVec2 uv0 = cpp::parse_fsize(sit->params[2]);
			ImVec2 uv1 = cpp::parse_fsize(sit->params[3]);
			if (uv0.x || uv0.y || uv1.x != 1 || uv1.y != 1)
				stretch = false;
		}

		RefreshTexture(ctx);
	}
}

std::vector<UINode::Prop>
Image::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "image.file_name", &fileName, true },
		{ "image.field_name", &fieldName },
		{ "stretch", &stretch },
		{ "size_x", &size_x },
		{ "size_y", &size_y },
		});
	return props;
}

bool Image::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("fileName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-2 * ImGui::GetFrameHeight());
		changed = InputBindable("##fileName", &fileName, ctx);
		if (ImGui::IsItemDeactivatedAfterEdit() || 
			(ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
			RefreshTexture(ctx);
		ImGui::SameLine(0, 0);
		if (ImGui::Button("...", { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() }) &&
			PickFileName(ctx)) 
		{
			changed = true;
			RefreshTexture(ctx);
		}
		ImGui::SameLine(0, 0);
		changed |= BindingButton("fileName", &fileName, ctx);
		break;
	case 1:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##fieldn", &fieldName, false, ctx);
		break;
	case 2:
		ImGui::Text("stretch");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##stretch", &stretch, ctx);
		break;
	case 3:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_x", &size_x, ctx);
		break;
	case 4:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, {}, true, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_y", &size_y, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 5, ctx);
	}
	return changed;
}

bool Image::PickFileName(UIContext& ctx)
{
	nfdchar_t *outPath = NULL;
	nfdfilteritem_t filterItem[1] = { { "All Images", "bmp,gif,jpg,jpeg,png,tga" } };
	nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);
	if (result != NFD_OKAY)
		return false;

	fileName = fs::relative(outPath, ctx.workingDir).generic_string();
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
	if (fs::path(fname).is_relative()) 
	{
		if (ctx.workingDir.empty() && !ctx.importState) {
			messageBox.title = "Warning";
			messageBox.message = "Please save the file first so that relative paths can work";
			messageBox.buttons = ImRad::Ok;
			messageBox.OpenPopup();
			return;
		}
		fname = (fs::path(ctx.workingDir) / fileName.value()).string();
	}

	tex = ImRad::LoadTextureFromFile(fname);
	if (!tex) 
	{
		if (!ctx.importState) {
			messageBox.title = "Error";
			messageBox.message = "Can't read " + fname;
			messageBox.buttons = ImRad::Ok;
			messageBox.OpenPopup();
		}
		else
			ctx.errors.push_back("Image: can't read " + fname);
	}
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
		ctx.errors.push_back("CustomWidget: OnDraw not set!");
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
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "size_x", &size_x },
		{ "size_y", &size_y },
		});
	return props;
}

bool CustomWidget::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_x", &size_x, ctx);
		break;
	case 1:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, {}, true, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_y", &size_y, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 2, ctx);
	}
	return changed;
}

std::vector<UINode::Prop>
CustomWidget::Events()
{
	auto props = Widget::Events();
	props.insert(props.begin(), {
		{ "onDraw", &onDraw },
		});
	return props;
}

bool CustomWidget::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("OnDraw");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##OnDraw", &onDraw, ctx);
		break;
	default:
		return Widget::EventUI(i - 1, ctx);
	}
	return changed;
}

//---------------------------------------------------

Table::ColumnData::ColumnData()
{
	flags.prefix("ImGuiTableColumnFlags_");
	flags.add$(ImGuiTableColumnFlags_Disabled);
	flags.add$(ImGuiTableColumnFlags_WidthStretch);
	flags.add$(ImGuiTableColumnFlags_WidthFixed);
	flags.add$(ImGuiTableColumnFlags_NoResize);
	flags.add$(ImGuiTableColumnFlags_NoHide);
	flags.add$(ImGuiTableColumnFlags_NoSort);
	flags.add$(ImGuiTableColumnFlags_NoHeaderWidth);
	flags.add$(ImGuiTableColumnFlags_AngledHeader);
}

Table::Table(UIContext& ctx)
{
	size_x = -1; //here 0 has the same effect as -1 but -1 works with our sizing visualization
	size_y = 0;

	flags.prefix("ImGuiTableFlags_");
	flags.add$(ImGuiTableFlags_Resizable);
	flags.add$(ImGuiTableFlags_Reorderable);
	flags.add$(ImGuiTableFlags_Hideable);
	flags.add$(ImGuiTableFlags_Sortable);
	flags.add$(ImGuiTableFlags_ContextMenuInBody);
	flags.separator();
	flags.add$(ImGuiTableFlags_RowBg);
	flags.add$(ImGuiTableFlags_BordersInnerH);
	flags.add$(ImGuiTableFlags_BordersInnerV);
	flags.add$(ImGuiTableFlags_BordersOuterH);
	flags.add$(ImGuiTableFlags_BordersOuterV);
	//flags.add$(ImGuiTableFlags_NoBordersInBody);
	//flags.add$(ImGuiTableFlags_NoBordersInBodyUntilResize);
	flags.separator();
	flags.add$(ImGuiTableFlags_SizingFixedFit);
	flags.add$(ImGuiTableFlags_SizingFixedSame);
	flags.add$(ImGuiTableFlags_SizingStretchProp);
	flags.add$(ImGuiTableFlags_SizingStretchSame);
	flags.separator();
	flags.add$(ImGuiTableFlags_PadOuterX);
	flags.add$(ImGuiTableFlags_NoPadOuterX);
	flags.add$(ImGuiTableFlags_NoPadInnerX);
	flags.add$(ImGuiTableFlags_ScrollX);
	flags.add$(ImGuiTableFlags_ScrollY);
	flags.add$(ImGuiTableFlags_HighlightHoveredColumn);

	columnData.resize(3);
	for (size_t i = 0; i < columnData.size(); ++i)
		columnData[i].label = char('A' + i);
}

std::unique_ptr<Widget> Table::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<Table>(*this);
	//rowCount can be shared
	sel->CloneChildrenFrom(*this, ctx);
	return sel;
}

ImDrawList* Table::DoDraw(UIContext& ctx)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	bool childPos = stx::count_if(children, [](const auto& ch) { return ch->hasPos; });

	if (style_cellPadding.has_value())
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, style_cellPadding.eval_px(ctx));
	if (!style_headerBg.empty())
		ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, style_headerBg.eval(ctx));
	if (!style_rowBg.empty())
		ImGui::PushStyleColor(ImGuiCol_TableRowBg, style_rowBg.eval(ctx));
	if (!style_rowBgAlt.empty())
		ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, style_rowBgAlt.eval(ctx));
	if (!style_childBg.empty())
		ImGui::PushStyleColor(ImGuiCol_ChildBg, style_childBg.eval(ctx));

	int n = std::max(1, (int)columnData.size());
	ImVec2 size{ size_x.eval_px(ImGuiAxis_X, ctx), size_y.eval_px(ImGuiAxis_Y, ctx) };
	float rh = rowHeight.eval_px(ImGuiAxis_Y, ctx);
	int fl = flags;
	if (stx::count(ctx.selected, this)) //force columns at design time
		fl |= ImGuiTableFlags_BordersInner;
	std::string name = "table" + std::to_string((uint64_t)this);
	if (ImGui::BeginTable(name.c_str(), n, fl, size))
	{
		//need to override drawList because when table is in a Child mode its drawList will be drawn on top
		drawList = ImGui::GetWindowDrawList();

		for (size_t i = 0; i < (int)columnData.size(); ++i)
			ImGui::TableSetupColumn(columnData[i].label.c_str(), columnData[i].flags, columnData[i].width);
		if (header)
			ImGui::TableHeadersRow();

		ImGui::TableNextRow(0, rh);
		ImGui::TableSetColumnIndex(0);
		
		for (int i = 0; i < (int)children.size(); ++i)
			if (!children[i]->hasPos)
				children[i]->Draw(ctx);
		
		int n = rowCount.limit.value();
		for (int r = ImGui::TableGetRowIndex() + 1; r < header + n; ++r)
			ImGui::TableNextRow(0, rh);

		if (childPos)
		{
			ImGuiWindow* inner = ImGui::GetCurrentWindow(); // ImGui::GetCurrentTable()->InnerWindow;
			ImGui::PushClipRect(inner->InnerRect.Min, inner->InnerClipRect.Max, false);
			for (int i = 0; i < (int)children.size(); ++i)
				if (children[i]->hasPos)
					children[i]->Draw(ctx);
			ImGui::PopClipRect();
		}

		ImGui::EndTable();
	}

	if (!style_childBg.empty())
		ImGui::PopStyleColor();
	if (!style_headerBg.empty())
		ImGui::PopStyleColor();
	if (!style_rowBg.empty())
		ImGui::PopStyleColor();
	if (!style_rowBgAlt.empty())
		ImGui::PopStyleColor();
	if (style_cellPadding.has_value())
		ImGui::PopStyleVar();

	return drawList;
}

std::vector<UINode::Prop>
Table::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "@style.headerBg", &style_headerBg },
		{ "@style.rowBg", &style_rowBg },
		{ "@style.rowBgAlt", &style_rowBgAlt },
		{ "@style.childBg", &style_childBg },
		{ "@style.cellPadding", &style_cellPadding },
		{ "table.flags", &flags },
		{ "table.columns", nullptr },
		{ "table.header", &header },
		{ "table.rowCount", &rowCount },
		{ "table.rowHeight", &rowHeight },
		{ "table.rowFilter", &rowFilter },
		{ "table.scrollWhenDragging", &scrollWhenDragging },
		{ "size_x", &size_x },
		{ "size_y", &size_y },
		});
	return props;
}

bool Table::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("headerBg");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##headerbg", &style_headerBg, ImGuiCol_TableHeaderBg, ctx);
		break;
	case 1:
		ImGui::Text("rowBg");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##rowbg", &style_rowBg, ImGuiCol_TableRowBg, ctx);
		break;
	case 2:
		ImGui::Text("rowBgAlt");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##rowbgalt", &style_rowBgAlt, ImGuiCol_TableRowBgAlt, ctx);
		break;
	case 3:
		ImGui::Text("childBg");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##bg", &style_childBg, ImGuiCol_ChildBg, ctx);
		break;
	case 4:
		ImGui::Text("cellPadding");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##style_cellPadding", &style_cellPadding, ctx);
		break;
	case 5:
		TreeNodeProp("flags", "...", [&] {
			ImGui::TableNextColumn();
			ImGui::Spacing();
			changed = CheckBoxFlags(&flags);
			});
		break;
	case 6:
	{
		ImGui::Text("columns");
		ImGui::TableNextColumn();
		if (ImGui::Selectable(ICON_FA_PEN_TO_SQUARE, false, 0, { ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight(), ImGui::GetFrameHeight() }))
		{
			changed = true;
			tableColumns.columnData = columnData;
			tableColumns.target = &columnData;
			tableColumns.defaultFont = ctx.defaultFont;
			tableColumns.OpenPopup();
		}
		break;
	}
	case 7:
		ImGui::Text("header");
		ImGui::TableNextColumn();
		changed = ImGui::Checkbox("##header", header.access());
		break;
	case 8:
		changed = DataLoopProp("rowCount", &rowCount, ctx);
		break;
	case 9:
		ImGui::Text("rowHeight");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##rowh", &rowHeight, {}, false, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("rowHeight", &rowHeight, ctx);
		break;
	case 10:
		ImGui::BeginDisabled(rowCount.empty());
		ImGui::Text("rowFilter");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputText("##rowFilter", rowFilter.access());
		ImGui::SameLine(0, 0);
		changed |= BindingButton("rowFilter", &rowFilter, ctx);
		ImGui::EndDisabled();
		break;
	case 11:
		ImGui::Text("scrollWhenDragging");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##swd", &scrollWhenDragging, ctx);
		break;
	case 12:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_x", &size_x, ctx);
		break;
	case 13:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, {}, true, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_y", &size_y, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 14, ctx);
	}
	return changed;
}

std::vector<UINode::Prop>
Table::Events()
{
	auto props = Widget::Events();
	props.insert(props.begin(), {
		{ "onBeginRow", &onBeginRow },
		{ "onEndRow", &onEndRow }, 
		});
	return props;
}

bool Table::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("OnBeginRow");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##OnBeginRow", &onBeginRow, ctx);
		break;
	case 1:
		ImGui::Text("OnEndRow");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##OnEndRow", &onEndRow, ctx);
		break;
	default:
		return Widget::EventUI(i - 2, ctx);
	}
	return changed;
}

void Table::DoExport(std::ostream& os, UIContext& ctx)
{
	bool childPos = stx::count_if(children, [](const auto& ch) { return ch->hasPos; });

	if (style_cellPadding.has_value())
	{
		os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, "
			<< style_cellPadding.to_arg(ctx.unit) << ");\n";
	}
	if (!style_headerBg.empty())
	{
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, "
			<< style_headerBg.to_arg() << ");\n";
	}
	if (!style_rowBg.empty())
	{
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_TableRowBg, "
			<< style_rowBg.to_arg() << ");\n";
	}
	if (!style_rowBgAlt.empty())
	{
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, "
			<< style_rowBgAlt.to_arg() << ");\n";
	}
	if (!style_childBg.empty())
	{
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_ChildBg, "
			<< style_childBg.to_arg() << ");\n";
	}
	if (scrollWhenDragging)
	{
		os << ctx.ind << "ImRad::PushInvisibleScrollbar();\n";
	}

	os << ctx.ind << "if (ImGui::BeginTable("
		<< "\"table" << ctx.varCounter << "\", " 
		<< columnData.size() << ", " 
		<< flags.to_arg() << ", "
		<< "{ " << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", " << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]) << " }"
		<< "))\n"
		<< ctx.ind << "{\n";
	ctx.ind_up();

	if (scrollWhenDragging)
	{
		os << ctx.ind << "ImRad::ScrollWhenDragging(true);\n";
	}

	for (const auto& cd : columnData)
	{
		os << ctx.ind << "ImGui::TableSetupColumn(\"" << cd.label << "\", "
			<< cd.flags.to_arg() << ", ";
		if (cd.flags & ImGuiTableColumnFlags_WidthFixed) {
			direct_val<dimension> dim(cd.width);
			os << dim.to_arg(ctx.unit);
		}
		else
			os << cd.width;
		os << ");\n";
	}
	if (header)
		os << ctx.ind << "ImGui::TableHeadersRow();\n";

	if (!rowCount.empty())
	{
		os << "\n" << ctx.ind << rowCount.to_arg(ctx.forVarName) << "\n" << ctx.ind << "{\n";
		ctx.ind_up();

		if (!rowFilter.empty())
		{
			os << ctx.ind << "if (!(" << rowFilter.to_arg() << "))\n";
			ctx.ind_up();
			os << ctx.ind << "continue;\n";
			ctx.ind_down();
		}
		
		os << ctx.ind << "ImGui::PushID(" << rowCount.index_name_or(CppGen::FOR_VAR) << ");\n";
	}
	
	os << ctx.ind << "ImGui::TableNextRow(0, ";
	if (!rowHeight.empty())
		os << rowHeight.to_arg(ctx.unit);
	else
		os << "0";
	os << ");\n";
	os << ctx.ind << "ImGui::TableSetColumnIndex(0);\n";
	
	if (!onBeginRow.empty())
		os << ctx.ind << onBeginRow.to_arg() << "();\n";

	os << ctx.ind << "/// @separator\n\n";

	for (auto& child : children)
		if (!child->hasPos)
			child->Export(os, ctx);

	os << "\n" << ctx.ind << "/// @separator\n";
	
	if (!onEndRow.empty())
		os << ctx.ind << onEndRow.to_arg() << "();\n";

	if (!rowCount.empty()) {
		os << ctx.ind << "ImGui::PopID();\n";
		ctx.ind_down();
		os << ctx.ind << "}\n";
	}

	if (childPos)
	{
		//draw overlay children at the end so they are visible,
		//inside table's child because ItemOverlap works only between items in same window
		os << ctx.ind << "ImGui::PushClipRect(ImGui::GetCurrentWindow()->InnerRect.Min, ImGui::GetCurrentWindow()->InnerRect.Max, false);\n";
		os << ctx.ind << "/// @separator\n\n";

		for (auto& child : children)
			if (child->hasPos)
				child->Export(os, ctx);
	
		os << ctx.ind << "/// @separator\n";
		os << ctx.ind << "ImGui::PopClipRect();\n";
	}

	os << ctx.ind << "ImGui::EndTable();\n";
	ctx.ind_down();
	os << ctx.ind << "}\n";

	if (!style_rowBg.empty())
		os << ctx.ind << "ImGui::PopStyleColor();\n";
	if (!style_rowBgAlt.empty())
		os << ctx.ind << "ImGui::PopStyleColor();\n";
	if (!style_childBg.empty())
		os << ctx.ind << "ImGui::PopStyleColor();\n";
	if (!style_headerBg.empty())
		os << ctx.ind << "ImGui::PopStyleColor();\n";
	if (style_cellPadding.has_value())
		os << ctx.ind << "ImGui::PopStyleVar();\n";
	if (scrollWhenDragging)
		os << ctx.ind << "ImRad::PopInvisibleScrollbar();\n";

	++ctx.varCounter;
}

void Table::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar")
	{
		if (sit->params.size() && sit->params[0] == "ImGuiStyleVar_CellPadding")
			style_cellPadding.set_from_arg(sit->params[1]);
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
	{
		if (sit->params.size() && sit->params[0] == "ImGuiCol_TableHeaderBg")
			style_headerBg.set_from_arg(sit->params[1]);
		if (sit->params.size() && sit->params[0] == "ImGuiCol_TableRowBg")
			style_rowBg.set_from_arg(sit->params[1]);
		if (sit->params.size() && sit->params[0] == "ImGuiCol_TableRowBgAlt")
			style_rowBgAlt.set_from_arg(sit->params[1]);
		if (sit->params.size() && sit->params[0] == "ImGuiCol_ChildBg")
			style_childBg.set_from_arg(sit->params[1]);
	}
	else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginTable")
	{
		header = false;
		columnData.clear();
		ctx.importLevel = sit->level;

		/*if (sit->params.size() >= 2) {
			std::istringstream is(sit->params[1]);
			size_t n = 3;
			is >> n;
		}*/

		if (sit->params.size() >= 3)
			flags.set_from_arg(sit->params[2]);

		if (sit->params.size() >= 4) {
			auto size = cpp::parse_size(sit->params[3]);
			size_x.set_from_arg(size.first);
			size_y.set_from_arg(size.second);
		}
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImRad::ScrollWhenDragging")
	{
		scrollWhenDragging = true;
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::TableSetupColumn")
	{
		ColumnData cd;
		cd.label = cpp::parse_str_arg(sit->params[0]);
		
		if (sit->params.size() >= 2)
			cd.flags.set_from_arg(sit->params[1]);
		
		if (sit->params.size() >= 3) {
			std::istringstream is(sit->params[2]);
			is >> cd.width;
		}
		
		columnData.push_back(std::move(cd));
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::TableHeadersRow")
	{
		header = true;
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::TableNextRow")
	{
		if (sit->params.size() >= 2)
			rowHeight.set_from_arg(sit->params[1]);
	}
	else if (sit->kind == cpp::ForBlock)
	{
		rowCount.set_from_arg(sit->line);
	}
	else if (sit->kind == cpp::CallExpr && sit->level == ctx.importLevel + 1 &&
		columnData.size() &&
		sit->callee.compare(0, 7, "ImGui::") &&
		sit->callee.compare(0, 7, "ImRad::"))
	{
		if (!onBeginRow.empty() || children.size()) //todo
			onEndRow.set_from_arg(sit->callee);
		else
			onBeginRow.set_from_arg(sit->callee);
	}
	else if (sit->kind == cpp::IfStmt && sit->level == ctx.importLevel + 1 &&
		columnData.size() &&
		!sit->cond.compare(0, 2, "!(") && sit->cond.back() == ')')
	{
		rowFilter.set_from_arg(sit->cond.substr(2, sit->cond.size() - 3));
	}
}

//---------------------------------------------------------

Child::Child(UIContext& ctx)
{
	//zero size will be rendered wrongly?
	//it seems 0 is equivalent to -1 but only if children exist which is confusing
	size_x = size_y = 20;

	flags.prefix("ImGuiChildFlags_");
	flags.add$(ImGuiChildFlags_Border);
	flags.add$(ImGuiChildFlags_AlwaysUseWindowPadding);
	flags.add$(ImGuiChildFlags_AlwaysAutoResize);
	flags.add$(ImGuiChildFlags_AutoResizeX);
	flags.add$(ImGuiChildFlags_AutoResizeY);
	flags.add$(ImGuiChildFlags_FrameStyle);
	flags.add$(ImGuiChildFlags_NavFlattened);

	wflags.prefix("ImGuiWindowFlags_");
	wflags.add$(ImGuiWindowFlags_AlwaysHorizontalScrollbar);
	wflags.add$(ImGuiWindowFlags_AlwaysVerticalScrollbar);
	wflags.add$(ImGuiWindowFlags_NoBackground);
	wflags.add$(ImGuiWindowFlags_NoScrollbar);
	wflags.add$(ImGuiWindowFlags_NoSavedSettings);
	wflags.add$(ImGuiWindowFlags_NoNavInputs);
}

std::unique_ptr<Widget> Child::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<Child>(*this);
	//itemCount is shared
	sel->CloneChildrenFrom(*this, ctx);
	return sel;
}

int Child::Behavior() 
{
	int fl = Widget::Behavior() | SnapInterior;
	if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) || !(flags & ImGuiChildFlags_AutoResizeX))
		fl |= HasSizeX;
	if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) || !(flags & ImGuiChildFlags_AutoResizeY))
		fl |= HasSizeY;
	return fl;
}

ImDrawList* Child::DoDraw(UIContext& ctx)
{
	if (flags & ImGuiWindowFlags_AlwaysAutoResize) 
	{
		ctx.isAutoSize = true;
		ImRad::HashCombine(ctx.layoutHash, true);
	}

	if (style_padding.has_value())
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style_padding.eval_px(ctx));
	if (style_rounding.has_value())
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, style_rounding.eval_px(ctx));
	if (style_borderSize.has_value())
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, style_borderSize.eval_px(ctx));
	if (!style_bg.empty())
		ImGui::PushStyleColor(ImGuiCol_ChildBg, style_bg.eval(ctx));
	
	ImVec2 sz;
	sz.x = size_x.eval_px(ImGuiAxis_X, ctx);
	sz.y = size_y.eval_px(ImGuiAxis_Y, ctx);
	if (!sz.x && children.empty())
		sz.x = 30;
	if (!sz.y && children.empty())
		sz.y = 30;
	
	ImRad::IgnoreWindowPaddingData data;
	if (!style_outer_padding)
		ImRad::PushIgnoreWindowPadding(&sz, &data);
	
	//after calling BeginChild, win->ContentSize and scrollbars are updated and drawn
	//only way how to disable scrollbars when using !style_outer_padding is to use
	//ImGuiWindowFlags_NoScrollbar
	
	ImGui::SetNextWindowScroll({ 0, 0 }); //click on a child of child causes scrolling which doesn't go back
	ImGui::BeginChild("", sz, flags, wflags);

	if (style_spacing.has_value())
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style_spacing);
	
	if (columnCount.has_value() && columnCount.value() >= 2)
		ImGui::Columns(columnCount.value(), "columns", columnBorder);

	for (size_t i = 0; i < children.size(); ++i)
	{
		children[i]->Draw(ctx);
	}
		
	if (style_spacing.has_value())
		ImGui::PopStyleVar();
	
	ImGui::EndChild();

	if (!style_outer_padding) 
		ImRad::PopIgnoreWindowPadding(data);

	if (!style_bg.empty())
		ImGui::PopStyleColor();
	if (style_rounding.has_value())
		ImGui::PopStyleVar();
	if (style_padding.has_value())
		ImGui::PopStyleVar();
	if (style_borderSize.has_value())
		ImGui::PopStyleVar();
	
	return ImGui::GetWindowDrawList();
}

void Child::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
	cached_pos = ImGui::GetItemRectMin();
	cached_size = ImGui::GetItemRectSize();
}

void Child::DoExport(std::ostream& os, UIContext& ctx)
{
	std::string datavar, szvar;
	if (!style_outer_padding)
	{
		datavar = "_data" + std::to_string(ctx.varCounter);
		szvar = "_sz" + std::to_string(ctx.varCounter);
		os << ctx.ind << "ImVec2 " << szvar << "{ "
			<< size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", "
			<< size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]) << " };\n";
		os << ctx.ind << "ImRad::IgnoreWindowPaddingData " << datavar << ";\n";
		os << ctx.ind << "ImRad::PushIgnoreWindowPadding(&" << szvar << ", &" << datavar << ");\n";
	}

	if (style_padding.has_value())
		os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, " << style_padding.to_arg(ctx.unit) << ");\n";
	if (style_rounding.has_value())
		os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, " << style_rounding.to_arg(ctx.unit) << ");\n";
	if (style_borderSize.has_value())
		os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, " << style_borderSize.to_arg(ctx.unit) << ");\n";
	if (!style_bg.empty())
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_ChildBg, " << style_bg.to_arg() << ");\n";
	
	os << ctx.ind << "ImGui::BeginChild(\"child" << ctx.varCounter << "\", ";
	if (szvar != "")
		os << szvar << ", ";
	else {
		os << "{ " << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", "
			<< size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]) << " }, ";
	}
	os << flags.to_arg() << ", " << wflags.to_arg() << ");\n";

	os << ctx.ind << "{\n";
	ctx.ind_up();
	++ctx.varCounter;

	if (scrollWhenDragging)
		os << ctx.ind << "ImRad::ScrollWhenDragging(false);\n";
	if (style_spacing.has_value())
		os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, " << style_spacing.to_arg(ctx.unit) << ");\n";

	bool hasColumns = !columnCount.has_value() || columnCount.value() >= 2;
	if (hasColumns) {
		os << ctx.ind << "ImGui::Columns(" << columnCount.to_arg() << ", \"\", "
			<< columnBorder.to_arg() << ");\n";
		//for (size_t i = 0; i < columnsWidths.size(); ++i)
			//os << ctx.ind << "ImGui::SetColumnWidth(" << i << ", " << columnsWidths[i].c_str() << ");\n";
	}

	if (!itemCount.empty()) {
		os << ctx.ind << itemCount.to_arg(ctx.forVarName) << "\n" << ctx.ind << "{\n";
		ctx.ind_up();
	}
	
	os << ctx.ind << "/// @separator\n\n";

	for (auto& child : children)
		child->Export(os, ctx);

	os << ctx.ind << "/// @separator\n";

	if (!itemCount.empty()) {
		if (hasColumns)
			os << ctx.ind << "ImGui::NextColumn();\n";
		ctx.ind_down();
		os << ctx.ind << "}\n";
	}

	if (style_spacing.has_value())
		os << ctx.ind << "ImGui::PopStyleVar();\n";
	
	os << ctx.ind << "ImGui::EndChild();\n";
	ctx.ind_down();
	os << ctx.ind << "}\n";

	if (!style_bg.empty())
		os << ctx.ind << "ImGui::PopStyleColor();\n";
	if (style_rounding.has_value())
		os << ctx.ind << "ImGui::PopStyleVar();\n";
	if (style_padding.has_value())
		os << ctx.ind << "ImGui::PopStyleVar();\n";
	if (style_borderSize.has_value())
		os << ctx.ind << "ImGui::PopStyleVar();\n";

	if (!style_outer_padding)
		os << ctx.ind << "ImRad::PopIgnoreWindowPadding(" << datavar << ");\n";

}

void Child::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
	{
		if (sit->params.size() == 2 && sit->params[0] == "ImGuiCol_ChildBg")
			style_bg.set_from_arg(sit->params[1]);
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar")
	{
		if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_WindowPadding")
			style_padding.set_from_arg(sit->params[1]);
		else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_ItemSpacing")
			style_spacing.set_from_arg(sit->params[1]);
		else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_ChildRounding")
			style_rounding.set_from_arg(sit->params[1]);
		else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_ChildBorderSize")
			style_borderSize.set_from_arg(sit->params[1]);
	}
	else if (sit->kind == cpp::Other && 
		(!sit->line.compare(0, 10, "ImVec2 _sz") || !sit->line.compare(0, 9, "ImVec2 sz"))) //sz for compatibility
	{
		style_outer_padding = false;
		size_t i = sit->line.find('{');
		if (i != std::string::npos) {
			auto size = cpp::parse_size(sit->line.substr(i));
			size_x.set_from_arg(size.first);
			size_y.set_from_arg(size.second);
		}
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::BeginChild")
	{
		if (sit->params.size() >= 2) {
			auto size = cpp::parse_size(sit->params[1]);
			if (size.first != "" && size.second != "") {
				size_x.set_from_arg(size.first);
				size_y.set_from_arg(size.second);
			}
		}

		if (sit->params.size() >= 3)
			flags.set_from_arg(sit->params[2]);
		if (sit->params.size() >= 4)
			wflags.set_from_arg(sit->params[3]);
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::Columns")
	{
		if (sit->params.size())
			*columnCount.access() = sit->params[0];

		if (sit->params.size() >= 3)
			columnBorder = sit->params[2] == "true";
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImRad::ScrollWhenDragging")
	{
		scrollWhenDragging = true;
	}
	else if (sit->kind == cpp::ForBlock)
	{
		itemCount.set_from_arg(sit->line);
	}
}


std::vector<UINode::Prop>
Child::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "@style.color", &style_bg },
		{ "@style.border", &style_border },
		{ "@style.padding", &style_padding },
		{ "@style.spacing", &style_spacing },
		{ "@style.rounding", &style_rounding },
		{ "@style.borderSize", &style_borderSize },
		{ "@style.outer_padding", &style_outer_padding },
		{ "child.flags", &flags },
		{ "child.wflags", &wflags },
		{ "child.column_count", &columnCount },
		{ "child.column_border", &columnBorder },
		{ "child.item_count", &itemCount },
		{ "scrollWhenDragging", &scrollWhenDragging },
		{ "size_x", &size_x },
		{ "size_y", &size_y },
		});
	return props;
}

bool Child::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("color");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##color", &style_bg, ImGuiCol_ChildBg, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("color", &style_bg, ctx);
		break;
	case 1:
		ImGui::Text("border");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##border", &style_border, ImGuiCol_Border, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("border", &style_border, ctx);
		break;
	case 2:
		ImGui::Text("padding");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##padding", &style_padding, ctx);
		break;
	case 3:
		ImGui::Text("spacing");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##spacing", &style_spacing, ctx);
		break;
	case 4:
		ImGui::Text("rounding");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##rounding", &style_rounding, ctx);
		break;
	case 5:
		ImGui::Text("borderSize");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##borderSize", &style_borderSize, ctx);
		break;
	case 6:
		ImGui::Text("outerPadding");
		ImGui::TableNextColumn();
		changed = InputDirectVal("##outerPadding", &style_outer_padding, ctx);
		break;
	case 7:
		TreeNodeProp("flags", "...", [&] {
			ImGui::TableNextColumn();
			ImGui::Spacing();
			int ch = CheckBoxFlags(&flags);
			if (ch) {
				changed = true;
				//these flags are difficult to get right and there are asserts so fix it here
				if (ch == ImGuiChildFlags_AutoResizeX || ch == ImGuiChildFlags_AutoResizeY) {
					if (flags & (ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY))
						flags |= ImGuiChildFlags_AlwaysAutoResize;
					else
						flags &= ~ImGuiChildFlags_AlwaysAutoResize;
				}
				if (ch == ImGuiChildFlags_AlwaysAutoResize) {
					if (flags & ImGuiChildFlags_AlwaysAutoResize) {
						if (!(flags & (ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)))
							flags |= ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY;
					}
					else
						flags &= ~(ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
				}
				//
				if ((flags & ImGuiChildFlags_AlwaysAutoResize) && (flags & ImGuiChildFlags_AutoResizeX))
					size_x = 0;
				if ((flags & ImGuiChildFlags_AlwaysAutoResize) && (flags & ImGuiChildFlags_AutoResizeY))
					size_y = 0;
			}
			});
		break;
	case 8:
		TreeNodeProp("windowFlags", "...", [&] {
			ImGui::TableNextColumn();
			ImGui::Spacing();
			changed = CheckBoxFlags(&wflags);
			});
		break;
	case 9:
		ImGui::Text("columnCount");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##columnCount", &columnCount, 1, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("columnCount", &columnCount, ctx);
		break;
	case 10:
		ImGui::Text("columnBorder");
		ImGui::TableNextColumn();
		changed = InputDirectVal("##columnBorder", &columnBorder, ctx);
		break;
	case 11:
		changed = DataLoopProp("itemCount", &itemCount, ctx);
		break;
	case 12:
		ImGui::Text("scrollWhenDragging");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##swd", &scrollWhenDragging, ctx);
		break;
	case 13:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		ImGui::BeginDisabled((flags & ImGuiChildFlags_AlwaysAutoResize) && (flags & ImGuiChildFlags_AutoResizeX));
		changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_x", &size_x, ctx);
		ImGui::EndDisabled();
		break;
	case 14:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		ImGui::BeginDisabled((flags & ImGuiChildFlags_AlwaysAutoResize) && (flags & ImGuiChildFlags_AutoResizeY));
		changed = InputBindable("##size_y", &size_y, {}, InputBindable_StretchButton, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_y", &size_y, ctx);
		ImGui::EndDisabled();
		break;
	default:
		return Widget::PropertyUI(i - 15, ctx);
	}
	return changed;
}

//---------------------------------------------------------

Splitter::Splitter(UIContext& ctx)
{
	size_x = size_y = -1;

	if (ctx.createVars) 
		position.set_from_arg(ctx.codeGen->CreateVar("float", "100", CppGen::Var::Interface));
}

std::unique_ptr<Widget> Splitter::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<Splitter>(*this);
	if (!position.empty() && ctx.createVars) {
		sel->position.set_from_arg(ctx.codeGen->CreateVar("float", "100", CppGen::Var::Interface));
	}
	sel->CloneChildrenFrom(*this, ctx);
	return sel;
}

ImDrawList* Splitter::DoDraw(UIContext& ctx)
{
	ImVec2 size;
	size.x = size_x.eval_px(ImGuiAxis_X, ctx);
	size.y = size_y.eval_px(ImGuiAxis_Y, ctx);
	
	if (!style_bg.empty())
		ImGui::PushStyleColor(ImGuiCol_ChildBg, style_bg.eval(ctx));
	ImGui::BeginChild("splitter", size);
	
	ImGuiAxis axis = ImGuiAxis_X;
	float th = 0, pos = 0;
	if (children.size() == 2) {
		axis = children[1]->sameLine ? ImGuiAxis_X : ImGuiAxis_Y;
		pos = children[0]->cached_pos[axis] + children[0]->cached_size[axis];
		th = children[1]->cached_pos[axis] - children[0]->cached_pos[axis] - children[0]->cached_size[axis];
	}
	
	ImGui::PushStyleColor(ImGuiCol_Separator, 0x00000000);
	ImRad::Splitter(axis == ImGuiAxis_X, th, &pos, min_size1, min_size2);
	ImGui::PopStyleColor();

	for (size_t i = 0; i < children.size(); ++i)
		children[i]->Draw(ctx);
	
	ImGui::EndChild();
	if (!style_bg.empty())
		ImGui::PopStyleColor();

	return ImGui::GetWindowDrawList();
}

void Splitter::DoExport(std::ostream& os, UIContext& ctx)
{
	if (children.size() != 2)
		ctx.errors.push_back("Splitter: need exactly 2 children");
	if (position.empty())
		ctx.errors.push_back("Splitter: position is unassigned");

	if (!style_bg.empty())
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_ChildBg, " << style_bg.to_arg() << ");\n";
	
	os << ctx.ind << "ImGui::BeginChild(\"splitter" << ctx.varCounter << "\", { "
		<< size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", " 
		<< size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1])
		<< " });\n" << ctx.ind << "{\n";
	ctx.ind_up();
	++ctx.varCounter;
	
	os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Separator, 0x00000000);\n";
	os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, 0x00000000);\n";
	if (!style_active.empty())
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_SeparatorActive, " << style_active.to_arg() << ");\n";

	bool axisX = true;
	direct_val<dimension> th = ImGui::GetStyle().ItemSpacing.x;
	if (children.size() == 2) {
		axisX = children[1]->sameLine;
		th = children[1]->cached_pos[!axisX] - children[0]->cached_pos[!axisX] - children[0]->cached_size[!axisX];
	}

	os << ctx.ind << "ImRad::Splitter(" 
		<< std::boolalpha << axisX 
		<< ", " << th.to_arg(ctx.unit)
		<< ", &" << position.to_arg()
		<< ", " << min_size1.to_arg(ctx.unit)
		<< ", " << min_size2.to_arg(ctx.unit)
		<< ");\n";

	os << ctx.ind << "ImGui::PopStyleColor();\n";
	os << ctx.ind << "ImGui::PopStyleColor();\n";
	if (!style_active.empty())
		os << ctx.ind << "ImGui::PopStyleColor();\n";

	os << ctx.ind << "/// @separator\n\n";

	for (const auto& child : children)
		child->Export(os, ctx);

	os << ctx.ind << "/// @separator\n";
	os << ctx.ind << "ImGui::EndChild();\n";
	ctx.ind_down();
	os << ctx.ind << "}\n";

	if (!style_bg.empty())
		os << ctx.ind << "ImGui::PopStyleColor();\n";
}

void Splitter::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::BeginChild")
	{
		if (sit->params.size() >= 2) {
			auto sz = cpp::parse_size(sit->params[1]);
			size_x.set_from_arg(sz.first);
			size_y.set_from_arg(sz.second);
		}
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImRad::Splitter")
	{
		if (sit->params.size() >= 3 && !sit->params[2].compare(0, 1, "&"))
			position.set_from_arg(sit->params[2].substr(1));
		if (sit->params.size() >= 4)
			min_size1.set_from_arg(sit->params[3]);
		if (sit->params.size() >= 5)
			min_size2.set_from_arg(sit->params[4]);
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
	{
		if (sit->params.size() >= 2 && sit->params[0] == "ImGuiCol_ChildBg")
			style_bg.set_from_arg(sit->params[1]);
		if (sit->params.size() >= 2 && sit->params[0] == "ImGuiCol_SeparatorActive")
			style_active.set_from_arg(sit->params[1]);
	}
}


std::vector<UINode::Prop>
Splitter::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "@style.bg", &style_bg },
		{ "@style.active", &style_active },
		{ "splitter.position", &position },
		{ "splitter.min1", &min_size1 },
		{ "splitter.min2", &min_size2 },
		{ "size_x", &size_x },
		{ "size_y", &size_y },
		});
	return props;
}

bool Splitter::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("bg");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##bg", &style_bg, ImGuiCol_ChildBg, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("bg", &style_bg, ctx);
		break;
	case 1:
		ImGui::Text("active");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##active", &style_active, ImGuiCol_SeparatorActive, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("active", &style_active, ctx);
		break;
	case 2:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("position");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##position", &position, false, ctx);
		break;
	case 3:
		ImGui::Text("minSize1");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputFloat("##min_size1", min_size1.access());
		break;
	case 4:
		ImGui::Text("minSize2");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputFloat("##min_size2", min_size2.access());
		break;
	case 5:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_x", &size_x, ctx);
		break;
	case 6:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, {}, true, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("size_y", &size_y, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 7, ctx);
	}
	return changed;
}

//---------------------------------------------------------

CollapsingHeader::CollapsingHeader(UIContext& ctx)
{
}

std::unique_ptr<Widget> CollapsingHeader::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<CollapsingHeader>(*this);
	sel->CloneChildrenFrom(*this, ctx);
	return sel;
}

ImDrawList* CollapsingHeader::DoDraw(UIContext& ctx)
{
	//automatically expand to show selected child and collapse to save space
	//in the window and avoid potential scrolling
	if (ctx.selected.size() == 1)
	{
		ImGui::SetNextItemOpen((bool)FindChild(ctx.selected[0]));
	}
	if (ImGui::CollapsingHeader(DRAW_STR(label)))
	{
		for (size_t i = 0; i < children.size(); ++i)
		{
			children[i]->Draw(ctx);
		}
	}

	return ImGui::GetWindowDrawList();
}

void CollapsingHeader::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
	ImVec2 pad = ImGui::GetStyle().FramePadding;
	cached_pos.x -= pad.x;
	cached_size.x = ImGui::GetContentRegionAvail().x + 2 * pad.x;
	cached_size.y = ImGui::GetCursorPosY() - p1.y - pad.y;
}

void CollapsingHeader::DoExport(std::ostream& os, UIContext& ctx)
{
	os << ctx.ind << "ImGui::SetNextItemOpen(";
	if (open.has_value())
		os << open.to_arg() << ", ImGuiCond_Appearing";
	else
		os << open.to_arg();
	os << ");\n";

	os << ctx.ind << "if (ImGui::CollapsingHeader(" << label.to_arg() << "))\n"
		<< ctx.ind << "{\n";
	ctx.ind_up();

	os << ctx.ind << "/// @separator\n\n";

	for (auto& child : children)
		child->Export(os, ctx);

	os << ctx.ind << "/// @separator\n";

	ctx.ind_down();
	os << ctx.ind << "}\n";
}

void CollapsingHeader::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextItemOpen")
	{
		if (sit->params.size() >= 1)
			open.set_from_arg(sit->params[0]);
	}
	else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::CollapsingHeader")
	{
		if (sit->params.size() >= 1)
			label.set_from_arg(sit->params[0]);
	}
}

std::vector<UINode::Prop>
CollapsingHeader::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "label", &label, true },
		{ "open", &open }
		});
	return props;
}

bool CollapsingHeader::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##label", &label, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("label", &label, ctx);
		break;
	case 1:
		ImGui::Text("open");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##open", &open, true, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("open", &open, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 2, ctx);
	}
	return changed;
}

//---------------------------------------------------------

TreeNode::TreeNode(UIContext& ctx)
{
	flags.prefix("ImGuiTreeNodeFlags_");
	//flags.add$(ImGuiTreeNodeFlags_Framed);
	flags.add$(ImGuiTreeNodeFlags_FramePadding);
	flags.add$(ImGuiTreeNodeFlags_Leaf);
	flags.add$(ImGuiTreeNodeFlags_Bullet);
	flags.add$(ImGuiTreeNodeFlags_OpenOnArrow);
	flags.add$(ImGuiTreeNodeFlags_OpenOnDoubleClick);
	flags.add$(ImGuiTreeNodeFlags_SpanAvailWidth);
	flags.add$(ImGuiTreeNodeFlags_SpanFullWidth);
	flags.add$(ImGuiTreeNodeFlags_SpanTextWidth);
	flags.add$(ImGuiTreeNodeFlags_SpanAllColumns);
}

std::unique_ptr<Widget> TreeNode::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<TreeNode>(*this);
	sel->CloneChildrenFrom(*this, ctx);
	return sel;
}

ImDrawList* TreeNode::DoDraw(UIContext& ctx)
{
	if (ctx.selected.size() == 1)
	{
		//automatically expand to show selection and collapse to save space
		//in the window and avoid potential scrolling
		ImGui::SetNextItemOpen((bool)FindChild(ctx.selected[0]));
	}
	lastOpen = false;
	if (ImGui::TreeNodeEx(DRAW_STR(label), flags))
	{
		lastOpen = true;
		for (const auto& child : children)
			child->Draw(ctx);

		ImGui::TreePop();
	}

	return ImGui::GetWindowDrawList();
}

void TreeNode::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
	cached_size = ImGui::GetItemRectSize();
	ImVec2 sp = ImGui::GetStyle().ItemSpacing;
	
	if (flags & (ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth))
	{
		cached_size.x = ImGui::GetContentRegionAvail().x - p1.x + sp.x;
	}
	else
	{
		cached_size.x = ImGui::CalcTextSize(label.c_str(), 0, true).x + 4 * sp.x;
	}

	if (lastOpen)
	{
		for (const auto& child : children)
		{
			auto p2 = child->cached_pos + child->cached_size;
			cached_size.x = std::max(cached_size.x, p2.x - cached_pos.x);
			cached_size.y = std::max(cached_size.y, p2.y - cached_pos.y);
		}
	}
}

void TreeNode::DoExport(std::ostream& os, UIContext& ctx)
{
	if (open.has_value())
		os << ctx.ind << "ImGui::SetNextItemOpen(" << open.to_arg() << ", ImGuiCond_Appearing);\n";
	else if (!open.empty())
		os << ctx.ind << "ImGui::SetNextItemOpen(" << open.to_arg() << ");\n";

	os << ctx.ind << "if (ImGui::TreeNodeEx(" << label.to_arg() << ", " << flags.to_arg() << "))\n";
	os << ctx.ind << "{\n";

	ctx.ind_up();
	os << ctx.ind << "/// @separator\n\n";

	for (const auto& child : children)
	{
		child->Export(os, ctx);
	}

	os << ctx.ind << "/// @separator\n";
	os << ctx.ind << "ImGui::TreePop();\n";
	ctx.ind_down();
	os << ctx.ind << "}\n";
}

void TreeNode::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::TreeNodeEx")
	{
		if (sit->params.size() >= 1)
			label.set_from_arg(sit->params[0]);

		if (sit->params.size() >= 2)
			flags.set_from_arg(sit->params[1]);
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextItemOpen")
	{
		if (sit->params.size())
			open.set_from_arg(sit->params[0]);
	}
}

std::vector<UINode::Prop>
TreeNode::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "flags", &flags },
		{ "label", &label, true },
		{ "open", &open },
		});
	return props;
}

bool TreeNode::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		TreeNodeProp("flags", "...", [&]{
			ImGui::TableNextColumn();
			ImGui::Spacing();
			changed = CheckBoxFlags(&flags);
			});
		break;
	case 1:
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##label", &label, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("label", &label, ctx);
		break;
	case 2:
		ImGui::Text("open");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##open", &open, true, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("open", &open, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 3, ctx);
	}
	return changed;
}

//--------------------------------------------------------------

TabBar::TabBar(UIContext& ctx)
{
	flags.prefix("ImGuiTabBarFlags_");
	flags.add$(ImGuiTabBarFlags_Reorderable);
	flags.add$(ImGuiTabBarFlags_TabListPopupButton);
	flags.add$(ImGuiTabBarFlags_NoTabListScrollingButtons);
	flags.add$(ImGuiTabBarFlags_DrawSelectedOverline);
	flags.add$(ImGuiTabBarFlags_FittingPolicyResizeDown);
	flags.add$(ImGuiTabBarFlags_FittingPolicyScroll);

	if (ctx.createVars)
		children.push_back(std::make_unique<TabItem>(ctx));
}

std::unique_ptr<Widget> TabBar::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<TabBar>(*this);
	//tabCount can be shared
	if (!activeTab.empty() && ctx.createVars) {
		sel->activeTab.set_from_arg(ctx.codeGen->CreateVar("int", "", CppGen::Var::Interface));
	}
	sel->CloneChildrenFrom(*this, ctx);
	return sel;
}

ImDrawList* TabBar::DoDraw(UIContext& ctx)
{
	std::string id = "##" + std::to_string((uintptr_t)this);
	//add tab names in order so that when tab order changes in designer we get a 
	//different id which will force ImGui to recalculate tab positions and
	//render tabs correctly
	for (const auto& child : children)
		id += std::to_string(uintptr_t(child.get()) & 0xffff);

	if (ImGui::BeginTabBar(id.c_str(), flags))
	{
		for (const auto& child : children)
			child->Draw(ctx);
			
		ImGui::EndTabBar();
	}

	return ImGui::GetWindowDrawList();
}

void TabBar::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
	ImVec2 pad = ImGui::GetStyle().FramePadding;
	cached_pos.x -= pad.x;
	cached_size.x = ImGui::GetContentRegionAvail().x + 2 * pad.x;
	cached_size.y = ImGui::GetCursorPosY() - p1.y - pad.y;
}

void TabBar::DoExport(std::ostream& os, UIContext& ctx)
{
	os << ctx.ind << "if (ImGui::BeginTabBar(\"tabBar" << ctx.varCounter << "\", "
		<< flags.to_arg() << "))\n";
	os << ctx.ind << "{\n";
	++ctx.varCounter;

	ctx.ind_up();
	if (!tabCount.empty())
	{
		os << ctx.ind << tabCount.to_arg(ctx.forVarName) << "\n";
		os << ctx.ind << "{\n";
		ctx.ind_up();
		//BeginTabBar does this
		//os << ctx.ind << "ImGui::PushID(" << tabCount.index_name_or(CppGen::FOR_VAR) << ");\n";
	}
	os << ctx.ind << "/// @separator\n\n";
	
	for (const auto& child : children)
		child->Export(os, ctx);

	os << ctx.ind << "/// @separator\n";
	if (!tabCount.empty())
	{
		//os << ctx.ind << "ImGui::PopID();\n"; EndTabBar does this
		ctx.ind_down();
		os << ctx.ind << "}\n";
	}
	
	os << ctx.ind << "ImGui::EndTabBar();\n";
	ctx.ind_down();
	os << ctx.ind << "}\n";
}

void TabBar::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginTabBar")
	{
		ctx.importLevel = sit->level;

		if (sit->params.size() >= 2)
			flags.set_from_arg(sit->params[1]);
	}
	else if (sit->kind == cpp::ForBlock && sit->level == ctx.importLevel + 1)
	{
		tabCount.set_from_arg(sit->line);
	}
}

std::vector<UINode::Prop>
TabBar::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "flags", &flags },
		{ "tabCount", &tabCount },
		{ "activeTab", &activeTab },
		});
	return props;
}

bool TabBar::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		TreeNodeProp("flags", "...", [&] {
			ImGui::TableNextColumn();
			ImGui::Spacing();
			changed = CheckBoxFlags(&flags);
			});
		break;
	case 1:
		changed = DataLoopProp("tabCount", &tabCount, ctx);
		break;
	case 2:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("activeTab");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##activeTab", &activeTab, true, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 3, ctx);
	}
	return changed;
}

//---------------------------------------------------------

TabItem::TabItem(UIContext& ctx)
{
}

std::unique_ptr<Widget> TabItem::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<TabItem>(*this);
	sel->CloneChildrenFrom(*this, ctx);
	return sel;
}

ImDrawList* TabItem::DoDraw(UIContext& ctx)
{
	bool sel = ctx.selected.size() == 1 && FindChild(ctx.selected[0]);
	bool tmp = true;
	if (ImGui::BeginTabItem(DRAW_STR(label), closeButton ? &tmp : nullptr, sel ? ImGuiTabItemFlags_SetSelected : 0))
	{
		for (const auto& child : children)
			child->Draw(ctx);

		ImGui::EndTabItem();
	}

	return ImGui::GetWindowDrawList();
}

void TabItem::DrawExtra(UIContext& ctx)
{
	if (ctx.parents.empty())
		return;

	bool tmp = ImGui::GetCurrentContext()->NavDisableMouseHover;
	ImGui::GetCurrentContext()->NavDisableMouseHover = false;
	ImGui::PushFont(nullptr);
	assert(ctx.parents.back() == this);
	auto* parent = ctx.parents[ctx.parents.size() - 2];
	size_t idx = stx::find_if(parent->children, [this](const auto& ch) { return ch.get() == this; })
		- parent->children.begin();

	ImGui::SetNextWindowPos(cached_pos, 0, { 0, 1.f });
	ImGui::Begin("extra", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);

	ImGui::BeginDisabled(!idx);
	if (ImGui::Button(ICON_FA_ANGLE_LEFT)) {
		auto ptr = std::move(parent->children[idx]);
		parent->children.erase(parent->children.begin() + idx);
		parent->children.insert(parent->children.begin() + idx - 1, std::move(ptr));
	}
	ImGui::EndDisabled();

	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_FOLDER_PLUS)) {
		parent->children.insert(parent->children.begin() + idx + 1, std::make_unique<TabItem>(ctx));
		if (ctx.selected.size() == 1 && ctx.selected[0] == this)
			ctx.selected[0] = (parent->children.begin() + idx + 1)->get();
	}

	ImGui::SameLine();
	ImGui::BeginDisabled(idx + 1 == parent->children.size());
	if (ImGui::Button(ICON_FA_ANGLE_RIGHT)) {
		auto ptr = std::move(parent->children[idx]);
		parent->children.erase(parent->children.begin() + idx);
		parent->children.insert(parent->children.begin() + idx + 1, std::move(ptr));
	}
	ImGui::EndDisabled();

	ImGui::End();
	ImGui::GetCurrentContext()->NavDisableMouseHover = tmp;
	ImGui::PopFont();
}

void TabItem::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
	const ImGuiTabBar* tabBar = ImGui::GetCurrentTabBar();
	cached_pos = tabBar->BarRect.GetTL();
	cached_size.x = tabBar->BarRect.GetWidth();
	cached_size.y = tabBar->BarRect.GetHeight();
	int idx = tabBar->LastTabItemIdx;
	if (idx >= 0) {
		const ImGuiTabItem& tab = tabBar->Tabs[idx];
		cached_pos.x += tab.Offset;
		cached_size.x = tab.Width;
	}
}

void TabItem::DoExport(std::ostream& os, UIContext& ctx)
{
	std::string var = "_open" + std::to_string(ctx.varCounter);
	if (closeButton) {
		os << ctx.ind << "bool " << var << " = true;\n";
		++ctx.varCounter;
	}

	os << ctx.ind << "if (ImGui::BeginTabItem(" << label.to_arg() << ", ";
	if (closeButton)
		os << "&" << var << ", ";
	else
		os << "nullptr, ";
	std::string idx;
	assert(ctx.parents.back() == this);
	const auto* tb = dynamic_cast<TabBar*>(ctx.parents[ctx.parents.size() - 2]);
	if (tb && !tb->activeTab.empty())
	{
		if (!tb->tabCount.empty())
		{
			idx = tb->tabCount.index_name_or(CppGen::FOR_VAR);
		}
		else
		{
			size_t n = stx::find_if(tb->children, [this](const auto& ch) {
				return ch.get() == this; }) - tb->children.begin();
			idx = std::to_string(n);
		}
		os << tb->activeTab.to_arg() << " == " << idx << " ? ImGuiTabItemFlags_SetSelected : 0";
	}
	else
	{
		os << "ImGuiTabItemFlags_None";
	}
	os << "))\n";
	os << ctx.ind << "{\n";

	ctx.ind_up();
	os << ctx.ind << "/// @separator\n\n";

	for (const auto& child : children)
	{
		child->Export(os, ctx);
	}

	os << ctx.ind << "/// @separator\n";
	os << ctx.ind << "ImGui::EndTabItem();\n";
	ctx.ind_down();
	os << ctx.ind << "}\n";

	if (closeButton && !onClose.empty())
	{
		os << ctx.ind << "if (!" << var << ")\n";
		ctx.ind_up();
		/*if (idx != "") no need to activate, user can check tabCount.index 
			os << ctx.ind << tb->activeTab.to_arg() << " = " << idx << ";\n";*/
		os << ctx.ind << onClose.to_arg() << "();\n";
		ctx.ind_down();
	}
	/*if (idx != "") user can add IsItemActivated event on his own and there is tabCount.index
	{
		os << ctx.ind << "if (ImGui::IsItemActivated())\n";
		ctx.ind_up();
		os << ctx.ind << tb->activeTab.to_arg() << " = " << idx << ";\n";
		ctx.ind_down();
	}*/
}

void TabItem::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginTabItem")
	{
		if (sit->params.size() >= 1)
			label.set_from_arg(sit->params[0]);

		if (sit->params.size() >= 2 && !sit->params[1].compare(0, 1, "&"))
			closeButton = true;

		assert(ctx.parents.back() == this);
		auto* tb = dynamic_cast<TabBar*>(ctx.parents[ctx.parents.size() - 2]);
		if (tb && sit->params.size() >= 3 && sit->params[2].size() > 32)
		{
			const auto& p = sit->params[2];
			size_t i = p.find("==");
			if (i != std::string::npos && p.substr(p.size() - 32, 32) == "?ImGuiTabItemFlags_SetSelected:0")
			{
				std::string var = p.substr(0, i); // i + 2, p.size() - 32 - i - 2);
				tb->activeTab.set_from_arg(var);
			}
		}
	}
	else if (sit->kind == cpp::IfBlock && !sit->cond.compare(0, 5, "!tmpOpen"))
	{
		ctx.importLevel = sit->level;
	}
	else if (sit->kind == cpp::CallExpr && sit->level == ctx.importLevel)
	{
		onClose.set_from_arg(sit->callee);
	}
}

std::vector<UINode::Prop>
TabItem::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "label", &label, true },
		{ "closeButton", &closeButton }
		});
	return props;
}

bool TabItem::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##label", &label, ctx);
		ImGui::SameLine(0, 0);
		changed |= BindingButton("label", &label, ctx);
		break;
	case 1:
		ImGui::Text("closeButton");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::Checkbox("##cb", closeButton.access());
		break;
	default:
		return Widget::PropertyUI(i - 2, ctx);
	}
	return changed;
}

std::vector<UINode::Prop>
TabItem::Events()
{
	auto props = Widget::Events();
	props.insert(props.begin(), {
		{ "onClose", &onClose },
		});
	return props;
}

bool TabItem::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::BeginDisabled(!closeButton);
		ImGui::Text("OnClose");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##onClose", &onClose, ctx);
		ImGui::EndDisabled();
		break;
	default:
		return Widget::EventUI(i - 1, ctx);
	}
	return changed;
}

//---------------------------------------------------------

MenuBar::MenuBar(UIContext& ctx)
{
	if (ctx.createVars)
		children.push_back(std::make_unique<MenuIt>(ctx));
}

std::unique_ptr<Widget> MenuBar::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<MenuBar>(*this);
	sel->CloneChildrenFrom(*this, ctx);
	return sel;
}

ImDrawList* MenuBar::DoDraw(UIContext& ctx)
{
	if (ImGui::BeginMenuBar())
	{
		//for (const auto& child : children) defend against insertion within the loop
		for (size_t i = 0; i < children.size(); ++i)
			children[i]->Draw(ctx);

		ImGui::EndMenuBar();
	}

	//menuBar is drawn from Begin anyways
	return ImGui::GetWindowDrawList();
}

void MenuBar::CalcSizeEx(ImVec2 x1, UIContext& ctx)
{
	cached_pos = ImGui::GetCurrentWindow()->MenuBarRect().GetTL();
	cached_size = ImGui::GetCurrentWindow()->MenuBarRect().GetSize();
}

void MenuBar::DoExport(std::ostream& os, UIContext& ctx)
{
	os << ctx.ind << "if (ImGui::BeginMenuBar())\n";
	os << ctx.ind << "{\n";
	ctx.ind_up();
	
	for (const auto& child : children) {
		MenuIt* it = dynamic_cast<MenuIt*>(child.get());
		if (it) it->ExportAllShortcuts(os, ctx);
	}

	os << ctx.ind << "/// @separator\n\n";

	for (const auto& child : children)
		child->Export(os, ctx);

	os << ctx.ind << "/// @separator\n";
	os << ctx.ind << "ImGui::EndMenuBar();\n";
	ctx.ind_down();
	os << ctx.ind << "}\n";
}

void MenuBar::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	auto* win = dynamic_cast<TopWindow*>(ctx.root);
	if (win)
		win->flags |= ImGuiWindowFlags_MenuBar;
}

//---------------------------------------------------------

MenuIt::MenuIt(UIContext& ctx)
{
	shortcut.set_global(true);
}

std::unique_ptr<Widget> MenuIt::Clone(UIContext& ctx)
{
	auto sel = std::make_unique<MenuIt>(*this);
	if (!checked.empty() && ctx.createVars) {
		sel->checked.set_from_arg(ctx.codeGen->CreateVar("bool", "false", CppGen::Var::Interface));
	}
	sel->CloneChildrenFrom(*this, ctx);
	return sel;
}

ImDrawList* MenuIt::DoDraw(UIContext& ctx)
{
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.f);
	
	if (separator)
		ImGui::Separator();
	
	if (contextMenu)
	{
		ctx.contextMenus.push_back(label);
		bool open = ctx.selected.size() == 1 && FindChild(ctx.selected[0]);
		if (open)
		{
			ImGui::SetNextWindowPos(cached_pos);
			if (style_padding.has_value())
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style_padding.eval_px(ctx));
			if (style_spacing.has_value())
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style_spacing.eval_px(ctx));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 
				style_rounding.empty() ? ImGui::GetStyle().PopupRounding : style_rounding.eval_px(ctx));

			std::string id = label + "##" + std::to_string((uintptr_t)this);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
			bool sel = stx::count(ctx.selected, this);
			if (sel)
				ImGui::PushStyleColor(ImGuiCol_Border, ctx.colors[UIContext::Selected]);
			ImGui::Begin(id.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings);
			{
				if (sel)
					ImGui::PopStyleColor(); 
				ctx.activePopups.push_back(ImGui::GetCurrentWindow());
				//for (const auto& child : children) defend against insertions within the loop
				for (size_t i = 0; i < children.size(); ++i)
					children[i]->Draw(ctx);

				ImGui::End();
			}
			ImGui::PopStyleVar();

			ImGui::PopStyleVar();
			if (style_spacing.has_value())
				ImGui::PopStyleVar();
			if (style_padding.has_value())
				ImGui::PopStyleVar();
		}
	}
	else if (children.size()) //normal menu
	{
		assert(ctx.parents.back() == this);
		const UINode* par = ctx.parents[ctx.parents.size() - 2];
		bool mbm = dynamic_cast<const MenuBar*>(par); //menu bar item

		ImGui::MenuItem(label.c_str());

		if (!mbm)
		{
			float w = ImGui::CalcTextSize(ICON_FA_ANGLE_RIGHT).x;
			ImGui::SameLine(0, 0);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() - w);
			ImGui::PushFont(nullptr);
			ImGui::Text(ICON_FA_ANGLE_RIGHT);
			ImGui::PopFont();
		}
		bool open = ctx.selected.size() == 1 && FindChild(ctx.selected[0]);
		if (open)
		{
			ImVec2 pos = cached_pos;
			if (mbm)
				pos.y += ctx.rootWin->MenuBarHeight;
			else {
				ImVec2 sp = ImGui::GetStyle().ItemSpacing;
				ImVec2 pad = ImGui::GetStyle().WindowPadding;
				pos.x += ImGui::GetWindowSize().x - sp.x;
				pos.y -= pad.y;
			}
			ImGui::SetNextWindowPos(pos);
			std::string id = label + "##" + std::to_string((uintptr_t)this);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
			ImGui::Begin(id.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings);
			{
				ctx.activePopups.push_back(ImGui::GetCurrentWindow());
				//for (const auto& child : children) defend against insertions within the loop
				for (size_t i = 0; i < children.size(); ++i)
					children[i]->Draw(ctx);
				ImGui::End();
			}
			ImGui::PopStyleVar();
		}
	}
	else if (ownerDraw)
	{
		std::string s = onChange.to_arg();
		if (s.empty())
			s = "OnDraw not set";
		else
			s += "()";
		ImGui::MenuItem(s.c_str(), nullptr, nullptr, false);
	}
	else //menuItem
	{
		bool check = !checked.empty();
		ImGui::MenuItem(DRAW_STR(label), shortcut.c_str(), check);
	}
	ImGui::PopStyleVar();
	return ImGui::GetWindowDrawList();
}

void MenuIt::DrawExtra(UIContext& ctx)
{
	if (ctx.parents.empty())
		return;
	assert(ctx.parents.back() == this);
	auto* parent = ctx.parents[ctx.parents.size() - 2];
	if (dynamic_cast<TopWindow*>(parent)) //no helper for context menu
		return;
	bool vertical = !dynamic_cast<MenuBar*>(parent);
	size_t idx = stx::find_if(parent->children, [this](const auto& ch) { return ch.get() == this; })
		- parent->children.begin();

	//draw toolbox
	//currently we always sit it on top of the menu so that it doesn't overlap with submenus
	//no WindowFlags_StayOnTop
	bool tmp = ImGui::GetCurrentContext()->NavDisableMouseHover;
	ImGui::GetCurrentContext()->NavDisableMouseHover = false;
	ImGui::PushFont(nullptr);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.f);

	ImVec2 sp = ImGui::GetStyle().ItemSpacing;
	const ImVec2 bsize{ 30, 30 };
	ImVec2 pos = cached_pos;
	if (vertical) {
		pos = ImGui::GetWindowPos();
		//pos.x -= sp.x;
	}
	ImGui::SetNextWindowPos(pos, 0, vertical ? ImVec2{ 0, 1.f } : ImVec2{ 0, 1.f });
	ImGui::Begin("extra", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);

	ImGui::BeginDisabled(!idx);
	if (ImGui::Button(vertical ? ICON_FA_ANGLE_UP : ICON_FA_ANGLE_LEFT, bsize)) {
		auto ptr = std::move(parent->children[idx]);
		parent->children.erase(parent->children.begin() + idx);
		parent->children.insert(parent->children.begin() + idx - 1, std::move(ptr));
	}
	ImGui::EndDisabled();

	ImGui::SameLine();
	if (ImGui::Button(vertical ? ICON_FA_PLUS ICON_FA_ANGLE_DOWN : ICON_FA_PLUS ICON_FA_ANGLE_RIGHT, bsize)) {
		parent->children.insert(parent->children.begin() + idx + 1, std::make_unique<MenuIt>(ctx));
		if (ctx.selected.size() == 1 && ctx.selected[0] == this)
			ctx.selected[0] = parent->children[idx + 1].get();
	}

	ImGui::SameLine();
	ImGui::BeginDisabled(children.size());
	if (ImGui::Button(vertical ? ICON_FA_PLUS ICON_FA_ANGLE_RIGHT : ICON_FA_PLUS ICON_FA_ANGLE_DOWN, bsize)) {
		children.push_back(std::make_unique<MenuIt>(ctx));
		if (ctx.selected.size() == 1 && ctx.selected[0] == this)
			ctx.selected[0] = children[0].get();
	}
	ImGui::EndDisabled();
	
	ImGui::SameLine();
	ImGui::BeginDisabled(idx + 1 == parent->children.size());
	if (ImGui::Button(vertical ? ICON_FA_ANGLE_DOWN : ICON_FA_ANGLE_RIGHT, bsize)) {
		auto ptr = std::move(parent->children[idx]);
		parent->children.erase(parent->children.begin() + idx);
		parent->children.insert(parent->children.begin() + idx + 1, std::move(ptr));
	}
	ImGui::EndDisabled();

	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopFont();
	ImGui::GetCurrentContext()->NavDisableMouseHover = tmp;
}

void MenuIt::CalcSizeEx(ImVec2 x1, UIContext& ctx)
{
	assert(ctx.parents.back() == this);
	const UINode* par = ctx.parents[ctx.parents.size() - 2];
	bool mbm = dynamic_cast<const MenuBar*>(par);
	
	ImVec2 sp = ImGui::GetStyle().ItemSpacing;
	const ImGuiMenuColumns* mc = &ImGui::GetCurrentWindow()->DC.MenuColumns;
	cached_pos.x += mc->OffsetLabel;
	cached_size = ImGui::CalcTextSize(label.c_str(), nullptr, true);
	cached_size.x += sp.x;
	if (contextMenu)
	{
		cached_pos = ctx.rootWin->Pos;
	}
	else if (mbm)
	{
		++cached_pos.y;
		cached_size.y = ctx.rootWin->MenuBarHeight - 2;
	}
	else
	{
		if (separator)
			cached_pos.y += sp.y;
	}
}

void MenuIt::DoExport(std::ostream& os, UIContext& ctx)
{
	assert(ctx.parents.back() == this);
	const UINode* par = ctx.parents[ctx.parents.size() - 2];
	
	if (separator)
		os << ctx.ind << "ImGui::Separator();\n";

	if (contextMenu)
	{
		ExportAllShortcuts(os, ctx);

		if (style_padding.has_value())
			os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, " << style_padding.to_arg(ctx.unit) << ");\n";
		if (style_rounding.has_value())
			os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, " << style_rounding.to_arg(ctx.unit) << ");\n";
		if (style_spacing.has_value())
			os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, " << style_spacing.to_arg(ctx.unit) << ");\n";
		
		os << ctx.ind << "if (ImGui::BeginPopup(" << label.to_arg() << ", "
			<< "ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings"
			<< "))\n";
		os << ctx.ind << "{\n";
		ctx.ind_up();

		os << ctx.ind << "/// @separator\n\n";

		for (const auto& child : children)
			child->Export(os, ctx);

		os << ctx.ind << "/// @separator\n";
		os << ctx.ind << "ImGui::EndPopup();\n";
		ctx.ind_down();
		os << ctx.ind << "}\n";

		if (style_spacing.has_value())
			os << ctx.ind << "ImGui::PopStyleVar();\n";
		if (style_rounding.has_value())
			os << ctx.ind << "ImGui::PopStyleVar();\n";
		if (style_padding.has_value())
			os << ctx.ind << "ImGui::PopStyleVar();\n";
	}
	else if (children.size())
	{
		os << ctx.ind << "if (ImGui::BeginMenu(" << label.to_arg() << "))\n";
		os << ctx.ind << "{\n";
		ctx.ind_up();
		os << ctx.ind << "/// @separator\n\n";

		for (const auto& child : children)
			child->Export(os, ctx);

		os << ctx.ind << "/// @separator\n";
		os << ctx.ind << "ImGui::EndMenu();\n";
		ctx.ind_down();
		os << ctx.ind << "}\n";
	}
	else if (ownerDraw)
	{
		if (onChange.empty())
			ctx.errors.push_back("MenuIt: ownerDraw is set but onChange is empty!");
		os << ctx.ind << onChange.to_arg() << "();\n";
	}
	else
	{
		bool ifstmt = !onChange.empty();
		os << ctx.ind;
		if (ifstmt)
			os << "if (";

		os << "ImGui::MenuItem(" << label.to_arg() << ", \""
			<< shortcut.c_str() << "\", ";
		if (checked.empty())
			os << "false";
		else
			os << "&" << checked.to_arg();
		os << ")";
		//ImGui::Shortcut is exported in ExportShortcut pass

		if (ifstmt) {
			os << ")\n";
			ctx.ind_up();
			os << ctx.ind << onChange.to_arg() << "();\n";
			ctx.ind_down();
		}
		else
			os << ";\n";
	}
}

//As of now ImGui still doesn't process shortcuts in unopened menus so
//separate Shortcut pass is needed
void MenuIt::ExportShortcut(std::ostream& os, UIContext& ctx)
{
	if (shortcut.empty() || ownerDraw)
		return;

	os << ctx.ind << "if (";
	if (!disabled.has_value() || disabled.value())
		os << "!(" << disabled.to_arg() << ") && ";
	//force RouteGlobal otherwise it won't get fired when menu popup is open
	os << "ImGui::Shortcut(" << shortcut.to_arg() << "))\n";
	ctx.ind_up();
	if (!onChange.empty())
		os << ctx.ind << onChange.to_arg() << "();\n";
	else
		os << ctx.ind << ";\n";
	ctx.ind_down();
}

void MenuIt::ExportAllShortcuts(std::ostream& os, UIContext& ctx)
{
	ExportShortcut(os, ctx);
	
	for (const auto& child : children) 
	{
		auto* it = dynamic_cast<MenuIt*>(child.get());
		if (it)	it->ExportAllShortcuts(os, ctx);
	}
}

void MenuIt::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if ((sit->kind == cpp::CallExpr && sit->callee == "ImGui::MenuItem") ||
		(sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::MenuItem"))
	{
		if (sit->params.size())
			label.set_from_arg(sit->params[0]);
		if (sit->params.size() >= 2 && cpp::is_cstr(sit->params[1]))
			*shortcut.access() = sit->params[1].substr(1, sit->params[1].size() - 2);
		if (sit->params.size() >= 3 && !sit->params[2].compare(0, 1, "&"))
			checked.set_from_arg(sit->params[2].substr(1));

		if (sit->kind == cpp::IfCallThenCall)
			onChange.set_from_arg(sit->callee2);
	}
	else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginPopup")
	{
		contextMenu = true;
		if (sit->params.size())
			label.set_from_arg(sit->params[0]);
	}
	else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginMenu")
	{
		if (sit->params.size())
			label.set_from_arg(sit->params[0]);
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::Separator")
	{
		separator = true;
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar")
	{
		if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_WindowPadding")
			style_padding.set_from_arg(sit->params[1]);
		else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_ItemSpacing")
			style_spacing.set_from_arg(sit->params[1]);
		else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_PopupRounding")
			style_rounding.set_from_arg(sit->params[1]);
	}
	else if (sit->kind == cpp::CallExpr && sit->callee.compare(0, 7, "ImGui::"))
	{
		ownerDraw = true;
		onChange.set_from_arg(sit->callee);
	}
}

std::vector<UINode::Prop>
MenuIt::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "@style.padding", &style_padding },
		{ "@style.spacing", &style_spacing },
		{ "@style.rounding", &style_rounding },
		{ "ownerDraw", &ownerDraw },
		{ "label", &label, true },
		{ "shortcut", &shortcut },
		{ "checked", &checked },
		{ "separator", &separator },
		});
	return props;
}

bool MenuIt::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::BeginDisabled(!contextMenu);
		ImGui::Text("padding");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##padding", &style_padding, ctx);
		ImGui::EndDisabled();
		break;
	case 1:
		ImGui::BeginDisabled(!contextMenu);
		ImGui::Text("spacing");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##spacing", &style_spacing, ctx);
		ImGui::EndDisabled();
		break;
	case 2:
		ImGui::BeginDisabled(!contextMenu);
		ImGui::Text("rounding");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##rounding", &style_rounding, ctx);
		ImGui::EndDisabled();
		break;
	case 3:
		ImGui::BeginDisabled(contextMenu);
		ImGui::Text("ownerDraw");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::Checkbox("##ownerDraw", ownerDraw.access());
		ImGui::EndDisabled();
		break;
	case 4:
		ImGui::BeginDisabled(ownerDraw);
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##label", &label, ctx);
		ImGui::EndDisabled();
		break;
	case 5:
		ImGui::BeginDisabled(ownerDraw || contextMenu || children.size());
		ImGui::Text("shortcut");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputDirectVal("##shortcut", &shortcut, false, ctx);
		ImGui::EndDisabled();
		break;
	case 6:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::BeginDisabled(ownerDraw || contextMenu || children.size());
		ImGui::Text("checked");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##checked", &checked, true, ctx);
		ImGui::EndDisabled();
		break;
	case 7:
		ImGui::BeginDisabled(contextMenu);
		ImGui::Text("separator");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::Checkbox("##separator", separator.access());
		ImGui::EndDisabled();
		break;
	default:
		return Widget::PropertyUI(i - 8, ctx);
	}
	return changed;
}

std::vector<UINode::Prop>
MenuIt::Events()
{
	auto props = Widget::Events();
	props.insert(props.begin(), {
		{ "onChange", &onChange },
		});
	return props;
}

bool MenuIt::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::BeginDisabled(children.size());
		ImGui::Text(ownerDraw ? "OnDraw" : "OnChange");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##onChange", &onChange, ctx);
		ImGui::EndDisabled();
		break;
	default:
		return Widget::EventUI(i - 1, ctx);
	}
	return changed;
}
