#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
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
    double lastSeenTime = 0.0;
};

class MultiplayerClient {
public:
    MultiplayerClient() = default;

    ~MultiplayerClient() {
        stopWorker();
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

    void sync(const vec2& localPos, double nowSeconds) {
        std::lock_guard<std::mutex> lock(stateMutex);
        latestLocalPos = localPos;

        const double staleTimeout = 5.0;
        for (auto it = remotes.begin(); it != remotes.end();) {
            if (nowSeconds - it->second.lastSeenTime > staleTimeout) {
                it = remotes.erase(it);
            } else {
                ++it;
            }
        }
    }

    void drawRemotePlayers(GLuint sharedTexture) {
        std::lock_guard<std::mutex> lock(stateMutex);
        for (const auto& entry : remotes) {
            const RemotePlayer& remote = entry.second;
            Image::Draw(sharedTexture, remote.pos, 150.0f);
            Text::DrawStringCentered(remote.playerName, vec2(remote.pos.x, remote.pos.y + 190.0f), 14.0f, 1.3f);
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
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            localName = playerName;
        }

        std::ostringstream body;
        body << "{\"playerName\":\"" << escapeJson(localName)
             << "\",\"x\":" << localPos.x
             << ",\"y\":" << localPos.y << "}";

        Http::Response response = tryPostJson(
            "/api/multiplayer/rooms/" + localRoomCode + "/presence",
            body.str()
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
            std::string name = extractJsonString(objectJson, "playerName");

            double x = 0.0;
            double y = 0.0;
            if (!id.empty() && !name.empty() && extractJsonNumber(objectJson, "x", x) && extractJsonNumber(objectJson, "y", y)) {
                if (name != localName) {
                    std::lock_guard<std::mutex> lock(stateMutex);
                    RemotePlayer& remote = remotes[id];
                    remote.playerId = id;
                    remote.playerName = name;
                    remote.pos = vec2(static_cast<float>(x), static_cast<float>(y));
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

            const auto now = std::chrono::steady_clock::now().time_since_epoch();
            const double nowSeconds = std::chrono::duration<double>(now).count();

            if (nowSeconds - lastPushTime >= 0.10) {
                postPresence(localPos, localRoomCode);
                lastPushTime = nowSeconds;
            }

            if (nowSeconds - lastPollTime >= 0.10) {
                pollPresence(localRoomCode, nowSeconds);
                lastPollTime = nowSeconds;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
};
