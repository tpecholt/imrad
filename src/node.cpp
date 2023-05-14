#include "node.h"
#include "stx.h"
#include "imrad.h"
#include "cppgen.h"
#include "binding_input.h"
#include "ui_table_columns.h"
#include "ui_message_box.h"
#include "ui_combo_dlg.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <IconsFontAwesome6.h>
#include <algorithm>
#include <array>

const color32 SNAP_COLOR[] {
	IM_COL32(128, 128, 255, 255),
	IM_COL32(255, 0, 255, 255),
	IM_COL32(0, 255, 255, 255),
	IM_COL32(0, 255, 0, 255),
};

const color32 FIELD_NAME_CLR = IM_COL32(202, 202, 255, 255);

void toggle(std::vector<UINode*>& c, UINode* val)
{
	auto it = stx::find(c, val);
	if (it == c.end())
		c.push_back(val);
	else
		c.erase(it);
}

/*std::string Str(const ImVec4& c)
{
	std::ostringstream os;
	os << "IM_COL32(" << c.x << ", " << c.y << ", " << c.z << ", " << c.w << ")";
	return os.str();
}*/

void UIContext::ind_up()
{
	ind += codeGen->INDENT;
}

void UIContext::ind_down()
{
	if (ind.size() >= codeGen->INDENT.size())
		ind.resize(ind.size() - codeGen->INDENT.size());
}

//----------------------------------------------------

void UINode::DrawSnap(UIContext& ctx)
{
	ctx.snapSameLine = false;
	ctx.snapNextColumn = false;
	ctx.snapBeginGroup = false;
	ctx.snapSetNextSameLine = false;

	const float MARGIN = 7;
	assert(ctx.parents.back() == this);
	size_t level = ctx.parents.size() - 1;
	int snapOp = SnapBehavior();
	ImVec2 m = ImGui::GetMousePos();
	ImVec2 d1 = m - cached_pos;
	ImVec2 d2 = cached_pos + cached_size - m;
	float mind = std::min({ d1.x, d1.y, d2.x, d2.y });
	
	//snap interior
	if ((snapOp & SnapInterior) && mind >= 0 &&
		!stx::count_if(children, [](const auto& ch) { return ch->SnapBehavior() & SnapSides; }))
	{
		ctx.snapParent = this;
		ctx.snapIndex = children.size();
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRect(cached_pos, cached_pos + cached_size, SNAP_COLOR[level % std::size(SNAP_COLOR)], 0, 0, 3);
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
	if (clip->SnapBehavior() & SnapGrandparentClip)
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
		bool leftmost = !i || !pch->sameLine || pch->nextColumn || pch->beginGroup;
		if (!leftmost) //handled by right(i-1)
			return;
		bool inGroup = pch->cached_pos.x - parent->cached_pos.x > 100; //todo
		if ((pch->nextColumn || pch->beginGroup || inGroup) && 
			m.x < pch->cached_pos.x)
			return;
		ctx.snapParent = parent;
		ctx.snapIndex = i;
		ctx.snapSameLine = pchildren[i]->sameLine;
		ctx.snapNextColumn = pchildren[i]->nextColumn;
		ctx.snapBeginGroup = pchildren[i]->beginGroup;
		ctx.snapSetNextSameLine = true;
		break;
	}
	case ImGuiDir_Right:
	{
		p = cached_pos + ImVec2(cached_size.x, 0);
		h = cached_size.y;
		auto *nch = i + 1 < pchildren.size() ? pchildren[i + 1].get() : nullptr;
		if (nch && (nch->sameLine || nch->nextColumn))
		{
			if (m.x >= nch->cached_pos.x) //no margin, left(i+1) can differ
				return;
		}
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
		ctx.snapNextColumn = false;
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
			if (j && !pchildren[j]->sameLine && !pchildren[j]->nextColumn)
				topmost = false;
		}
		size_t i1 = i, i2 = i;
		for (int j = (int)i - 1; j >= 0; --j)
		{
			if (!pchildren[j + 1]->sameLine)
				break;
			if (pchildren[j + 1]->beginGroup)
				break;
			assert(!pchildren[j + 1]->nextColumn);
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
			if (!pchildren[j]->sameLine)
				break;
			assert(!pchildren[j]->nextColumn);
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
			bool inGroup = nch && nch->cached_pos.x - parent->cached_pos.x > 100; //todo
			if (nch && !nch->nextColumn) // && !inGroup)
			{
				if (m.y >= nch->cached_pos.y + MARGIN)
					return;
				p.y = (p.y + nch->cached_pos.y) / 2;
			}
			ctx.snapParent = parent;
			ctx.snapIndex = i2 + 1;
			ctx.snapSameLine = pchildren[i1]->sameLine && pchildren[i1]->beginGroup;
			ctx.snapNextColumn = false;
		}
		else
		{
			if (!topmost) //up(i) is handled by down(i-1)
				return;
			ctx.snapParent = parent;
			ctx.snapIndex = i1;
			ctx.snapSameLine = pchildren[i1]->sameLine && pchildren[i1]->beginGroup;
			ctx.snapNextColumn = pchildren[i1]->nextColumn;
			ctx.snapBeginGroup = pchildren[i1]->beginGroup;
		}
		break;
	}
	default:
		return;
	}

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddLine(p, p + ImVec2(w, h), SNAP_COLOR[(level - 1) % std::size(SNAP_COLOR)], 3);
}

bool UINode::FindChild(const UINode* ch)
{
	if (ch == this)
		return true;
	for (const auto& child : children)
		if (child->FindChild(ch))
			return true;
	return false;
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
{
	flags.prefix("ImGuiWindowFlags_");
	flags.add$(ImGuiWindowFlags_AlwaysAutoResize);
	flags.add$(ImGuiWindowFlags_AlwaysHorizontalScrollbar);
	flags.add$(ImGuiWindowFlags_AlwaysVerticalScrollbar);
	flags.add$(ImGuiWindowFlags_MenuBar);
	flags.add$(ImGuiWindowFlags_NoCollapse);
	flags.add$(ImGuiWindowFlags_NoDocking);
	flags.add$(ImGuiWindowFlags_NoResize);
	flags.add$(ImGuiWindowFlags_NoTitleBar);
}

void TopWindow::Draw(UIContext& ctx)
{
	ctx.groupLevel = 0;
	ctx.root = this;
	ctx.popupWins.clear();
	ctx.parents = { this };
	ctx.selUpdated = false;
	ctx.hovered = nullptr;
	ctx.snapParent = nullptr;
	ctx.modalPopup = modalPopup;
	ctx.table = false;

	std::string cap = title.value();
	if (cap.empty())
		cap = "error";
	cap += "###TopWindow"; //don't clash 
	int fl = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings;
	fl |= flags;

	if (stylePading)
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, *stylePading);
	if (styleSpacing)
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, *styleSpacing);
	
	ImGui::SetNextWindowPos(ctx.wpos); // , ImGuiCond_Always, { 0.5, 0.5 });
	
	if (!(fl & ImGuiWindowFlags_AlwaysAutoResize))
		ImGui::SetNextWindowSize({ size_x, size_y });

	bool tmp;
	ImGui::Begin(cap.c_str(), &tmp, fl);

	ctx.rootWin = ImGui::FindWindowByName(cap.c_str());
	assert(ctx.rootWin);
	
	ImGui::GetCurrentContext()->NavDisableMouseHover = true;
	for (size_t i = 0; i < children.size(); ++i)
	{
		children[i]->Draw(ctx);
	}
	ImGui::GetCurrentContext()->NavDisableMouseHover = false;

	//use all client area to allow snapping close to the border
	auto pad = ImGui::GetStyle().WindowPadding - ImVec2(3+1, 3);
	auto mi = ImGui::GetWindowContentRegionMin() - pad;
	auto ma = ImGui::GetWindowContentRegionMax() + pad;
	cached_pos = ImGui::GetWindowPos() + mi;
	cached_size = ma - mi;

	if (!ctx.snapMode && !ctx.selUpdated &&
		ImGui::IsWindowHovered() &&
		ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
			; //don't participate in group selection
		else
			ctx.selected = { this };
		ImGui::SetKeyboardFocusHere(); //for DEL hotkey reaction
	}
	if (ctx.snapMode && !ctx.snapParent && ImGui::IsWindowHovered())
	{
		DrawSnap(ctx);
	}
	
	/*if (ctx.selected == this)
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRect(cached_pos, cached_pos + cached_size, IM_COL32(255, 0, 0, 255));
	}*/

	while (ctx.groupLevel) { //fix missing endGroup
		ImGui::EndGroup();
		--ctx.groupLevel;
	}

	ImGui::End();
	
	if (styleSpacing)
		ImGui::PopStyleVar();
	if (stylePading)
		ImGui::PopStyleVar();
}

