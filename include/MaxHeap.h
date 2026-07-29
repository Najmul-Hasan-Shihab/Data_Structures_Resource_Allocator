#pragma once

#include "Emergency.h"

#include <algorithm>
#include <optional>
#include <unordered_map>
#include <vector>

namespace era {

class MaxHeap {
public:
    bool empty() const {
        return heap_.empty();
    }

    std::size_t size() const {
        return heap_.size();
    }

    const Emergency* peek() const {
        if (heap_.empty()) {
            return nullptr;
        }
        return &heap_.front();
    }

    bool insert(const Emergency& emergency) {
        if (indexById_.count(emergency.id) != 0U) {
            return false;
        }
        heap_.push_back(emergency);
        indexById_[emergency.id] = heap_.size() - 1U;
        heapifyUp(heap_.size() - 1U);
        return true;
    }

    std::optional<Emergency> extractMax() {
        if (heap_.empty()) {
            return std::nullopt;
        }
        Emergency result = heap_.front();
        indexById_.erase(result.id);
        if (heap_.size() == 1U) {
            heap_.pop_back();
            return result;
        }
        heap_.front() = heap_.back();
        indexById_[heap_.front().id] = 0U;
        heap_.pop_back();
        heapifyDown(0U);
        return result;
    }

    bool updatePriority(int emergencyId, int newSeverity) {
        auto found = indexById_.find(emergencyId);
        if (found == indexById_.end()) {
            return false;
        }
        std::size_t index = found->second;
        int oldSeverity = heap_[index].severity;
        heap_[index].severity = newSeverity;
        if (newSeverity > oldSeverity) {
            heapifyUp(index);
        } else if (newSeverity < oldSeverity) {
            heapifyDown(index);
        }
        return true;
    }

    bool contains(int emergencyId) const {
        return indexById_.count(emergencyId) != 0U;
    }

    std::vector<Emergency> snapshot() const {
        return heap_;
    }

    std::vector<Emergency> topK(std::size_t k) const {
        std::vector<Emergency> copy = heap_;
        std::vector<Emergency> result;
        std::make_heap(copy.begin(), copy.end(), compareForStdHeap);
        for (std::size_t i = 0; i < k && !copy.empty(); ++i) {
            std::pop_heap(copy.begin(), copy.end(), compareForStdHeap);
            result.push_back(copy.back());
            copy.pop_back();
        }
        return result;
    }

private:
    static bool lowerPriority(const Emergency& lhs, const Emergency& rhs) {
        if (lhs.severity != rhs.severity) {
            return lhs.severity < rhs.severity;
        }
        return lhs.arrivalTime > rhs.arrivalTime;
    }

    static bool compareForStdHeap(const Emergency& lhs, const Emergency& rhs) {
        return lowerPriority(lhs, rhs);
    }

    void swapNodes(std::size_t lhs, std::size_t rhs) {
        std::swap(heap_[lhs], heap_[rhs]);
        indexById_[heap_[lhs].id] = lhs;
        indexById_[heap_[rhs].id] = rhs;
    }

    void heapifyUp(std::size_t index) {
        while (index > 0U) {
            std::size_t parent = (index - 1U) / 2U;
            if (!lowerPriority(heap_[parent], heap_[index])) {
                break;
            }
            swapNodes(parent, index);
            index = parent;
        }
    }

    void heapifyDown(std::size_t index) {
        while (true) {
            std::size_t left = 2U * index + 1U;
            std::size_t right = left + 1U;
            std::size_t largest = index;

            if (left < heap_.size() && lowerPriority(heap_[largest], heap_[left])) {
                largest = left;
            }
            if (right < heap_.size() && lowerPriority(heap_[largest], heap_[right])) {
                largest = right;
            }
            if (largest == index) {
                break;
            }
            swapNodes(index, largest);
            index = largest;
        }
    }

    std::vector<Emergency> heap_;
    std::unordered_map<int, std::size_t> indexById_;
};

}  // namespace era
