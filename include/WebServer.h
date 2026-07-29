#pragma once

#include "AllocatorService.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <initializer_list>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <stdexcept>

#include <winsock2.h>
#include <ws2tcpip.h>

namespace era {

class WebServer {
public:
    WebServer(AllocatorService& service, std::filesystem::path root, unsigned short port = 8080)
        : service_(service), root_(std::move(root)), port_(port) {
    }

    int run() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "Failed to initialize Winsock.\n";
            return 1;
        }

        SOCKET serverSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create server socket.\n";
            WSACleanup();
            return 1;
        }

        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        serverAddress.sin_port = htons(port_);

        const int reuseEnabled = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuseEnabled), sizeof(reuseEnabled));

        if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
            std::cerr << "Failed to bind to port " << port_ << ".\n";
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Failed to listen on port " << port_ << ".\n";
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        std::cout << "Web server running at http://localhost:" << port_ << "/\n";
        std::cout << "Serving UI from " << root_.string() << "\n";
        std::cout << "Press Ctrl+C to stop.\n";

        while (true) {
            sockaddr_in clientAddress{};
            int clientLength = sizeof(clientAddress);
            SOCKET clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientLength);
            if (clientSocket == INVALID_SOCKET) {
                continue;
            }

            handleClient(clientSocket);
            closesocket(clientSocket);
        }

        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }

