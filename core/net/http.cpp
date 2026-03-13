#include "http.hpp"

#include <sstream>
#include <string>

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

namespace Http {

namespace {

Response ParseHttpResponse(const std::string& raw) {
    Response response;
    response.raw = raw;

    std::size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        response.body = raw;
        return response;
    }

    std::string headers = raw.substr(0, headerEnd);
    response.body = raw.substr(headerEnd + 4);

    std::size_t firstLineEnd = headers.find("\r\n");
    std::string statusLine = headers.substr(0, firstLineEnd);

    std::istringstream lineStream(statusLine);
    std::string version;
    lineStream >> version >> response.statusCode;

    return response;
}

Response DoRequest(const std::string& host, int port, const std::string& request) {
    Response response;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return response;
    }

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* info = nullptr;
    std::string portString = std::to_string(port);

    if (getaddrinfo(host.c_str(), portString.c_str(), &hints, &info) != 0) {
        WSACleanup();
        return response;
    }

    SOCKET sock = INVALID_SOCKET;
    for (addrinfo* p = info; p != nullptr; p = p->ai_next) {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock == INVALID_SOCKET) {
            continue;
        }

        if (connect(sock, p->ai_addr, static_cast<int>(p->ai_addrlen)) == 0) {
            break;
        }

        closesocket(sock);
        sock = INVALID_SOCKET;
    }

    freeaddrinfo(info);

    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return response;
    }

    std::size_t sent = 0;
    while (sent < request.size()) {
        int wrote = send(sock, request.data() + sent, static_cast<int>(request.size() - sent), 0);
        if (wrote <= 0) {
            closesocket(sock);
            WSACleanup();
            return response;
        }
        sent += static_cast<std::size_t>(wrote);
    }

    std::string raw;
    char buffer[4096];
    while (true) {
        int bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            break;
        }
        raw.append(buffer, static_cast<std::size_t>(bytes));
    }

    closesocket(sock);
    WSACleanup();
    return ParseHttpResponse(raw);
}

} // namespace

Response Get(const std::string& host, int port, const std::string& path) {
    std::ostringstream req;
    req << "GET " << path << " HTTP/1.1\r\n"
        << "Host: " << host << ':' << port << "\r\n"
        << "Connection: close\r\n"
        << "Accept: application/json\r\n\r\n";

    return DoRequest(host, port, req.str());
}

Response PostJson(const std::string& host, int port, const std::string& path, const std::string& jsonBody) {
    std::ostringstream req;
    req << "POST " << path << " HTTP/1.1\r\n"
        << "Host: " << host << ':' << port << "\r\n"
        << "Connection: close\r\n"
        << "Content-Type: application/json\r\n"
        << "Accept: application/json\r\n"
        << "Content-Length: " << jsonBody.size() << "\r\n\r\n"
        << jsonBody;

    return DoRequest(host, port, req.str());
}

}
#else
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

namespace Http {

namespace {

Response ParseHttpResponse(const std::string& raw) {
    Response response;
    response.raw = raw;

    std::size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        response.body = raw;
        return response;
    }

    std::string headers = raw.substr(0, headerEnd);
    response.body = raw.substr(headerEnd + 4);

    std::size_t firstLineEnd = headers.find("\r\n");
    std::string statusLine = headers.substr(0, firstLineEnd);

    std::istringstream lineStream(statusLine);
    std::string version;
    lineStream >> version >> response.statusCode;

    return response;
}

Response DoRequest(const std::string& host, int port, const std::string& request) {
    Response response;

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* info = nullptr;
    std::string portString = std::to_string(port);

    if (getaddrinfo(host.c_str(), portString.c_str(), &hints, &info) != 0) {
        return response;
    }

    int sock = -1;
    for (addrinfo* p = info; p != nullptr; p = p->ai_next) {
        sock = static_cast<int>(socket(p->ai_family, p->ai_socktype, p->ai_protocol));
        if (sock < 0) {
            continue;
        }

        if (connect(sock, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }

        close(sock);
        sock = -1;
    }

    freeaddrinfo(info);

    if (sock < 0) {
        return response;
    }

    std::size_t sent = 0;
    while (sent < request.size()) {
        ssize_t wrote = send(sock, request.data() + sent, request.size() - sent, 0);
        if (wrote <= 0) {
            close(sock);
            return response;
        }
        sent += static_cast<std::size_t>(wrote);
    }

    std::string raw;
    char buffer[4096];
    while (true) {
        ssize_t bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            break;
        }
        raw.append(buffer, static_cast<std::size_t>(bytes));
    }

    close(sock);
    return ParseHttpResponse(raw);
}

} // namespace

Response Get(const std::string& host, int port, const std::string& path) {
    std::ostringstream req;
    req << "GET " << path << " HTTP/1.1\r\n"
        << "Host: " << host << ':' << port << "\r\n"
        << "Connection: close\r\n"
        << "Accept: application/json\r\n\r\n";

    return DoRequest(host, port, req.str());
}

Response PostJson(const std::string& host, int port, const std::string& path, const std::string& jsonBody) {
    std::ostringstream req;
    req << "POST " << path << " HTTP/1.1\r\n"
        << "Host: " << host << ':' << port << "\r\n"
        << "Connection: close\r\n"
        << "Content-Type: application/json\r\n"
        << "Accept: application/json\r\n"
        << "Content-Length: " << jsonBody.size() << "\r\n\r\n"
        << jsonBody;

    return DoRequest(host, port, req.str());
}

} // namespace Http
#endif
