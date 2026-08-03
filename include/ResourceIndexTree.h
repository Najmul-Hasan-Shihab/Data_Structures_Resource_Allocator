#pragma once

#include "Resource.h"

#include <vector>

namespace era {

class ResourceIndexTree {
public:
    ResourceIndexTree() = default;
    ResourceIndexTree(const ResourceIndexTree&) = delete;
    ResourceIndexTree& operator=(const ResourceIndexTree&) = delete;
    ~ResourceIndexTree() {
        clear();
    }

    void clear() {
        clearNode(root_);
        root_ = nullptr;
    }

    bool insert(Resource* resource) {
        if (resource == nullptr) {
            return false;
        }
        bool inserted = false;
        root_ = insertNode(root_, resource, inserted);
        return inserted;
    }

    std::vector<Resource> snapshotOrdered() const {
        std::vector<Resource> result;
        inorder(root_, result);
        return result;
    }

private:
    struct Node {
        explicit Node(Resource* value)
            : resource(value) {
        }

        Resource* resource = nullptr;
        Node* left = nullptr;
        Node* right = nullptr;
    };

    static Node* insertNode(Node* node, Resource* resource, bool& inserted) {
        if (node == nullptr) {
            inserted = true;
            return new Node(resource);
        }
        if (resource->id < node->resource->id) {
            node->left = insertNode(node->left, resource, inserted);
        } else if (resource->id > node->resource->id) {
            node->right = insertNode(node->right, resource, inserted);
        }
        return node;
    }

    static void inorder(Node* node, std::vector<Resource>& result) {
        if (node == nullptr) {
            return;
        }
        inorder(node->left, result);
        result.push_back(*node->resource);
        inorder(node->right, result);
    }

    static void clearNode(Node* node) {
        if (node == nullptr) {
            return;
        }
        clearNode(node->left);
        clearNode(node->right);
        delete node;
    }

    Node* root_ = nullptr;
};

}  // namespace era
