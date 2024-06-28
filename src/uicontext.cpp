#include "uicontext.h"
#include "cppgen.h"

void UIContext::ind_up()
{
	ind += codeGen->INDENT;
}

void UIContext::ind_down()
{
	if (ind.size() >= codeGen->INDENT.size())
		ind.resize(ind.size() - codeGen->INDENT.size());
}
