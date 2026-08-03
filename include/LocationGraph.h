#pragma once

#include <algorithm>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

namespace era {

struct RouteResult {
    std::vector<int> path;
    int distance = -1;
};

class LocationGraph {
public:
    void clear() {
        adjacency_.clear();
    }

    void addEdge(int from, int to, int weight) {
        if (from == to) {
            return;
        }
        adjacency_[from].push_back({to, weight});
        adjacency_[to].push_back({from, weight});
    }

    std::optional<RouteResult> shortestPath(int start, int goal) const {
        if (start == goal) {
            return RouteResult{{start}, 0};
        }

        const int infinity = std::numeric_limits<int>::max() / 4;
        std::unordered_map<int, int> distance;
        std::unordered_map<int, int> previous;
        using QueueItem = std::pair<int, int>;
        std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem>> queue;

        distance[start] = 0;
        queue.push({0, start});

        while (!queue.empty()) {
            const auto [currentDistance, node] = queue.top();
            queue.pop();
            if (currentDistance != distance[node]) {
                continue;
            }
            if (node == goal) {
                break;
            }

            const auto adjacencyIter = adjacency_.find(node);
            if (adjacencyIter == adjacency_.end()) {
                continue;
            }

            for (const auto& edge : adjacencyIter->second) {
                const int nextNode = edge.first;
                const int nextDistance = currentDistance + edge.second;
                const auto existing = distance.find(nextNode);
                if (existing == distance.end() || nextDistance < existing->second) {
                    distance[nextNode] = nextDistance;
                    previous[nextNode] = node;
                    queue.push({nextDistance, nextNode});
                }
            }
        }

        const auto goalDistance = distance.find(goal);
        if (goalDistance == distance.end()) {
            return std::nullopt;
        }

        std::vector<int> path;
        for (int current = goal;; current = previous[current]) {
            path.push_back(current);
            if (current == start) {
                break;
            }
            if (previous.find(current) == previous.end()) {
                return std::nullopt;
            }
        }

        std::reverse(path.begin(), path.end());
        return RouteResult{path, goalDistance->second};
    }

private:
    std::unordered_map<int, std::vector<std::pair<int, int>>> adjacency_;
};

}  // namespace era
