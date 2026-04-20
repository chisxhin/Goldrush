#include "history.hpp"

ActionHistory::ActionHistory(size_t maxEntries)
    : limit(maxEntries) {
}

void ActionHistory::add(const std::string& entry) {
    if (entry.empty()) {
        return;
    }
    entries.push_front(entry);
    while (entries.size() > limit) {
        entries.pop_back();
    }
}

void ActionHistory::clear() {
    entries.clear();
}

std::vector<std::string> ActionHistory::recent() const {
    return std::vector<std::string>(entries.begin(), entries.end());
}

void ActionHistory::restore(const std::vector<std::string>& snapshot) {
    clear();
    for (std::vector<std::string>::const_reverse_iterator it = snapshot.rbegin();
         it != snapshot.rend();
         ++it) {
        add(*it);
    }
}
