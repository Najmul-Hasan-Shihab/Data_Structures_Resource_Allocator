#pragma once

#include "AllocationRecord.h"
#include "Emergency.h"
#include "LocationGraph.h"
#include "MaxHeap.h"
#include "PatientRegistry.h"
#include "ResourceIndexTree.h"
#include "ResourceTable.h"
#include "WaitingQueue.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace era {

struct DashboardState {
    std::size_t pendingCount = 0U;
    std::size_t availableCount = 0U;
    std::size_t waitingCount = 0U;
    std::vector<Emergency> pending;
    std::vector<Resource> resources;
    std::vector<Resource> orderedResources;
    std::vector<PatientRecord> patients;
    std::vector<Emergency> waiting;
    std::vector<AllocationRecord> history;
};

struct RouteSuggestion {
    int patientId = 0;
    int resourceId = -1;
    int fromLocation = 0;
    int toLocation = 0;
    int distance = -1;
    std::vector<int> path;
};

class AllocatorService {
public:
    bool addRoad(int from, int to, int weight, bool quiet = false) {
        routeGraph_.addEdge(from, to, weight);
        if (!quiet) {
            std::cout << "Added road: " << from << " <-> " << to << " (w=" << weight << ")\n";
        }
        return true;
    }

    bool addResource(int id, const std::string& type, int location, bool quiet = false) {
        Resource resource;
        resource.id = id;
        resource.type = type;
        resource.location = location;
        if (resources_.add(resource)) {
            Resource* stored = resources_.find(id);
            if (stored != nullptr) {
                resourceIndex_.insert(stored);
            }
            if (!quiet) {
                std::cout << "Added resource: " << resource << '\n';
            }
            retryWaitingCases(quiet);
            return true;
        }
        if (!quiet) {
            std::cout << "Resource #" << id << " already exists.\n";
        }
        return false;
    }

    bool addEmergency(int id, const std::string& name, int severity, const std::string& type, const std::string& requiredResourceType, bool quiet = false) {
        return addEmergency(id, name, severity, type, requiredResourceType, 0, quiet);
    }

    bool addEmergency(int id, const std::string& name, int severity, const std::string& type, const std::string& requiredResourceType, int location, bool quiet = false) {
        Emergency emergency;
        emergency.id = id;
        emergency.patientName = name;
        emergency.severity = severity;
        emergency.type = type;
        emergency.requiredResourceType = requiredResourceType.empty() ? type : requiredResourceType;
        emergency.location = location;
        emergency.arrivalTime = currentTime_++;
        if (pending_.insert(emergency)) {
            patients_.upsert(PatientRecord{emergency.id, emergency.patientName, emergency.type, emergency.requiredResourceType, emergency.location, emergency.severity, "PENDING", -1});
            if (!quiet) {
                std::cout << "Queued emergency: " << emergency << '\n';
            }
            return true;
        }
        if (!quiet) {
            std::cout << "Emergency #" << id << " already exists in the heap.\n";
        }
        return false;
    }

    bool updateEmergencySeverity(int emergencyId, int newSeverity, bool quiet = false) {
        if (pending_.updatePriority(emergencyId, newSeverity)) {
            if (PatientRecord* patient = patients_.find(emergencyId)) {
                patient->severity = newSeverity;
            }
            if (!quiet) {
                std::cout << "Updated severity for emergency #" << emergencyId << " to " << newSeverity << '\n';
            }
            return true;
        }
        if (!quiet) {
            std::cout << "Emergency #" << emergencyId << " not found in pending heap.\n";
        }
        return false;
    }

    bool serveNext(bool quiet = false) {
        auto next = pending_.extractMax();
        if (!next.has_value()) {
            if (!quiet) {
                std::cout << "No pending emergencies to serve.\n";
            }
            return false;
        }
        dispatchEmergency(next.value(), "dispatched from heap", quiet);
        return true;
    }

