#pragma once

#include "Emergency.h"

#include <queue>
#include <vector>

namespace era {

class WaitingQueue {
public:
    bool empty() const {
        return queue_.empty();
    }

    std::size_t size() const {
        return queue_.size();
    }

    void push(const Emergency& emergency) {
        queue_.push(emergency);
    }

    Emergency pop() {
        Emergency emergency = queue_.front();
        queue_.pop();
        return emergency;
    }

    std::vector<Emergency> snapshot() const {
        std::queue<Emergency> copy = queue_;
        std::vector<Emergency> result;
        while (!copy.empty()) {
            result.push_back(copy.front());
            copy.pop();
        }
        return result;
    }

    void clear() {
        while (!queue_.empty()) {
            queue_.pop();
        }
    }

private:
    std::queue<Emergency> queue_;
};

}  // namespace era