void TopWindow::Export(std::ostream& os, UIContext& ctx)
{
	ctx.groupLevel = 0;
	ctx.modalPopup = modalPopup;
	ctx.errors.clear();
	
	std::string tit = title.to_arg();
	if (tit.empty())
		ctx.errors.push_back("TopWindow: title can't be empty");

	os << ctx.ind << "/// @begin TopWindow\n";
	
	if (stylePading)
	{
		os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { "
			<< stylePading->x << ", " << stylePading->y << " });\n";
	}
	if (styleSpacing)
	{
		os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { "
			<< styleSpacing->x << ", " << styleSpacing->y << " });\n";
	}

	if ((flags & ImGuiWindowFlags_AlwaysAutoResize) == 0)
	{
		os << ctx.ind << "ImGui::SetNextWindowSize({ " << size_x << ", " << size_y << " }"
			<< ", ImGuiCond_Appearing);\n";
	}

	if (modalPopup)
	{
		os << ctx.ind << "if (requestOpen) {\n";
		ctx.ind_up();
		os << ctx.ind << "requestOpen = false;\n";
		os << ctx.ind << "ImGui::OpenPopup(" << tit << ");\n";
		ctx.ind_down();
		os << ctx.ind << "}\n";
		os << ctx.ind << "ImVec2 center = ImGui::GetMainViewport()->GetCenter();\n";
		os << ctx.ind << "ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, { 0.5f, 0.5f });\n";
		os << ctx.ind << "bool tmpOpen = true;\n";
		os << ctx.ind << "if (ImGui::BeginPopupModal(" << tit << ", &tmpOpen, " << flags.to_arg() << "))\n";
		os << ctx.ind << "{\n";
		ctx.ind_up();
		os << ctx.ind << "if (requestClose)\n";
		ctx.ind_up();
		os << ctx.ind << "ImGui::CloseCurrentPopup();\n";
		ctx.ind_down();
	}
	else
	{
		os << ctx.ind << "if (isOpen && ImGui::Begin(" << tit << ", &isOpen, " << flags.to_arg() << "))\n";
		os << ctx.ind << "{\n";
		ctx.ind_up();
	}
	os << ctx.ind << "/// @separator\n\n";
		
	for (const auto& ch : children)
		ch->Export(os, ctx);

	if (ctx.groupLevel)
		ctx.errors.push_back("missing EndGroup");

	os << ctx.ind << "/// @separator\n";
	
	if (modalPopup)
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

	if (styleSpacing)
		os << ctx.ind << "ImGui::PopStyleVar();\n";
	if (stylePading)
		os << ctx.ind << "ImGui::PopStyleVar();\n";

	os << ctx.ind << "/// @end TopWindow\n";
}

void TopWindow::Import(cpp::stmt_iterator& sit, UIContext& ctx)
{
	ctx.errors.clear();
	ctx.importState = 1;
	ctx.userCode = "";
	ctx.root = this;
	ctx.parents = { this };

	while (sit != cpp::stmt_iterator())
	{
		if (sit->kind == cpp::Comment && !sit->line.compare(0, 11, "/// @begin "))
		{
			ctx.importState = 1;
			sit.enable_parsing(true);
			auto node = Widget::Create(sit->line.substr(11), ctx);
			if (node) {
				node->Import(++sit, ctx);
				children.push_back(std::move(node));
				ctx.importState = 2;
				ctx.userCode = "";
				sit.enable_parsing(false);
			}
		}
		else if (sit->kind == cpp::Comment && !sit->line.compare(0, 14, "/// @separator"))
		{
			if (ctx.importState == 1) {
				ctx.importState = 2;
				ctx.userCode = "";
				sit.enable_parsing(false);
			}
			else {
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
		else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar")
		{
			if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_WindowPadding")
				stylePading = cpp::parse_fsize(sit->params[1]);
			else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_ItemSpacing")
				styleSpacing = cpp::parse_fsize(sit->params[1]);
		}
		if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextWindowSize")
		{
			if (sit->params.size()) {
				auto size = cpp::parse_fsize(sit->params[0]);
				size_x = size.x;
				size_y = size.y;
			}
		}
		else if ((sit->kind == cpp::IfCallBlock || sit->kind == cpp::CallExpr) &&
			sit->callee == "ImGui::BeginPopupModal")
		{
			modalPopup = true;
			title.set_from_arg(sit->params[0]);

			if (sit->params.size() >= 3)
				flags.set_from_arg(sit->params[2]);
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::Begin")
		{
			modalPopup = false;
			title.set_from_arg(sit->params[0]);

			if (sit->params.size() >= 3)
				flags.set_from_arg(sit->params[2]);
		}
		++sit;
	}

	ctx.importState = 0;
}

void TopWindow::TreeUI(UIContext& ctx)
{
	ctx.parents = { this };
	ImGui::SetNextItemOpen(true, ImGuiCond_Always);
	std::string str = ctx.codeGen->GetName();
	bool selected = stx::count(ctx.selected, this);
	if (selected)
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
	if (ImGui::TreeNodeEx(str.c_str(), 0))
	{
		if (selected)
			ImGui::PopStyleColor();
		if (ImGui::IsItemClicked())
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
	else if (selected)
		ImGui::PopStyleColor();
}

std::vector<UINode::Prop>
TopWindow::Properties()
{
	return {
		{ "top.flags", nullptr },
		{ "title", &title, true },
		{ "top.modalPopup", nullptr },
		{ "size_x", nullptr },
		{ "size_y", nullptr },
		{ "top.style_padding", nullptr },
		{ "top.style_spacing", nullptr },
	};
}

bool TopWindow::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Unindent();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, 0.0f });
		if (ImGui::TreeNode("flags")) {
			ImGui::TableNextColumn();
			changed = CheckBoxFlags(&flags);
			bool hasMB = children.size() && dynamic_cast<MenuBar*>(children[0].get());
			bool flagsMB = flags & ImGuiWindowFlags_MenuBar;
			if (flagsMB && !hasMB)
				children.insert(children.begin(), std::make_unique<MenuBar>(ctx));
			else if (!flagsMB && hasMB)
				children.erase(children.begin());
			ImGui::TreePop();
		}
		ImGui::Spacing();
		ImGui::PopStyleVar();
		ImGui::Indent();
		break;
	case 1:
		ImGui::Text("title");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##title", &title, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("title", &title, ctx);
		break;
	case 2:
		ImGui::Text("modalPopup");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::Checkbox("##modal", &modalPopup);
		break;
	case 3:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputFloat("##size_x", &size_x);
		break;
	case 4:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputFloat("##size_y", &size_y);
		break;
	case 5:
	{
		ImGui::Text("style_padding");
		ImGui::TableNextColumn();
		int v[2] = { -1, -1 }; 
		if (stylePading) {
			v[0] = (int)stylePading->x;
			v[1] = (int)stylePading->y;
		}
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (ImGui::InputInt2("##style_padding", v))
		{
			changed = true;
			if (v[0] >= 0 || v[1] >= 0)
				stylePading = { (float)v[0], (float)v[1] };
			else
				stylePading = {};
		}
		break;
	}
	case 6:
	{
		ImGui::Text("style_spacing");
		ImGui::TableNextColumn();
		int v[2] = { -1, -1 };
		if (styleSpacing) {
			v[0] = (int)styleSpacing->x;
			v[1] = (int)styleSpacing->y;
		}
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (ImGui::InputInt2("##style_spacing", v))
		{
			changed = true;
			if (v[0] >= 0 || v[1] >= 0)
				styleSpacing = { (float)v[0], (float)v[1] };
			else
				styleSpacing = {};
		}
		break;
	}
	default:
		return false;
	}
	return changed;
}

std::vector<UINode::Prop>
TopWindow::Events()
{
	return {};
}

bool TopWindow::EventUI(int i, UIContext& ctx)
{
	return false;
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
	else if (name == "ColorEdit")
		return std::make_unique<ColorEdit>(ctx);
	else if (name == "Image")
		return std::make_unique<Image>(ctx);
	else if (name == "Separator")
		return std::make_unique<Separator>(ctx);
	else if (name == "UserWidget")
		return std::make_unique<UserWidget>(ctx);
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
	else
		return {};
}

void Widget::Draw(UIContext& ctx)
{
	if (nextColumn) {
		if (ctx.table)
			ImGui::TableNextColumn();
		else
			ImGui::NextColumn();
	}
	if (sameLine) {
		ImGui::SameLine(0, spacing * ImGui::GetStyle().ItemSpacing.x);
	} 
	else {
		for (int i = 0; i < spacing; ++i)
			ImGui::Spacing();
	}
	if (indent)
		ImGui::Indent(indent * ImGui::GetStyle().IndentSpacing / 2);
	if (beginGroup) {
		ImGui::BeginGroup();
		++ctx.groupLevel;
	}
	
	ImGui::PushID(this);
	ctx.parents.push_back(this);
	auto lastHovered = ctx.hovered;
	cached_pos = ImGui::GetCursorScreenPos();
	auto p1 = ImGui::GetCursorPos();
	ImGui::BeginDisabled((disabled.has_value() && disabled.value()) || (visible.has_value() && !visible.value()));
	DoDraw(ctx);
	ImGui::EndDisabled();
	CalcSizeEx(p1, ctx);
	
	//doesn't work for open CollapsingHeader etc:
	//bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
	bool allowed = ctx.popupWins.empty() || stx::count(ctx.popupWins, ImGui::GetCurrentWindow());
	bool hovered = ImGui::IsMouseHoveringRect(cached_pos, cached_pos + cached_size);
	if (!ctx.snapMode && allowed && 
		ctx.hovered == lastHovered && hovered)
	{
		ctx.hovered = this;
	}
	if (!ctx.snapMode && allowed && 
		!ctx.selUpdated && hovered && 
		ImGui::IsMouseClicked(0)) //this works even for non-items like TabControl etc.  
	{
		ctx.selUpdated = true;
		if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
			toggle(ctx.selected, this);
		else
			ctx.selected = { this };
		ImGui::SetKeyboardFocusHere(); //for DEL hotkey reaction
	}
	bool selected = stx::count(ctx.selected, this);
	if (selected || ctx.hovered == this)
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRect(cached_pos, cached_pos + cached_size, 
			selected ? 0xff0000ff : 0x8000ffff,
			0, 0, selected ? 2.f : 1.f);
	}
	if (selected)
	{
		DrawExtra(ctx);
	}

	if (ctx.snapMode && !ctx.snapParent)
	{
		DrawSnap(ctx);
	}
	
	ctx.parents.pop_back();
	ImGui::PopID();
	
	if (endGroup && ctx.groupLevel) {
		ImGui::EndGroup();
		--ctx.groupLevel;
	}
}

void Widget::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
	cached_size = ImGui::GetItemRectSize();
}

