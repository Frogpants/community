#pragma once

#include <algorithm>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
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
    void setServer(const std::string& hostValue, int portValue) {
        host = hostValue;
        port = portValue;
    }

    void setPlayerName(const std::string& value) {
        playerName = sanitizeName(value);
    }

    bool initOrJoin(const std::string& roomCodeOverride) {
        roomCode.clear();
        connected = false;
        statusText = "multiplayer offline";

        if (playerName.empty()) {
            statusText = "missing player name";
            return false;
        }

        Http::Response roomResponse;
        if (roomCodeOverride.empty()) {
            roomResponse = tryPostJson(
                "/api/multiplayer/rooms",
                std::string("{\"playerName\":\"") + escapeJson(playerName) + "\"}"
            );
        } else {
            std::string roomPath = "/api/multiplayer/rooms/" + roomCodeOverride + "/join";
            roomResponse = tryPostJson(
                roomPath,
                std::string("{\"playerName\":\"") + escapeJson(playerName) + "\"}"
            );
        }

        if (roomResponse.statusCode < 200 || roomResponse.statusCode >= 300) {
            statusText = "backend unavailable";
            return false;
        }

        std::string parsedRoomCode = extractJsonString(roomResponse.body, "roomCode");
        if (parsedRoomCode.empty()) {
            statusText = "room join failed";
            return false;
        }

        roomCode = parsedRoomCode;
        connected = true;
        statusText = "room " + roomCode;

        return true;
    }

    void sync(const vec2& localPos, double nowSeconds) {
        if (!connected) {
            return;
        }

        if (nowSeconds - lastPushTime >= 0.10) {
            postPresence(localPos);
            lastPushTime = nowSeconds;
        }

        if (nowSeconds - lastPollTime >= 0.10) {
            pollPresence(nowSeconds);
            lastPollTime = nowSeconds;
        }

        const double staleTimeout = 5.0;
        for (auto it = remotes.begin(); it != remotes.end();) {
            if (nowSeconds - it->second.lastSeenTime > staleTimeout) {
                it = remotes.erase(it);
                continue;
            }
            ++it;
        }
    }

    void drawRemotePlayers(GLuint sharedTexture) {
        for (const auto& entry : remotes) {
            const RemotePlayer& remote = entry.second;
            Image::Draw(sharedTexture, remote.pos, 150.0f);
            Text::DrawStringCentered(remote.playerName, vec2(remote.pos.x, remote.pos.y + 190.0f), 14.0f, 1.3f);
        }
    }

    const std::string& getStatusText() const {
        return statusText;
    }

    bool isConnected() const {
        return connected;
    }

private:
    std::string host = "127.0.0.1";
    int port = 8080;

    std::string playerName;
    std::string roomCode;
    std::string statusText = "multiplayer offline";

    bool connected = false;
    double lastPushTime = 0.0;
    double lastPollTime = 0.0;

    std::map<std::string, RemotePlayer> remotes;

    std::vector<std::string> buildHostFallbacks() const {
        std::vector<std::string> hosts;
        hosts.push_back(host);

        if (host == "localhost") {
            hosts.push_back("127.0.0.1");
        } else if (host == "127.0.0.1") {
            hosts.push_back("localhost");
        }

        return hosts;
    }

    Http::Response tryPostJson(const std::string& path, const std::string& jsonBody) const {
        Http::Response response;
        std::vector<std::string> hosts = buildHostFallbacks();
        for (const std::string& candidateHost : hosts) {
            response = Http::PostJson(candidateHost, port, path, jsonBody);
            if (response.statusCode >= 200 && response.statusCode < 300) {
                return response;
            }
        }

        return response;
    }

    Http::Response tryGet(const std::string& path) const {
        Http::Response response;
        std::vector<std::string> hosts = buildHostFallbacks();
        for (const std::string& candidateHost : hosts) {
            response = Http::Get(candidateHost, port, path);
            if (response.statusCode >= 200 && response.statusCode < 300) {
                return response;
            }
        }

        return response;
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

    void postPresence(const vec2& localPos) {
        std::ostringstream body;
        body << "{\"playerName\":\"" << escapeJson(playerName)
             << "\",\"x\":" << localPos.x
             << ",\"y\":" << localPos.y << "}";

        Http::Response response = tryPostJson(
            "/api/multiplayer/rooms/" + roomCode + "/presence",
            body.str()
        );

        if (response.statusCode < 200 || response.statusCode >= 300) {
            connected = false;
            statusText = "multiplayer disconnected";
        }
    }

    void pollPresence(double nowSeconds) {
        Http::Response response = tryGet(
            "/api/multiplayer/rooms/" + roomCode + "/presence"
        );

        if (response.statusCode < 200 || response.statusCode >= 300) {
            connected = false;
            statusText = "multiplayer disconnected";
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
                if (name != playerName) {
                    RemotePlayer& remote = remotes[id];
                    remote.playerId = id;
                    remote.playerName = name;
                    remote.pos = vec2(static_cast<float>(x), static_cast<float>(y));
                    remote.lastSeenTime = nowSeconds;
                }
            }

            pos = objEnd;
        }

        statusText = "room " + roomCode + " players " + std::to_string(static_cast<int>(remotes.size()) + 1);
    }
};
