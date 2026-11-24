/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_LRUCACHE_H
#define LESSAMPLER_LRUCACHE_H

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>

namespace lessampler {

    /*
 * Small, non-thread-safe LRU cache. The owner supplies locking.
 */
    template<typename Key, typename Value, typename Hash = std::hash<Key>>
    class LruCache {
    public:
        explicit LruCache(std::size_t capacity) : capacity_(capacity) {}

        [[nodiscard]] std::optional<Value> Get(const Key &key) {
            const auto it = entries_.find(key);
            if (it == entries_.end()) {
                return std::nullopt;
            }
            order_.splice(order_.begin(), order_, it->second.order);
            return it->second.value;
        }

        void Put(const Key &key, Value value) {
            if (capacity_ == 0) {
                return;
            }

            if (const auto it = entries_.find(key); it != entries_.end()) {
                it->second.value = std::move(value);
                order_.splice(order_.begin(), order_, it->second.order);
                return;
            }

            order_.push_front(key);
            entries_.emplace(key, Entry{std::move(value), order_.begin()});
            while (entries_.size() > capacity_) {
                entries_.erase(order_.back());
                order_.pop_back();
            }
        }

        void Clear() {
            entries_.clear();
            order_.clear();
        }

        [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }

    private:
        struct Entry {
            Value value;
            typename std::list<Key>::iterator order;
        };

        std::size_t capacity_;
        std::list<Key> order_;
        std::unordered_map<Key, Entry, Hash> entries_;
    };

}// namespace lessampler

#endif//LESSAMPLER_LRUCACHE_H