    bool releaseResource(int resourceId, bool quiet = false) {
        const Resource* resource = resources_.find(resourceId);
        if (resource == nullptr || resource->available) {
            if (!quiet) {
                std::cout << "Resource #" << resourceId << " could not be released.\n";
            }
            return false;
        }
        if (resource->assignedEmergencyId >= 0) {
            patients_.updateStatus(resource->assignedEmergencyId, "COMPLETED", resourceId);
        }
        if (!resources_.release(resourceId)) {
            if (!quiet) {
                std::cout << "Resource #" << resourceId << " could not be released.\n";
            }
            return false;
        }
        if (!quiet) {
            std::cout << "Released resource #" << resourceId << '\n';
        }
        retryWaitingCases(quiet);
        return true;
    }

    void reset() {
        pending_ = MaxHeap{};
        resources_ = ResourceTable{};
        waiting_ = WaitingQueue{};
        patients_.clear();
        resourceIndex_.clear();
        routeGraph_.clear();
        history_.clear();
        currentTime_ = 0;
    }

    void seedDemoData(bool quiet = true) {
        reset();
        addRoad(1, 2, 4, quiet);
        addRoad(2, 3, 3, quiet);
        addRoad(3, 4, 2, quiet);
        addRoad(1, 5, 6, quiet);
        addRoad(4, 5, 2, quiet);

        addResource(101, "Ambulance", 1, quiet);
        addResource(102, "Ambulance", 2, quiet);
        addResource(201, "Doctor", 1, quiet);
        addResource(202, "Doctor", 2, quiet);
        addResource(301, "ICU_Bed", 3, quiet);

        addEmergency(1, "Asha", 7, "Trauma", "Ambulance", 5, quiet);
        addEmergency(2, "Ravi", 9, "Cardiac", "Doctor", 2, quiet);
        addEmergency(3, "Neha", 5, "Trauma", "Ambulance", 4, quiet);
        addEmergency(4, "Imran", 10, "Critical", "ICU_Bed", 3, quiet);
        addEmergency(5, "Meera", 6, "Cardiac", "Doctor", 1, quiet);
    }

    DashboardState snapshot(std::size_t topK = 5U) const {
        DashboardState state;
        state.pendingCount = pending_.size();
        state.availableCount = resources_.availableCount();
        state.waitingCount = waiting_.size();
        state.pending = pending_.snapshot();
        state.resources = resources_.snapshot();
        state.orderedResources = resourceIndex_.snapshotOrdered();
        state.patients = patients_.snapshot();
        state.waiting = waiting_.snapshot();
        state.history = history_;
        return state;
    }

    const PatientRecord* findPatient(int patientId) const {
        return patients_.find(patientId);
    }

    void printPatient(int patientId) const {
        const PatientRecord* patient = findPatient(patientId);
        if (patient == nullptr) {
            std::cout << "Patient #" << patientId << " not found.\n";
            return;
        }
        std::cout << *patient << '\n';
        int preferredResourceId = patient->assignedResourceId >= 0 ? patient->assignedResourceId : -1;
        if (auto route = suggestRoute(patientId, preferredResourceId)) {
            std::cout << "  route: ";
            printRoute(*route);
            std::cout << '\n';
        }
    }

    void printOrderedResources() const {
        std::cout << "\nOrdered resources (BST):\n";
        auto resources = resourceIndex_.snapshotOrdered();
        if (resources.empty()) {
            std::cout << "  (none)\n";
            return;
        }
        for (const auto& resource : resources) {
            std::cout << "  " << resource << '\n';
        }
    }

    void printPatients() const {
        std::cout << "\nPatient registry (linked list):\n";
        auto patients = patients_.snapshot();
        if (patients.empty()) {
            std::cout << "  (none)\n";
            return;
        }
        for (const auto& patient : patients) {
            std::cout << "  " << patient << '\n';
        }
    }

    void printRouteForPatient(int patientId) const {
        const PatientRecord* patient = findPatient(patientId);
        int preferredResourceId = patient != nullptr && patient->assignedResourceId >= 0 ? patient->assignedResourceId : -1;
        auto route = suggestRoute(patientId, preferredResourceId);
        if (!route.has_value()) {
            std::cout << "No route suggestion available for patient #" << patientId << "\n";
            return;
        }
        std::cout << "Route suggestion for patient #" << patientId << ": ";
        printRoute(route.value());
        std::cout << '\n';
    }

