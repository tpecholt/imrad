#include "uicontext.h"
#include "cppgen.h"

void UIContext::ind_up()
{
	ind += codeGen->GetIndent();
}

void UIContext::ind_down()
{
	if (ind.size() >= codeGen->GetIndent().size())
		ind.resize(ind.size() - codeGen->GetIndent().size());
}
