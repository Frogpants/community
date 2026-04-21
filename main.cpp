#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#include <GLES/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <filesystem>
#include <fstream>
#include <functional>
#include <unordered_map>

#include "game/player.hpp"
#include "game/character.hpp"
#include "game/camera.hpp"
#ifdef __EMSCRIPTEN__
class MultiplayerClient {
public:
    void setServer(const std::string&, int) {}
    void setPlayerName(const std::string&) {}
    bool initOrJoin(const std::string&) { return false; }
    void shutdown() {}
    void sync(const vec2&) {}
    void drawRemotePlayers(GLuint) {}
    std::string getStatusText() const { return "multiplayer unavailable on web build"; }
};
#else
#include "game/multiplayer.hpp"
#endif
#include "game/tile.hpp"
#include "game/task.hpp"
#include "game/minigames.hpp"

#include "core/manager.hpp"
#include "core/essentials.hpp"
#include "core/collision.hpp"
#include "core/images/image.hpp"
#include "core/images/stb_image.h"
#include "core/net/http.hpp"
#include "core/text.hpp"
#include "core/file.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>

static std::function<void()>* gWebFrame = nullptr;

EM_JS(void, WebFrontendTaskSaved, (const char* characterName, const char* taskName, int room, int taskId, int completed), {
    if (window.CommunityTaskUI && typeof window.CommunityTaskUI.saveTask === "function") {
        window.CommunityTaskUI.saveTask(
            UTF8ToString(characterName),
            UTF8ToString(taskName),
            room,
            taskId,
            completed !== 0
        );
    }
});

EM_JS(void, WebFrontendTaskCompleted, (const char* characterName, const char* taskName, int room, int taskId), {
    if (window.CommunityTaskUI && typeof window.CommunityTaskUI.completeTask === "function") {
        window.CommunityTaskUI.completeTask(UTF8ToString(characterName), UTF8ToString(taskName), room, taskId);
    }
});

static void WebFrameTrampoline() {
    if (gWebFrame != nullptr) {
        (*gWebFrame)();
    }
}
#else
static void WebFrontendTaskSaved(const char* characterName, const char* taskName, int room, int taskId, int completed);
static void WebFrontendTaskCompleted(const char* characterName, const char* taskName, int room, int taskId);
#endif


// Game Assets

Player player;
Camera camera;

std::vector<Character> characters;
std::vector<Task> objectives;

struct Door {
    vec2 hubPos = vec2(0.0f);
    vec2 roomPos = vec2(0.0f);
    vec2 dim = vec2(32.0f, 64.0f);
    int roomId = 0;
};

std::vector<std::string> loadTileLibrary() {
    std::vector<std::string> files = grabFiles("dist/assets/tiles");
    std::vector<std::string> townFiles = grabFiles("dist/assets/town");
    files.insert(files.end(), townFiles.begin(), townFiles.end());

    if (!files.empty()) {
        return files;
    }

    files = grabFiles("assets/tiles");
    townFiles = grabFiles("assets/town");
    files.insert(files.end(), townFiles.begin(), townFiles.end());
    return files;
}

std::vector<std::string> loadAssetFiles(const std::string& relativePath) {
    std::vector<std::string> files = grabFiles("dist/assets/" + relativePath);
    if (!files.empty()) {
        return files;
    }
    return grabFiles("assets/" + relativePath);
}

struct HouseTilesetGroup {
    std::string name;
    int start = 0;
    std::vector<std::string> files;
    std::vector<GLuint> textures;
};

std::string getFileBaseName(const std::string& filePath) {
    return std::filesystem::path(filePath).filename().string();
}

std::vector<HouseTilesetGroup> loadHouseTilesets() {
    std::vector<HouseTilesetGroup> groups;

    groups.push_back(HouseTilesetGroup{
        "walls-floors",
        0,
        loadAssetFiles("house/walls-floors"),
        {}
    });

    groups.push_back(HouseTilesetGroup{
        "furniture",
        0,
        loadAssetFiles("house/furniture"),
        {}
    });

    groups.push_back(HouseTilesetGroup{
        "pets",
        0,
        loadAssetFiles("house/pets"),
        {}
    });

    int nextStart = 0;
    for (HouseTilesetGroup& group : groups) {
        group.start = nextStart;
        group.textures = loadTextures(group.files);
        nextStart += static_cast<int>(group.textures.size());
    }

    return groups;
}

std::vector<std::string> tileTex = loadTileLibrary();
std::vector<std::string> itemTex = loadAssetFiles("items");
std::vector<std::string> heartTex = loadAssetFiles("stats/health");

// Game Control Variables

std::string server = "localhost";

int tick = 1;

bool editor = false;
int mode = 0;
int selectMode = 0;
int selected = 0;
int tile = 0;
int tilesetTile = 0;
int platformTile = 0;
int houseTile = 0;
int editorTileSource = 0;
int tilesetStartId = 0;
int tilesetCount = 0;
int tilesetCols = 0;
int tilesetRows = 0;
int tilesetWidth = 0;
int tilesetHeight = 0;
const int tilesetTilePixels = 16;
GLuint tilesetTexture = 0;
int platformStartId = 0;
int platformCount = 0;
int platformCols = 0;
int platformRows = 0;
int platformWidth = 0;
int platformHeight = 0;
const int platformTilePixels = 16;
GLuint platformTexture = 0;
int houseStartId = 0;
int houseCount = 0;
int houseTilesetStartId = 0;
int houseTilesetCount = 0;
int houseTilesetGroup = 0;
int houseTilesetTile = 0;
std::vector<HouseTilesetGroup> houseTilesets;
bool editorNextRoomHeld = false;
bool editorPrevRoomHeld = false;
bool editorHintsOpen = false;
bool editorTaskPlacementMode = false;
int editorDraggedObjectiveIndex = -1;

const int treeSpawnMarkerTileId = -100;
const int collisionMarkerTileId = -101;

struct TreeProp {
    vec2 pos = vec2(0.0f);
    int room = 0;
};

const std::string fixedDoorTexturePath = "assets/door.png";
const vec2 fixedDoorPosition = vec2(128.0f, 32.0f);
const vec2 fixedDoorSize = vec2(32.0f, 32.0f);
const float doorSpacing = 112.0f;

float zoom = 2.0;

int running = 1;
bool inMainMenu = true;
bool multiplayerStarted = false;

std::string getEnvOrDefault(const char* key, const std::string& fallback) {
    const char* value = std::getenv(key);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }
    return std::string(value);
}

int getEnvIntOrDefault(const char* key, int fallback) {
    const char* value = std::getenv(key);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }

    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

#ifndef __EMSCRIPTEN__
namespace {
std::string EscapeJson(const std::string& raw) {
    std::string escaped;
    escaped.reserve(raw.size());

    for (char c : raw) {
        if (c == '"' || c == '\\') {
            escaped.push_back('\\');
        }
        escaped.push_back(c);
    }

    return escaped;
}

Http::Response PostTaskJson(const std::string& path, const std::string& jsonBody) {
    std::string host = getEnvOrDefault("COMMUNITY_BACKEND_HOST", server);
    int port = getEnvIntOrDefault("COMMUNITY_BACKEND_PORT", 8080);

    std::vector<std::string> hosts;
    hosts.push_back(host);
    if (host == "localhost") {
        hosts.push_back("127.0.0.1");
    } else if (host == "127.0.0.1") {
        hosts.push_back("localhost");
    }

    Http::Response response;
    for (const std::string& candidateHost : hosts) {
        response = Http::PostJson(candidateHost, port, path, jsonBody);
        if (response.statusCode >= 200 && response.statusCode < 300) {
            return response;
        }
    }

    return response;
}

void SyncTaskToBackend(const std::string& characterName, const std::string& taskName, int room, int taskId, bool completed) {
    std::ostringstream body;
    body << "{\"userName\":\"" << EscapeJson(characterName)
         << "\",\"taskName\":\"" << EscapeJson(taskName)
         << "\",\"room\":" << room
         << ",\"taskId\":" << taskId
         << ",\"completed\":" << (completed ? "true" : "false") << "}";

    const std::string path = completed ? "/api/tasks/complete" : "/api/tasks";
    Http::Response response = PostTaskJson(path, body.str());
    if (response.statusCode < 200 || response.statusCode >= 300) {
        std::cout << "Task sync failed (" << response.statusCode << ") for "
                  << taskName << " in room " << room << std::endl;
    } else if (completed) {
        std::cout << characterName << " completed " << taskName << std::endl;
    }
}
} // namespace

