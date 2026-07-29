#include "AllocatorService.h"
#include "WebServer.h"

#include <filesystem>
#include <iostream>

int main() {
    era::AllocatorService service;
    era::WebServer server(service, std::filesystem::path{"ui"}, 8080);
    return server.run();
}
