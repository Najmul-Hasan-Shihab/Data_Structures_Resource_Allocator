#include "AllocatorService.h"

#include <iostream>
#include <limits>
#include <string>

using era::AllocatorService;

namespace {

void clearInputLine() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int readInt(const std::string& prompt) {
    int value = 0;
    std::cout << prompt;
    while (!(std::cin >> value)) {
        std::cin.clear();
        clearInputLine();
        std::cout << "Enter a valid integer: ";
    }
    clearInputLine();
    return value;
}

std::string readString(const std::string& prompt) {
    std::string value;
    std::cout << prompt;
    std::getline(std::cin, value);
    return value;
}

void printMenu() {
    std::cout << "\nEmergency Resource Allocator\n"
              << "1. Add resource\n"
              << "2. Add emergency\n"
              << "3. Serve next emergency\n"
              << "4. Update emergency severity\n"
              << "5. Release resource\n"
              << "6. Show status\n"
              << "7. Run screenshot demo\n"
              << "8. Search patient record\n"
              << "9. Show ordered resources\n"
              << "10. Add road connection\n"
              << "11. Show route for patient\n"
              << "12. Run verbose demo scenario\n"
              << "0. Exit\n";
}

}  // namespace

int main() {
    AllocatorService service;

    while (true) {
        printMenu();
        int choice = readInt("Choose an option: ");

        switch (choice) {
            case 1: {
                int id = readInt("Resource ID: ");
                std::string type = readString("Resource type: ");
                int location = readInt("Location node: ");
                service.addResource(id, type, location);
                break;
            }
            case 2: {
                int id = readInt("Emergency ID: ");
                std::string name = readString("Patient name: ");
                int severity = readInt("Severity (1-10): ");
                std::string type = readString("Emergency type: ");
                std::string required = readString("Required resource type: ");
                int location = readInt("Patient location node (0 if unknown): ");
                service.addEmergency(id, name, severity, type, required, location);
                break;
            }
            case 3:
                service.serveNext();
                break;
            case 4: {
                int id = readInt("Emergency ID: ");
                int severity = readInt("New severity: ");
                service.updateEmergencySeverity(id, severity);
                break;
            }
            case 5: {
                int id = readInt("Resource ID: ");
                service.releaseResource(id);
                break;
            }
            case 6:
                service.printStatus();
                break;
            case 7:
                service.runDemoScenario();
                break;
            case 8: {
                int id = readInt("Patient ID: ");
                service.printPatient(id);
                break;
            }
            case 9:
                service.printOrderedResources();
                break;
            case 10: {
                int from = readInt("From location node: ");
                int to = readInt("To location node: ");
                int weight = readInt("Road weight: ");
                service.addRoad(from, to, weight);
                break;
            }
            case 11: {
                int id = readInt("Patient ID: ");
                service.printRouteForPatient(id);
                break;
            }
            case 12:
                service.runVerboseDemoScenario();
                break;
            case 0:
                std::cout << "Exiting.\n";
                return 0;
            default:
                std::cout << "Invalid choice.\n";
                break;
        }
    }
}
