#pragma once

#include <filesystem>
#include <fstream>
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

void save(const std::vector<Tile>& tiles, const std::string& filename)
{
    std::ofstream out(filename, std::ios::binary);
    if (!out) return;

    uint32_t count = static_cast<uint32_t>(tiles.size());
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const Tile& t : tiles)
    {
        out.write(reinterpret_cast<const char*>(&t.pos.x), sizeof(float));
        out.write(reinterpret_cast<const char*>(&t.pos.y), sizeof(float));
        out.write(reinterpret_cast<const char*>(&t.id), sizeof(int));
        out.write(reinterpret_cast<const char*>(&t.layer), sizeof(int));
    }
}

std::vector<Tile> load(const std::string& filename)
{
    std::vector<Tile> tiles;

    std::ifstream in(filename, std::ios::binary);
    if (!in) return tiles;

    uint32_t count;
    if (!in.read(reinterpret_cast<char*>(&count), sizeof(count)))
        return tiles;

    tiles.reserve(count);

    for (uint32_t i = 0; i < count; i++)
    {
        Tile t;

        in.read(reinterpret_cast<char*>(&t.pos.x), sizeof(float));
        in.read(reinterpret_cast<char*>(&t.pos.y), sizeof(float));
        in.read(reinterpret_cast<char*>(&t.id), sizeof(int));
        in.read(reinterpret_cast<char*>(&t.layer), sizeof(int));

        tiles.push_back(t);
    }

    return tiles;
}

bool isEmpty(const std::string& filename)
{
    if (!std::filesystem::exists(filename))
        return true;

    return std::filesystem::file_size(filename) == 0;
}