void Widget::Export(std::ostream& os, UIContext& ctx)
{
	if (userCode != "")
		os << userCode << "\n";
	
	std::string stype = typeid(*this).name();
	auto i = stype.find(' ');
	if (i != std::string::npos)
		stype.erase(0, i + 1);
	auto it = stx::find_if(stype, [](char c) { return isalpha(c);});
	if (it != stype.end())
		stype.erase(0, it - stype.begin());
	os << ctx.ind << "/// @begin " << stype << "\n";

	if (nextColumn)
	{
		if (ctx.table)
			os << ctx.ind << "ImGui::TableNextColumn();\n";
		else
			os << ctx.ind << "ImGui::NextColumn();\n";
	}
	if (sameLine)
	{
		os << ctx.ind << "ImGui::SameLine(";
		if (spacing)
			os << "0, " << spacing << " * ImGui::GetStyle().ItemSpacing.x";
		os << ");\n";
	}
	else
	{
		for (int i = 0; i < spacing; ++i)
			os << ctx.ind << "ImGui::Spacing();\n";
	}
	if (indent)
	{
		os << ctx.ind << "ImGui::Indent(" << indent << " * ImGui::GetStyle().IndentSpacing / 2);\n";
	}
	if (beginGroup)
	{
		os << ctx.ind << "ImGui::BeginGroup();\n";
		++ctx.groupLevel;
	}
	if (!visible.has_value() || !visible.value())
	{
		os << ctx.ind << "if (" << visible.c_str() << ")\n" << ctx.ind << "{\n"; 
		ctx.ind_up();
	}
	if (!disabled.has_value() || disabled.value())
	{
		os << ctx.ind << "ImGui::BeginDisabled(" << disabled.c_str() << ");\n";
	}

	ctx.parents.push_back(this);
	DoExport(os, ctx);
	ctx.parents.pop_back();

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
	if (!disabled.has_value() || disabled.value())
	{
		os << ctx.ind << "ImGui::EndDisabled();\n";
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
		os << ctx.ind << "if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())\n";
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
	if (endGroup && ctx.groupLevel) 
	{
		os << ctx.ind << "ImGui::EndGroup();\n";
		--ctx.groupLevel;
	}

	os << ctx.ind << "/// @end " << stype << "\n\n";
}

void Widget::Import(cpp::stmt_iterator& sit, UIContext& ctx)
{
	ctx.importState = 1;
	ctx.parents.push_back(this);
	userCode = ctx.userCode;

	while (sit != cpp::stmt_iterator())
	{
		if (sit->kind == cpp::Comment && !sit->line.compare(0, 11, "/// @begin "))
		{
			ctx.importState = 1;
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
			w->Import(++sit, ctx);
			children.push_back(std::move(w));
			ctx.importState = 2;
			ctx.userCode = "";
		}
		else if (sit->kind == cpp::Comment && !sit->line.compare(0, 9, "/// @end "))
		{
			break;
		}
		else if (sit->kind == cpp::Comment && !sit->line.compare(0, 14, "/// @separator"))
		{
			if (ctx.importState == 1) {
				ctx.importState = 2;
				ctx.userCode = "";
			}
			else
				ctx.importState = 1;
		}
		else if (ctx.importState == 2)
		{
			if (ctx.userCode != "")
				ctx.userCode += "\n";
			ctx.userCode += sit->line;
		}
		else if (sit->kind == cpp::IfBlock) //todo: weak condition
		{
			visible.set_from_arg(sit->cond);
		}
		else if (sit->kind == cpp::CallExpr && 
			(sit->callee == "ImGui::NextColumn" || sit->callee == "ImGui::TableNextColumn"))
		{
			nextColumn = true;
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SameLine")
		{
			sameLine = true;
			if (sit->params.size() == 2)
				spacing.set_from_arg(sit->params[1]);
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::Spacing")
		{
			++spacing;
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::Indent")
		{
			if (sit->params.size())
				indent.set_from_arg(sit->params[0]);
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::BeginGroup")
		{
			beginGroup = true;
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::EndGroup")
		{
			endGroup = true;
		}
		else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::BeginDisabled")
		{
			if (sit->params.size())
				disabled.set_from_arg(sit->params[0]);
		}
		else if (sit->kind == cpp::IfCallThenCall && sit->cond == "ImGui::IsItemHovered")
		{
			if (sit->callee == "ImGui::SetTooltip")
				tooltip.set_from_arg(sit->params2[0]);
			else if (sit->callee == "ImGui::SetMouseCursor")
				cursor.set_from_arg(sit->params2[0]);
			else
				onItemHovered.set_from_arg(sit->callee);
		}
		else if (sit->kind == cpp::IfCallThenCall && sit->cond == "ImGui::IsItemClicked")
		{
			onItemClicked.set_from_arg(sit->callee);
		}
		else if (sit->kind == cpp::IfCallThenCall && sit->cond == "ImGui::IsMouseDoubleClicked")
		{
			onItemDoubleClicked.set_from_arg(sit->callee);
		}
		else if (sit->kind == cpp::IfCallThenCall && sit->cond == "ImGui::IsItemFocused")
		{
			onItemFocused.set_from_arg(sit->callee);
		}
		else if (sit->kind == cpp::IfCallThenCall && sit->cond == "ImGui::IsItemActivated")
		{
			onItemActivated.set_from_arg(sit->callee);
		}
		else if (sit->kind == cpp::IfCallThenCall && sit->cond == "ImGui::IsItemDeactivated")
		{
			onItemDeactivated.set_from_arg(sit->callee);
		}
		else if (sit->kind == cpp::IfCallThenCall && sit->cond == "ImGui::IsItemDeactivatedAfterEdit")
		{
			onItemDeactivatedAfterEdit.set_from_arg(sit->callee);
		}
		else
		{
			DoImport(sit, ctx);
		}
		++sit;
	}
}

std::vector<UINode::Prop>
Widget::Properties()
{
	std::vector<UINode::Prop> props{
		{ "visible", &visible },
		{ "cursor", &cursor },
		{ "tooltip", &tooltip },
		{ "disabled", &disabled },
	};
	if (SnapBehavior() & SnapSides)
		props.insert(props.end(), {
			{ "indent", &indent },
			{ "spacing", &spacing },
			{ "sameLine", &sameLine },
			{ "beginGroup", &beginGroup },
			{ "endGroup", &endGroup },
			{ "nextColumn", &nextColumn },
			});
	return props;
}

bool Widget::PropertyUI(int i, UIContext& ctx)
{
	int sat = (i & 1) ? 202 : 164;
	if (i <= 3)
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(sat, 255, sat, 255));
	else
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(255, 255, sat, 255));
	
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("visible");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##visible", &visible, true, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("visible", &visible, ctx);
		break;
	case 1:
		ImGui::Text("cursor");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::Combo("##cursor", cursor.access(), "Arrow\0TextInput\0ResizeAll\0ResizeNS\0ResizeEW\0ResizeNESW\0ResizeNWSE\0Hand\0NotAllowed\0\0");
		break;
	case 2:
		ImGui::Text("tooltip");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##tooltip", &tooltip, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("tooltip", &tooltip, ctx);
		break;
	case 3:
		ImGui::Text("disabled");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##disabled", &disabled, false, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("disabled", &disabled, ctx);
		break;
	case 4:
		ImGui::BeginDisabled(sameLine);
		ImGui::Text("indent");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputInt("##indent", indent.access());
		if (ImGui::IsItemDeactivatedAfterEdit() && indent < 0)
		{
			changed = true;
			indent = 0;
		}
		ImGui::EndDisabled();
		break;
	case 5:
		ImGui::Text("spacing");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputInt("##spacing", spacing.access());
		if (ImGui::IsItemDeactivatedAfterEdit() && spacing < 0)
		{
			changed = true;
			spacing = 0;
		}
		break;
	case 6:
		ImGui::BeginDisabled(nextColumn);
		ImGui::Text("sameLine");
		ImGui::TableNextColumn();
		if (ImGui::Checkbox("##sameLine", sameLine.access())) {
			changed = true;
			if (sameLine)
				indent = 0;
		}
		ImGui::EndDisabled();
		break;
	case 7:
		ImGui::Text("beginGroup");
		ImGui::TableNextColumn();
		changed = ImGui::Checkbox("##beginGroup", beginGroup.access());
		break;
	case 8:
		ImGui::Text("endGroup");
		ImGui::TableNextColumn();
		changed = ImGui::Checkbox("##endGroup", endGroup.access());
		break;
	case 9:
		ImGui::Text("nextColumn");
		ImGui::TableNextColumn();
		if (ImGui::Checkbox("##nextColumn", nextColumn.access())) {
			changed = true;
			if (nextColumn)
				sameLine = false;
		}
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
		ImGui::Text("IsItemHovered");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##IsItemHovered", &onItemHovered, ctx);
		break;
	case 1:
		ImGui::Text("IsItemClicked");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##itemclicked", &onItemClicked, ctx);
		break;
	case 2:
		ImGui::Text("IsItemDoubleClicked");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##itemdblclicked", &onItemDoubleClicked, ctx);
		break;
	case 3:
		ImGui::Text("IsItemFocused");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##IsItemFocused", &onItemFocused, ctx);
		break;
	case 4:
		ImGui::Text("IsItemActivated");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##IsItemActivated", &onItemActivated, ctx);
		break;
	case 5:
		ImGui::Text("IsItemDeactivated");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##IsItemDeactivated", &onItemDeactivated, ctx);
		break;
	case 6:
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
			label = p.property->c_str();
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
	std::string icon;
	if (GetIcon()) {
		icon = GetIcon();
	}
	std::string suff;
	if (sameLine)
		suff += "L";
	if (beginGroup)
		suff += "B";
	if (endGroup)
		suff += "E";
	if (nextColumn)
		suff += "C";

	bool selected = stx::count(ctx.selected, this);
	if (icon != "")
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
	else if (selected)
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
	else
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Text]);

	ImGui::SetNextItemOpen(true, ImGuiCond_Always);
	int flags = ImGuiTreeNodeFlags_SpanAvailWidth;
	if (children.empty())
		flags |= ImGuiTreeNodeFlags_Leaf;
	if (ImGui::TreeNodeEx((icon != "" ? icon.c_str() : label.c_str()), flags))
	{
		if (ImGui::IsItemClicked())
		{
			if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
				toggle(ctx.selected, this);
			else
				ctx.selected = { this };
		}
		if (icon != "") 
		{
			ImGui::PopStyleColor();
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[selected ? ImGuiCol_ButtonHovered : ImGuiCol_Text]);
			ImGui::SameLine();
			ImGui::Text(label.c_str());
		}
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::TextDisabled(suff.c_str());
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

Separator::Separator(UIContext&)
{}

void Separator::DoDraw(UIContext& ctx)
{
	ImGui::SeparatorEx(sameLine ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal);
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
		cached_pos.x -= ImGui::GetStyle().ItemSpacing.x;
		cached_pos.y -= sp;
		cached_size.y += 2 * sp;
	}
}

void Separator::DoExport(std::ostream& os, UIContext& ctx)
{
	os << ctx.ind << "ImGui::SeparatorEx("
		<< (sameLine ? "ImGuiSeparatorFlags_Vertical" : "ImGuiSeparatorFlags_Horizontal")
		<< ");\n";
}

void Separator::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
}

bool Separator::PropertyUI(int i, UIContext& ctx)
{
	return Widget::PropertyUI(i, ctx);
}

//----------------------------------------------------

Text::Text(UIContext& ctx)
{
}

void Text::DoDraw(UIContext& ctx)
{
	//ImGui::GetIO().Fonts->AddFontFromFileTTF("font.ttf", size_pixels);
	//PushFont
	//PopFont

	std::optional<color32> clr;
	if (grayed)
		clr = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
	else if (color.has_value())
		clr = color.value();
	if (clr)
		ImGui::PushStyleColor(ImGuiCol_Text, *clr);
	
	if (alignToFrame)
		ImGui::AlignTextToFramePadding();

	if (wrap) 
	{
		//float x1 = ImGui::GetCursorPosX();
		//float w = wrap_x.value();
		//if (w < 0) w += ImGui::GetContentRegionAvail().x;
		//ImGui::PushTextWrapPos(x1 + w);
		ImGui::PushTextWrapPos(0);
		ImGui::TextUnformatted(text.c_str());
		ImGui::PopTextWrapPos();
		//ImGui::SameLine(x1 + w);
		//ImGui::NewLine();
	}
	else 
	{
		ImGui::TextUnformatted(text.c_str());
	}

	if (clr)
		ImGui::PopStyleColor();
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
	std::string clr;
	if (grayed)
		clr = "ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]";
	else if (!color.empty())
		clr = color.to_arg();

	if (clr != "")
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Text, " << clr << ");\n";

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

	if (clr != "")
		os << ctx.ind << "ImGui::PopStyleColor();\n";
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
		if (sit->params.size() >= 1)
			text = cpp::parse_str_arg(sit->params[0]);
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
	{
		if (sit->params.size() >= 2 && sit->params[0] == "ImGuiCol_Text")
		{
			if (sit->params[1] == "ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]")
				grayed = true;
			else
				color.set_from_arg(sit->params[1]);
		}
	}
}

std::vector<UINode::Prop>
Text::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "text", &text, true },
		{ "text.grayed", &grayed },
		{ "color", &color },
		{ "text.alignToFramePadding", &alignToFrame },
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
		ImGui::Text("text");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##text", &text, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("text", &text, ctx);
		break;
	case 1:
		ImGui::Text("grayed");
		ImGui::TableNextColumn();
		changed = ImGui::Checkbox("##grayed", grayed.access());
		break;
	case 2:
	{
		ImGui::BeginDisabled(grayed);
		ImGui::Text("color");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##color", &color, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("color", &color, ctx);
		ImGui::EndDisabled();
		break;
	}
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

	horizAlignment.add("AlignLeft", ImRad::AlignLeft);
	horizAlignment.add("AlignHCenter", ImRad::AlignHCenter);
	horizAlignment.add("AlignRight", ImRad::AlignRight);
	vertAlignment.add("AlignTop", ImRad::AlignTop);
	vertAlignment.add("AlignVCenter", ImRad::AlignVCenter);
	vertAlignment.add("AlignBottom", ImRad::AlignBottom);
}

void Selectable::DoDraw(UIContext& ctx)
{
	std::optional<color32> clr;
	if (color.has_value())
		clr = color.value();
	if (clr)
		ImGui::PushStyleColor(ImGuiCol_Text, *clr);

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

	ImVec2 size(0, 0);
	if (size_x.has_value())
		size.x = size_x.value();
	if (size_y.has_value())
		size.y = size_y.value();
	ImGui::Selectable(label.c_str(), false, flags, size);

	ImGui::PopStyleVar();

	if (clr)
		ImGui::PopStyleColor();
}

void Selectable::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
	cached_size = ImGui::GetItemRectSize();
	cached_size.x -= ImGui::GetStyle().ItemSpacing.x;
}

void Selectable::DoExport(std::ostream& os, UIContext& ctx)
{
	std::string clr;
	/*if (grayed)
		clr = "ImGui::GetStyle().Colors[ImGuiCol_TextDisabled])";
	else*/ if (!color.empty())
		clr = color.to_arg();

	if (clr != "")
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Header, " << clr << ");\n";

	os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { "
		<< (horizAlignment == ImRad::AlignLeft ? "0" : horizAlignment == ImRad::AlignHCenter ? "0.5f" : "1.f")
		<< ", "
		<< (vertAlignment == ImRad::AlignTop ? "0" : vertAlignment == ImRad::AlignVCenter ? "0.5f" : "1.f")
		<< " });\n";

	os << ctx.ind;
	if (!onChange.empty())
		os << "if (";
	
	os << "ImGui::Selectable(" << label.to_arg() << ", ";
	if (fieldName.empty())
		os << "false";
	else
		os << "&" << fieldName.to_arg();
	
	os << ", " << flags.to_arg() << ", "
		<< "{ " << size_x.to_arg() << ", " << size_y.to_arg() << " })";
	
	if (!onChange.empty()) {
		os << ")\n";
		ctx.ind_up();
		os << ctx.ind << onChange.to_arg() << "();\n";
		ctx.ind_down();
	}
	else {
		os << ";\n";
	}

	os << ctx.ind << "ImGui::PopStyleVar();\n";

	if (clr != "")
		os << ctx.ind << "ImGui::PopStyleColor();\n";
}

