#include "uicontext.h"
#include "cppgen.h"
#include "node_standard.h"

UIContext& UIContext::Defaults()
{
    static UIContext ctx;
    ctx.createVars = false;
    return ctx;
}

void UIContext::ind_up()
{
    ind += codeGen->INDENT;
}

void UIContext::ind_down()
{
    if (ind.size() >= codeGen->INDENT.size())
        ind.resize(ind.size() - codeGen->INDENT.size());
}

std::string UIContext::GetCurrentArray()
{
    for (size_t i = parents.size() - 1; i < parents.size(); --i) {
        const auto* parent = dynamic_cast<Widget*>(parents[i]);
        if (!parent)
            break;
        std::string id = parent->itemCount.container_var();
        if (id != "")
            return id;
    }
    return "";
}