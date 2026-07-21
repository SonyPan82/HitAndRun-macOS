#include "MacGameData.h"

#include <cstdlib>
#include <filesystem>
#include <mach-o/dyld.h>

namespace
{
std::string BundleDataRoot()
{
    char executablePath[4096] = {};
    uint32_t length = sizeof(executablePath);
    if (_NSGetExecutablePath(executablePath, &length) != 0)
        return {};

    const std::filesystem::path executable(executablePath);
    return (executable.parent_path().parent_path() / "Resources" / "GameData").string();
}

bool IsComplete(const std::string& root)
{
    namespace fs = std::filesystem;
    return fs::exists(fs::path(root) / "Simpsons.exe") &&
           fs::exists(fs::path(root) / "art") &&
           fs::exists(fs::path(root) / "scripts") &&
           fs::exists(fs::path(root) / "sound");
}
}

std::string MacGameDataRoot()
{
    if (const char* configuredRoot = std::getenv("HMR_DATA_ROOT"); configuredRoot != nullptr && configuredRoot[0] != '\0')
        return configuredRoot;
    const std::string bundledRoot = BundleDataRoot();
    if (IsComplete(bundledRoot))
        return bundledRoot;
    return HMR_DEFAULT_DATA_ROOT;
}

bool MacGameDataIsComplete(const std::string& root, std::string* missingItem)
{
    namespace fs = std::filesystem;
    const char* required[] = { "Simpsons.exe", "art", "scripts", "sound" };
    for (const char* item : required)
        if (!fs::exists(fs::path(root) / item))
        {
            if (missingItem != nullptr) *missingItem = item;
            return false;
        }
    return true;
}
