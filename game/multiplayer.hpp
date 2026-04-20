#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cmath>
#include <deque>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "../core/essentials.hpp"
#include "../core/images/image.hpp"
#include "../core/net/http.hpp"
#include "../core/text.hpp"

struct RemotePlayer {
    std::string playerId;
    std::string playerName;
    vec2 pos = vec2(0.0f);
    vec2 renderPos = vec2(0.0f);
    bool hasRenderPos = false;
    int room = -1;
    int level = -1;
    struct PositionSample {
        vec2 pos = vec2(0.0f);
        double timestamp = 0.0;
    };
    std::deque<PositionSample> positionBuffer;
    double lastSeenTime = 0.0;
    std::string taskProgress;
};

struct PlayerJsonPackage {
    std::string playerName;
    vec2 pos = vec2(0.0f);
    int room = -1;
    int level = -1;
    std::string taskProgress;
};

class MultiplayerClient {
public:
    MultiplayerClient() = default;

    ~MultiplayerClient() {
        shutdown();
    }

    MultiplayerClient(const MultiplayerClient&) = delete;
    MultiplayerClient& operator=(const MultiplayerClient&) = delete;

    void setServer(const std::string& hostValue, int portValue) {
        std::lock_guard<std::mutex> lock(stateMutex);
        host = hostValue;
        port = portValue;
    }

    void setPlayerName(const std::string& value) {
        std::lock_guard<std::mutex> lock(stateMutex);
        playerName = sanitizeName(value);
    }

    void setTaskProgress(const std::string& value) {
        std::lock_guard<std::mutex> lock(stateMutex);
        latestTaskProgress = value;
    }

    std::vector<std::string> getRemoteTaskProgressStates() const {
        std::vector<std::string> states;
        std::lock_guard<std::mutex> lock(stateMutex);
        for (const auto& entry : remotes) {
            if (!entry.second.taskProgress.empty()) {
                states.push_back(entry.second.taskProgress);
            }
        }
        return states;
    }

    bool initOrJoin(const std::string& roomCodeOverride) {
        stopWorker();

        {
            std::lock_guard<std::mutex> lock(stateMutex);
            requestedRoomCode = roomCodeOverride;
            roomCode.clear();
            connected = false;
            remotes.clear();
            statusText = "connecting...";
        }

        stopRequested = false;
        worker = std::thread(&MultiplayerClient::workerLoop, this);
        return true;
    }

    void shutdown() {
        stopWorker();
        leaveRoom();
    }

    void sync(const vec2& localPos, int localRoom, int localLevel) {
        double nowSeconds = getCurrentTimeSeconds();
        std::lock_guard<std::mutex> lock(stateMutex);
        latestLocalPos = localPos;
        latestLocalRoom = localRoom;
        latestLocalLevel = localLevel;

        const double staleTimeout = 5.0;
        const double interpolationDelay = 0.10;
        const double maxExtrapolation = 0.10;
        for (auto it = remotes.begin(); it != remotes.end();) {
            if (nowSeconds - it->second.lastSeenTime > staleTimeout) {
                it = remotes.erase(it);
            } else {
                RemotePlayer& remote = it->second;
                vec2 targetPos = remote.pos;

                if (!remote.positionBuffer.empty()) {
                    double renderTime = nowSeconds - interpolationDelay;

                    if (remote.positionBuffer.size() == 1) {
                        targetPos = remote.positionBuffer.back().pos;
                    } else {
                        bool foundInterval = false;
                        for (std::size_t index = 1; index < remote.positionBuffer.size(); ++index) {
                            const RemotePlayer::PositionSample& previous = remote.positionBuffer[index - 1];
                            const RemotePlayer::PositionSample& next = remote.positionBuffer[index];

                            if (renderTime >= previous.timestamp && renderTime <= next.timestamp) {
                                double interval = next.timestamp - previous.timestamp;
                                float alpha = 0.0f;
                                if (interval > 0.0001) {
                                    alpha = static_cast<float>((renderTime - previous.timestamp) / interval);
                                }
                                alpha = std::clamp(alpha, 0.0f, 1.0f);
                                targetPos = previous.pos + (next.pos - previous.pos) * alpha;
                                foundInterval = true;
                                break;
                            }
                        }

                        if (!foundInterval) {
                            const RemotePlayer::PositionSample& latest = remote.positionBuffer.back();
                            const RemotePlayer::PositionSample& previous = remote.positionBuffer[remote.positionBuffer.size() - 2];
                            vec2 velocity = vec2(0.0f);
                            double dt = latest.timestamp - previous.timestamp;
                            if (dt > 0.0001) {
                                velocity = (latest.pos - previous.pos) / static_cast<float>(dt);
                            }

                            double extrapolation = std::clamp(renderTime - latest.timestamp, 0.0, maxExtrapolation);
                            targetPos = latest.pos + velocity * static_cast<float>(extrapolation);
                        }
                    }
                }

                if (!remote.hasRenderPos) {
                    remote.renderPos = targetPos;
                    remote.hasRenderPos = true;
                }

                const float catchupFactor = 0.30f;
                remote.renderPos.x += (targetPos.x - remote.renderPos.x) * catchupFactor;
                remote.renderPos.y += (targetPos.y - remote.renderPos.y) * catchupFactor;
                ++it;
            }
        }
    }