    std::optional<RouteSuggestion> routeForPatient(int patientId, int preferredResourceId = -1) const {
        return suggestRoute(patientId, preferredResourceId);
    }

    void runDemoScenario() {
        std::cout << "\n=== Emergency Resource Allocator Demo (Screenshot Mode) ===\n";
        seedDemoData(false);

        std::cout << "Seeded demo data: 5 emergencies, 5 resources, 4 roads.\n";
        std::cout << "Top priority queue snapshot: \n";
        auto topItems = pending_.topK(3U);
        for (const auto& emergency : topItems) {
            std::cout << "  " << emergency << '\n';
        }

        std::cout << "\nServing one case to show dispatch and history...\n";
        serveNext(false);

        std::cout << "\nRoute preview for patient #4: ";
        printRouteForPatient(4);

        std::cout << "\nPatient registry snapshot: \n";
        auto patients = patients_.snapshot();
        for (const auto& patient : patients) {
            std::cout << "  " << patient << '\n';
        }

        std::cout << "\nOrdered resources (BST):\n";
        auto orderedResources = resourceIndex_.snapshotOrdered();
        for (const auto& resource : orderedResources) {
            std::cout << "  " << resource << '\n';
        }

        std::cout << "=== End Screenshot Demo ===\n\n";
    }

    void runVerboseDemoScenario() {
        std::cout << "\n=== Emergency Resource Allocator Demo ===\n";
        seedDemoData(false);

        printStatus();

        serveNext(false);
        serveNext(false);
        updateEmergencySeverity(3, 10, false);
        serveNext(false);
        serveNext(false);

        releaseResource(201, false);
        printOrderedResources();
        printPatients();
        printRouteForPatient(4);
        printStatus();
        std::cout << "=== End Demo ===\n\n";
    }

    void printStatus(std::size_t topK = 5U) const {
        std::cout << "\n--- Current Status ---\n";
        std::cout << "Pending emergencies: " << pending_.size() << '\n';
        std::cout << "Available resources: " << resources_.availableCount() << '\n';
        std::cout << "Waiting queue: " << waiting_.size() << '\n';

        std::cout << "Top priorities:\n";
        auto topItems = pending_.topK(topK);
        if (topItems.empty()) {
            std::cout << "  (none)\n";
        } else {
            for (const auto& emergency : topItems) {
                std::cout << "  " << emergency << '\n';
            }
        }

        std::cout << "Resources:\n";
        auto resources = resources_.snapshot();
        if (resources.empty()) {
            std::cout << "  (none)\n";
        } else {
            for (const auto& resource : resources) {
                std::cout << "  " << resource << '\n';
            }
        }

        std::cout << "Ordered resources (BST):\n";
        auto orderedResources = resourceIndex_.snapshotOrdered();
        if (orderedResources.empty()) {
            std::cout << "  (none)\n";
        } else {
            for (const auto& resource : orderedResources) {
                std::cout << "  " << resource << '\n';
            }
        }

        std::cout << "Patients (linked list):\n";
        auto patients = patients_.snapshot();
        if (patients.empty()) {
            std::cout << "  (none)\n";
        } else {
            for (const auto& patient : patients) {
                std::cout << "  " << patient << '\n';
            }
        }

        std::cout << "Waiting queue:\n";
        auto waiting = waiting_.snapshot();
        if (waiting.empty()) {
            std::cout << "  (none)\n";
        } else {
            for (const auto& emergency : waiting) {
                std::cout << "  " << emergency << '\n';
            }
        }

        std::cout << "History:\n";
        if (history_.empty()) {
            std::cout << "  (none)\n";
        } else {
            for (const auto& record : history_) {
                std::cout << "  emergency #" << record.emergencyId
                          << " -> resource #" << record.resourceId
                          << " [" << record.status << "]"
                          << " @t" << record.timeStamp << '\n';
            }
        }
        std::cout << "----------------------\n";
    }

private:
    void dispatchEmergency(const Emergency& emergency, const std::string& note, bool quiet) {
        Resource* resource = resources_.reserveByType(emergency.requiredResourceType, emergency);
        if (resource == nullptr) {
            waiting_.push(emergency);
            history_.push_back({emergency.id, emergency.patientName, -1, emergency.requiredResourceType, "WAITING", currentTime_++});
            patients_.updateStatus(emergency.id, "WAITING", -1);
            if (!quiet) {
                std::cout << "Emergency queued for later: " << emergency << '\n';
            }
            return;
        }

        patients_.updateStatus(emergency.id, "ASSIGNED", resource->id);
        std::string actionNote = note;
        if (auto route = suggestRoute(emergency.id, resource->id)) {
            actionNote += " | route=";
            actionNote += routeToString(*route);
        }
        history_.push_back({emergency.id, emergency.patientName, resource->id, resource->type, actionNote, currentTime_++});
        if (!quiet) {
            std::cout << "Assigned emergency #" << emergency.id << " to resource #" << resource->id << " (" << resource->type << ")\n";
            if (auto route = suggestRoute(emergency.id, resource->id)) {
                std::cout << "  route: ";
                printRoute(*route);
                std::cout << '\n';
            }
        }
    }