static void WebFrontendTaskSaved(const char* characterName, const char* taskName, int room, int taskId, int completed) {
    SyncTaskToBackend(
        characterName == nullptr ? "" : std::string(characterName),
        taskName == nullptr ? "" : std::string(taskName),
        room,
        taskId,
        completed != 0
    );
}

static void WebFrontendTaskCompleted(const char* characterName, const char* taskName, int room, int taskId) {
    SyncTaskToBackend(
        characterName == nullptr ? "" : std::string(characterName),
        taskName == nullptr ? "" : std::string(taskName),
        room,
        taskId,
        true
    );
}
#endif

std::string getRandomUsername() {
    const std::vector<std::string> adjectives = {
        "brisk", "calm", "clever", "fuzzy", "gentle", "lucky", "mellow", "nimble", "quiet", "rapid", "sunny", "zesty"
    };

    const std::vector<std::string> nouns = {
        "otter", "fox", "raven", "koala", "panda", "sparrow", "beaver", "lynx", "wolf", "badger", "falcon", "gecko"
    };

    int adjectiveIndex = randInt(0, static_cast<int>(adjectives.size()) - 1);
    int nounIndex = randInt(0, static_cast<int>(nouns.size()) - 1);
    int suffix = randInt(100, 999);

    return adjectives[adjectiveIndex] + "-" + nouns[nounIndex] + std::to_string(suffix);
}

std::vector<std::string> getCharacterTasks() {
    return {"wash dishes", "take out trash", "do laundry"};
}

Character makeCharacterForRoom(int roomId, int stageIndex) {
    Character c;
    c.room = roomId;

    int xBucket = ((roomId * 37) % 5) - 2;
    int yBucket = -2;
    c.pos = vec2(300.0f + static_cast<float>(xBucket * 80), static_cast<float>(yBucket * 64));
    c.target = c.pos;
    c.roamCenter = c.pos;
    c.name = "Character " + std::to_string(stageIndex + 1);
    c.level = 0;
    c.isRoaming = false;
    c.tasksGiven = 0;
    c.tasksCompleted = 0;
    c.nextStageSpawned = false;
    c.tasks = getCharacterTasks();
    return c;
}

Door makeDoorForRoom(int roomId, const std::vector<vec2>& hubDoorPositions) {
    Door door;
    door.roomId = roomId;
    if (roomId - 1 >= 0 && roomId - 1 < static_cast<int>(hubDoorPositions.size())) {
        door.hubPos = hubDoorPositions[roomId - 1];
    } else {
        door.hubPos = fixedDoorPosition + vec2(doorSpacing * static_cast<float>(roomId - 1), 0.0f);
    }
    door.roomPos = fixedDoorPosition;
    door.dim = fixedDoorSize;
    return door;
}

Character* getCharacterForRoom(std::vector<Character>& chars, int roomId) {
    for (Character& c : chars) {
        if (c.room == roomId) {
            return &c;
        }
    }
    return nullptr;
}

void spawnNextStage(std::vector<Character>& chars, std::vector<Door>& doors, int& nextRoomId, Character& completedCharacter, const std::vector<vec2>& hubDoorPositions) {
    if (completedCharacter.nextStageSpawned) {
        return;
    }

    const int roomId = nextRoomId;

    if (roomId - 1 < 0 || roomId - 1 >= static_cast<int>(hubDoorPositions.size())) {
        std::cout << "no more identifiable houses for another door" << std::endl;
        completedCharacter.nextStageSpawned = true;
        return;
    }

    nextRoomId += 1;

    doors.push_back(makeDoorForRoom(roomId, hubDoorPositions));
    chars.push_back(makeCharacterForRoom(roomId, roomId - 1));
    chars.back().texture = Image::Load("assets/npcs/character.png");
    completedCharacter.nextStageSpawned = true;
}

void drawTreeSpawnMarkerTile(const vec2& pos)
{
    const vec2 halfSize = vec2(32.0f);

    glPushMatrix();
    glTranslatef(pos.x, pos.y, 0.0f);
    glBindTexture(GL_TEXTURE_2D, 0);
    glColor4f(0.2f, 0.6f, 1.0f, 1.0f);
    glLineWidth(2.0f);

    glBegin(GL_LINE_LOOP);
        glVertex2f(-halfSize.x, -halfSize.y);
        glVertex2f( halfSize.x, -halfSize.y);
        glVertex2f( halfSize.x,  halfSize.y);
        glVertex2f(-halfSize.x,  halfSize.y);
    glEnd();

    glPopMatrix();
}

void drawCollisionMarkerTile(const vec2& pos)
{
    const vec2 halfSize = vec2(32.0f);

    glPushMatrix();
    glTranslatef(pos.x, pos.y, 0.0f);
    glBindTexture(GL_TEXTURE_2D, 0);
    glColor4f(1.0f, 0.2f, 0.2f, 1.0f);
    glLineWidth(2.0f);

    glBegin(GL_LINE_LOOP);
        glVertex2f(-halfSize.x, -halfSize.y);
        glVertex2f( halfSize.x, -halfSize.y);
        glVertex2f( halfSize.x,  halfSize.y);
        glVertex2f(-halfSize.x,  halfSize.y);
    glEnd();

    glPopMatrix();
}

bool collidesWithCollisionMarker(const vec2& pos, const vec2& dim, int room, const std::vector<Tile>& tiles)
{
    for (const Tile& tile : tiles) {
        if (tile.room != room || tile.id != collisionMarkerTileId) {
            continue;
        }

        if (BoxCollide(pos, dim, tile.pos, vec2(16.0f))) {
            return true;
        }
    }

    return false;
}

vec2 moveWithCollisionMarkers(const vec2& currentPos, const vec2& dim, const vec2& delta, int room, const std::vector<Tile>& tiles)
{
    vec2 resolvedPos = currentPos;

    vec2 tryX = vec2(currentPos.x + delta.x, currentPos.y);
    if (!collidesWithCollisionMarker(tryX, dim, room, tiles)) {
        resolvedPos.x = tryX.x;
    }

    vec2 tryY = vec2(resolvedPos.x, currentPos.y + delta.y);
    if (!collidesWithCollisionMarker(tryY, dim, room, tiles)) {
        resolvedPos.y = tryY.y;
    }

    return resolvedPos;
}

void spawnTreeOnMarkerForRoom(const std::vector<Tile>& tiles, std::vector<TreeProp>& trees, int room)
{
    std::vector<std::pair<vec2, int>> availableMarkers;

    for (const Tile& tile : tiles) {
        if (tile.id != treeSpawnMarkerTileId || tile.room != room) {
            continue;
        }

        bool alreadyOccupied = false;
        for (const TreeProp& tree : trees) {
            if (tree.room == room && tree.pos.x == tile.pos.x && tree.pos.y == tile.pos.y) {
                alreadyOccupied = true;
                break;
            }
        }

        if (!alreadyOccupied) {
            availableMarkers.push_back({tile.pos, tile.room});
        }
    }

    if (availableMarkers.empty()) {
        for (const Tile& tile : tiles) {
            if (tile.id != treeSpawnMarkerTileId) {
                continue;
            }

            bool alreadyOccupied = false;
            for (const TreeProp& tree : trees) {
                if (tree.room == tile.room && tree.pos.x == tile.pos.x && tree.pos.y == tile.pos.y) {
                    alreadyOccupied = true;
                    break;
                }
            }

            if (!alreadyOccupied) {
                availableMarkers.push_back({tile.pos, tile.room});
            }
        }
    }

    if (availableMarkers.empty()) {
        std::cout << "nowhere to put tree" << std::endl;
        return;
    }

    int markerIndex = randInt(0, static_cast<int>(availableMarkers.size()) - 1);
    TreeProp tree;
    tree.room = availableMarkers[markerIndex].second;
    tree.pos = availableMarkers[markerIndex].first;
    trees.push_back(tree);
}