    void drawRemotePlayers(GLuint sharedTexture) {
        std::lock_guard<std::mutex> lock(stateMutex);
        for (const auto& entry : remotes) {
            const RemotePlayer& remote = entry.second;
            bool sameRoom = remote.room >= 0 && remote.room == latestLocalRoom;
            bool sameLevel = remote.level >= 0 && remote.level == latestLocalRoom;
            if (!sameRoom && !sameLevel) {
                continue;
            }

            vec2 drawPos = remote.hasRenderPos ? remote.renderPos : remote.pos;
            Image::Draw(sharedTexture, drawPos, 150.0f);
            Text::DrawStringCentered(remote.playerName, vec2(drawPos.x, drawPos.y + 190.0f), 14.0f, 1.3f);
        }
    }

    std::string getStatusText() const {
        std::lock_guard<std::mutex> lock(stateMutex);
        return statusText;
    }

    bool isConnected() const {
        std::lock_guard<std::mutex> lock(stateMutex);
        return connected;
    }

private:
    mutable std::mutex stateMutex;

    std::string host = "127.0.0.1";
    int port = 8080;

    std::string playerName;
    std::string requestedRoomCode;
    std::string roomCode;
    std::string statusText = "multiplayer offline";

    bool connected = false;
    vec2 latestLocalPos = vec2(0.0f);
    int latestLocalRoom = 0;
    int latestLocalLevel = -1;
    std::string latestTaskProgress;

    std::map<std::string, RemotePlayer> remotes;

    std::thread worker;
    std::atomic<bool> stopRequested = false;

    Http::Response tryPostJson(const std::string& path, const std::string& jsonBody) const {
        std::string currentHost;
        int currentPort;
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            currentHost = host;
            currentPort = port;
        }

        Http::Response response;
        std::vector<std::string> hosts;
        hosts.push_back(currentHost);
        if (currentHost == "localhost") {
            hosts.push_back("127.0.0.1");
        } else if (currentHost == "127.0.0.1") {
            hosts.push_back("localhost");
        }

        for (const std::string& candidateHost : hosts) {
            response = Http::PostJson(candidateHost, currentPort, path, jsonBody);
            if (response.statusCode >= 200 && response.statusCode < 300) {
                return response;
            }
        }

