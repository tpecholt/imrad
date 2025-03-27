#include "node_extra.h"
#include "binding_input.h"
#include "ui_combo_dlg.h"

const float DEFAULT_SPLIT_RATIO = 0.3f;

DockSpace::DockSpace(UIContext& ctx)
{
    flags.add$(ImGuiDockNodeFlags_KeepAliveOnly);
    flags.add$(ImGuiDockNodeFlags_NoDockingOverCentralNode);
    flags.add$(ImGuiDockNodeFlags_PassthruCentralNode);
}

std::unique_ptr<Widget> DockSpace::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<DockSpace>(*this);
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* DockSpace::DoDraw(UIContext& ctx)
{
    ImGuiID dockId = ImGui::GetID(("DockSpace" + std::to_string((uintptr_t)this)).c_str());
    ImVec2 sz{ size_x.eval_px(ImGuiAxis_X, ctx), size_y.eval_px(ImGuiAxis_Y, ctx) };
    ImVec2 dockSize = ImGui::CalcItemSize(sz, ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
    
    ImU32 hash = CalcHash(ctx);
    if (hash != ctx.prevDockspaceHash || ctx.beingResized)
    {
        ctx.prevDockspaceHash = hash;
        //this needs to be run only when nodes change
        //ImGui::DockBuilderRemoveNode(dockId); not needed
        ImGuiID rootId = ImGui::DockBuilderAddNode(dockId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(rootId, dockSize);
        
        for (auto& child : children) 
        {
            auto* node = dynamic_cast<DockNode*>(child.get());
            rootId = node->SplitNode(rootId, ctx);
        }

        ImGui::DockBuilderFinish(dockId);
    }

    if (!style_emptyBg.empty())
        ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, style_emptyBg.eval(ImGuiCol_DockingEmptyBg, ctx));

    //PushStyleVar: DockSpace increases CursorPos by spacing which leads to ScrollbarY being shown
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
    ImGui::DockSpace(dockId, dockSize, flags);
    ImGui::PopStyleVar();

    if (!style_emptyBg.empty())
        ImGui::PopStyleColor();

    for (auto& child : children) 
        child->Draw(ctx);

    return ImGui::GetWindowDrawList();
}

uint32_t DockSpace::CalcHash(UIContext& ctx)
{
    //force hash change when beingResized changes to reflect splitRatios
    uint32_t hash = ctx.beingResized;
    HashCombineData(hash, size_x.value());
    HashCombineData(hash, size_y.value());

    for (const auto& child : children) {
        auto* node = dynamic_cast<DockNode*>(child.get());
        if (node)
            node->CalcHash(hash);
    }
    return hash;
}

void DockSpace::DoDrawTools(UIContext& ctx)
{
    ImGui::SetNextWindowPos(cached_pos, 0, { 0, 1.f });
    ImGui::Begin("extra", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoSavedSettings);

    static const char* DIRNAME[] = { "Split Left", "Split Right", "Split Up", "Split Down" };
    for (int dir = ImGuiDir_Left; dir != ImGuiDir_COUNT; ++dir)
    {
        if (ImGui::Button(DIRNAME[dir])) {
            auto node = std::make_unique<DockNode>(ctx);
            node->splitDir = ImGuiDir(dir);
            node->splitRatio = DEFAULT_SPLIT_RATIO;
            //ctx.selected = { node.get() };
            children.push_back(std::move(node));
        }
        ImGui::SameLine();
    }

    /*ImGui::SameLine();
    if (ImGui::Button("Add Floating")) {
        auto node = std::make_unique<DockNode>(ctx);
        node->splitDir = ImGuiDir_None;
        node->splitRatio = 0;
        ctx.selected = { node.get() };
        children.push_back(std::move(node));
    }*/

    ImGui::End();
}

void DockSpace::DoExport(std::ostream& os, UIContext& ctx)
{
    os << ctx.ind << "IM_ASSERT(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable);\n";
    os << ctx.ind << "ImGuiID dockId = ImGui::GetID(\"DockSpace\");\n";
    os << ctx.ind << "ImVec2 dockSize = ImGui::CalcItemSize({ "
        << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", "
        << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]) << " }, "
        << "ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);\n";

    os << ctx.ind << "if (!ImGui::DockBuilderGetNode(dockId))\n";
    os << ctx.ind << "{\n";
    ctx.ind_up();
    os << ctx.ind << "ImGuiID rootId = ImGui::DockBuilderAddNode(dockId, ImGuiDockNodeFlags_DockSpace);\n";
    os << ctx.ind << "ImGui::DockBuilderSetNodeSize(rootId, dockSize);\n";

    os << ctx.ind << "/// @separator\n\n";

    ctx.parentVarName = "rootId";
    for (const auto& child : children)
        child->Export(os, ctx);

    os << ctx.ind << "/// @separator\n";

    os << ctx.ind << "ImGui::DockBuilderFinish(dockId);\n";
    ctx.ind_down();
    os << ctx.ind << "}\n\n";

    if (!style_preview.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_DockingPreview, " << style_preview.to_arg() << ");\n";
    if (!style_emptyBg.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, " << style_emptyBg.to_arg() << ");\n";

    os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 }); //fixes scrollbar issue\n";
    os << ctx.ind << "ImGui::DockSpace(dockId, dockSize, " << flags.to_arg() << ");\n";
    os << ctx.ind << "ImGui::PopStyleVar();\n";

    if (!style_preview.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_emptyBg.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";

    os << "\n";
    os << ctx.ind << "// Docked windows sample code\n";
    os << ctx.ind << "/*\n";
    for (const auto& child : children)
    {
        DockNode* chnode = dynamic_cast<DockNode*>(child.get());
        if (chnode) chnode->ExportHelp(os, ctx);
    }
    os << ctx.ind << "*/\n";
}