void Selectable::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if ((sit->kind == cpp::CallExpr && sit->callee == "ImGui::Selectable") ||
		(sit->kind == cpp::IfCallThenCall && sit->cond == "ImGui::Selectable"))
	{
		if (sit->params.size() >= 1)
			label.set_from_arg(sit->params[0]);
		if (sit->params.size() >= 2 && !sit->params[1].compare(0, 1, "&"))
			fieldName.set_from_arg(sit->params[1].substr(1));
		if (sit->params.size() >= 3)
			flags.set_from_arg(sit->params[2]);
		if (sit->params.size() >= 4) {
			auto sz = cpp::parse_size(sit->params[3]);
			size_x.set_from_arg(sz.first);
			size_y.set_from_arg(sz.second);
		}

		if (sit->kind == cpp::IfCallThenCall)
			onChange.set_from_arg(sit->callee);
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
	{
		if (sit->params.size() >= 2 && sit->params[0] == "ImGuiCol_Header")
		{
			color.set_from_arg(sit->params[1]);
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
}

std::vector<UINode::Prop>
Selectable::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "selectable.flags", &flags },
		{ "label", &label, true },
		{ "color", &color },
		{ "horizAlignment", &horizAlignment },
		{ "vertAlignment", &vertAlignment },
		{ "selectable.fieldName", &fieldName },
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
		ImGui::Unindent();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, 0.0f });
		if (ImGui::TreeNode("flags")) {
			ImGui::TableNextColumn();
			changed = CheckBoxFlags(&flags);
			ImGui::TreePop();
		}
		ImGui::PopStyleVar();
		ImGui::Spacing();
		ImGui::Indent();
		break;
	case 1:
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##label", &label, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("label", &label, ctx);
		break;
	case 2:
		ImGui::Text("color");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##color", &color, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("color", &color, ctx);
		break;
	case 3:
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
	case 4:
		ImGui::BeginDisabled(size_y.has_value() && !size_y.value());
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
	case 5:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##fieldName", &fieldName, true, ctx);
		break;
	case 6:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_x", &size_x, ctx);
		break;
	case 7:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_y", &size_y, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 8, ctx);
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
		ImGui::Text("onChange");
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
	modalResult.add$(ImRad::All);
	
	arrowDir.add$(ImGuiDir_None);
	arrowDir.add$(ImGuiDir_Left);
	arrowDir.add$(ImGuiDir_Right);
	arrowDir.add$(ImGuiDir_Up);
	arrowDir.add$(ImGuiDir_Down);
}

void Button::DoDraw(UIContext& ctx)
{
	if (color.has_value())
		ImGui::PushStyleColor(ImGuiCol_Button, color.value());
	
	if (arrowDir != ImGuiDir_None)
		ImGui::ArrowButton("##", arrowDir);
	else if (small)
		ImGui::SmallButton(label.c_str());
	else
	{
		ImVec2 size{ 0, 0 };
		if (size_x.has_value())
			size.x = size_x.value();
		if (size_y.has_value())
			size.y = size_y.value();
		ImGui::Button(label.c_str(), size);

		//if (ctx.modalPopup && text.value() == "OK")
			//ImGui::SetItemDefaultFocus();
	}
	
	if (color.has_value())
		ImGui::PopStyleColor();
}

std::string CodeShortcut(const std::string& sh)
{
	if (sh.empty())
		return "";
	size_t i = -1;
	std::ostringstream os;
	int count = 0;
	while (true) 
	{
		++count;
		size_t j = sh.find_first_of("+-", i + 1);
		if (j == std::string::npos)
			j = sh.size();
		std::string key = sh.substr(i + 1, j - i - 1);
		if (key.empty() && j < sh.size()) //like in ctrl-+
			key = sh[j];
		if (key.empty())
			break;
		std::string lk = key;
		stx::for_each(lk, [](char& c) { c = std::tolower(c); });
		//todo
		if (lk == "ctrl")
			os << " && ImGui::GetIO().KeyCtrl";
		else if (lk == "alt")
			os << " && ImGui::GetIO().KeyAlt";
		else if (lk == "shift")
			os << " && ImGui::GetIO().KeyShift";
		else if (key == "+")
			os << " && ImGui::IsKeyPressed(ImGuiKey_Equal, false)";
		else if (key == "-")
			os << " && ImGui::IsKeyPressed(ImGuiKey_Minus, false)";
		else if (key == ".")
			os << " && ImGui::IsKeyPressed(ImGuiKey_Period, false)";
		else if (key == ",")
			os << " && ImGui::IsKeyPressed(ImGuiKey_Comma, false)";
		else if (key == "/")
			os << " && ImGui::IsKeyPressed(ImGuiKey_Slash, false)";
		else
			os << " && ImGui::IsKeyPressed(ImGuiKey_" << (char)std::toupper(key[0])
			<< key.substr(1) << ", false)";
		i = j;
		if (j == sh.size())
			break;
	}
	std::string code;
	if (count > 1)
		code += "(";
	code += os.str().substr(4);
	if (count > 1)
		code += ")";
	return code;
}

std::string ParseShortcut(const std::string& line)
{
	std::string sh;
	size_t i = -1;
	while (true)
	{
		size_t j1 = line.find("ImGui::IsKeyPressed(ImGuiKey_", i + 1);
		size_t j2 = line.find("ImGui::GetIO().Key", i + 1);
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
			j2 += 18;
			sh += "+";
			while (j2 < line.size() && std::isalpha(line[j2]))
				sh += line[j2++];
			i = j2;
		}
	}
	if (sh.size())
		sh.erase(sh.begin());
	return sh;
}

void Button::DoExport(std::ostream& os, UIContext& ctx)
{
	if (!color.empty())
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Button, " << color.to_arg() << ");\n";

	bool closePopup = ctx.modalPopup && modalResult != ImRad::None;
	os << ctx.ind;
	if (!onChange.empty() || closePopup)
		os << "if (";
	
	if (arrowDir != ImGuiDir_None)
	{
		os << "ImGui::ArrowButton(\"###arrow\", " << arrowDir.to_arg() << ")";
	}
	else if (small)
	{
		os << "ImGui::SmallButton(" << label.to_arg() << ")";
	}
	else
	{
		os << "ImGui::Button(" << label.to_arg() << ", "
			<< "{ " << size_x.to_arg() << ", " << size_y.to_arg() << " }"
			<< ")";
	}

	if (shortcut != "") {
		os << " ||\n";
		ctx.ind_up();
		os << ctx.ind << CodeShortcut(shortcut);
		ctx.ind_down();
	}

	if (!onChange.empty() || closePopup)
	{
		os << ")\n" << ctx.ind << "{\n";
		ctx.ind_up();
			
		if (!onChange.empty())
			os << ctx.ind << onChange.to_arg() << "();\n";
		if (closePopup) {
			os << ctx.ind << "ClosePopup();\n";
			if (modalResult != ImRad::Cancel)
				//no if => easier parsing
				os << ctx.ind << "callback(" << modalResult.to_arg() << ");\n";
		}

		ctx.ind_down();			
		os << ctx.ind << "}\n";
	}
	else
	{
		os << ";\n";
	}

	if (!color.empty())
		os << ctx.ind << "ImGui::PopStyleColor();\n";
}

void Button::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
	{
		if (sit->params.size() >= 2 && sit->params[0] == "ImGuiCol_Button")
			color.set_from_arg(sit->params[1]);
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

		shortcut = ParseShortcut(sit->line);
	}
	else if ((sit->kind == cpp::CallExpr || sit->kind == cpp::IfCallBlock) &&
		sit->callee == "ImGui::SmallButton")
	{
		ctx.importLevel = sit->level;
		label.set_from_arg(sit->params[0]);
	}
	if ((sit->kind == cpp::CallExpr || sit->kind == cpp::IfCallBlock) &&
		sit->callee == "ImGui::ArrowButton")
	{
		ctx.importLevel = sit->level;
		if (sit->params.size() >= 2)
			arrowDir.set_from_arg(sit->params[1]);
	}
	else if (sit->kind == cpp::CallExpr && sit->level == ctx.importLevel + 1) 
	{
		if (sit->callee == "ClosePopup") {
			if (modalResult == ImRad::None)
				modalResult = ImRad::Cancel;
		}
		else if (sit->callee == "callback" && sit->params.size())
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
		{ "button.arrowDir", &arrowDir },
		{ "label", &label, true },
		{ "button.modalResult", &modalResult },
		{ "shortcut", &shortcut },
		{ "color", &color },
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
	case 1:
		ImGui::BeginDisabled(arrowDir != ImGuiDir_None);
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##label", &label, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("label", &label, ctx);
		ImGui::EndDisabled();
		break;
	case 2:
	{
		ImGui::BeginDisabled(!ctx.modalPopup);
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
	case 3:
		ImGui::Text("shortcut");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputText("##shortcut", shortcut.access());
		break;
	case 4:
		ImGui::Text("color");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##color", &color, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("color", &color, ctx);
		break;
	case 5:
		ImGui::Text("small");
		ImGui::TableNextColumn();
		changed = ImGui::Checkbox("##small", small.access());
		break;
	case 6:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_x", &size_x, ctx);
		break;
	case 7:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_y", &size_y, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 8, ctx);
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
		ImGui::Text("onChange");
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
	if (!ctx.importState)
		fieldName.set_from_arg(ctx.codeGen->CreateVar("bool", initValue ? "true" : "false", CppGen::Var::Interface));
}

void CheckBox::DoDraw(UIContext& ctx)
{
	static bool dummy;
	dummy = initValue;
	ImGui::Checkbox(label.c_str(), &dummy);
}

void CheckBox::DoExport(std::ostream& os, UIContext& ctx)
{
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
}

void CheckBox::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if ((sit->kind == cpp::CallExpr && sit->callee == "ImGui::Checkbox") ||
		(sit->kind == cpp::IfCallThenCall && sit->cond == "ImGui::Checkbox"))
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
			else
				initValue = var->init == "true";
		}

		if (sit->kind == cpp::IfCallThenCall)
			onChange.set_from_arg(sit->callee);
	}
}

std::vector<UINode::Prop>
CheckBox::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "label", &label, true },
		{ "check.init_value", &initValue },
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
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##label", &label, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("label", &label, ctx);
		break;
	case 1:
		ImGui::Text("initValue");
		ImGui::TableNextColumn();
		if (ImGui::Checkbox("##init_value", initValue.access()))
		{
			changed = true;
			ctx.codeGen->ChangeVar(fieldName.c_str(), "bool", initValue ? "true" : "false");
		}
		break;
	case 2:
	{
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##field", &fieldName, false, ctx);
		break;
	}
	default:
		return Widget::PropertyUI(i - 3, ctx);
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
		ImGui::Text("onChange");
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

void RadioButton::DoDraw(UIContext& ctx)
{
	ImGui::RadioButton(label.c_str(), valueID==0);
}

void RadioButton::DoExport(std::ostream& os, UIContext& ctx)
{
	if (!ctx.codeGen->GetVar(fieldName.c_str()))
		ctx.errors.push_back("RadioButon: field_name variable doesn't exist");

	os << ctx.ind << "ImGui::RadioButton("
		<< label.to_arg() << ", "
		<< "&" << fieldName.c_str() << ", "
		<< valueID << ");\n";
}

void RadioButton::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::RadioButton")
	{
		if (sit->params.size())
			label.set_from_arg(sit->params[0]);

		if (sit->params.size() >= 2 && !sit->params[1].compare(0, 1, "&"))
		{
			fieldName.set_from_arg(sit->params[1].substr(1));
		}
	}
}

