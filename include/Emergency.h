#pragma once

#include <ostream>
#include <string>

namespace era {

struct Emergency {
    int id = 0;
    std::string patientName;
    int severity = 1;
    std::string type;
    std::string requiredResourceType;
    int arrivalTime = 0;
};

inline std::ostream& operator<<(std::ostream& os, const Emergency& emergency) {
    os << "#" << emergency.id << " " << emergency.patientName
       << " | severity=" << emergency.severity
       << " | type=" << emergency.type
       << " | needs=" << emergency.requiredResourceType
       << " | t=" << emergency.arrivalTime;
    return os;
}

}  // namespace era