    void retryWaitingCases(bool quiet) {
        if (waiting_.empty()) {
            return;
        }

        std::size_t pendingCount = waiting_.size();
        WaitingQueue deferred;
        for (std::size_t i = 0; i < pendingCount; ++i) {
            Emergency emergency = waiting_.pop();
            Resource* resource = resources_.reserveByType(emergency.requiredResourceType, emergency);
            if (resource == nullptr) {
                deferred.push(emergency);
                continue;
            }
            patients_.updateStatus(emergency.id, "ASSIGNED", resource->id);
            std::string note = "served from waiting queue";
            if (auto route = suggestRoute(emergency.id, resource->id)) {
                note += " | route=";
                note += routeToString(*route);
            }
            history_.push_back({emergency.id, emergency.patientName, resource->id, resource->type, note, currentTime_++});
            if (!quiet) {
                std::cout << "Served waiting emergency #" << emergency.id << " using resource #" << resource->id << '\n';
                if (auto route = suggestRoute(emergency.id, resource->id)) {
                    std::cout << "  route: ";
                    printRoute(*route);
                    std::cout << '\n';
                }
            }
        }

        while (!deferred.empty()) {
            waiting_.push(deferred.pop());
        }
    }

    std::optional<RouteSuggestion> suggestRoute(int patientId, int preferredResourceId = -1) const {
        const PatientRecord* patient = patients_.find(patientId);
        if (patient == nullptr || patient->location <= 0) {
            return std::nullopt;
        }

        RouteSuggestion best;
        bool found = false;

        for (const auto& resource : resources_.snapshot()) {
            if (resource.type != patient->requiredResourceType || resource.location <= 0) {
                continue;
            }
            if (preferredResourceId < 0 && !resource.available) {
                continue;
            }
            if (preferredResourceId >= 0 && resource.id != preferredResourceId) {
                continue;
            }

            auto path = routeGraph_.shortestPath(patient->location, resource.location);
            if (!path.has_value()) {
                continue;
            }

            const RouteResult& result = path.value();
            if (!found || result.distance < best.distance) {
                found = true;
                best.patientId = patientId;
                best.resourceId = resource.id;
                best.fromLocation = patient->location;
                best.toLocation = resource.location;
                best.distance = result.distance;
                best.path = result.path;
            }
        }

        if (!found) {
            return std::nullopt;
        }
        return best;
    }

    static std::string routeToString(const RouteSuggestion& route) {
        std::ostringstream output;
        for (std::size_t i = 0; i < route.path.size(); ++i) {
            if (i > 0U) {
                output << " -> ";
            }
            output << route.path[i];
        }
        output << " (distance=" << route.distance << ", resource=#" << route.resourceId << ")";
        return output.str();
    }

    static void printRoute(const RouteSuggestion& route) {
        std::cout << routeToString(route);
    }

    MaxHeap pending_;
    ResourceTable resources_;
    ResourceIndexTree resourceIndex_;
    PatientRegistry patients_;
    LocationGraph routeGraph_;
    WaitingQueue waiting_;
    std::vector<AllocationRecord> history_;
    int currentTime_ = 0;
};

}  // namespace era
