#include "uicontext.h"
#include "cppgen.h"

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