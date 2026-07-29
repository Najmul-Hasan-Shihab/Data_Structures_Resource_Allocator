#pragma once

#include <string>

namespace era {

struct AllocationRecord {
    int emergencyId = 0;
    std::string emergencyName;
    int resourceId = -1;
    std::string resourceType;
    std::string status;
    int timeStamp = 0;
};

}  // namespace era
