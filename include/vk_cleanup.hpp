#pragma once

#include <functional>
#include <queue>
struct DeletionQueue {
    // TODO: in future, implement this as a series of vectors containing vulkan
    // handles, which deletes them all
    std::vector<std::function<void()>> deleters;

    void push(std::function<void()>&& fn) {
        deleters.push_back(std::forward<std::function<void()>>(fn));
    }
    void pop_all() {
        for (auto const& deleter : deleters) {
            std::invoke(deleter);
        }
    }
};
