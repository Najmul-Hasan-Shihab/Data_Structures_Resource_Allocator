#pragma once

#include <iosfwd>
#include <ostream>
#include <cstddef>
#include <string>
#include <vector>

namespace era {

struct PatientRecord {
    int patientId = 0;
    std::string patientName;
    std::string conditionType;
    std::string requiredResourceType;
    int location = 0;
    int severity = 0;
    std::string status;
    int assignedResourceId = -1;
};

inline std::ostream& operator<<(std::ostream& os, const PatientRecord& patient) {
    os << "#" << patient.patientId << " " << patient.patientName
       << " | condition=" << patient.conditionType
       << " | severity=" << patient.severity
       << " | need=" << patient.requiredResourceType
       << " | loc=" << patient.location
       << " | status=" << patient.status;
    if (patient.assignedResourceId >= 0) {
        os << " | resource=#" << patient.assignedResourceId;
    }
    return os;
}

class PatientRegistry {
public:
    PatientRegistry() = default;
    PatientRegistry(const PatientRegistry&) = delete;
    PatientRegistry& operator=(const PatientRegistry&) = delete;
    ~PatientRegistry() {
        clear();
    }

    void clear() {
        Node* current = head_;
        while (current != nullptr) {
            Node* next = current->next;
            delete current;
            current = next;
        }
        head_ = nullptr;
        tail_ = nullptr;
        count_ = 0U;
    }

    bool upsert(const PatientRecord& patient) {
        Node* node = findNode(patient.patientId);
        if (node != nullptr) {
            node->data = patient;
            return false;
        }

        Node* created = new Node{patient, nullptr, tail_};
        if (tail_ != nullptr) {
            tail_->next = created;
        } else {
            head_ = created;
        }
        tail_ = created;
        ++count_;
        return true;
    }

    PatientRecord* find(int patientId) {
        Node* node = findNode(patientId);
        return node == nullptr ? nullptr : &node->data;
    }

    const PatientRecord* find(int patientId) const {
        const Node* node = findNode(patientId);
        return node == nullptr ? nullptr : &node->data;
    }

    bool updateStatus(int patientId, const std::string& status, int assignedResourceId = -1) {
        PatientRecord* patient = find(patientId);
        if (patient == nullptr) {
            return false;
        }
        patient->status = status;
        patient->assignedResourceId = assignedResourceId;
        return true;
    }

    std::vector<PatientRecord> snapshot() const {
        std::vector<PatientRecord> result;
        result.reserve(count_);
        for (const Node* current = head_; current != nullptr; current = current->next) {
            result.push_back(current->data);
        }
        return result;
    }

private:
    struct Node {
        PatientRecord data;
        Node* next = nullptr;
        Node* prev = nullptr;
    };

    Node* findNode(int patientId) {
        Node* current = head_;
        while (current != nullptr) {
            if (current->data.patientId == patientId) {
                return current;
            }
            current = current->next;
        }
        return nullptr;
    }

    const Node* findNode(int patientId) const {
        const Node* current = head_;
        while (current != nullptr) {
            if (current->data.patientId == patientId) {
                return current;
            }
            current = current->next;
        }
        return nullptr;
    }

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t count_ = 0U;
};

}  // namespace era
