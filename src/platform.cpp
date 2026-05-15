#include "platform.h"
#include "utils.h"
#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <Windows.h>
  #define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
  #include <mach-o/dyld.h>
  #define GLFW_EXPOSE_NATIVE_COCOA
#else //linux
  #include <sys/inotify.h>
  #include <unistd.h>
  #define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <nfd.h>
#include <thread>
#include <filesystem>

std::thread stylesWatcher;


nfdwindowhandle_t GetNfdHandle()
{
    nfdwindowhandle_t h;
#ifdef _WIN32
    h.type = NFD_WINDOW_HANDLE_TYPE_WINDOWS;
    h.handle = glfwGetWin32Window(glfwWindow);
#elif defined(__APPLE__)
    h.type = NFD_WINDOW_HANDLE_TYPE_COCOA;
    h.handle = glfwGetCocoaWindow(glfwWindow);
#else
    h.type = NFD_WINDOW_HANDLE_TYPE_X11;
    h.handle = (void*)glfwGetX11Window(glfwWindow);
#endif
    return h;
}

std::string OpenFileDialog(const std::vector<const char*>& filters)
{
    nfdchar_t* outPath = NULL;
    std::vector<std::string> buf;
    buf.reserve(filters.size());
    std::vector<nfdfilteritem_t> filterItems;
    for (const char* filter : filters) {
        auto& tmp = buf.emplace_back(filter);
        size_t i = tmp.find("|");
        if (i != std::string::npos) {
            tmp[i] = 0;
            filterItems.push_back({ tmp.c_str(), tmp.c_str() + i + 1 });
        }
    }
    nfdopendialogu8args_t args{};
    args.filterList = filterItems.data();
    args.filterCount = (nfdfiltersize_t)filterItems.size();
    args.parentWindow = GetNfdHandle();
    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    if (result != NFD_OKAY)
        return "";

    std::string path = outPath;
    NFD_FreePath(outPath);
    return path;
}

std::string SaveFileDialog(const char* defaultName, const std::vector<const char*>& filters)
{
    nfdchar_t* outPath = NULL;
    std::vector<std::string> buf;
    buf.reserve(filters.size());
    std::vector<nfdfilteritem_t> filterItems;
    for (const char* filter : filters) {
        auto& tmp = buf.emplace_back(filter);
        size_t i = tmp.find("|");
        if (i != std::string::npos) {
            tmp[i] = 0;
            filterItems.push_back({ tmp.c_str(), tmp.c_str() + i + 1 });
        }
    }
    nfdsavedialogu8args_t args{};
    args.filterList = filterItems.data();
    args.filterCount = (nfdfiltersize_t)filterItems.size();
    args.defaultName = defaultName;
    args.parentWindow = GetNfdHandle();
    nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
    if (result != NFD_OKAY)
        return "";

    std::string path = outPath;
    NFD_FreePath(outPath);
    return path;
}

std::string GetRootPath()
{
#ifdef WIN32
    wchar_t tmp[1024];
    int n = GetModuleFileNameW(NULL, tmp, (int)std::size(tmp));
    return generic_u8string(fs::path(tmp).parent_path()); //need generic for CMake template path substitutions
    //test utf8 path: return u8string(L"c:/work/de�o/latest");
#elif defined(__APPLE__)
    char executablePath[PATH_MAX];
    uint32_t len = PATH_MAX;
    if (_NSGetExecutablePath(executablePath, &len) != 0) {
        std::cerr << "Error: _NSGetExecutablePath failed, fallback to current directory\n";
        return u8string(fs::current_path());
    }
    try {
        fs::path pexe = fs::canonical(u8path(executablePath));
        return u8string(pexe.parent_path());
    }
    catch (const std::exception& e) {
        std::cerr << "Error: fs::canonical failed: " << e.what() << ", fallback to current directory\n";
        return u8string(fs::current_path());
    }
#else
    fs::path pexe = fs::canonical("/proc/self/exe");
    return u8string(pexe.parent_path());
#endif
}

void InitStylesWatcher(std::function<void()> fun)
{
    std::string rootPath = GetRootPath();
#ifdef WIN32
    stylesWatcher = std::thread([rootPath,fun] {
        HANDLE hDir = CreateFileW(
            u8path(rootPath + "/style").wstring().c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL
        );
        if (hDir == INVALID_HANDLE_VALUE)
            return;
        DWORD bytesReturned;
        std::vector<std::byte> buffer(1024);
        while (programState != Shutdown) {
            if (ReadDirectoryChangesW(
                hDir,
                buffer.data(),
                (DWORD)buffer.size(),
                false,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                &bytesReturned,
                NULL,
                NULL
            )) {
                fun();
            }
        }
        });
    stylesWatcher.detach();
#elif defined(__APPLE__)
    //todo
#else
    stylesWatcher = std::thread([rootPath,fun] {
        int fd = inotify_init();
        if (fd < 0)
            return;
        int wd = inotify_add_watch(fd, u8path(rootPath + "/style").string().c_str(), IN_MODIFY);
        std::vector<std::byte> buffer(1024 * (sizeof(struct inotify_event) + 16));
        while (programState != Shutdown) {
            int len = read(fd, buffer.data(), buffer.size());
            if (len < 0)
                break;
            fun();
        }
        inotify_rm_watch(fd, wd);
        close(fd);
        });
    stylesWatcher.detach();
#endif
}