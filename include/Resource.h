#pragma once

#include <ostream>
#include <string>

namespace era {

struct Resource {
    int id = 0;
    std::string type;
    bool available = true;
    int location = 0;
    int assignedEmergencyId = -1;
    std::string assignedEmergencyName;
};

inline std::ostream& operator<<(std::ostream& os, const Resource& resource) {
    os << "#" << resource.id << " " << resource.type
       << " | " << (resource.available ? "available" : "busy")
       << " | loc=" << resource.location;
    if (!resource.available) {
        os << " | assigned to #" << resource.assignedEmergencyId << " " << resource.assignedEmergencyName;
    }
    return os;
}

}  // namespace era