void DockSpace::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::DockSpace")
    {
        if (sit->params.size() >= 3)
            flags.set_from_arg(sit->params[2]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::CalcItemSize")
    {
        if (sit->params.size()) {
            auto sz = cpp::parse_size(sit->params[0]);
            size_x.set_from_arg(sz.first);
            size_y.set_from_arg(sz.second);
        }
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
    {
        if (sit->params.size() == 2 && sit->params[0] == "ImGuiCol_DockingPreview")
            style_preview.set_from_arg(sit->params[1]);
        if (sit->params.size() == 2 && sit->params[0] == "ImGuiCol_DockingEmptyBg")
            style_emptyBg.set_from_arg(sit->params[1]);
    }
}

std::vector<UINode::Prop>
DockSpace::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.dockingPreview", &style_preview },
        { "appearance.dockingEmptyBg", &style_emptyBg },
        { "behavior.flags##dockspace", &flags },
        });
    return props;
}

bool DockSpace::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("dockingPreview");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_preview, ImGuiCol_DockingPreview, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("dockingPreview", &style_preview, ctx);
        break;
    case 1:
        ImGui::Text("dockingEmptyBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_emptyBg, ImGuiCol_DockingEmptyBg, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("dockingEmptyBg", &style_emptyBg, ctx);
        break;
    case 2:
        changed = InputFlags("flags", &flags, Defaults().flags, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 3, ctx);
    }
    return changed;
}

//--------------------------------------------------------------------

DockNode::DockNode(UIContext& ctx)
{
    flags.add$(ImGuiDockNodeFlags_AutoHideTabBar);
    flags.add$(ImGuiDockNodeFlags_HiddenTabBar);
    flags.add$(ImGuiDockNodeFlags_NoCloseButton);
    flags.add$(ImGuiDockNodeFlags_NoDockingSplit);
    flags.add$(ImGuiDockNodeFlags_NoResize);
    flags.add$(ImGuiDockNodeFlags_NoTabBar);
    flags.add$(ImGuiDockNodeFlags_NoUndocking);
    flags.add$(ImGuiDockNodeFlags_NoWindowMenuButton);
    
    //splitDir.add$(ImGuiDir_None);
    splitDir.add$(ImGuiDir_Left);
    splitDir.add$(ImGuiDir_Right);
    splitDir.add$(ImGuiDir_Up);
    splitDir.add$(ImGuiDir_Down);
}

std::unique_ptr<Widget> DockNode::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<DockNode>(*this);
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImGuiID DockNode::SplitNode(ImGuiID parentId, UIContext& ctx)
{
    float ratio = splitRatio.eval(ctx);
    if (!ratio)
        ratio = DEFAULT_SPLIT_RATIO;
    ImGui::DockBuilderSplitNode(parentId, splitDir, ratio, &nodeId, &parentId);
    ImGui::DockBuilderGetNode(nodeId)->LocalFlags = flags | ImGuiDockNodeFlags_NoWindowMenuButton;

    if (children.empty())
    {
        std::istringstream is(labels=="" ? " " : labels);
        std::string label;
        while (std::getline(is, label)) {
            std::string id = "###" + label + std::to_string((uint64_t)this);
            ImGui::DockBuilderDockWindow(id.c_str(), nodeId);
        }
    }
    
    ImGuiID splitId = nodeId;
    for (const auto& child : children)
    {
        DockNode* chnode = dynamic_cast<DockNode*>(child.get());
        if (chnode)
            splitId = chnode->SplitNode(splitId, ctx);
    }

    return parentId;
}