std::vector<UINode::Prop>
RadioButton::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
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
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##label", &label, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("label", &label, ctx);
		break;
	case 1:
		ImGui::Text("valueID");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputInt("##valueID", valueID.access());
		break;
	case 2:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##fieldName", &fieldName, false, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 3, ctx);
	}
	return changed;
}

//---------------------------------------------------

Input::Input(UIContext& ctx)
{
	flags.prefix("ImGuiInputTextFlags_");
	flags.add$(ImGuiInputTextFlags_CharsDecimal);
	flags.add$(ImGuiInputTextFlags_CharsHexadecimal);
	flags.add$(ImGuiInputTextFlags_CharsScientific);
	flags.add$(ImGuiInputTextFlags_CharsUppercase);
	flags.add$(ImGuiInputTextFlags_CharsNoBlank);
	flags.add$(ImGuiInputTextFlags_AutoSelectAll);
	flags.add$(ImGuiInputTextFlags_CallbackCompletion);
	flags.add$(ImGuiInputTextFlags_CallbackHistory);
	flags.add$(ImGuiInputTextFlags_CallbackAlways);
	flags.add$(ImGuiInputTextFlags_CallbackCharFilter);
	flags.add$(ImGuiInputTextFlags_CtrlEnterForNewLine);
	flags.add$(ImGuiInputTextFlags_ReadOnly);
	flags.add$(ImGuiInputTextFlags_Password);
	flags.add$(ImGuiInputTextFlags_Multiline);

	if (!ctx.importState)
		fieldName.set_from_arg(ctx.codeGen->CreateVar(type, "", CppGen::Var::Interface));
}

void Input::DoDraw(UIContext& ctx)
{
	float ftmp[4] = {};
	int itmp[4] = {};
	double dtmp[4] = {};
	std::string stmp;

	std::string id = label;
	if (id.empty())
		id = "##" + fieldName.value();
	if (flags & ImGuiInputTextFlags_Multiline)
	{
		ImVec2 size{ 0, 0 };
		if (size_x.has_value())
			size.x = size_x.value();
		if (size_y.has_value())
			size.y = size_y.value();
		ImGui::InputTextMultiline(id.c_str(), &stmp, size, flags);
	}
	else if (type == "std::string")
	{
		if (size_x.has_value())
			ImGui::SetNextItemWidth(size_x.value());
		if (hint != "")
			ImGui::InputTextWithHint(id.c_str(), hint.c_str(), &stmp, flags);
		else
			ImGui::InputText(id.c_str(), &stmp, flags);
	}
	else
	{
		if (size_x.has_value())
			ImGui::SetNextItemWidth(size_x.value());
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
	
}

void Input::DoExport(std::ostream& os, UIContext& ctx)
{
	if (keyboardFocus)
	{
		os << ctx.ind << "if (ImGui::IsWindowAppearing())\n";
		ctx.ind_up();
		os << ctx.ind << "ImGui::SetKeyboardFocusHere();\n";
		ctx.ind_down();
	}

	os << ctx.ind << "ImGui::SetNextItemWidth(" << size_x.to_arg() << ");\n";
	
	os << ctx.ind;
	if (!onChange.empty())
		os << "if (";

	std::string cap = type;
	cap[0] = std::toupper(cap[0]);
	std::string id = label.to_arg();
	if (label.empty())
		id = "\"##" + fieldName.value() + "\"";
	
	if (type == "int")
	{
		os << "ImGui::InputInt(" << id << ", &"
			<< fieldName.to_arg() << ", " << (int)step << ")";
	}
	else if (type == "float")
	{
		os << "ImGui::InputFloat(" << id << ", &"
			<< fieldName.to_arg() << ", " << step.to_arg() << ", 0.f, "
			<< format.to_arg() << ")";
	}
	else if (type == "double")
	{
		os << "ImGui::InputDouble(" << id << ", &"
			<< fieldName.to_arg() << ", " << step.to_arg() << ", 0.0, "
			<< format.to_arg() << ")";
	}
	else if (!type.access()->compare(0, 3, "int"))
	{
		os << "ImGui::Input"<< cap << "(" << id << ", &"
			<< fieldName.to_arg() << ")";
	}
	else if (!type.access()->compare(0, 5, "float"))
	{
		os << "ImGui::Input" << cap << "(" << id << ", &"
			<< fieldName.to_arg() << ", " << format.to_arg() << ")";
	}
	else if (flags & ImGuiInputTextFlags_Multiline)
	{
		os << "ImGui::InputTextMultiline(" << id << ", &" 
			<< fieldName.to_arg()
			<< ", { " << size_x.to_arg() << ", " << size_y.to_arg() << " }, "
			<< flags.to_arg() << ")";
	}
	else
	{
		if (hint != "")
			os << "ImGui::InputTextWithHint(" << id << ", " << hint.to_arg() << ", ";
		else
			os << "ImGui::InputText(" << id << ", ";
		os << "&" << fieldName.to_arg() << ", " << flags.to_arg() << ")";
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

void Input::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if ((sit->kind == cpp::CallExpr && sit->callee == "ImGui::InputTextMultiline") ||
		(sit->kind == cpp::IfCallThenCall && sit->cond == "ImGui::InputTextMultiline"))
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
			onChange.set_from_arg(sit->callee);
	}
	else if ((sit->kind == cpp::CallExpr && !sit->callee.compare(0, 16, "ImGui::InputText")) ||
		(sit->kind == cpp::IfCallThenCall && !sit->cond.compare(0, 16, "ImGui::InputText")))
	{
		type = "std::string";

		size_t ex = 0;
		if (sit->callee == "ImGui::InputTextWithHint" || sit->cond == "ImGui::InputTextWithHint") {
			hint = cpp::parse_str_arg(sit->params[1]);
			++ex;
		}

		if (sit->params.size() > 1 + ex && !sit->params[1 + ex].compare(0, 1, "&"))
			fieldName.set_from_arg(sit->params[1 + ex].substr(1));

		if (sit->params.size() > 2 + ex)
			flags.set_from_arg(sit->params[2 + ex]);

		if (sit->kind == cpp::IfCallThenCall)
			onChange.set_from_arg(sit->callee);
	}
	else if ((sit->kind == cpp::CallExpr && !sit->callee.compare(0, 12, "ImGui::Input")) ||
			(sit->kind == cpp::IfCallThenCall && !sit->cond.compare(0, 12, "ImGui::Input")))
	{
		if (sit->kind == cpp::CallExpr)
			type = sit->callee.substr(12);
		else
			type = sit->cond.substr(12);
		
		type.access()->front() = std::tolower(std::string(type)[0]);

		if (sit->params.size() >= 2 && !sit->params[1].compare(0, 1, "&"))
			fieldName.set_from_arg(sit->params[1].substr(1));

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
			onChange.set_from_arg(sit->callee);
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextItemWidth")
	{
		if (sit->params.size()) {
			size_x.set_from_arg(sit->params[0]);
		}
	}
	else if (sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::SetKeyboardFocusHere")
	{
		keyboardFocus = true;
	}
}

std::vector<UINode::Prop>
Input::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "input.flags", &flags },
		{ "label", &label, true },
		{ "input.type", &type },
		{ "input.field_name", &fieldName },
		{ "input.hint", &hint },
		{ "input.step", &step },
		{ "input.format", &format },
		{ "keyboard_focus", &keyboardFocus },
		{ "size_x", &size_x },
		{ "size_y", &size_y }, 
	});
	return props;
}

bool Input::PropertyUI(int i, UIContext& ctx)
{
	static const char* TYPES[] {
		"std::string",
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
		ImGui::Unindent();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, 0.0f });
		if (ImGui::TreeNode("flags")) {
			ImGui::TableNextColumn();
			changed = CheckBoxFlags(&flags);
			ImGui::TreePop();
		}
		ImGui::Spacing();
		ImGui::PopStyleVar();
		ImGui::Indent();
		break;
	case 1:
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputText("##label", label.access());
		break;
	case 2:
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
	case 3:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##fieldName", &fieldName, type, false, ctx);
		break;
	case 4:
		ImGui::BeginDisabled(type != "std::string");
		ImGui::Text("hint");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputText("##hint", hint.access());
		ImGui::EndDisabled();
		break;
	case 5:
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
	case 6:
		ImGui::BeginDisabled(type.access()->compare(0, 5, "float"));
		ImGui::Text("format");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputText("##format", format.access());
		ImGui::EndDisabled();
		break;
	case 7:
		ImGui::Text("keyboardFocus");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::Checkbox("##kbf", keyboardFocus.access());
		break;
	case 8:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_x", &size_x, ctx);
		break;
	case 9:
		ImGui::BeginDisabled(type != "std::string" || !(flags & ImGuiInputTextFlags_Multiline));
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_y", &size_y, ctx);
		ImGui::EndDisabled();
		break;
	default:
		return Widget::PropertyUI(i - 10, ctx);
	}
	return changed;
}

std::vector<UINode::Prop>
Input::Events()
{
	auto props = Widget::Events();
	props.insert(props.begin(), {
		{ "onChange", &onChange }
		});
	return props;
}

bool Input::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("onChange");
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

Combo::Combo(UIContext& ctx)
{
	if (!ctx.importState)
		fieldName.set_from_arg(ctx.codeGen->CreateVar("int", "-1", CppGen::Var::Interface));
}

void Combo::DoDraw(UIContext& ctx)
{
	int zero = 0;
	if (size_x.has_value())
		ImGui::SetNextItemWidth(size_x.value());
	std::string id = label;
	if (id.empty())
		id = std::string("##") + fieldName.c_str();
	auto vars = items.used_variables();
	if (vars.empty())
	{
		ImGui::Combo(id.c_str(), &zero, items.c_str());
	}
	else
	{
		std::string tmp = '{' + vars[0] + "}\0";
		ImGui::Combo(id.c_str(), &zero, tmp.c_str());
	}
}