std::vector<Tile> genWorld(vec2 dim) {
    std::vector<Tile> ts; // Nora shhhhh
    Tile t;
    for (int x = 0; x < dim.x; ++x) {
        for (int y = 0; y < dim.y; ++y) {
            if (randInt(0, 1) == 0) {
                t.id = 1;
            } else {
                t.id = randInt(2, 3);
            }
            t.pos = snap(vec2((x - dim.x/2.0)*64, (y - dim.y/2.0)*64), 64.0);
            ts.push_back(t);
        }
    }
    return ts;
}


vec2 GetMouseWorld(GLFWwindow* window) {
    vec2 world;

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    float mouseNDC_X = (float)Mouse::X() / windowWidth;
    float mouseNDC_Y = (float)Mouse::Y() / windowHeight;

    world.x = (mouseNDC_X * 2.0f - 1.0f) * screen.x / zoom + camera.pos.x;
    world.y = (1.0f - mouseNDC_Y * 2.0f) * screen.y / zoom + camera.pos.y;

    return world;
}

vec2 GetMouseUI(GLFWwindow* window) {
    vec2 ui;

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    float mouseNDC_X = static_cast<float>(Mouse::X()) / static_cast<float>(windowWidth);
    float mouseNDC_Y = static_cast<float>(Mouse::Y()) / static_cast<float>(windowHeight);

    ui.x = (mouseNDC_X * 2.0f - 1.0f) * screen.x / zoom;
    ui.y = (1.0f - mouseNDC_Y * 2.0f) * screen.y / zoom;

    return ui;
}

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    // Update the screen variable with new dimensions
    screen = vec2(width, height);
    
    // Update the OpenGL viewport
    glViewport(0, 0, width, height);
    
    // Update the projection matrix with new dimensions
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-screen.x / zoom, screen.x / zoom, -screen.y / zoom, screen.y / zoom, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}

bool addTaskForCharacter(Character& character, Player& localPlayer) {
    if (character.tasksGiven >= static_cast<int>(character.tasks.size())) {
        return false;
    }

    int taskId = character.tasksGiven;
    Task task;
    task.id = taskId;
    task.name = character.tasks[taskId];
    task.assignedBy = character.name;
    task.pos = GetTaskSpawnPosition(character.tasks[taskId], taskId);
    task.room = character.room;

    for (const Task& existing : objectives) {
        if (existing.room == task.room && existing.id == task.id) {
            return false;
        }
    }

    objectives.push_back(task);

    localPlayer.tasks.push_back(character.tasks[taskId]);
    character.tasksGiven += 1;
    WebFrontendTaskSaved(task.assignedBy.c_str(), task.name.c_str(), task.room, task.id, 0);
    return true;
}

void EnsureObjectiveExists(const Character& character, int taskId) {
    if (taskId < 0 || taskId >= static_cast<int>(character.tasks.size())) {
        return;
    }

    for (const Task& existing : objectives) {
        if (existing.room == character.room && existing.id == taskId) {
            return;
        }
    }

    Task task;
    task.id = taskId;
    task.name = character.tasks[taskId];
    task.pos = GetTaskSpawnPosition(character.tasks[taskId], taskId);
    task.room = character.room;
    objectives.push_back(task);
}

std::string SerializeTaskProgress(const std::vector<Character>& chars) {
    std::ostringstream out;
    bool first = true;
    for (const Character& c : chars) {
        if (!first) {
            out << ",";
        }
        out << c.room << ":" << c.tasksGiven << ":" << c.tasksCompleted;
        first = false;
    }
    return out.str();
}

void RemoveCompletedObjectivesForRoom(int room, int completedCount) {
    objectives.erase(
        std::remove_if(
            objectives.begin(),
            objectives.end(),
            [room, completedCount](const Task& task) {
                return task.room == room && task.id < completedCount;
            }
        ),
        objectives.end()
    );
}

void ApplyRemoteTaskProgressState(const std::string& serializedState,
                                  std::vector<Character>& chars,
                                  const std::vector<Tile>& tiles,
                                  std::vector<TreeProp>& trees,
                                  std::vector<Door>& doors,
                                  int& nextRoomId,
                                  const std::vector<vec2>& hubDoorPositions) {
    if (serializedState.empty()) {
        return;
    }

    std::stringstream stream(serializedState);
    std::string token;
    while (std::getline(stream, token, ',')) {
        std::size_t separator = token.find(':');
        if (separator == std::string::npos || separator == 0 || separator + 1 >= token.size()) {
            continue;
        }

        std::size_t secondSeparator = token.find(':', separator + 1);

        int roomId = 0;
        int givenCount = 0;
        int completedCount = 0;
        try {
            roomId = std::stoi(token.substr(0, separator));
            if (secondSeparator != std::string::npos) {
                givenCount = std::stoi(token.substr(separator + 1, secondSeparator - separator - 1));
                completedCount = std::stoi(token.substr(secondSeparator + 1));
            } else {
                completedCount = std::stoi(token.substr(separator + 1));
                givenCount = completedCount;
            }
        } catch (...) {
            continue;
        }

        Character* character = getCharacterForRoom(chars, roomId);
        if (character == nullptr) {
            continue;
        }

        int maxTasks = static_cast<int>(character->tasks.size());
        givenCount = std::max(0, std::min(givenCount, maxTasks));
        completedCount = std::max(0, std::min(completedCount, maxTasks));
        if (completedCount > givenCount) {
            givenCount = completedCount;
        }

        bool changed = false;
        if (givenCount > character->tasksGiven) {
            character->tasksGiven = givenCount;
            changed = true;
        }

        if (completedCount > character->tasksCompleted) {
            character->tasksCompleted = completedCount;
            changed = true;
        }

        if (!changed) {
            continue;
        }

        character->tasksGiven = std::max(character->tasksGiven, character->tasksCompleted);
        character->level = character->tasksCompleted * 30;

        RemoveCompletedObjectivesForRoom(character->room, character->tasksCompleted);
        for (int taskId = character->tasksCompleted; taskId < character->tasksGiven; ++taskId) {
            EnsureObjectiveExists(*character, taskId);
        }

        if (character->tasksCompleted >= maxTasks && !character->isRoaming) {
            character->isRoaming = true;
            spawnTreeOnMarkerForRoom(tiles, trees, character->room);
            spawnNextStage(chars, doors, nextRoomId, *character, hubDoorPositions);
        }
    }
}


