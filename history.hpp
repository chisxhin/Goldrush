#pragma once

#include <cstddef>
#include <deque>
#include <string>
#include <vector>

class ActionHistory {
public:
    explicit ActionHistory(size_t maxEntries = 6);

    void add(const std::string& entry);
    void clear();
    std::vector<std::string> recent() const;
    void restore(const std::vector<std::string>& snapshot);

private:
    std::deque<std::string> entries;
    size_t limit;
};
