#pragma once

#include "AllocationRecord.h"
#include "Emergency.h"
#include "MaxHeap.h"
#include "ResourceTable.h"
#include "WaitingQueue.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
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
    std::vector<Emergency> waiting;
    std::vector<AllocationRecord> history;
};

class AllocatorService {
public:
    bool addResource(int id, const std::string& type, int location, bool quiet = false) {
        Resource resource;
        resource.id = id;
        resource.type = type;
        resource.location = location;
        if (resources_.add(resource)) {
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
        Emergency emergency;
        emergency.id = id;
        emergency.patientName = name;
        emergency.severity = severity;
        emergency.type = type;
        emergency.requiredResourceType = requiredResourceType.empty() ? type : requiredResourceType;
        emergency.arrivalTime = currentTime_++;
        if (pending_.insert(emergency)) {
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
        history_.clear();
        currentTime_ = 0;
    }

    void seedDemoData(bool quiet = true) {
        reset();
        addResource(101, "Ambulance", 1, quiet);
        addResource(102, "Ambulance", 2, quiet);
        addResource(201, "Doctor", 1, quiet);
        addResource(202, "Doctor", 2, quiet);
        addResource(301, "ICU_Bed", 3, quiet);

        addEmergency(1, "Asha", 7, "Trauma", "Ambulance", quiet);
        addEmergency(2, "Ravi", 9, "Cardiac", "Doctor", quiet);
        addEmergency(3, "Neha", 5, "Trauma", "Ambulance", quiet);
        addEmergency(4, "Imran", 10, "Critical", "ICU_Bed", quiet);
        addEmergency(5, "Meera", 6, "Cardiac", "Doctor", quiet);
    }

    DashboardState snapshot(std::size_t topK = 5U) const {
        DashboardState state;
        state.pendingCount = pending_.size();
        state.availableCount = resources_.availableCount();
        state.waitingCount = waiting_.size();
        state.pending = pending_.snapshot();
        state.resources = resources_.snapshot();
        state.waiting = waiting_.snapshot();
        state.history = history_;
        return state;
    }

    void runDemoScenario() {
        std::cout << "\n=== Emergency Resource Allocator Demo ===\n";
        seedDemoData(false);

        printStatus();

        serveNext(false);
        serveNext(false);
        updateEmergencySeverity(3, 10, false);
        serveNext(false);
        serveNext(false);

        releaseResource(201, false);
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
            if (!quiet) {
                std::cout << "Emergency queued for later: " << emergency << '\n';
            }
            return;
        }

        history_.push_back({emergency.id, emergency.patientName, resource->id, resource->type, note, currentTime_++});
        if (!quiet) {
            std::cout << "Assigned emergency #" << emergency.id << " to resource #" << resource->id << " (" << resource->type << ")\n";
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
            history_.push_back({emergency.id, emergency.patientName, resource->id, resource->type, "served from waiting queue", currentTime_++});
            if (!quiet) {
                std::cout << "Served waiting emergency #" << emergency.id << " using resource #" << resource->id << '\n';
            }
        }

        while (!deferred.empty()) {
            waiting_.push(deferred.pop());
        }
    }

    MaxHeap pending_;
    ResourceTable resources_;
    WaitingQueue waiting_;
    std::vector<AllocationRecord> history_;
    int currentTime_ = 0;
};

}  // namespace era