void DockNode::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    ImGuiDockNode* node = ImGui::DockBuilderGetNode(nodeId);
    if (node) {
        cached_pos = node->Pos;
        cached_size = node->Size;
    }
}

void DockNode::CalcHash(uint32_t& hash)
{
    HashCombineData(hash, (ImGuiDir)splitDir);
    HashCombineData(hash, splitRatio.value());
    HashCombineData(hash, (int)flags);
    HashCombineData(hash, labels.value().size());

    for (const auto& child : children) {
        auto* node = dynamic_cast<DockNode*>(child.get());
        if (node)
            node->CalcHash(hash);
    }
}

ImDrawList* DockNode::DoDraw(UIContext& ctx)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (children.empty())
    {
        ImVec2 tsize = ImGui::CalcTextSize(labels.c_str());
        std::istringstream is(labels=="" ? " " : labels);
        std::string label;
        while (std::getline(is, label))
        {
            std::string id = label + "###" + label + std::to_string((uint64_t)this);
            //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
            //ImGui::PushStyleColor(ImGuiCol_TabSelected, 0xff00ff80);
            if (ImGui::Begin(id.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing))
            {
                //ImGui::PopStyleVar();
                dl = ImGui::GetWindowDrawList();
                
                if (!ctx.beingResized)
                {
                    ImGui::SetCursorScreenPos({
                        cached_pos.x + (cached_size.x - tsize.x) / 2,
                        cached_pos.y + (cached_size.y - tsize.y) / 2
                        });
                    ImGui::TextDisabled("%s", labels.c_str());
                }

                if (ctx.mode == UIContext::NormalSelection &&
                    ImGui::IsWindowHovered() && !ImGui::GetTopMostAndVisiblePopupModal())
                {
                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                        (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && !ImGui::IsKeyDown(ImGuiKey_RightCtrl)))
                    {
                        ctx.selected = { this };
                        ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left] = false;
                    }
                }
                bool allowed = !ImGui::GetTopMostAndVisiblePopupModal() &&
                    (ctx.activePopups.empty() || stx::count(ctx.activePopups, ImGui::GetCurrentWindow()));
                if (ctx.mode == UIContext::NormalSelection &&
                    allowed && ImGui::IsWindowHovered())
                {
                    ctx.hovered = this;
                }
            }
            ImGui::End();
            //ImGui::PopStyleColor();
        }
    }
    else
    {
        for (const auto& child : children)
            child->Draw(ctx);
    }

    return dl;
}

void DockNode::DoDrawTools(UIContext& ctx)
{
    if (ctx.parents.empty())
        return;

    assert(ctx.parents.back() == this);
    auto* parent = ctx.parents[ctx.parents.size() - 2];
    DockNode* pnode = dynamic_cast<DockNode*>(parent);

    ImGui::SetNextWindowPos(cached_pos, 0, { 0, 1.f });
    ImGui::Begin("extra", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoSavedSettings);
    
    ImGui::BeginDisabled(children.size());
    if (ImGui::Button("Split 2x"))
    {
        if (!pnode) //parent is DockSpace => split in 2
        {
            ImGuiDir newSplitDir = splitDir == ImGuiDir_Left || splitDir == ImGuiDir_Right ?
                ImGuiDir_Up : ImGuiDir_Left;
            auto chnode = std::make_unique<DockNode>(ctx);
            chnode->splitDir = newSplitDir;
            chnode->splitRatio = DEFAULT_SPLIT_RATIO;
            ctx.selected = { chnode.get() };
            children.push_back(std::move(chnode));

            chnode = std::make_unique<DockNode>(ctx);
            chnode->splitDir = newSplitDir;
            chnode->splitRatio = 0;
            children.push_back(std::move(chnode));
        }
        else //parent is DockNode => insert 1 sibling
        {
            auto chnode = std::make_unique<DockNode>(ctx);
            chnode->splitDir = splitDir;
            chnode->splitRatio = DEFAULT_SPLIT_RATIO;
            size_t i = stx::find_if(pnode->children, [this](const auto& node) { 
                return node.get() == this; 
                }) - pnode->children.begin();
            pnode->children.insert(pnode->children.begin() + i + 1, std::move(chnode));
        }
    }
    
    ImGui::EndDisabled();

    ImGui::End();
}