        return response;
    }

    Http::Response tryGet(const std::string& path) const {
        std::string currentHost;
        int currentPort;
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            currentHost = host;
            currentPort = port;
        }

        Http::Response response;
        std::vector<std::string> hosts;
        hosts.push_back(currentHost);
        if (currentHost == "localhost") {
            hosts.push_back("127.0.0.1");
        } else if (currentHost == "127.0.0.1") {
            hosts.push_back("localhost");
        }

        for (const std::string& candidateHost : hosts) {
            response = Http::Get(candidateHost, currentPort, path);
            if (response.statusCode >= 200 && response.statusCode < 300) {
                return response;
            }
        }

        return response;
    }

    void stopWorker() {
        stopRequested = true;
        if (worker.joinable()) {
            worker.join();
        }
    }

    void leaveRoom() {
        std::string localRoomCode;
        std::string localName;

        {
            std::lock_guard<std::mutex> lock(stateMutex);
            if (roomCode.empty() || playerName.empty()) {
                connected = false;
                remotes.clear();
                statusText = "multiplayer offline";
                return;
            }

            localRoomCode = roomCode;
            localName = playerName;
        }

        std::ostringstream body;
        body << "{\"playerName\":\"" << escapeJson(localName) << "\"}";
        tryPostJson("/api/multiplayer/rooms/" + localRoomCode + "/leave", body.str());

        std::lock_guard<std::mutex> lock(stateMutex);
        connected = false;
        roomCode.clear();
        remotes.clear();
        statusText = "multiplayer offline";
    }

    static double getCurrentTimeSeconds() {
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        return std::chrono::duration<double>(now).count();
    }

    static std::string sanitizeName(const std::string& raw) {
        std::string out;
        for (char c : raw) {
            if ((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == ' ' || c == '_' || c == '-') {
                out.push_back(c);
            }
        }

        if (out.empty()) {
            out = "player";
        }

        if (out.size() > 24) {
            out.resize(24);
        }

        return out;
    }

    static std::string escapeJson(const std::string& raw) {
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

    static std::string buildPlayerJsonPackage(const PlayerJsonPackage& playerPackage) {
        std::ostringstream body;
        PlayerJsonPackage normalizedPackage = playerPackage;
        normalizedPackage.level = normalizedPackage.room;

        body << "{\"playerName\":\"" << escapeJson(playerPackage.playerName)
             << "\",\"room\":" << normalizedPackage.room
             << ",\"currentRoom\":" << normalizedPackage.room
             << ",\"level\":" << normalizedPackage.level
             << ",\"currentLevel\":" << normalizedPackage.level
             << ",\"x\":" << normalizedPackage.pos.x
             << ",\"y\":" << normalizedPackage.pos.y;

        if (!normalizedPackage.taskProgress.empty()) {
            body << ",\"taskProgress\":\"" << escapeJson(normalizedPackage.taskProgress) << "\"";
        }

        body << ",\"player\":{\"playerName\":\"" << escapeJson(normalizedPackage.playerName)
             << "\",\"room\":" << normalizedPackage.room
             << ",\"currentRoom\":" << normalizedPackage.room
             << ",\"level\":" << normalizedPackage.level
             << ",\"currentLevel\":" << normalizedPackage.level
             << ",\"position\":{\"x\":" << normalizedPackage.pos.x
             << ",\"y\":" << normalizedPackage.pos.y << "}";

        if (!normalizedPackage.taskProgress.empty()) {
            body << ",\"taskProgress\":\"" << escapeJson(normalizedPackage.taskProgress) << "\"";
        }

        body << "}}";
        return body.str();
    }

    static bool tryExtractPlayerJsonPackage(const std::string& objectJson, PlayerJsonPackage& outPackage) {
        std::string playerJson = extractJsonObject(objectJson, "player");
        if (playerJson.empty()) {
            playerJson = objectJson;
        }

        std::string name = extractJsonString(objectJson, "playerName");
        if (name.empty()) {
            name = extractJsonString(playerJson, "playerName");
        }

        std::string taskProgress = extractJsonString(objectJson, "taskProgress");
        if (taskProgress.empty()) {
            taskProgress = extractJsonString(playerJson, "taskProgress");
        }

        std::string positionJson = extractJsonObject(playerJson, "position");

        double x = 0.0;
        double y = 0.0;
        bool hasPosition = false;
        if (!positionJson.empty()) {
            hasPosition = extractJsonNumber(positionJson, "x", x) && extractJsonNumber(positionJson, "y", y);
        }
        if (!hasPosition) {
            hasPosition = extractJsonNumber(playerJson, "x", x) && extractJsonNumber(playerJson, "y", y);
        }
        if (!hasPosition) {
            hasPosition = extractJsonNumber(objectJson, "x", x) && extractJsonNumber(objectJson, "y", y);
        }

        int room = -1;
        double roomValue = 0.0;
        if (extractJsonNumber(playerJson, "room", roomValue) ||
            extractJsonNumber(objectJson, "room", roomValue) ||
            extractJsonNumber(playerJson, "currentRoom", roomValue) ||
            extractJsonNumber(objectJson, "currentRoom", roomValue)) {
            room = static_cast<int>(roomValue);
        }

        int level = -1;
        double levelValue = 0.0;
        if (extractJsonNumber(playerJson, "level", levelValue) ||
            extractJsonNumber(objectJson, "level", levelValue) ||
            extractJsonNumber(playerJson, "currentLevel", levelValue) ||
            extractJsonNumber(objectJson, "currentLevel", levelValue)) {
            level = static_cast<int>(levelValue);
        }

        if (room >= 0) {
            level = room;
        }

        if (name.empty() || !hasPosition) {
            return false;
        }

        outPackage.playerName = name;
        outPackage.pos = vec2(static_cast<float>(x), static_cast<float>(y));
        outPackage.room = room;
        outPackage.level = level;
        outPackage.taskProgress = taskProgress;
        return true;
    }

    static std::string extractJsonString(const std::string& json, const std::string& key, std::size_t from = 0) {
        std::string token = "\"" + key + "\"";
        std::size_t keyPos = json.find(token, from);
        if (keyPos == std::string::npos) {
            return "";
        }

        std::size_t colonPos = json.find(':', keyPos + token.size());
        if (colonPos == std::string::npos) {
            return "";
        }

        std::size_t firstQuote = json.find('"', colonPos + 1);
        if (firstQuote == std::string::npos) {
            return "";
        }

        std::size_t endQuote = firstQuote + 1;
        while (endQuote < json.size()) {
            if (json[endQuote] == '"' && json[endQuote - 1] != '\\') {
                break;
            }
            ++endQuote;
        }

        if (endQuote >= json.size()) {
            return "";
        }

        return json.substr(firstQuote + 1, endQuote - firstQuote - 1);
    }

    static std::string extractJsonObject(const std::string& json, const std::string& key, std::size_t from = 0) {
        std::string token = "\"" + key + "\"";
        std::size_t keyPos = json.find(token, from);
        if (keyPos == std::string::npos) {
            return "";
        }

        std::size_t colonPos = json.find(':', keyPos + token.size());
        if (colonPos == std::string::npos) {
            return "";
        }

        std::size_t objectStart = json.find('{', colonPos + 1);
        if (objectStart == std::string::npos) {
            return "";
        }

        int depth = 0;
        std::size_t objectEnd = objectStart;
        while (objectEnd < json.size()) {
            char c = json[objectEnd];
            if (c == '{') {
                ++depth;
            } else if (c == '}') {
                --depth;
                if (depth == 0) {
                    ++objectEnd;
                    break;
                }
            }
            ++objectEnd;
        }

        if (depth != 0) {
            return "";
        }

        return json.substr(objectStart, objectEnd - objectStart);
    }

    static std::string extractFirstRoomCode(const std::string& json) {
        return extractJsonString(json, "roomCode");
    }

    static bool extractJsonNumber(const std::string& json, const std::string& key, double& value, std::size_t from = 0) {
        std::string token = "\"" + key + "\"";
        std::size_t keyPos = json.find(token, from);
        if (keyPos == std::string::npos) {
            return false;
        }

        std::size_t colonPos = json.find(':', keyPos + token.size());
        if (colonPos == std::string::npos) {
            return false;
        }

        std::size_t start = colonPos + 1;
        while (start < json.size() && (json[start] == ' ' || json[start] == '\n' || json[start] == '\r' || json[start] == '\t')) {
            ++start;
        }

        std::size_t end = start;
        while (end < json.size()) {
            char c = json[end];
            if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E') {
                ++end;
                continue;
            }
            break;
        }

        if (end <= start) {
            return false;
        }

        try {
            value = std::stod(json.substr(start, end - start));
        } catch (...) {
            return false;
        }

        return true;
    }

    bool tryConnectRoom() {
        std::string localName;
        std::string localRequestedRoom;
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            localName = playerName;
            localRequestedRoom = requestedRoomCode;
        }

        if (localName.empty()) {
            std::lock_guard<std::mutex> lock(stateMutex);
            statusText = "missing player name";
            return false;
        }

        std::string resolvedRoomToJoin = localRequestedRoom;
        if (resolvedRoomToJoin.empty()) {
            Http::Response listResponse = tryGet("/api/multiplayer/rooms?limit=1");
            if (listResponse.statusCode >= 200 && listResponse.statusCode < 300) {
                resolvedRoomToJoin = extractFirstRoomCode(listResponse.body);
            }
        }

        Http::Response roomResponse;
        if (resolvedRoomToJoin.empty()) {
            roomResponse = tryPostJson(
                "/api/multiplayer/rooms",
                std::string("{\"playerName\":\"") + escapeJson(localName) + "\"}"
            );
        } else {
            std::string roomPath = "/api/multiplayer/rooms/" + resolvedRoomToJoin + "/join";
            roomResponse = tryPostJson(
                roomPath,
                std::string("{\"playerName\":\"") + escapeJson(localName) + "\"}"
            );
        }

        if (roomResponse.statusCode < 200 || roomResponse.statusCode >= 300) {
            std::lock_guard<std::mutex> lock(stateMutex);
            statusText = "backend unavailable (retrying)";
            connected = false;
            return false;
        }

        std::string parsedRoomCode = extractJsonString(roomResponse.body, "roomCode");
        if (parsedRoomCode.empty()) {
            std::lock_guard<std::mutex> lock(stateMutex);
            statusText = "room join failed (retrying)";
            connected = false;
            return false;
        }

        std::lock_guard<std::mutex> lock(stateMutex);
        roomCode = parsedRoomCode;
        connected = true;
        statusText = "room " + roomCode;
        return true;
    }

    void postPresence(const vec2& localPos, const std::string& localRoomCode) {
        std::string localName;
        int localRoom = 0;
        std::string localTaskProgress;
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            localName = playerName;
            localRoom = latestLocalRoom;
            localTaskProgress = latestTaskProgress;
        }

        PlayerJsonPackage playerPackage;
        playerPackage.playerName = localName;
        playerPackage.pos = localPos;
        playerPackage.room = localRoom;
        playerPackage.level = latestLocalLevel;
        playerPackage.taskProgress = localTaskProgress;

        Http::Response response = tryPostJson(
            "/api/multiplayer/rooms/" + localRoomCode + "/presence",
            buildPlayerJsonPackage(playerPackage)
        );

        if (response.statusCode < 200 || response.statusCode >= 300) {
            std::lock_guard<std::mutex> lock(stateMutex);
            connected = false;
            statusText = "backend unavailable (retrying)";
        }
    }

    void pollPresence(const std::string& localRoomCode, double nowSeconds) {
        std::string localName;
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            localName = playerName;
        }

        Http::Response response = tryGet(
            "/api/multiplayer/rooms/" + localRoomCode + "/presence"
        );

        if (response.statusCode < 200 || response.statusCode >= 300) {
            std::lock_guard<std::mutex> lock(stateMutex);
            connected = false;
            statusText = "backend unavailable (retrying)";
            return;
        }

        std::size_t playersToken = response.body.find("\"players\"");
        if (playersToken == std::string::npos) {
            return;
        }

        std::size_t arrayStart = response.body.find('[', playersToken);
        std::size_t arrayEnd = response.body.find(']', arrayStart == std::string::npos ? playersToken : arrayStart + 1);

        if (arrayStart == std::string::npos || arrayEnd == std::string::npos || arrayEnd <= arrayStart) {
            return;
        }

        std::string playersArray = response.body.substr(arrayStart + 1, arrayEnd - arrayStart - 1);

        std::size_t pos = 0;
        while (pos < playersArray.size()) {
            std::size_t objStart = playersArray.find('{', pos);
            if (objStart == std::string::npos) {
                break;
            }

            int depth = 1;
            std::size_t objEnd = objStart + 1;
            while (objEnd < playersArray.size() && depth > 0) {
                if (playersArray[objEnd] == '{') {
                    depth++;
                } else if (playersArray[objEnd] == '}') {
                    depth--;
                }
                ++objEnd;
            }

            if (depth != 0 || objEnd <= objStart + 1) {
                break;
            }

            std::string objectJson = playersArray.substr(objStart, objEnd - objStart);
            std::string id = extractJsonString(objectJson, "playerId");
            if (id.empty()) {
                std::string playerJson = extractJsonObject(objectJson, "player");
                if (!playerJson.empty()) {
                    id = extractJsonString(playerJson, "playerId");
                }
            }

            PlayerJsonPackage remotePackage;
            if (!id.empty() && tryExtractPlayerJsonPackage(objectJson, remotePackage)) {
                if (remotePackage.playerName != localName) {
                    std::lock_guard<std::mutex> lock(stateMutex);
                    RemotePlayer& remote = remotes[id];
                    remote.playerId = id;
                    remote.playerName = remotePackage.playerName;
                    remote.pos = remotePackage.pos;
                    remote.room = remotePackage.room;
                    remote.level = remotePackage.level;
                    remote.taskProgress = remotePackage.taskProgress;

                    if (!remote.positionBuffer.empty()) {
                        const RemotePlayer::PositionSample& lastSample = remote.positionBuffer.back();
                        if (std::abs(lastSample.pos.x - remote.pos.x) < 0.001f &&
                            std::abs(lastSample.pos.y - remote.pos.y) < 0.001f) {
                            remote.positionBuffer.back().timestamp = nowSeconds;
                        } else {
                            remote.positionBuffer.push_back({remote.pos, nowSeconds});
                        }
                    } else {
                        remote.positionBuffer.push_back({remote.pos, nowSeconds});
                    }

                    const std::size_t maxBufferSize = 12;
                    if (remote.positionBuffer.size() > maxBufferSize) {
                        remote.positionBuffer.pop_front();
                    }

                    if (!remote.hasRenderPos) {
                        remote.renderPos = remote.pos;
                        remote.hasRenderPos = true;
                    }

                    remote.lastSeenTime = nowSeconds;
                }
            }

            pos = objEnd;
        }

        std::lock_guard<std::mutex> lock(stateMutex);
        statusText = "room " + localRoomCode + " players " + std::to_string(static_cast<int>(remotes.size()) + 1);
    }

    void workerLoop() {
        double lastPushTime = 0.0;
        double lastPollTime = 0.0;

        while (!stopRequested.load()) {
            bool localConnected = false;
            std::string localRoomCode;
            vec2 localPos;

            {
                std::lock_guard<std::mutex> lock(stateMutex);
                localConnected = connected;
                localRoomCode = roomCode;
                localPos = latestLocalPos;
            }

            if (!localConnected) {
                tryConnectRoom();
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                continue;
            }

            const double nowSeconds = getCurrentTimeSeconds();

            if (nowSeconds - lastPushTime >= 0.05) {
                postPresence(localPos, localRoomCode);
                lastPushTime = nowSeconds;
            }

            if (nowSeconds - lastPollTime >= 0.05) {
                pollPresence(localRoomCode, nowSeconds);
                lastPollTime = nowSeconds;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
};
