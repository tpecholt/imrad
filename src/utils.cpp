#include "utils.h"
#if WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#endif

void ShellExec(const std::string& path)
{
#ifdef WIN32
	ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
	system(("xdg-open " + path).c_str());
#endif
}
