#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

namespace fs = std::filesystem;

inline std::vector<std::string> grabFiles(const std::string& folderPath) {
    std::vector<std::string> files;

    // Check if directory exists before iterating
    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        return files;
    }

    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            // Add folder path to filename
            fs::path fullPath = fs::path(folderPath) / entry.path().filename();
            files.push_back(fullPath.string());
        }
    }

    // Sort files alphabetically for consistent ordering across platforms
    std::sort(files.begin(), files.end());

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
        out.write(reinterpret_cast<const char*>(&t.room), sizeof(int));
    }
}

std::vector<Tile> load(const std::string& filename)
{
    std::vector<Tile> tiles;

    std::ifstream in(filename, std::ios::binary);
    if (!in) return tiles;

    in.seekg(0, std::ios::end);
    std::streamoff totalSize = in.tellg();
    in.seekg(0, std::ios::beg);

    uint32_t count;
    if (!in.read(reinterpret_cast<char*>(&count), sizeof(count)))
        return tiles;

    const std::streamoff headerSize = static_cast<std::streamoff>(sizeof(uint32_t));
    const std::streamoff legacyTileSize = static_cast<std::streamoff>(sizeof(float) * 2 + sizeof(int) * 2);
    const std::streamoff roomTileSize = static_cast<std::streamoff>(sizeof(float) * 2 + sizeof(int) * 3);
    const std::streamoff payloadSize = totalSize - headerSize;
    const std::streamoff expectedRoomPayload = static_cast<std::streamoff>(count) * roomTileSize;
    const bool hasRoom = (count > 0 && payloadSize >= expectedRoomPayload);

    tiles.reserve(count);

    for (uint32_t i = 0; i < count; i++)
    {
        Tile t;

        if (!in.read(reinterpret_cast<char*>(&t.pos.x), sizeof(float))) {
            break;
        }

        if (!in.read(reinterpret_cast<char*>(&t.pos.y), sizeof(float))) {
            break;
        }

        if (!in.read(reinterpret_cast<char*>(&t.id), sizeof(int))) {
            break;
        }

        if (!in.read(reinterpret_cast<char*>(&t.layer), sizeof(int))) {
            break;
        }

        if (hasRoom) {
            if (!in.read(reinterpret_cast<char*>(&t.room), sizeof(int))) {
                break;
            }
        } else {
            t.room = 0;
        }

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

std::vector<GLuint> loadTextures(std::vector<std::string> files) {
    std::vector<GLuint> textures;
    for (const std::string& path : files)
    {
        GLuint img = Image::Load(path.c_str());
        textures.push_back(img);
    }

    return textures;
}