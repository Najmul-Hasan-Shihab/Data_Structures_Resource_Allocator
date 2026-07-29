#pragma once

#include "Emergency.h"
#include "Resource.h"

#include <cstddef>
#include <list>
#include <string>
#include <vector>

namespace era {

class ResourceTable {
public:
    explicit ResourceTable(std::size_t bucketCount = 11U)
        : buckets_(bucketCount) {
    }

    bool add(const Resource& resource) {
        if (find(resource.id) != nullptr) {
            return false;
        }
        buckets_[bucketIndex(resource.id)].push_back(resource);
        return true;
    }

    Resource* find(int resourceId) {
        return const_cast<Resource*>(static_cast<const ResourceTable*>(this)->find(resourceId));
    }

    const Resource* find(int resourceId) const {
        const auto& bucket = buckets_[bucketIndex(resourceId)];
        for (const auto& resource : bucket) {
            if (resource.id == resourceId) {
                return &resource;
            }
        }
        return nullptr;
    }

    Resource* reserveByType(const std::string& requiredType, const Emergency& emergency) {
        for (auto& bucket : buckets_) {
            for (auto& resource : bucket) {
                if (resource.available && resource.type == requiredType) {
                    resource.available = false;
                    resource.assignedEmergencyId = emergency.id;
                    resource.assignedEmergencyName = emergency.patientName;
                    return &resource;
                }
            }
        }
        return nullptr;
    }

    bool release(int resourceId) {
        Resource* resource = find(resourceId);
        if (resource == nullptr || resource->available) {
            return false;
        }
        resource->available = true;
        resource->assignedEmergencyId = -1;
        resource->assignedEmergencyName.clear();
        return true;
    }

    std::size_t availableCount() const {
        std::size_t count = 0U;
        for (const auto& bucket : buckets_) {
            for (const auto& resource : bucket) {
                if (resource.available) {
                    ++count;
                }
            }
        }
        return count;
    }

    std::vector<Resource> snapshot() const {
        std::vector<Resource> result;
        for (const auto& bucket : buckets_) {
            for (const auto& resource : bucket) {
                result.push_back(resource);
            }
        }
        return result;
    }

private:
    std::size_t bucketIndex(int resourceId) const {
        return static_cast<std::size_t>(resourceId) % buckets_.size();
    }

    std::vector<std::list<Resource>> buckets_;
};

}  // namespace era
