#include <string>
#include <vector>
#include <functional>

///\param filter "Headers|h,hpp"
std::string OpenFileDialog(const std::vector<const char*>& filters);
std::string SaveFileDialog(const char* defaultName, const std::vector<const char*>& filters);

std::string GetRootPath();

void InitStylesWatcher(std::function<void()>);