#include "setup.hpp"

#include <cstdlib>
#include <ctime>
#include <string>

extern void LoadTaskPositionOverrides(const std::string& filePath);

namespace {
const std::string kTaskPositionOverridesPath = "game/data/task_positions.dat";
}

void SetupCommunityApp()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    LoadTaskPositionOverrides(kTaskPositionOverridesPath);
}