int main()
{
    srand((unsigned int)time(nullptr));

    LoadTaskPositionOverrides(taskPositionOverridesPath);

    std::vector<Tile> tiles;
    tiles = genWorld(vec2(120,100));

    if (!isEmpty("game/data/map.dat")) {
        std::vector<Tile> lst = load("game/data/map.dat");
        tiles.insert(tiles.end(), lst.begin(), lst.end());
    }

    if (!glfwInit())
        return -1;

    GLFWwindow* window = glfwCreateWindow(screen.x, screen.y, "Community", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Load and set window icon
    int iconWidth, iconHeight, iconChannels;
    unsigned char* iconPixels = stbi_load("assets/icon.png", &iconWidth, &iconHeight, &iconChannels, 4);
    if (iconPixels) {
        GLFWimage icon;
        icon.width = iconWidth;
        icon.height = iconHeight;
        icon.pixels = iconPixels;
        glfwSetWindowIcon(window, 1, &icon);
        stbi_image_free(iconPixels);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Register window resize callback
    glfwSetWindowSizeCallback(window, windowResizeCallback);
    
    // Initialize screen variable with actual window size
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    screen = vec2(windowWidth, windowHeight);

    Manager::Init(window);
    Image::Init();
    if (!Text::Init()) {
        std::cout << "Failed to initialize text glyph textures." << std::endl;
    }

    std::vector<GLuint> tileTextures = loadTextures(tileTex);
    std::vector<GLuint> itemTextures = loadTextures(itemTex);
    std::vector<GLuint> heartTextures = loadTextures(heartTex);
    std::vector<GLuint> houseTextures;
    std::vector<vec2> houseFootprints;

    tilesetStartId = static_cast<int>(tileTextures.size());
    tilesetTexture = Image::Load("dist/assets/tileset/tileset1.png", false);
    if (tilesetTexture == 0) {
        tilesetTexture = Image::Load("assets/tileset/tileset1.png");
    }

    if (tilesetTexture != 0 && Image::GetTextureSize(tilesetTexture, tilesetWidth, tilesetHeight)) {
        tilesetCols = tilesetWidth / tilesetTilePixels;
        tilesetRows = tilesetHeight / tilesetTilePixels;
        tilesetCount = tilesetCols * tilesetRows;
    }

    platformStartId = tilesetStartId + tilesetCount;
    platformTexture = Image::Load("dist/assets/PlatformerPack/OverWorldPropsTowns.png", false);
    if (platformTexture == 0) {
        platformTexture = Image::Load("assets/PlatformerPack/OverWorldPropsTowns.png");
    }

    if (platformTexture != 0 && Image::GetTextureSize(platformTexture, platformWidth, platformHeight)) {
        platformCols = platformWidth / platformTilePixels;
        platformRows = platformHeight / platformTilePixels;
        platformCount = platformCols * platformRows;
    }

    houseStartId = platformStartId + platformCount;
    std::vector<std::string> houseFiles = {
        "PlatformerPack/Objects/Houses/House.png",
        "PlatformerPack/Objects/Houses/Inn.png",
        "PlatformerPack/Objects/Houses/Larger House.png",
        "PlatformerPack/Objects/Houses/Smaller House.png"
    };

    for (const std::string& houseFile : houseFiles) {
        GLuint houseTexture = Image::Load(("dist/assets/" + houseFile).c_str(), false);
        if (houseTexture == 0) {
            houseTexture = Image::Load(("assets/" + houseFile).c_str());
        }

        int houseWidth = 0;
        int houseHeight = 0;
        if (houseTexture != 0 && Image::GetTextureSize(houseTexture, houseWidth, houseHeight)) {
            houseTextures.push_back(houseTexture);
            houseFootprints.push_back(vec2(static_cast<float>(houseWidth / 16), static_cast<float>(houseHeight / 16)));
        }
    }
    houseCount = static_cast<int>(houseTextures.size());

    houseTilesets = loadHouseTilesets();
    houseTilesetStartId = houseStartId + houseCount;
    houseTilesetCount = 0;
    for (const HouseTilesetGroup& group : houseTilesets) {
        houseTilesetCount += static_cast<int>(group.textures.size());
    }

    auto getHubHouseDoorPositions = [&](const std::vector<Tile>& mapTiles) {
        std::vector<vec2> positions;

        for (const Tile& tile : mapTiles) {
            if (tile.room != 0) {
                continue;
            }

            if (tile.id < houseStartId || tile.id >= houseStartId + houseCount) {
                continue;
            }

            int houseIndex = tile.id - houseStartId;
            if (houseIndex < 0 || houseIndex >= static_cast<int>(houseFootprints.size())) {
                continue;
            }

            vec2 footprint = houseFootprints[houseIndex];
            vec2 drawHalf = vec2(footprint.x * 32.0f, footprint.y * 32.0f);
            vec2 drawPos = tile.pos + vec2((footprint.x - 1.0f) * 32.0f, (footprint.y - 1.0f) * 32.0f);

            float rightEdge = drawPos.x + drawHalf.x;
            float bottomEdge = drawPos.y - drawHalf.y;
            vec2 doorPos = vec2(rightEdge - 64.0f, bottomEdge + 32.0f);
            positions.push_back(doorPos);
        }

        std::sort(positions.begin(), positions.end(), [](const vec2& a, const vec2& b) {
            return a.x < b.x;
        });

        return positions;
    };

    std::vector<vec2> hubDoorPositions = getHubHouseDoorPositions(tiles);


    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-screen.x / zoom, screen.x / zoom, -screen.y / zoom, screen.y / zoom, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    player.texture = Image::Load("assets/agent-bullet.png");
    characters.push_back(makeCharacterForRoom(1, 0));
    characters[0].texture = Image::Load("assets/npcs/character.png");

    std::vector<Door> doors;
    if (!hubDoorPositions.empty()) {
        doors.push_back(makeDoorForRoom(1, hubDoorPositions));
    } else {
        std::cout << "no identifiable houses found for door placement" << std::endl;
    }
    int nextRoomId = 2;

    GLuint taskTex = Image::Load("assets/box.png");
    GLuint treeTex = Image::Load("dist/assets/tree.png", false);
    if (treeTex == 0) {
        treeTex = Image::Load("assets/tree.png");
    }
    GLuint pause = Image::Load("assets/pause-improved.png");
    GLuint deleteTex = Image::Load("assets/delete.png");
    GLuint selectTex = Image::Load("assets/interact-select.png");
    GLuint fixedDoorTex = Image::Load(fixedDoorTexturePath.c_str());
    std::vector<TreeProp> spawnedTrees;
    std::unordered_map<int, vec2> roomReturnPositions;

    // Main Menu

    GLuint playButton = Image::Load("assets/menu/play-button.png");

    MultiplayerClient multiplayer;
    multiplayer.setServer(getEnvOrDefault("COMMUNITY_BACKEND_HOST", server.c_str()), getEnvIntOrDefault("COMMUNITY_BACKEND_PORT", 8080));
    gLocalPlayerName = getEnvOrDefault("COMMUNITY_PLAYER_NAME", getRandomUsername());
    multiplayer.setPlayerName(gLocalPlayerName);
    std::string requestedRoomCode = getEnvOrDefault("COMMUNITY_ROOM_CODE", "");

    UI::Clear();
    Menu& mainMenu = UI::CreateMenu("main-menu");

    UiPanel& menuBackground = UI::AddPanel(
        mainMenu,
        "menu-background",
        vec2(0.0f, 0.0f),
        vec2(screen.x / zoom, screen.y / zoom),
        vec4(0.94f, 0.97f, 0.92f, 1.0f)
    );
    menuBackground.dynamicDim = [&]() {
        return vec2(screen.x / zoom, screen.y / zoom);
    };

    UiLabel& menuTitle = UI::AddLabel(mainMenu, "menu-title", "welcome to your community", vec2(0.0f, 180.0f), 42.0f / zoom, true);
    menuTitle.dynamicPos = [&]() {
        return vec2(0.0f, 180.0f + 5.0f * std::sin(timer * 0.08f));
    };

    UI::AddLabel(mainMenu, "menu-subtitle", "click play to start", vec2(0.0f, -220.0f), 18.0f / zoom, true);
    UI::AddLabel(mainMenu, "menu-footnote", "idiot", vec2(0.0f, -240.0f), 9.0f / zoom, true);

    Button& playMenuButton = UI::AddButton(mainMenu, "play", "play", vec2(0.0f, -80.0f), vec2(140.0f, 70.0f), playButton);
    playMenuButton.labelSize = 28.0f / zoom;
    playMenuButton.labelSpacing = 1.3f;
    playMenuButton.dynamicPos = [&]() {
        return vec2(0.0f, -80.0f + 10.0f * std::sin(timer * 0.1f));
    };
    playMenuButton.onClick = [&]() {
        inMainMenu = false;
        if (!multiplayerStarted) {
            multiplayer.initOrJoin(requestedRoomCode);
            multiplayerStarted = true;
        }
    };

    auto frame = [&]() {
        if (glfwWindowShouldClose(window)) {
        #ifndef __EMSCRIPTEN__
            return;
        #else
            emscripten_cancel_main_loop();
            return;
        #endif
        }

        glfwPollEvents();

        glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glLoadIdentity();

        timer += 0.01;

        if (inMainMenu) {
            mainMenu.visible = true;
            mainMenu.enabled = true;
            UI::Draw();

            Manager::Update();
            glfwSwapBuffers(window);
            return;
        }

        mainMenu.visible = false;
        mainMenu.enabled = false;

        glTranslatef(-camera.pos.x, -camera.pos.y, 0);

        auto drawFromSheet = [](GLuint texture, int atlasWidth, int atlasHeight, int cols, int tilePixels, int index, vec2 pos) {
            if (texture == 0 || atlasWidth <= 0 || atlasHeight <= 0 || cols <= 0 || tilePixels <= 0 || index < 0) {
                return;
            }

            int sheetX = index % cols;
            int sheetY = index / cols;

            float texelU = 0.5f / static_cast<float>(atlasWidth);
            float texelV = 0.5f / static_cast<float>(atlasHeight);

            float u0 = static_cast<float>(sheetX * tilePixels) / static_cast<float>(atlasWidth) + texelU;
            float v0 = static_cast<float>(sheetY * tilePixels) / static_cast<float>(atlasHeight) + texelV;
            float u1 = static_cast<float>((sheetX + 1) * tilePixels) / static_cast<float>(atlasWidth) - texelU;
            float v1 = static_cast<float>((sheetY + 1) * tilePixels) / static_cast<float>(atlasHeight) - texelV;

            Image::DrawRegion(texture, pos, 32, u0, v0, u1, v1, 0.0);
        };

        auto drawHousePrefab = [&](const Tile& tile, bool render, vec2* outDrawPos, vec2* outDrawHalf) {
            if (tile.id < houseStartId || tile.id >= houseStartId + houseCount) {
                return false;
            }

            int houseIndex = tile.id - houseStartId;
            if (houseIndex < 0 || houseIndex >= houseCount || houseTextures[houseIndex] == 0) {
                return false;
            }

            vec2 footprint = houseFootprints[houseIndex];
            vec2 drawHalf = vec2(footprint.x * 32.0f, footprint.y * 32.0f);
            vec2 drawPos = tile.pos + vec2((footprint.x - 1.0f) * 32.0f, (footprint.y - 1.0f) * 32.0f);

            if (render) {
                Image::Draw(houseTextures[houseIndex], drawPos, drawHalf);
            }

            if (outDrawPos != nullptr) {
                *outDrawPos = drawPos;
            }
            if (outDrawHalf != nullptr) {
                *outDrawHalf = drawHalf;
            }

            return true;
        };

        auto drawHouseTilesetTile = [&](int tileId, const vec2& pos) {
            if (tileId < houseTilesetStartId || tileId >= houseTilesetStartId + houseTilesetCount) {
                return false;
            }

            int localIndex = tileId - houseTilesetStartId;
            for (const HouseTilesetGroup& group : houseTilesets) {
                int groupCount = static_cast<int>(group.textures.size());
                if (groupCount <= 0) {
                    continue;
                }

                if (localIndex >= group.start && localIndex < group.start + groupCount) {
                    int groupIndex = localIndex - group.start;
                    if (groupIndex >= 0 && groupIndex < static_cast<int>(group.textures.size()) && group.textures[groupIndex] != 0) {
                        Image::Draw(group.textures[groupIndex], pos, 32);
                        return true;
                    }
                }
            }

            return false;
        };

        vec2 mouse = GetMouseWorld(window);
        int i = 0;
        selected = -1;
        for (const Tile& t : tiles) {
            if (t.room != player.room) {
                ++i;
                continue;
            }

            vec2 drawPos = t.pos;
            vec2 drawHalf = vec2(32.0f);
            bool isHouse = drawHousePrefab(t, false, &drawPos, &drawHalf);

            if (abs(drawPos.x - camera.pos.x) < (screen.x / zoom) + drawHalf.x) {
                if (abs(drawPos.y - camera.pos.y) < (screen.y / zoom) + drawHalf.y) {
                    if (t.id == treeSpawnMarkerTileId) {
                        if (mode) {
                            drawTreeSpawnMarkerTile(t.pos);
                        }
                    } else if (t.id == collisionMarkerTileId) {
                        if (mode) {
                            drawCollisionMarkerTile(t.pos);
                        }
                    } else if (isHouse) {
                        drawHousePrefab(t, true, nullptr, nullptr);
                    } else if (t.id >= 0 && t.id < static_cast<int>(tileTextures.size())) {
                        Image::Draw(tileTextures[t.id], t.pos, 32, 0.0);
                    } else if (tilesetTexture != 0 && t.id >= tilesetStartId && t.id < tilesetStartId + tilesetCount && tilesetCols > 0 && tilesetRows > 0) {
                        drawFromSheet(tilesetTexture, tilesetWidth, tilesetHeight, tilesetCols, tilesetTilePixels, t.id - tilesetStartId, t.pos);
                    } else if (platformTexture != 0 && t.id >= platformStartId && t.id < platformStartId + platformCount && platformCols > 0 && platformRows > 0) {
                        drawFromSheet(platformTexture, platformWidth, platformHeight, platformCols, platformTilePixels, t.id - platformStartId, t.pos);
                    } else if (drawHouseTilesetTile(t.id, t.pos)) {
                        // rendered from the new house tileset library
                    }

                    if (BoxCollide(mouse, vec2(0.0), drawPos, drawHalf)) {
                        selected = i;
                    }
                }
            }
            ++i;
        }

        if (fixedDoorTex != 0) {
            for (const Door& door : doors) {
                if (player.room == 0) {
                    Image::Draw(fixedDoorTex, door.hubPos, door.dim);
                } else if (player.room == door.roomId) {
                    Image::Draw(fixedDoorTex, door.roomPos, door.dim);
                }
            }
        }

        if (treeTex != 0) {
            for (const TreeProp& tree : spawnedTrees) {
                if (tree.room == player.room) {
                    Image::Draw(treeTex, tree.pos, 64.0f);
                }
            }
        }

        if (running) {
            tick += 1;

            if(Input::IsPressed("0")) {
                mode = 1 - mode;
            }

            HouseTilesetGroup* activeHouseTilesetGroup = nullptr;

            if (mode) {
                camera.controls();

                bool nextRoomDown = Input::IsPressed("e");
                bool prevRoomDown = Input::IsPressed("q");

                if (nextRoomDown) {
                    player.room += 1;
                }

                if (prevRoomDown) {
                    player.room = std::max(0, player.room - 1);
                }

                if (Input::IsPressed("1")) {
                    selectMode = 0;
                }

                if (Input::IsPressed("2")) {
                    selectMode = 1;
                }

                if (Input::IsPressed("3")) {
                    editorTileSource = (editorTileSource == 1) ? 0 : 1;
                }

                if (Input::IsPressed("4")) {
                    editorTileSource = (editorTileSource == 2) ? 0 : 2;
                }

                if (Input::IsPressed("5")) {
                    editorTileSource = (editorTileSource == 3) ? 0 : 3;
                }

                if (Input::IsPressed("6")) {
                    editorTileSource = (editorTileSource == 4) ? 0 : 4;
                }

                if (Input::IsPressed("7")) {
                    editorTileSource = (editorTileSource == 5) ? 0 : 5;
                }

                if (Input::IsPressed("8")) {
                    editorTileSource = (editorTileSource == 6) ? 0 : 6;
                }

                if (Input::IsPressed("t")) {
                    editorTaskPlacementMode = !editorTaskPlacementMode;
                    if (!editorTaskPlacementMode) {
                        editorDraggedObjectiveIndex = -1;
                    }
                }

                if (editorTileSource == 6 && !houseTilesets.empty()) {
                    if (Input::IsPressed("9")) {
                        houseTilesetGroup += 1;
                        if (houseTilesetGroup >= static_cast<int>(houseTilesets.size())) {
                            houseTilesetGroup = 0;
                        }
                        houseTilesetTile = 0;
                    }

                    if (houseTilesetGroup < 0) {
                        houseTilesetGroup = 0;
                    } else if (houseTilesetGroup >= static_cast<int>(houseTilesets.size())) {
                        houseTilesetGroup = static_cast<int>(houseTilesets.size()) - 1;
                    }

                    activeHouseTilesetGroup = &houseTilesets[houseTilesetGroup];
                }

                Tile t;
                t.pos = snap(mouse + 32, 64.0);
                t.room = player.room;

                int tileCount = static_cast<int>(tileTextures.size());
                int tileDelta = 0;

                if (Input::IsPressed("x")) {
                    tileDelta += 1;
                }

                if (Input::IsPressed("z")) {
                    tileDelta -= 1;
                }

                if (Mouse::ScrollY() > 0.0) {
                    tileDelta += 1;
                } else if (Mouse::ScrollY() < 0.0) {
                    tileDelta -= 1;
                }

                if (editorTileSource == 1 && tilesetCount > 0) {
                    tilesetTile += tileDelta;
                } else if (editorTileSource == 2 && platformCount > 0) {
                    platformTile += tileDelta;
                } else if (editorTileSource == 3 && houseCount > 0) {
                    houseTile += tileDelta;
                } else if (editorTileSource == 6 && activeHouseTilesetGroup != nullptr && !activeHouseTilesetGroup->textures.empty()) {
                    houseTilesetTile += tileDelta;
                } else {
                    tile += tileDelta;
                }

                if (tileCount > 0) {
                    if (tile < 0) {
                        tile = tileCount - 1;
                    } else if (tile >= tileCount) {
                        tile = 0;
                    }
                } else {
                    tile = 0;
                }

                if (tilesetCount > 0) {
                    if (tilesetTile < 0) {
                        tilesetTile = tilesetCount - 1;
                    } else if (tilesetTile >= tilesetCount) {
                        tilesetTile = 0;
                    }
                } else {
                    tilesetTile = 0;
                }

                if (platformCount > 0) {
                    if (platformTile < 0) {
                        platformTile = platformCount - 1;
                    } else if (platformTile >= platformCount) {
                        platformTile = 0;
                    }
                } else {
                    platformTile = 0;
                }

                if (houseCount > 0) {
                    if (houseTile < 0) {
                        houseTile = houseCount - 1;
                    } else if (houseTile >= houseCount) {
                        houseTile = 0;
                    }
                } else {
                    houseTile = 0;
                }

                if (activeHouseTilesetGroup != nullptr && !activeHouseTilesetGroup->textures.empty()) {
                    if (houseTilesetTile < 0) {
                        houseTilesetTile = static_cast<int>(activeHouseTilesetGroup->textures.size()) - 1;
                    } else if (houseTilesetTile >= static_cast<int>(activeHouseTilesetGroup->textures.size())) {
                        houseTilesetTile = 0;
                    }
                } else {
                    houseTilesetTile = 0;
                }

                if (editorTileSource == 1 && tilesetCount > 0) {
                    t.id = tilesetStartId + tilesetTile;
                } else if (editorTileSource == 2 && platformCount > 0) {
                    t.id = platformStartId + platformTile;
                } else if (editorTileSource == 3 && houseCount > 0) {
                    t.id = houseStartId + houseTile;
                } else if (editorTileSource == 6 && activeHouseTilesetGroup != nullptr && !activeHouseTilesetGroup->textures.empty()) {
                    t.id = houseTilesetStartId + activeHouseTilesetGroup->start + houseTilesetTile;
                } else if (editorTileSource == 4) {
                    t.id = treeSpawnMarkerTileId;
                } else if (editorTileSource == 5) {
                    t.id = collisionMarkerTileId;
                } else {
                    t.id = tile;
                }

                if (editorTaskPlacementMode) {
                    int hoveredObjective = -1;
                    for (int objectiveIndex = 0; objectiveIndex < static_cast<int>(objectives.size()); ++objectiveIndex) {
                        const Task& objective = objectives[objectiveIndex];
                        if (objective.room != player.room) {
                            continue;
                        }

                        if (BoxCollide(mouse, vec2(0.0f), objective.pos, objective.dim)) {
                            hoveredObjective = objectiveIndex;
                            break;
                        }
                    }

                    vec2 snappedTaskPos = snap(mouse + 32.0f, 64.0f);
                    Image::Draw(selectTex, snappedTaskPos, 32);

                    if (Mouse::IsPressed(0) && hoveredObjective != -1) {
                        editorDraggedObjectiveIndex = hoveredObjective;
                    }

                    if (editorDraggedObjectiveIndex >= 0 && editorDraggedObjectiveIndex < static_cast<int>(objectives.size())) {
                        Task& draggedObjective = objectives[editorDraggedObjectiveIndex];

                        if (draggedObjective.room == player.room && Mouse::IsDown(0)) {
                            draggedObjective.pos = snappedTaskPos;
                        }

                        Image::DrawRect(draggedObjective.pos, draggedObjective.dim, 0.2f, 0.9f, 0.2f, 1.0f, 0.0f);

                        if (Mouse::IsReleased(0)) {
                            UpdateTaskPositionOverride(draggedObjective);
                            SaveTaskPositionOverrides(taskPositionOverridesPath);
                            editorDraggedObjectiveIndex = -1;
                        }
                    } else {
                        editorDraggedObjectiveIndex = -1;
                    }

                    if (hoveredObjective != -1) {
                        Image::DrawRect(objectives[hoveredObjective].pos, objectives[hoveredObjective].dim, 0.2f, 0.7f, 1.0f, 1.0f, 0.0f);
                    }
                } else {
                    if (selectMode == 0 && tileCount > 0) {
                        if (editorTileSource == 4) {
                            drawTreeSpawnMarkerTile(t.pos);
                        } else if (editorTileSource == 5) {
                            drawCollisionMarkerTile(t.pos);
                        } else if (editorTileSource == 1 && tilesetCount > 0 && tilesetTexture != 0 && tilesetCols > 0 && tilesetRows > 0) {
                            drawFromSheet(tilesetTexture, tilesetWidth, tilesetHeight, tilesetCols, tilesetTilePixels, tilesetTile, t.pos);
                        } else if (editorTileSource == 2 && platformCount > 0 && platformTexture != 0 && platformCols > 0 && platformRows > 0) {
                            drawFromSheet(platformTexture, platformWidth, platformHeight, platformCols, platformTilePixels, platformTile, t.pos);
                        } else if (editorTileSource == 3 && houseCount > 0) {
                            drawHousePrefab(t, true, nullptr, nullptr);
                        } else if (editorTileSource == 6 && activeHouseTilesetGroup != nullptr && !activeHouseTilesetGroup->textures.empty()) {
                            Image::Draw(activeHouseTilesetGroup->textures[houseTilesetTile], t.pos, 32);
                        } else {
                            Image::Draw(tileTextures[tile], t.pos, 32);
                        }
                    } else {
                        Image::Draw(deleteTex, t.pos, 32);
                    }

                    if (Mouse::IsPressed(0)) {
                        if (selectMode == 0) {
                            tiles.push_back(t);
                        } else {
                            if (selected != -1) {
                                tiles.erase(tiles.begin() + selected);
                            }
                        }

                        save(tiles, "game/data/map.dat");
                    } else if (Mouse::IsDown(1)) {
                        if (selectMode == 0) {
                            tiles.push_back(t);
                        } else {
                            if (selected != -1) {
                                tiles.erase(tiles.begin() + selected);
                            }
                        }

                        save(tiles, "game/data/map.dat");
                    }
                }

            } else {
                camera.target = player.pos;
                camera.follow();

                if (!Minigames::IsTaskOpen()) {
                    player.controls();
                    vec2 oldPlayerPos = player.pos;
                    player.pos = moveWithCollisionMarkers(player.pos, player.dim, player.vel, player.room, tiles);
                    if (player.pos.x == oldPlayerPos.x) {
                        player.vel.x = 0.0f;
                    }
                    if (player.pos.y == oldPlayerPos.y) {
                        player.vel.y = 0.0f;
                    }
                    multiplayer.sync(player.pos);
                }

                Character* currentCharacter = getCharacterForRoom(characters, player.room);

                if (!Minigames::IsTaskOpen() && currentCharacter != nullptr && BoxCollide(player.pos, player.dim, currentCharacter->pos, currentCharacter->dim) && Input::IsPressed("e")) {
                    if (currentCharacter->isRoaming) {
                        std::cout << "This character is dancing. Door opened for the next room." << std::endl;
                    } else if (addTaskForCharacter(*currentCharacter, player)) {
                        std::cout << "Task added: " << player.tasks.back() << std::endl;
                    } else {
                        std::cout << "All tasks completed. No more tasks available." << std::endl;
                    }
                }

                for (const Door& door : doors) {
                    vec2 doorPos = vec2(-999999.0f);
                    if (player.room == 0) {
                        doorPos = door.hubPos;
                    } else if (player.room == door.roomId) {
                        doorPos = door.roomPos;
                    }

                    if (doorPos.x > -999998.0f && BoxCollide(player.pos, player.dim, doorPos, door.dim)) {
                        if (Input::IsPressed("e")) {
                            if (player.room == 0) {
                                roomReturnPositions[door.roomId] = player.pos;
                                player.room = door.roomId;
                                player.pos = door.roomPos;
                                camera.pos = player.pos;
                                camera.target = player.pos;
                            } else {
                                player.room = 0;
                                auto returnPos = roomReturnPositions.find(door.roomId);
                                if (returnPos != roomReturnPositions.end()) {
                                    player.pos = returnPos->second;
                                } else {
                                    player.pos = door.hubPos;
                                }
                                camera.pos = player.pos;
                                camera.target = player.pos;
                            }
                            break;
                        }
                    }
                }

                if (currentCharacter != nullptr && currentCharacter->isRoaming) {
                    vec2 oldCharacterPos = currentCharacter->pos;
                    currentCharacter->roam();
                    if (collidesWithCollisionMarker(currentCharacter->pos, currentCharacter->dim, currentCharacter->room, tiles)) {
                        currentCharacter->pos = oldCharacterPos;
                        currentCharacter->roamTimer = 0.0f;
                    }
                }

                Minigames::TryTakeOutTrashOutsideDropoff(player.pos, player.dim, player.room, Input::IsPressed("e"));

                Image::Draw(selectTex, snap(mouse + 32, 64.0), 32);
            }

            player.health = clamp(0.0, 100.0, player.health);
        } else {
            // Game Paused

            //Image::Draw(pause, camera.pos, 1500);
        }

        int id = 0;
        for (const Task& t : objectives) {
            if (t.room != player.room) {
                ++id;
                continue;
            }

            if (!Minigames::IsTaskOpen() && BoxCollide(player.pos, player.dim, t.pos, t.dim) && Input::IsPressed("e")) {
                Minigames::OpenTask(id, t.name, t.room);
            }
            ++id;
            if (editorTaskPlacementMode) {
                Image::Draw(taskTex, t.pos, 45.0);
            }
        }

        int pendingDropoffRoom = -1;
        std::string pendingDropoffTaskName;
        if (Minigames::ConsumeTakeOutTrashDropoffPlacementRequest(pendingDropoffRoom, pendingDropoffTaskName)) {
            const vec2 trashCanForwardOffset = vec2(0.0f, -192.0f);
            vec2 outsideTrashcanPos = vec2(220.0f, -250.0f);
            for (const Door& door : doors) {
                if (door.roomId == pendingDropoffRoom) {
                    outsideTrashcanPos = door.hubPos + trashCanForwardOffset;
                    break;
                }
            }

            Minigames::SetTakeOutTrashDropoffWorldPos(pendingDropoffTaskName, pendingDropoffRoom, outsideTrashcanPos);
        }

        Minigames::DrawTakeOutTrashWorldPrompt(player.room, zoom);

        Image::Draw(player.texture, player.pos, 150);
        multiplayer.drawRemotePlayers(player.texture);
        for (const Character& c : characters) {
            if (c.room == player.room) {
                Image::Draw(c.texture, c.pos, 150);
            }
        }

        // UI
        glLoadIdentity();
        Character* uiCharacter = getCharacterForRoom(characters, player.room);
        int uiLevel = (uiCharacter != nullptr) ? uiCharacter->level : 0;
        
        for (int i = 0; i < 9; ++i) {
            float perc = (uiLevel / 90.0f) - (i * 0.1f);
            GLuint tex;
            if (perc >= 0.1) {
                tex = heartTextures[1];
            } else if (perc >= 0.05) {
                tex = heartTextures[2];
            } else {
                tex = heartTextures[0];
            }
            Image::Draw(tex, vec2(-screen.x + 64*i + 48, screen.y - 48) / zoom, 16);
        }

        std::string levelText = "level " + std::to_string(uiLevel);
        Text::DrawString(levelText, vec2(-screen.x + 40, screen.y - 130) / zoom, 24.0f / zoom, 1.5f);

        std::string taskText = "objectives " + std::to_string(player.tasks.size());
        Text::DrawString(taskText, vec2(screen.x - 600, screen.y - 48) / zoom, 24.0f / zoom, 1.5f);

        std::string roomText = "room " + std::to_string(player.room);
        Text::DrawString(roomText, vec2(-screen.x + 40, screen.y - 230) / zoom, 20.0f / zoom, 1.5f);

        if (mode) {
            HouseTilesetGroup* activeHouseTilesetGroup = nullptr;
            if (editorTileSource == 6 && !houseTilesets.empty()) {
                if (houseTilesetGroup < 0) {
                    houseTilesetGroup = 0;
                } else if (houseTilesetGroup >= static_cast<int>(houseTilesets.size())) {
                    houseTilesetGroup = static_cast<int>(houseTilesets.size()) - 1;
                }

                if (houseTilesetGroup >= 0 && houseTilesetGroup < static_cast<int>(houseTilesets.size())) {
                    activeHouseTilesetGroup = &houseTilesets[houseTilesetGroup];
                }
            }

            std::string activeTilesetName = "library";
            std::string activeToggleKey = "default";
            std::string tilesetRowText;
            vec2 hintsHeaderPos = vec2(-screen.x + 40, screen.y - 370) / zoom;
            std::string taskEditorModeText = std::string("task editor: ") + (editorTaskPlacementMode ? "on" : "off") + " (toggle t)";

            if (Input::IsPressed("enter")) {
                editorHintsOpen = !editorHintsOpen;
            }

            if (editorTileSource == 1 && tilesetCount > 0) {
                activeTilesetName = "tileset1";
                activeToggleKey = "3";
            } else if (editorTileSource == 2 && platformCount > 0) {
                activeTilesetName = "platformer";
                activeToggleKey = "4";
            } else if (editorTileSource == 3 && houseCount > 0) {
                activeTilesetName = "houses";
                activeToggleKey = "5";
            } else if (editorTileSource == 6 && !houseTilesets.empty() && activeHouseTilesetGroup != nullptr) {
                activeTilesetName = "house/" + activeHouseTilesetGroup->name;
                activeToggleKey = "8";
            } else if (editorTileSource == 4) {
                activeTilesetName = "spawn marker";
                activeToggleKey = "6";
            } else if (editorTileSource == 5) {
                activeTilesetName = "collision marker";
                activeToggleKey = "7";
            }

            tilesetRowText = "tileset: " + activeTilesetName + " (toggle " + activeToggleKey + ")";
            Text::DrawString(tilesetRowText, vec2(-screen.x + 40, screen.y - 280) / zoom, 18.0f / zoom, 1.5f);
            Text::DrawString(taskEditorModeText, vec2(-screen.x + 40, screen.y - 310) / zoom, 16.0f / zoom, 1.5f);

            if (editorTileSource == 6 && activeHouseTilesetGroup != nullptr && !activeHouseTilesetGroup->textures.empty()) {
                const std::string& activeHouseTileFile = activeHouseTilesetGroup->files[houseTilesetTile];
                std::string houseTileRowText = "house tile: " + std::to_string(houseTilesetTile + 1) + "/" + std::to_string(activeHouseTilesetGroup->textures.size()) + " " + getFileBaseName(activeHouseTileFile);
                Text::DrawString(houseTileRowText, vec2(-screen.x + 40, screen.y - 340) / zoom, 16.0f / zoom, 1.5f);
                Text::DrawString("9: next house folder", vec2(-screen.x + 40, screen.y - 370) / zoom, 14.0f / zoom, 1.5f);
            }

            std::string hintsHeaderText = std::string(editorHintsOpen ? "[-] " : "[+] ") + "press enter for text editor hints";
            Text::DrawString(hintsHeaderText, hintsHeaderPos, 16.0f / zoom, 1.5f);

            if (editorHintsOpen) {
                Text::DrawString("0: toggle on/off", vec2(-screen.x + 40, screen.y - 400) / zoom, 14.0f / zoom, 1.5f);
                Text::DrawString("1: draw", vec2(-screen.x + 40, screen.y - 430) / zoom, 14.0f / zoom, 1.5f);
                Text::DrawString("2: erase", vec2(-screen.x + 40, screen.y - 460) / zoom, 14.0f / zoom, 1.5f);
                Text::DrawString("3: tileset1 folder", vec2(-screen.x + 40, screen.y - 490) / zoom, 14.0f / zoom, 1.5f);
                Text::DrawString("4: platformer folder", vec2(-screen.x + 40, screen.y - 520) / zoom, 14.0f / zoom, 1.5f);
                Text::DrawString("5: houses", vec2(-screen.x + 40, screen.y - 550) / zoom, 14.0f / zoom, 1.5f);
                Text::DrawString("6: spawn marker", vec2(-screen.x + 40, screen.y - 580) / zoom, 14.0f / zoom, 1.5f);
                Text::DrawString("7: collision marker", vec2(-screen.x + 40, screen.y - 610) / zoom, 14.0f / zoom, 1.5f);
                Text::DrawString("8: house tilesets", vec2(-screen.x + 40, screen.y - 640) / zoom, 14.0f / zoom, 1.5f);
                Text::DrawString("9: next house folder", vec2(-screen.x + 40, screen.y - 670) / zoom, 14.0f / zoom, 1.5f);
                Text::DrawString("t: toggle task placement editor", vec2(-screen.x + 40, screen.y - 700) / zoom, 14.0f / zoom, 1.5f);
                Text::DrawString("task mode: click objective to move + save", vec2(-screen.x + 40, screen.y - 730) / zoom, 14.0f / zoom, 1.5f);
                Text::DrawString("x/z or wheel: select tile", vec2(-screen.x + 40, screen.y - 760) / zoom, 14.0f / zoom, 1.5f);
                Text::DrawString("left click: place/remove", vec2(-screen.x + 40, screen.y - 790) / zoom, 14.0f / zoom, 1.5f);
                Text::DrawString("q/e: prev/next room", vec2(-screen.x + 40, screen.y - 820) / zoom, 14.0f / zoom, 1.5f);
            }
        }

        Text::DrawString(multiplayer.getStatusText(), vec2(-screen.x + 40, screen.y - 180) / zoom, 20.0f / zoom, 1.5f);

        float y = 112.0;
        for (const std::string& t : player.tasks) {
            taskText = "- " + t;
            Text::DrawString(taskText, vec2(screen.x - 600, screen.y - y) / zoom, 24.0f / zoom, 1.5f);
            y += 64.0;
        }

        if (Minigames::IsTaskOpen()) {
            vec2 mouseUI = GetMouseUI(window);
            Minigames::DrawTaskPopup(screen, zoom, mouseUI);
        }

        if (Minigames::ConsumeTaskCompleteRequest()) {
            int activeTaskIndex = Minigames::GetCompletionTaskIndex();
            int completionTaskRoom = Minigames::GetCompletionTaskRoom();
            if ((activeTaskIndex < 0 || activeTaskIndex >= static_cast<int>(objectives.size())) && !Minigames::GetCompletionTaskName().empty()) {
                for (int objectiveIndex = 0; objectiveIndex < static_cast<int>(objectives.size()); ++objectiveIndex) {
                    if (objectives[objectiveIndex].name == Minigames::GetCompletionTaskName() &&
                        (completionTaskRoom < 0 || objectives[objectiveIndex].room == completionTaskRoom)) {
                        activeTaskIndex = objectiveIndex;
                        break;
                    }
                }
            }

            if (activeTaskIndex >= 0 && activeTaskIndex < static_cast<int>(objectives.size())) {
                Task completedTask = objectives[activeTaskIndex];
                Character* completedCharacter = getCharacterForRoom(characters, completedTask.room);

                objectives.erase(objectives.begin() + activeTaskIndex);
                if (!completedTask.name.empty()) {
                    auto taskIt = std::find(player.tasks.begin(), player.tasks.end(), completedTask.name);
                    if (taskIt != player.tasks.end()) {
                        player.tasks.erase(taskIt);
                    }
                }

                if (completedCharacter != nullptr) {
                    completedCharacter->tasksCompleted = std::min(completedCharacter->tasksCompleted + 1, static_cast<int>(completedCharacter->tasks.size()));
                    completedCharacter->level = completedCharacter->tasksCompleted * 30;
                    if (completedCharacter->tasksCompleted >= static_cast<int>(completedCharacter->tasks.size())) {
                        completedCharacter->isRoaming = true;
                        spawnTreeOnMarkerForRoom(tiles, spawnedTrees, completedCharacter->room);
                        spawnNextStage(characters, doors, nextRoomId, *completedCharacter, hubDoorPositions);
                    }
                }

                std::string completedBy = gLocalPlayerName;
                if (completedBy.empty()) {
                    completedBy = completedTask.assignedBy;
                }
                if (completedBy.empty() && completedCharacter != nullptr) {
                    completedBy = completedCharacter->name;
                }

                WebFrontendTaskCompleted(completedBy.c_str(), completedTask.name.c_str(), completedTask.room, completedTask.id);
            }

            Minigames::CloseTask();
        }

        if (!running) {
            Text::DrawStringCentered("paused", vec2(0.0f, screen.y - 90.0f) / zoom, 40.0f / zoom, 1.5f);
        }

        if(Input::IsPressed("escape")) {
            running = 1 - running;
        }

        Manager::Update();

        glfwSwapBuffers(window);

    };

#ifdef __EMSCRIPTEN__
    std::function<void()> frameFn = frame;
    gWebFrame = &frameFn;
    emscripten_set_main_loop(WebFrameTrampoline, 0, 1);
#else
    while (!glfwWindowShouldClose(window)) {
        frame();
    }

    multiplayer.shutdown();
    glfwTerminate();
    return 0;
#endif
}