void DockNode::DoExport(std::ostream& os, UIContext& ctx)
{
    std::string nodeIdVar = "nodeId" + std::to_string(ctx.varCounter++);
    
    os << ctx.ind << "ImGuiID " << nodeIdVar << " = ImGui::DockBuilderSplitNode(" 
        << ctx.parentVarName << ", " << splitDir.to_arg() << ", " << splitRatio.to_arg() 
        << ", nullptr" << ", &" << ctx.parentVarName << ");\n";
    
    if (flags)
    {
        os << ctx.ind << "ImGui::DockBuilderGetNode(" << nodeIdVar << ")->LocalFlags = "
            << flags.to_arg() << ";\n";
    }

    if (children.empty())
    {
        if (labels.empty())
            ctx.errors.push_back("DockNode: label not set");

        std::istringstream is(labels);
        std::string label;
        while (std::getline(is, label))
        {
            os << ctx.ind << "ImGui::DockBuilderDockWindow(\"" << label
                << "\", " << nodeIdVar << ");\n";
        }
    }
    else
    {
        std::string tmp = ctx.parentVarName;
        ctx.parentVarName = nodeIdVar;
        os << ctx.ind << "/// @separator\n\n";

        for (const auto& child : children)
        {
            DockNode* chnode = dynamic_cast<DockNode*>(child.get());
            if (chnode)
                chnode->Export(os, ctx);
        }

        ctx.parentVarName = tmp;
    }
}

void DockNode::ExportHelp(std::ostream& os, UIContext& ctx)
{
    if (children.empty())
    {
        std::istringstream is(labels);
        std::string label;
        while (std::getline(is, label))
        {
            os << ctx.ind << "ImGui::Begin(\"" << label << "\");\n";
            os << ctx.ind << "ImGui::End();\n";
        }
    }
    else
    {
        for (const auto& child : children)
        {
            DockNode* chnode = dynamic_cast<DockNode*>(child.get());
            if (chnode)
                chnode->ExportHelp(os, ctx);
        }
    }
}

void DockNode::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::DockBuilderSplitNode")
    {
        labels = "";
        if (sit->params.size() >= 2)
            splitDir.set_from_arg(sit->params[1]);
        if (sit->params.size() >= 3)
            splitRatio.set_from_arg(sit->params[2]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::DockBuilderDockWindow")
    {
        if (sit->params.size() >= 1) {
            std::string tmp = labels;
            labels.set_from_arg(sit->params[0]);
            if (tmp.size())
                labels = tmp + "\n" + labels;
        }
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::DockBuilderGetNode")
    {
        size_t i = sit->line.find("LocalFlags=");
        if (i != std::string::npos)
            flags.set_from_arg(sit->line.substr(i + 11, sit->line.size() - i - 11));
    }
}

std::vector<UINode::Prop>
DockNode::Properties()
{
    std::vector<UINode::Prop> props;
    props.insert(props.begin(), {
        { "behavior.flags##docknode", &flags },
        { "behavior.splitDir##docknode", &splitDir },
        { "behavior.splitRatio##docknode", &splitRatio },
        { "behavior.labels##docknode", &labels, true },
        });
    return props;
}

bool DockNode::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        changed = InputFlags("flags", &flags, Defaults().flags, ctx);
        break;
    case 1:
        ImGui::Text("splitDir");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&splitDir, InputDirectVal_Modified, ctx);
        break;
   case 2:
        ImGui::Text("splitRatio");
        ImGui::TableNextColumn();
        ImGui::BeginDisabled(splitDir == ImGuiDir_None);
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = splitRatio != Defaults().splitRatio ? InputBindable_Modified : 0;
        changed = InputBindable(&splitRatio, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("splitRatio", &splitRatio, ctx);
        ImGui::EndDisabled();
        break;
   case 3:
   {
       ImGui::BeginDisabled(children.size());
       ImGui::Text("labels");
       ImGui::TableNextColumn();
       ImGui::PushFont(labels.empty() ? ctx.pgFont : ctx.pgbFont);
       size_t n = labels.empty() ? 0 : stx::count(*labels.access(), '\n') + 1;
       std::string label = "[" + std::to_string(n) + "]";
       if (ImRad::Selectable(label.c_str(), false, 0, { -ImGui::GetFrameHeight(), 0 }))
       {
           changed = true;
           comboDlg.title = "Window Names";
           comboDlg.value = labels;
           comboDlg.font = nullptr;
           comboDlg.OpenPopup([this](ImRad::ModalResult) {
               *labels.access() = Trim(comboDlg.value);
               });
       }
       ImGui::PopFont();
       ImGui::EndDisabled();
       break;
    }
    }
    return changed;
}

std::vector<UINode::Prop>
DockNode::Events()
{
    return {};
}