#pragma once

#include <string>

// Returns the user-provided data directory when HMR_DATA_ROOT is set,
// otherwise the local PC data staged in this project.
std::string MacGameDataRoot();
bool MacGameDataIsComplete(const std::string& root, std::string* missingItem = nullptr);
