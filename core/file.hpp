#pragma once

#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

inline std::vector<std::string> grabFiles(const std::string& folderPath) {
    std::vector<std::string> files;

    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            // Add folder path to filename
            fs::path fullPath = fs::path(folderPath) / entry.path().filename();
            files.push_back(fullPath.string());
        }
    }

    return files;
}