void Combo::DoExport(std::ostream& os, UIContext& ctx)
{
	if (!size_x.empty())
		os << ctx.ind << "ImGui::SetNextItemWidth(" << size_x.to_arg() << ");\n";

	os << ctx.ind;
	if (!onChange.empty())
		os << "if (";

	std::string id = label.to_arg();
	if (label.empty())
		id = std::string("\"##") + fieldName.c_str() + "\"";
	auto vars = items.used_variables();
	if (vars.empty())
	{
		os << "ImGui::Combo(" << id << ", &"
			<< fieldName.to_arg() << ", " << items.to_arg() << ")";
	}
	else
	{
		os << "ImRad::Combo(" << id << ", &"
			<< fieldName.to_arg() << ", " << items.to_arg() << ")";
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

void Combo::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if ((sit->kind == cpp::CallExpr && (sit->callee == "ImGui::Combo" || sit->callee == "ImRad::Combo")) ||
		(sit->kind == cpp::IfCallThenCall && (sit->cond == "ImGui::Combo" || sit->cond == "ImRad::Combo")))
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

		if (sit->kind == cpp::IfCallThenCall)
			onChange.set_from_arg(sit->callee);
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
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputText("##label", label.access());
		break;
	case 1:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##fieldName", &fieldName, false, ctx);
		break;
	case 2:
		ImGui::Text("items");
		ImGui::TableNextColumn();
		//ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		if (ImGui::Selectable("...", false, 0, { ImGui::GetContentRegionAvail().x-ImGui::GetFrameHeight(), ImGui::GetFrameHeight() }))
		{
			changed = true;
			std::string tmp = *items.access(); //preserve embeded nulls
			stx::replace(tmp, '\0', '\n');
			comboDlg.value = tmp;
			comboDlg.OpenPopup([this](ImRad::ModalResult) {
				std::string tmp = comboDlg.value;
				if (!tmp.empty() && tmp.back() != '\n')
					tmp.push_back('\n');
				stx::replace(tmp, '\n', '\0');
				*items.access() = tmp;
				});
		}
		ImGui::SameLine(0, 0);
		BindingButton("items", &items, ctx);
		break;
	case 3:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_x", &size_x, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 4, ctx);
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
		ImGui::Text("onChange");
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
	if (!ctx.importState)
		fieldName.set_from_arg(ctx.codeGen->CreateVar(type=="angle" ? "float" : type, "", CppGen::Var::Interface));
}

void Slider::DoDraw(UIContext& ctx)
{
	float ftmp[4] = {};
	int itmp[4] = {};

	if (size_x.has_value())
		ImGui::SetNextItemWidth(size_x.value());

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
}

void Slider::DoExport(std::ostream& os, UIContext& ctx)
{
	os << ctx.ind << "ImGui::SetNextItemWidth(" << size_x.to_arg() << ");\n";

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
		(sit->kind == cpp::IfCallThenCall && !sit->cond.compare(0, 13, "ImGui::Slider")))
	{
		if (sit->kind == cpp::CallExpr)
			type = sit->callee.substr(13);
		else
			type = sit->cond.substr(13);
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
			onChange.set_from_arg(sit->callee);
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
		changed = ImGui::InputText("##label", label.access());
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
		changed = InputBindable("##size_x", &size_x, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_x", &size_x, ctx);
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
		ImGui::Text("onChange");
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

ColorEdit::ColorEdit(UIContext& ctx)
{
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

	if (!ctx.importState)
		fieldName.set_from_arg(ctx.codeGen->CreateVar(type, "", CppGen::Var::Interface));
}

void ColorEdit::DoDraw(UIContext& ctx)
{
	float ftmp[4] = {};
	
	std::string id = label;
	if (id.empty())
		id = "##" + fieldName.value();
	if (size_x.has_value())
		ImGui::SetNextItemWidth(size_x.value());
	
	if (type == "color3")
		ImGui::ColorEdit3(id.c_str(), ftmp, flags);
	else if (type == "color4")
		ImGui::ColorEdit4(id.c_str(), ftmp, flags);
}

void ColorEdit::DoExport(std::ostream& os, UIContext& ctx)
{
	if (!size_x.empty())
		os << ctx.ind << "ImGui::SetNextItemWidth(" << size_x.to_arg() << ");\n";

	os << ctx.ind;
	if (!onChange.empty())
		os << "if (";

	std::string id = label.to_arg();
	if (label.empty())
		id = "\"##" + fieldName.value() + "\"";

	if (type == "color3")
	{
		os << "ImGui::ColorEdit3(" << id << ", "
			<< fieldName.to_arg() << ", " << flags.to_arg() << ")";
	}
	else if (type == "color4")
	{
		os << "ImGui::ColorEdit4(" << id << ", "
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
		(sit->kind == cpp::IfCallThenCall && !sit->cond.compare(0, 16, "ImGui::ColorEdit")))
	{
		if (sit->kind == cpp::CallExpr)
			type = sit->callee.substr(16, 1) == "3" ? "color3" : "color4";
		else
			type = sit->cond.substr(16, 1) == "3" ? "color3" : "color4";

		if (sit->params.size()) {
			label.set_from_arg(sit->params[0]);
			if (!label.access()->compare(0, 2, "##"))
				label = "";
		}

		if (sit->params.size() >= 2) {
			fieldName.set_from_arg(sit->params[1]);
			std::string fn = fieldName.c_str();
			if (!ctx.codeGen->GetVar(fn))
				ctx.errors.push_back("Input: field_name variable '" + fn + "' doesn't exist");
		}

		if (sit->params.size() >= 3)
			flags.set_from_arg(sit->params[2]);

		if (sit->kind == cpp::IfCallThenCall)
			onChange.set_from_arg(sit->callee);
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
		ImGui::Unindent();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, 0.0f });
		if (ImGui::TreeNode("flags")) {
			ImGui::TableNextColumn();
			changed = CheckBoxFlags(&flags);
			ImGui::TreePop();
		}
		ImGui::Spacing();
		ImGui::PopStyleVar();
		ImGui::Indent();
		break;
	case 1:
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputText("##label", label.access());
		break;
	case 2:
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
	case 3:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##fieldName", &fieldName, type, false, ctx);
		break;
	case 4:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_x", &size_x, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 5, ctx);
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
		ImGui::Text("onChange");
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
	if (!ctx.importState)
		*fieldName.access() = ctx.codeGen->CreateVar("ImRad::Texture", "", CppGen::Var::Impl);
}

void Image::DoDraw(UIContext& ctx)
{
	if (!tex.id) {
		ImGui::Dummy({ 20, 20 });
		return;
	}

	float w = (float)tex.w;
	if (size_x.has_value() && size_x.value())
		w = size_x.value();
	float h = (float)tex.h;
	if (size_y.has_value() && size_y.value())
		h = size_y.value();
	
	ImGui::Image(tex.id, { w, h });
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
	
	if (size_x.has_value() && !size_x.value())
		os << "(float)" << fieldName.to_arg() << ".w";
	else
		os << size_x.to_arg();
	
	os << ", ";
	
	if (size_y.has_value() && !size_y.value())
		os << "(float)" << fieldName.to_arg() << ".h";
	else
		os << size_y.to_arg();
	
	os << " });\n";
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

		RefreshTexture(ctx);
	}
}

std::vector<UINode::Prop>
Image::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "file_name", &fileName, true },
		{ "field_name", &fieldName },
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
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##fileName", &fileName, ctx);
		if (ImGui::IsItemDeactivatedAfterEdit() || 
			(ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
			RefreshTexture(ctx);
		ImGui::SameLine(0, 0);
		BindingButton("fileName", &fileName, ctx);
		break;
	case 1:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("fieldName");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##fieldn", &fieldName, false, ctx);
		break;
	case 2:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_x", &size_x, ctx);
		break;
	case 3:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_y", &size_y, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 4, ctx);
	}
	return changed;
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
		if (ctx.fname.empty() && !ctx.importState) {
			messageBox.title = "Warning";
			messageBox.message = "Please save the file first so that relative paths can work";
			messageBox.buttons = ImRad::Ok;
			messageBox.OpenPopup();
			return;
		}
		assert(ctx.fname != "");
		fname = (fs::path(ctx.fname).parent_path() / fileName.value()).string();
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

UserWidget::UserWidget(UIContext& ctx)
{
}

void UserWidget::DoDraw(UIContext& ctx)
{
	ImVec2 size{ 20, 20 };
	if (size_x.has_value())
		size.x = size_x.value();
	if (size_y.has_value())
		size.y = size_y.value();

	std::string id = std::to_string((uintptr_t)this);
	ImGui::BeginChild(id.c_str(), size, true);
	ImGui::EndChild();
}

void UserWidget::DoExport(std::ostream& os, UIContext& ctx)
{
	if (onDraw.empty()) {
		ctx.errors.push_back("UserWidget: OnDraw not set!");
		return;
	}

	os << ctx.ind << onDraw.to_arg() << "({ " 
		<< size_x.to_arg() << ", " << size_y.to_arg() 
		<< " });\n";
}

void UserWidget::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
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
UserWidget::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "size_x", &size_x },
		{ "size_y", &size_y },
		});
	return props;
}

bool UserWidget::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_x", &size_x, ctx);
		break;
	case 1:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_y", &size_y, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 2, ctx);
	}
	return changed;
}

std::vector<UINode::Prop>
UserWidget::Events()
{
	auto props = Widget::Events();
	props.insert(props.begin(), {
		{ "onDraw", &onDraw },
		});
	return props;
}

bool UserWidget::EventUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Text("onDraw");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		changed = InputEvent("##onDraw", &onDraw, ctx);
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
}

Table::Table(UIContext& ctx)
{
	flags.prefix("ImGuiTableFlags_");
	flags.add$(ImGuiTableFlags_Resizable);
	flags.add$(ImGuiTableFlags_Reorderable);
	flags.add$(ImGuiTableFlags_Hideable);
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

	columnData.resize(3);
	for (size_t i = 0; i < columnData.size(); ++i)
		columnData[i].label = char('A' + i);
}

void Table::DoDraw(UIContext& ctx)
{
	int n = std::max(1, (int)columnData.size());
	if (!style_padding)
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 1, 1 });
	ImVec2 size{ 0, 0 };
	if (size_x.has_value())
		size.x = size_x.value();
	if (size_y.has_value())
		size.y = size_y.value();
	if (ImGui::BeginTable(("table" + std::to_string((uint64_t)this)).c_str(), n, flags, size))
	{
		for (size_t i = 0; i < (int)columnData.size(); ++i)
			ImGui::TableSetupColumn(columnData[i].label.c_str(), columnData[i].flags, columnData[i].width);
		if (header)
			ImGui::TableHeadersRow();
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		
		bool tmp = ctx.table;
		ctx.table = true;
		for (int i = 0; i < (int)children.size(); ++i)
		{
			children[i]->Draw(ctx);
			//ImGui::Text("cell");
		}
		ctx.table = tmp;

		ImGui::EndTable();
	}
	if (!style_padding)
		ImGui::PopStyleVar();
}

std::vector<UINode::Prop>
Table::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "table.flags", &flags },
		{ "table.columns", nullptr },
		{ "style_padding", &style_padding },
		{ "table.header", &header },
		{ "table.row_count", &rowCount },
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
		ImGui::Unindent();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, 0.0f });
		if (ImGui::TreeNode("flags"))
		{
			ImGui::TableNextColumn();
			changed = CheckBoxFlags(&flags);
			ImGui::TreePop();
		}
		ImGui::Spacing();
		ImGui::PopStyleVar();
		ImGui::Indent();
		break;
	case 1:
	{
		ImGui::Text("columns");
		ImGui::TableNextColumn();
		if (ImGui::Selectable("...", false, 0, { ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight(), ImGui::GetFrameHeight() }))
		{
			changed = true;
			tableColumns.columnData = columnData;
			tableColumns.target = &columnData;
			tableColumns.OpenPopup();
		}
		/*auto tmp = std::to_string(columnData.size());
		ImGui::SetNextItemWidth(-2 * ImGui::GetFrameHeight());
		ImGui::InputText("##columns", &tmp, ImGuiInputTextFlags_ReadOnly);
		ImGui::SameLine(0, 0);
		if (ImGui::Button("..."))
		{
			changed = true;
			tableColumns.columnData = columnData;
			tableColumns.target = &columnData;
			tableColumns.OpenPopup();
		}*/
		break;
	}
	case 2:
		ImGui::Text("style_padding");
		ImGui::TableNextColumn();
		changed = ImGui::Checkbox("##style_padding", style_padding.access());
		break;
	case 3:
		ImGui::Text("header");
		ImGui::TableNextColumn();
		changed = ImGui::Checkbox("##header", header.access());
		break;
	case 4:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("rowCount");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##rowCount", &rowCount, true, ctx);
		break;
	case 5:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_x", &size_x, ctx);
		break;
	case 6:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_y", &size_y, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 7, ctx);
	}
	return changed;
}