private:
    struct HttpRequest {
        std::string method;
        std::string target;
        std::string path;
        std::string query;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
    };

    struct HttpResponse {
        int status = 200;
        std::string contentType = "text/plain; charset=utf-8";
        std::string body;
    };

    void handleClient(SOCKET clientSocket) {
        try {
            std::string rawRequest = readRequest(clientSocket);
            if (rawRequest.empty()) {
                return;
            }

            HttpRequest request = parseRequest(rawRequest);
            HttpResponse response = route(request);
            sendResponse(clientSocket, response);
        } catch (const std::exception& error) {
            std::cerr << "Request handling error: " << error.what() << '\n';
            try {
                sendResponse(clientSocket, errorResponse(500, "Internal server error."));
            } catch (...) {
                // Ignore secondary failures while keeping the server alive.
            }
        }
    }

    std::string readRequest(SOCKET clientSocket) {
        std::string request;
        char buffer[4096];
        int bytesRead = 0;

        while (request.find("\r\n\r\n") == std::string::npos) {
            bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0) {
                return {};
            }
            request.append(buffer, buffer + bytesRead);
            if (request.size() > 64U * 1024U) {
                return {};
            }
        }

        const std::size_t headerEnd = request.find("\r\n\r\n");
        const std::string headerBlock = request.substr(0, headerEnd);
        const auto headers = parseHeaders(headerBlock);
            const auto contentLengthIter = headers.find("content-length");
            std::size_t contentLength = 0U;
            if (contentLengthIter != headers.end()) {
                contentLength = parseUnsigned(contentLengthIter->second);
            }

        std::size_t bodyBytes = request.size() - (headerEnd + 4U);
        while (bodyBytes < contentLength) {
            bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0) {
                break;
            }
            request.append(buffer, buffer + bytesRead);
            bodyBytes += static_cast<std::size_t>(bytesRead);
        }

        return request;
    }

    HttpRequest parseRequest(const std::string& rawRequest) const {
        HttpRequest request;
        const std::size_t headerEnd = rawRequest.find("\r\n\r\n");
        const std::string headerBlock = rawRequest.substr(0, headerEnd);
        request.body = headerEnd == std::string::npos ? std::string{} : rawRequest.substr(headerEnd + 4U);

        std::istringstream stream(headerBlock);
        std::string requestLine;
        std::getline(stream, requestLine);
        if (!requestLine.empty() && requestLine.back() == '\r') {
            requestLine.pop_back();
        }

        std::istringstream lineStream(requestLine);
        lineStream >> request.method >> request.target;
        auto [path, query] = splitTarget(request.target);
        request.path = path;
        request.query = query;

        std::string headerLine;
        while (std::getline(stream, headerLine)) {
            if (!headerLine.empty() && headerLine.back() == '\r') {
                headerLine.pop_back();
            }
            const std::size_t colonPos = headerLine.find(':');
            if (colonPos == std::string::npos) {
                continue;
            }
            std::string key = headerLine.substr(0, colonPos);
            std::string value = headerLine.substr(colonPos + 1U);
            trim(key);
            trim(value);
            toLower(key);
            request.headers[key] = value;
        }

        return request;
    }

    HttpResponse route(const HttpRequest& request) {
        if (request.method == "GET") {
            if (request.path == "/" || request.path == "/index.html") {
                return serveStatic("index.html", "text/html; charset=utf-8");
            }
            if (request.path == "/styles.css") {
                return serveStatic("styles.css", "text/css; charset=utf-8");
            }
            if (request.path == "/app.js") {
                return serveStatic("app.js", "application/javascript; charset=utf-8");
            }
            if (request.path == "/api/state") {
                return jsonResponse(stateJson());
            }
            return notFound();
        }

        if (request.method == "POST") {
            const auto form = parseFormUrlEncoded(request.body);
            if (request.path == "/api/resource") {
                if (!hasFields(form, {"id", "type", "location"})) {
                    return badRequest("Missing resource fields.");
                }
                bool ok = service_.addResource(toInt(form, "id"), form.at("type"), toInt(form, "location"), true);
                return ok ? jsonResponse(stateJson()) : badRequest("Resource already exists.");
            }
            if (request.path == "/api/emergency") {
                if (!hasFields(form, {"id", "patientName", "severity", "type", "requiredResourceType"})) {
                    return badRequest("Missing emergency fields.");
                }
                bool ok = service_.addEmergency(
                    toInt(form, "id"),
                    form.at("patientName"),
                    toInt(form, "severity"),
                    form.at("type"),
                    form.at("requiredResourceType"),
                    true);
                return ok ? jsonResponse(stateJson()) : badRequest("Emergency already exists.");
            }
            if (request.path == "/api/serve-next") {
                service_.serveNext(true);
                return jsonResponse(stateJson());
            }
            if (request.path == "/api/update-severity") {
                if (!hasFields(form, {"id", "severity"})) {
                    return badRequest("Missing severity fields.");
                }
                bool ok = service_.updateEmergencySeverity(toInt(form, "id"), toInt(form, "severity"), true);
                return ok ? jsonResponse(stateJson()) : badRequest("Emergency not found in pending queue.");
            }
            if (request.path == "/api/release-resource") {
                if (!hasFields(form, {"resourceId"})) {
                    return badRequest("Missing resourceId field.");
                }
                bool ok = service_.releaseResource(toInt(form, "resourceId"), true);
                return ok ? jsonResponse(stateJson()) : badRequest("Resource is not assigned.");
            }
            if (request.path == "/api/demo") {
                service_.seedDemoData(true);
                return jsonResponse(stateJson());
            }
            if (request.path == "/api/reset") {
                service_.reset();
                return jsonResponse(stateJson());
            }
            return notFound();
        }

        if (request.method == "OPTIONS") {
            HttpResponse response;
            response.status = 204;
            response.contentType = "text/plain; charset=utf-8";
            return response;
        }

        return methodNotAllowed();
    }

    HttpResponse serveStatic(const std::string& fileName, const std::string& contentType) const {
        const std::filesystem::path filePath = root_ / fileName;
        std::ifstream stream(filePath, std::ios::binary);
        if (!stream) {
            return notFound();
        }

        std::string body((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        HttpResponse response;
        response.status = 200;
        response.contentType = contentType;
        response.body = std::move(body);
        return response;
    }

    HttpResponse jsonResponse(const std::string& body) const {
        HttpResponse response;
        response.status = 200;
        response.contentType = "application/json; charset=utf-8";
        response.body = body;
        return response;
    }

    HttpResponse badRequest(const std::string& message) const {
        return errorResponse(400, message);
    }

    HttpResponse notFound() const {
        return errorResponse(404, "Not found.");
    }

    HttpResponse methodNotAllowed() const {
        return errorResponse(405, "Method not allowed.");
    }

    HttpResponse errorResponse(int status, const std::string& message) const {
        HttpResponse response;
        response.status = status;
        response.contentType = "application/json; charset=utf-8";
        response.body = std::string{"{\"error\":\""} + jsonEscape(message) + "\"}";
        return response;
    }

    void sendResponse(SOCKET clientSocket, const HttpResponse& response) {
        std::ostringstream output;
        output << "HTTP/1.1 " << response.status << ' ' << statusText(response.status) << "\r\n"
               << "Content-Type: " << response.contentType << "\r\n"
               << "Content-Length: " << response.body.size() << "\r\n"
               << "Connection: close\r\n"
               << "Access-Control-Allow-Origin: *\r\n"
               << "\r\n"
               << response.body;
        const std::string data = output.str();
        sendAll(clientSocket, data);
    }

    static void sendAll(SOCKET clientSocket, const std::string& data) {
        std::size_t totalSent = 0U;
        while (totalSent < data.size()) {
            const int sent = send(clientSocket, data.data() + totalSent, static_cast<int>(data.size() - totalSent), 0);
            if (sent == SOCKET_ERROR || sent == 0) {
                break;
            }
            totalSent += static_cast<std::size_t>(sent);
        }
    }

    std::string stateJson() const {
        DashboardState state = service_.snapshot();
        std::ostringstream output;
        output << '{'
               << "\"pendingCount\":" << state.pendingCount << ','
               << "\"availableCount\":" << state.availableCount << ','
               << "\"waitingCount\":" << state.waitingCount << ','
               << "\"pending\":" << emergenciesJson(state.pending) << ','
               << "\"resources\":" << resourcesJson(state.resources) << ','
               << "\"waiting\":" << emergenciesJson(state.waiting) << ','
               << "\"history\":" << historyJson(state.history)
               << '}';
        return output.str();
    }

    static std::string emergenciesJson(const std::vector<Emergency>& emergencies) {
        std::ostringstream output;
        output << '[';
        for (std::size_t i = 0; i < emergencies.size(); ++i) {
            if (i > 0U) {
                output << ',';
            }
            const Emergency& emergency = emergencies[i];
            output << '{'
                   << "\"id\":" << emergency.id << ','
                   << "\"patientName\":\"" << jsonEscape(emergency.patientName) << "\"," 
                   << "\"severity\":" << emergency.severity << ','
                   << "\"type\":\"" << jsonEscape(emergency.type) << "\"," 
                   << "\"requiredResourceType\":\"" << jsonEscape(emergency.requiredResourceType) << "\"," 
                   << "\"arrivalTime\":" << emergency.arrivalTime
                   << '}';
        }
        output << ']';
        return output.str();
    }

    static std::string resourcesJson(const std::vector<Resource>& resources) {
        std::ostringstream output;
        output << '[';
        for (std::size_t i = 0; i < resources.size(); ++i) {
            if (i > 0U) {
                output << ',';
            }
            const Resource& resource = resources[i];
            output << '{'
                   << "\"id\":" << resource.id << ','
                   << "\"type\":\"" << jsonEscape(resource.type) << "\"," 
                   << "\"available\":" << (resource.available ? "true" : "false") << ','
                   << "\"location\":" << resource.location << ','
                   << "\"assignedEmergencyId\":" << resource.assignedEmergencyId << ','
                   << "\"assignedEmergencyName\":\"" << jsonEscape(resource.assignedEmergencyName) << "\""
                   << '}';
        }
        output << ']';
        return output.str();
    }

    static std::string historyJson(const std::vector<AllocationRecord>& history) {
        std::ostringstream output;
        output << '[';
        for (std::size_t i = 0; i < history.size(); ++i) {
            if (i > 0U) {
                output << ',';
            }
            const AllocationRecord& record = history[i];
            output << '{'
                   << "\"emergencyId\":" << record.emergencyId << ','
                   << "\"emergencyName\":\"" << jsonEscape(record.emergencyName) << "\"," 
                   << "\"resourceId\":" << record.resourceId << ','
                   << "\"resourceType\":\"" << jsonEscape(record.resourceType) << "\"," 
                   << "\"status\":\"" << jsonEscape(record.status) << "\"," 
                   << "\"timeStamp\":" << record.timeStamp
                   << '}';
        }
        output << ']';
        return output.str();
    }

    static std::unordered_map<std::string, std::string> parseHeaders(const std::string& headerBlock) {
        std::unordered_map<std::string, std::string> headers;
        std::istringstream stream(headerBlock);
        std::string line;
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            const std::size_t colonPos = line.find(':');
            if (colonPos == std::string::npos) {
                continue;
            }
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1U);
            trim(key);
            trim(value);
            toLower(key);
            headers.emplace(std::move(key), std::move(value));
        }
        return headers;
    }

    static std::pair<std::string, std::string> splitTarget(const std::string& target) {
        const std::size_t questionMark = target.find('?');
        if (questionMark == std::string::npos) {
            return {target, {}};
        }
        return {target.substr(0, questionMark), target.substr(questionMark + 1U)};
    }

    static std::unordered_map<std::string, std::string> parseFormUrlEncoded(const std::string& body) {
        std::unordered_map<std::string, std::string> result;
        std::size_t start = 0U;
        while (start < body.size()) {
            const std::size_t amp = body.find('&', start);
            const std::string pair = body.substr(start, amp == std::string::npos ? std::string::npos : amp - start);
            const std::size_t equals = pair.find('=');
            if (equals != std::string::npos) {
                std::string key = urlDecode(pair.substr(0U, equals));
                std::string value = urlDecode(pair.substr(equals + 1U));
                result.emplace(std::move(key), std::move(value));
            }
            if (amp == std::string::npos) {
                break;
            }
            start = amp + 1U;
        }
        return result;
    }

    static bool hasFields(const std::unordered_map<std::string, std::string>& form, std::initializer_list<const char*> fields) {
        for (const char* field : fields) {
            if (form.find(field) == form.end()) {
                return false;
            }
        }
        return true;
    }

    static int toInt(const std::unordered_map<std::string, std::string>& form, const std::string& key) {
        return std::stoi(form.at(key));
    }

    static std::size_t parseUnsigned(const std::string& value) {
        std::size_t index = 0U;
        unsigned long parsed = std::stoul(value, &index);
        if (index != value.size()) {
            throw std::invalid_argument("Invalid unsigned value");
        }
        return static_cast<std::size_t>(parsed);
    }

    static std::string urlDecode(const std::string& value) {
        std::string decoded;
        decoded.reserve(value.size());
        for (std::size_t i = 0; i < value.size(); ++i) {
            const char ch = value[i];
            if (ch == '+') {
                decoded.push_back(' ');
            } else if (ch == '%' && i + 2U < value.size()) {
                const std::string hex = value.substr(i + 1U, 2U);
                decoded.push_back(static_cast<char>(std::stoi(hex, nullptr, 16)));
                i += 2U;
            } else {
                decoded.push_back(ch);
            }
        }
        return decoded;
    }

    static void trim(std::string& value) {
        const auto left = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
        const auto right = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
        if (left >= right) {
            value.clear();
            return;
        }
        value.assign(left, right);
    }

    static void toLower(std::string& value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
    }

    static std::string jsonEscape(const std::string& value) {
        std::string escaped;
        escaped.reserve(value.size() + 8U);
        for (const char ch : value) {
            switch (ch) {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\b': escaped += "\\b"; break;
                case '\f': escaped += "\\f"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(ch) < 0x20U) {
                        std::ostringstream hex;
                        hex << "\\u" << std::hex << std::uppercase << static_cast<int>(static_cast<unsigned char>(ch));
                        escaped += hex.str();
                    } else {
                        escaped.push_back(ch);
                    }
                    break;
            }
        }
        return escaped;
    }

    static const char* statusText(int status) {
        switch (status) {
            case 200: return "OK";
            case 204: return "No Content";
            case 400: return "Bad Request";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            default: return "OK";
        }
    }

    AllocatorService& service_;
    std::filesystem::path root_;
    unsigned short port_;
};

}  // namespace era
