#pragma once

#include <cstddef>
#include <vector>

#include "random_service.hpp"

template <typename T>
class Deck {
public:
    Deck()
        : random(nullptr) {
    }

    explicit Deck(RandomService& randomService)
        : random(&randomService) {
    }

    void bindRandom(RandomService& randomService) {
        random = &randomService;
    }

    void reset(const std::vector<T>& cards, bool reshuffle = true) {
        drawPile = cards;
        discardPile.clear();
        if (reshuffle) {
            shuffle();
        }
    }

    void shuffle() {
        if (random == nullptr) {
            return;
        }
        random->shuffle(drawPile);
    }

    bool draw(T& card) {
        replenishIfNeeded();
        if (drawPile.empty()) {
            return false;
        }

        card = drawPile.back();
        drawPile.pop_back();
        return true;
    }

    void discard(const T& card) {
        discardPile.push_back(card);
    }

    bool empty() const {
        return drawPile.empty() && discardPile.empty();
    }

    std::size_t size() const {
        return drawPile.size();
    }

    std::size_t discardSize() const {
        return discardPile.size();
    }

    const T* peek() const {
        if (drawPile.empty()) {
            return nullptr;
        }
        return &drawPile.back();
    }

    std::vector<T> peek(std::size_t count) const {
        std::vector<T> cards;
        if (count == 0 || drawPile.empty()) {
            return cards;
        }

        if (count > drawPile.size()) {
            count = drawPile.size();
        }

        cards.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            cards.push_back(drawPile[drawPile.size() - 1 - i]);
        }
        return cards;
    }

private:
    void replenishIfNeeded() {
        if (!drawPile.empty() || discardPile.empty()) {
            return;
        }

        drawPile.swap(discardPile);
        shuffle();
    }

    RandomService* random;
    std::vector<T> drawPile;
    std::vector<T> discardPile;
};