void Table::DoExport(std::ostream& os, UIContext& ctx)
{
	if (!style_padding)
		os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 1, 1 });\n";
	
	os << ctx.ind << "if (ImGui::BeginTable("
		<< "\"table" << (uint64_t)this << "\", " 
		<< columnData.size() << ", " 
		<< flags.to_arg() << ", "
		<< "{ " << size_x.to_arg() << ", " << size_y.to_arg() << " }"
		<< "))\n"
		<< ctx.ind << "{\n";
	ctx.ind_up();

	for (const auto& cd : columnData)
	{
		os << ctx.ind << "ImGui::TableSetupColumn(\"" << cd.label << "\", "
			<< cd.flags.to_arg() << ", " << cd.width << ");\n";
	}
	if (header)
		os << ctx.ind << "ImGui::TableHeadersRow();\n";

	bool tmp = ctx.table;
	ctx.table = true;
	if (!rowCount.empty()) 
	{
		os << "\n" << ctx.ind << "for (size_t " << FOR_VAR << " = 0; " << FOR_VAR 
			<< " < " << rowCount.to_arg() << "; ++" << FOR_VAR
			<< ")\n" << ctx.ind << "{\n";
		ctx.ind_up();
		os << ctx.ind << "ImGui::TableNextRow();\n";
		os << ctx.ind << "ImGui::TableSetColumnIndex(0);\n";
		os << ctx.ind << "ImGui::PushID((int)" << FOR_VAR << ");\n";
	}
	else 
	{
		os << ctx.ind << "ImGui::TableNextRow();\n";
		os << ctx.ind << "ImGui::TableSetColumnIndex(0);\n";
	}
	
	os << ctx.ind << "/// @separator\n\n";

	for (auto& child : children)
		child->Export(os, ctx);

	os << "\n" << ctx.ind << "/// @separator\n";

	if (!rowCount.empty()) {
		os << ctx.ind << "ImGui::PopID();\n";
		ctx.ind_down();
		os << ctx.ind << "}\n";
	}
	ctx.table = tmp;

	os << ctx.ind << "ImGui::EndTable();\n";
	ctx.ind_down();
	os << ctx.ind << "}\n";

	if (!style_padding)
		os << ctx.ind << "ImGui::PopStyleVar();\n";
}

void Table::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar")
	{
		if (sit->params.size() && sit->params[0] == "ImGuiStyleVar_CellPadding")
			style_padding = false;
	}
	else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginTable")
	{
		header = false;
		ctx.importLevel = 0;

		if (sit->params.size() >= 2) {
			std::istringstream is(sit->params[1]);
			size_t n = 3;
			is >> n;
			columnData.resize(n);
		}

		if (sit->params.size() >= 3)
			flags.set_from_arg(sit->params[2]);

		if (sit->params.size() >= 4) {
			auto size = cpp::parse_size(sit->params[3]);
			size_x.set_from_arg(size.first);
			size_y.set_from_arg(size.second);
		}
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
		
		columnData[ctx.importLevel++] = std::move(cd);
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::TableHeadersRow")
	{
		header = true;
	}
	else if (sit->kind == cpp::ForBlock)
	{
		if (!sit->cond.compare(0, FOR_VAR.size()+1, std::string(FOR_VAR) + "<")) //VS bug without std::string()
			rowCount.set_from_arg(sit->cond.substr(FOR_VAR.size() + 1));
	}
}

//---------------------------------------------------------

Child::Child(UIContext& ctx)
{
}

void Child::DoDraw(UIContext& ctx)
{
	if (!style_padding)
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	if (styleBg.has_value())
		ImGui::PushStyleColor(ImGuiCol_ChildBg, styleBg.value());

	ImVec2 sz(0, 0);
	if (size_x.has_value()) 
		sz.x = size_x.value();
	if (size_y.has_value())
		sz.y = size_y.value();
	if (!sz.x && children.empty())
		sz.x = 30;
	if (!sz.y && children.empty())
		sz.y = 30;
	//very weird - using border controls window padding
	ImGui::BeginChild("", sz, border); 
	//draw border for visual distinction
	//not needed - hovered item gets highlited anyway
	/*if (!border && !styleBg.has_value()) {
		ImDrawList* dl = ImGui::GetWindowDrawList();
		auto clr = ImGui::GetStyle().Colors[ImGuiCol_Border];
		dl->AddRect(cached_pos, cached_pos + cached_size, ImGui::ColorConvertFloat4ToU32(clr), 0, 0, 1);
	}*/

	if (columnCount.has_value() && columnCount.value() >= 2)
	{
		int n = columnCount.value();
		//ImGui::SetColumnWidth doesn't support auto size columns
		//We compute it ourselves
		float fixedWidth = (n - 1) * 10.f; //spacing
		int autoSized = 0;
		for (size_t i = 0; i < columnsWidths.size(); ++i)
		{
			if (columnsWidths[i].has_value() && columnsWidths[i].value())
				fixedWidth += columnsWidths[i].value();
			else
				++autoSized;
		}
		float autoSizeW = (ImGui::GetContentRegionAvail().x - fixedWidth) / autoSized;
		
		ImGui::Columns(n, "columns", columnBorder);
		for (int i = 0; i < (int)columnsWidths.size(); ++i)
		{
			if (columnsWidths[i].has_value() && columnsWidths[i].value())
				ImGui::SetColumnWidth(i, columnsWidths[i].value());
			else
				ImGui::SetColumnWidth(i, autoSizeW);
		}
	}

	for (size_t i = 0; i < children.size(); ++i)
	{
		children[i]->Draw(ctx);
	}
		
	ImGui::EndChild();

	if (styleBg.has_value())
		ImGui::PopStyleColor();
	if (!style_padding)
		ImGui::PopStyleVar();
}

void Child::DoExport(std::ostream& os, UIContext& ctx)
{
	if (!style_padding)
		os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });\n";
	if (!styleBg.empty())
		os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_ChildBg, " << styleBg.to_arg() << ");\n";

	os << ctx.ind << "ImGui::BeginChild(\"child" << (uint64_t)this << "\", "
		<< "{ " << size_x.to_arg() << ", " << size_y.to_arg() << " }, "
		<< std::boolalpha << border
		<< ");\n";
	os << ctx.ind << "{\n";
	ctx.ind_up();

	bool hasColumns = !columnCount.has_value() || columnCount.value() >= 2;
	if (hasColumns) {
		os << ctx.ind << "ImGui::Columns(" << columnCount.to_arg() << ", \"\", "
			<< columnBorder.to_arg() << ");\n";
		//for (size_t i = 0; i < columnsWidths.size(); ++i)
			//os << ctx.ind << "ImGui::SetColumnWidth(" << i << ", " << columnsWidths[i].c_str() << ");\n";
	}

	if (!data_size.empty())
	{
		os << ctx.ind << "for (size_t " << FOR_VAR << " = 0; " << FOR_VAR
			<< " < " << data_size.to_arg() << "; ++" << FOR_VAR
			<< ")\n" << ctx.ind << "{\n";
		ctx.ind_up();
	}
	
	os << ctx.ind << "/// @separator\n\n";

	for (auto& child : children)
		child->Export(os, ctx);

	os << ctx.ind << "/// @separator\n";

	if (!data_size.empty()) {
		if (hasColumns)
			os << ctx.ind << "ImGui::NextColumn();\n";
		ctx.ind_down();
		os << ctx.ind << "}\n";
	}
	
	os << ctx.ind << "ImGui::EndChild();\n";
	ctx.ind_down();
	os << ctx.ind << "}\n";

	if (!styleBg.empty())
		os << ctx.ind << "ImGui::PopStyleColor();\n";
	if (!style_padding)
		os << ctx.ind << "ImGui::PopStyleVar();\n";
}

void Child::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
	{
		if (sit->params.size() == 2 && sit->params[0] == "ImGuiCol_ChildBg")
			styleBg.set_from_arg(sit->params[1]);
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar")
	{
		if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_WindowPadding") {
			auto size = cpp::parse_fsize(sit->params[1]);
			if (size.x == 0 && size.y == 0)
				style_padding = false;
		}
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::BeginChild")
	{
		if (sit->params.size() >= 2) {
			auto size = cpp::parse_size(sit->params[1]);
			size_x.set_from_arg(size.first);
			size_y.set_from_arg(size.second);
		}

		if (sit->params.size() >= 3)
			border = sit->params[2] == "true";
	}
	else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::Columns")
	{
		if (sit->params.size())
			*columnCount.access() = sit->params[0];

		if (sit->params.size() >= 3)
			columnBorder = sit->params[2] == "true";

		if (columnCount.has_value())
			columnsWidths.resize(columnCount.value(), 0.f);
		else
			columnsWidths.resize(1, 0.f);
	}
	else if (sit->kind == cpp::ForBlock)
	{
		if (!sit->cond.compare(0, FOR_VAR.size() + 1, FOR_VAR + "<"))
			data_size.set_from_arg(sit->cond.substr(FOR_VAR.size() + 1));
	}
}


std::vector<UINode::Prop>
Child::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "style_color", &styleBg },
		{ "border", &border },
		{ "child.column_count", &columnCount },
		{ "child.column_border", &columnBorder },
		{ "child.data_size", &data_size },
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
		ImGui::Text("style_color");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##color", &styleBg, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("style_color", &styleBg, ctx);
		break;
	case 1:
		ImGui::Text("border");
		ImGui::TableNextColumn();
		changed = ImGui::Checkbox("##border", border.access());
		break;
	case 2:
		ImGui::Text("columnCount");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##columnCount", &columnCount, 1, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("columnCount", &columnCount, ctx);
		break;
	case 3:
		ImGui::Text("columnBorder");
		ImGui::TableNextColumn();
		changed = ImGui::Checkbox("##columnBorder", columnBorder.access());
		break;
	case 4:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("data_size");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##data_size", &data_size, true, ctx);
		break;
	case 5:
		ImGui::Text("size_x");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_x", &size_x, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_x", &size_x, ctx);
		break;
	case 6:
		ImGui::Text("size_y");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##size_y", &size_y, 0.f, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("size_y", &size_y, ctx);
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

void CollapsingHeader::DoDraw(UIContext& ctx)
{
	//automatically expand to show selected child and collapse to save space
	//in the window and avoid potential scrolling
	if (ctx.selected.size() == 1)
	{
		ImGui::SetNextItemOpen(FindChild(ctx.selected[0]));
	}
	if (ImGui::CollapsingHeader(label.c_str()))
	{
		for (size_t i = 0; i < children.size(); ++i)
		{
			children[i]->Draw(ctx);
		}
	}
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
		BindingButton("label", &label, ctx);
		break;
	case 1:
		ImGui::Text("open");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##open", &open, true, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("open", &open, ctx);
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
	flags.add$(ImGuiTreeNodeFlags_OpenOnArrow);
	flags.add$(ImGuiTreeNodeFlags_OpenOnDoubleClick);
	flags.add$(ImGuiTreeNodeFlags_SpanAvailWidth);
	flags.add$(ImGuiTreeNodeFlags_SpanFullWidth);
}

void TreeNode::DoDraw(UIContext& ctx)
{
	if (ctx.selected.size() == 1)
	{
		//automatically expand to show selection and collapse to save space
		//in the window and avoid potential scrolling
		ImGui::SetNextItemOpen(FindChild(ctx.selected[0]));
	}
	lastOpen = false;
	if (ImGui::TreeNodeEx(label.c_str(), flags))
	{
		lastOpen = true;
		for (const auto& child : children)
			child->Draw(ctx);

		ImGui::TreePop();
	}
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
		ImGui::Unindent();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, 0.0f });
		if (ImGui::TreeNode("flags")) {
			ImGui::TableNextColumn();
			changed = CheckBoxFlags(&flags);
			ImGui::TreePop();
		}
		ImGui::Spacing();
		ImGui::PopStyleVar();
		ImGui::Indent();
		break;
	case 1:
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##label", &label, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("label", &label, ctx);
		break;
	case 2:
		ImGui::Text("open");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputBindable("##open", &open, true, ctx);
		ImGui::SameLine(0, 0);
		BindingButton("open", &open, ctx);
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
	flags.add$(ImGuiTabBarFlags_NoTabListScrollingButtons);

	if (!ctx.importState)
		children.push_back(std::make_unique<TabItem>(ctx));
}

void TabBar::DoDraw(UIContext& ctx)
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
	std::string name = "tabBar" + std::to_string((uintptr_t)this);
	os << ctx.ind << "if (ImGui::BeginTabBar(\"" << name << "\", "
		<< flags.to_arg() << "))\n";
	os << ctx.ind << "{\n";
	
	ctx.ind_up();
	if (!tabCount.empty())
	{
		os << ctx.ind << "for (size_t " << FOR_VAR << " = 0; " << FOR_VAR << " < "
			<< tabCount.to_arg() << "; ++" << FOR_VAR << ")\n";
		os << ctx.ind << "{\n";
		ctx.ind_up();
		os << ctx.ind << "ImGui::PushID((int)" << FOR_VAR << ");\n";
	}
	os << ctx.ind << "/// @separator\n\n";
	
	for (const auto& child : children)
		child->Export(os, ctx);

	os << ctx.ind << "/// @separator\n";
	if (!tabCount.empty())
	{
		os << ctx.ind << "ImGui::PopID();\n";
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
		std::string cmp;
		cmp += FOR_VAR;
		cmp += "<"; //to prevent VS bug
		if (!sit->cond.compare(0, cmp.size(), cmp))
			tabCount.set_from_arg(sit->cond.substr(cmp.size()));
	}
}

std::vector<UINode::Prop>
TabBar::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "flags", &flags },
		{ "tabCount", &tabCount },
		{ "tabIndex", &tabIndex },
		});
	return props;
}

bool TabBar::PropertyUI(int i, UIContext& ctx)
{
	bool changed = false;
	switch (i)
	{
	case 0:
		ImGui::Unindent();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, 0.0f });
		if (ImGui::TreeNode("flags")) {
			ImGui::TableNextColumn();
			changed = CheckBoxFlags(&flags);
			ImGui::TreePop();
		}
		ImGui::Spacing();
		ImGui::PopStyleVar();
		ImGui::Indent();
		break;
	case 1:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("tabCount");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##tabCount", &tabCount, true, ctx);
		break;
	case 2:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::Text("tabIndex");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##tabIndex", &tabIndex, true, ctx);
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

void TabItem::DoDraw(UIContext& ctx)
{
	bool sel = ctx.selected.size() == 1 && FindChild(ctx.selected[0]);
	if (ImGui::BeginTabItem(label.c_str(), nullptr, sel ? ImGuiTabItemFlags_SetSelected : 0))
	{
		for (const auto& child : children)
			child->Draw(ctx);

		ImGui::EndTabItem();
	}
}

void TabItem::DrawExtra(UIContext& ctx)
{
	if (ctx.parents.empty())
		return;

	bool tmp = ImGui::GetCurrentContext()->NavDisableMouseHover;
	ImGui::GetCurrentContext()->NavDisableMouseHover = false;
	assert(ctx.parents.back() == this);
	auto* parent = ctx.parents[ctx.parents.size() - 2];
	size_t idx = stx::find_if(parent->children, [this](const auto& ch) { return ch.get() == this; })
		- parent->children.begin();

	ImGui::SetNextWindowPos(cached_pos, 0, { 0, 1.f });
	ImGui::Begin("extra", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration);

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
}

void TabItem::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
	const ImGuiTabBar* tabBar = ImGui::GetCurrentTabBar();
	int idx = tabBar->LastTabItemIdx;
	const ImGuiTabItem& tab = tabBar->Tabs[idx];
	cached_pos = tabBar->BarRect.GetTL();
	cached_pos.x += tab.Offset;
	cached_size.x = tab.Width;
	cached_size.y = tabBar->BarRect.GetHeight();
}

void TabItem::DoExport(std::ostream& os, UIContext& ctx)
{
	os << ctx.ind << "if (ImGui::BeginTabItem(" << label.to_arg() << ", nullptr, ";
	std::string idx;
	assert(ctx.parents.back() == this);
	const auto* tb = dynamic_cast<TabBar*>(ctx.parents[ctx.parents.size() - 2]);
	if (tb && !tb->tabIndex.empty())
	{
		if (!tb->tabCount.empty())
		{
			idx = "(int)";
			idx += FOR_VAR;
		}
		else
		{
			size_t n = stx::find_if(tb->children, [this](const auto& ch) {
				return ch.get() == this; }) - tb->children.begin();
				idx = std::to_string(n);
		}
		os << idx << " == " << tb->tabIndex.to_arg() << " ? ImGuiTabItemFlags_SetSelected : 0";
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

	if (tb && !tb->tabIndex.empty())
	{
		os << ctx.ind << "if (ImGui::IsItemActivated())\n";
		ctx.ind_up();
		os << ctx.ind << tb->tabIndex.to_arg() << " = " << idx << ";\n";
		ctx.ind_down();
	}
}

void TabItem::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginTabItem")
	{
		if (sit->params.size() >= 1)
			label.set_from_arg(sit->params[0]);

		assert(ctx.parents.back() == this);
		auto* tb = dynamic_cast<TabBar*>(ctx.parents[ctx.parents.size() - 2]);
		if (tb && sit->params.size() >= 3 && sit->params[2].size() > 32)
		{
			const auto& p = sit->params[2];
			size_t i = p.find("==");
			if (i != std::string::npos && p.substr(p.size() - 32, 32) == "?ImGuiTabItemFlags_SetSelected:0")
			{
				std::string var = p.substr(i + 2, p.size() - 32 - i - 2);
				tb->tabIndex.set_from_arg(var);
			}
		}
	}
}

std::vector<UINode::Prop>
TabItem::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
		{ "label", &label, true },
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
		BindingButton("label", &label, ctx);
		break;
	default:
		return Widget::PropertyUI(i - 1, ctx);
	}
	return changed;
}

//---------------------------------------------------------

MenuBar::MenuBar(UIContext& ctx)
{
	if (!ctx.importState)
		children.push_back(std::make_unique<MenuIt>(ctx));
}

void MenuBar::DoDraw(UIContext& ctx)
{
	if (ImGui::BeginMenuBar())
	{
		//for (const auto& child : children) defend against insertion within the loop
		for (size_t i = 0; i < children.size(); ++i)
			children[i]->Draw(ctx);

		ImGui::EndMenuBar();
	}
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
}

void MenuIt::DoDraw(UIContext& ctx)
{
	if (separator)
		ImGui::Separator();
	
	if (children.empty()) //menuItem
	{
		bool check = !checked.empty();
		ImGui::MenuItem(label.c_str(), shortcut.c_str(), check);
	}
	else //menu
	{
		assert(ctx.parents.back() == this);
		const UINode* par = ctx.parents[ctx.parents.size() - 2];
		bool top = dynamic_cast<const MenuBar*>(par);

		ImGui::MenuItem(label.c_str());
		if (!top)
		{
			float w = ImGui::CalcTextSize(ICON_FA_ANGLE_RIGHT).x;
			ImGui::SameLine(0, 0);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() - w);
			ImGui::Text(ICON_FA_ANGLE_RIGHT);
		}
		bool open = ctx.selected.size() == 1 && FindChild(ctx.selected[0]);
		if (open) 
		{
			ImVec2 pos = cached_pos;
			if (top)
				pos.y += ctx.rootWin->MenuBarHeight();
			else {
				ImVec2 sp = ImGui::GetStyle().ItemSpacing;
				ImVec2 pad = ImGui::GetStyle().WindowPadding;
				pos.x += ImGui::GetWindowSize().x - sp.x;
				pos.y -= pad.y;
			}
			ImGui::SetNextWindowPos(pos);
			std::string id = label + "##" + std::to_string((uintptr_t)this);
			ImGui::Begin(id.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing);
			{
				ctx.popupWins.push_back(ImGui::GetCurrentWindow());
				//for (const auto& child : children) defend against insertions within the loop
				for (size_t i = 0; i < children.size(); ++i)
					children[i]->Draw(ctx);
				ImGui::End();
			}
		}
	}
}

void MenuIt::DrawExtra(UIContext& ctx)
{
	if (ctx.parents.empty())
		return;
	assert(ctx.parents.back() == this);
	auto* parent = ctx.parents[ctx.parents.size() - 2];
	bool vertical = !dynamic_cast<MenuBar*>(parent);
	size_t idx = stx::find_if(parent->children, [this](const auto& ch) { return ch.get() == this; })
		- parent->children.begin();

	//draw toolbox
	//currently we always sit it on top of the menu so that it doesn't overlap with submenus
	//no WindowFlags_StayOnTop
	bool tmp = ImGui::GetCurrentContext()->NavDisableMouseHover;
	ImGui::GetCurrentContext()->NavDisableMouseHover = false;
	
	ImVec2 sp = ImGui::GetStyle().ItemSpacing;
	const ImVec2 bsize{ 30, 30 };
	ImVec2 pos = cached_pos;
	if (vertical) {
		pos = ImGui::GetWindowPos();
		//pos.x -= sp.x;
	}
	ImGui::SetNextWindowPos(pos, 0, vertical ? ImVec2{ 0, 1.f } : ImVec2{ 0, 1.f });
	ImGui::Begin("extra", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);

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
	ImGui::GetCurrentContext()->NavDisableMouseHover = tmp;
}

void MenuIt::CalcSizeEx(ImVec2 x1, UIContext& ctx)
{
	ImVec2 sp = ImGui::GetStyle().ItemSpacing;
	const ImGuiMenuColumns* mc = &ImGui::GetCurrentWindow()->DC.MenuColumns;
	cached_pos.x += mc->OffsetLabel;
	cached_size = ImGui::CalcTextSize(label.c_str(), nullptr, true);
	cached_size.x += sp.x;
	assert(ctx.parents.back() == this);
	const UINode* par = ctx.parents[ctx.parents.size() - 2];
	bool top = dynamic_cast<const MenuBar*>(par);
	if (top)
	{
		++cached_pos.y;
		cached_size.y = ctx.rootWin->MenuBarHeight() - 2;
	}
	else
	{
		if (separator)
			cached_pos.y += sp.y;
	}
}

void MenuIt::DoExport(std::ostream& os, UIContext& ctx)
{
	if (separator)
		os << ctx.ind << "ImGui::Separator();\n";

	if (children.empty())
	{
		bool ifstmt = !onChange.empty();
		os << ctx.ind;
		if (ifstmt)
			os << "if (";

		os << "ImGui::MenuItem(" << label.to_arg() << ", "
			<< shortcut.to_arg() << ", ";
		if (checked.empty())
			os << "false";
		else
			os << "&" << checked.to_arg();
		os << ")";

		std::string sh = CodeShortcut(shortcut.c_str());
		if (sh.size())
			os << " ||\n" << ctx.ind << (ifstmt ? "   " : "") << sh;

		if (ifstmt) {
			os << ")\n";
			ctx.ind_up();
			os << ctx.ind << onChange.to_arg() << "();\n";
			ctx.ind_down();
		}
		else
			os << ";\n";
	}
	else
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
}

void MenuIt::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
	if ((sit->kind == cpp::CallExpr && sit->callee == "ImGui::MenuItem") ||
		(sit->kind == cpp::IfCallThenCall && sit->cond == "ImGui::MenuItem"))
	{
		if (sit->params.size())
			label.set_from_arg(sit->params[0]);
		if (sit->params.size() >= 2)
			shortcut.set_from_arg(sit->params[1]);
		if (sit->params.size() >= 3 && !sit->params[2].compare(0, 1, "&"))
			checked.set_from_arg(sit->params[2].substr(1));

		if (sit->kind == cpp::IfCallThenCall)
			onChange.set_from_arg(sit->callee);
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
}

std::vector<UINode::Prop>
MenuIt::Properties()
{
	auto props = Widget::Properties();
	props.insert(props.begin(), {
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
		ImGui::Text("label");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputText("##label", label.access());
		break;
	case 1:
		ImGui::BeginDisabled(children.size());
		ImGui::Text("shortcut");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::InputText("##shortcut", shortcut.access());
		ImGui::EndDisabled();
		break;
	case 2:
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_NAME_CLR);
		ImGui::BeginDisabled(children.size());
		ImGui::Text("checked");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = InputFieldRef("##checked", &checked, true, ctx);
		ImGui::EndDisabled();
		break;
	case 3:
		ImGui::Text("separator");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
		changed = ImGui::Checkbox("##separator", separator.access());
		break;
	default:
		return Widget::PropertyUI(i - 4, ctx);
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
		ImGui::Text("onChange");
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
