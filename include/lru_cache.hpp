#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <atomic>
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace lazyjson {

// Forward declaration
class DataElement;

class LRUCache {
private:
    struct CacheNode {
        std::shared_ptr<DataElement> value;
        uint64_t timestamp;
        
        CacheNode(std::shared_ptr<DataElement> v)
            : value(v), timestamp(get_timestamp()) {}
        
        void update_timestamp() {
            timestamp = get_timestamp();
        }
    private:
        static uint64_t get_timestamp() {
            static std::atomic<uint64_t> counter{0};
            return counter.fetch_add(1, std::memory_order_relaxed);
        }
    };
    
    size_t max_size_;
    std::unordered_map<std::string, CacheNode> cache_;
    
    void batch_evict() {
        constexpr size_t SAMPLE_SIZE = 8;
        constexpr size_t EVICT_COUNT = 3; // Rimuovi 3 elementi alla volta
        
        // Raccogli candidati per eviction
        std::vector<std::pair<uint64_t, decltype(cache_.begin())>> candidates;
        candidates.reserve(SAMPLE_SIZE);
        
        auto it = cache_.begin();
        for (size_t i = 0; i < SAMPLE_SIZE && it != cache_.end(); ++i) {
            candidates.emplace_back(it->second.timestamp, it);
            
            // Salta alcuni elementi per distribuzione migliore
            size_t skip = 1 + (cache_.size() / SAMPLE_SIZE);
            for (size_t j = 0; j < skip && it != cache_.end(); ++j) {
                ++it;
            }
        }
        
        // Ordina per timestamp (più vecchi prima)
        std::sort(candidates.begin(), candidates.end());
        
        // Rimuovi i EVICT_COUNT elementi più vecchi
        size_t to_remove = std::min(EVICT_COUNT, candidates.size());
        for (size_t i = 0; i < to_remove; ++i) {
            cache_.erase(candidates[i].second);
        }
    }

public:
    explicit LRUCache(size_t max_size) : max_size_(max_size) {
        if (max_size == 0) {
            throw std::invalid_argument("Cache size must be greater than 0");
        }
        // Reserve spazio per evitare rehashing frequenti
        cache_.reserve(max_size * 5 / 4); // 25% extra capacity
    }
    
    std::shared_ptr<DataElement> get(const std::string& key) {
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return nullptr;
        }
        
        // Aggiorna timestamp per indicare accesso recente
        it->second.update_timestamp();
        return it->second.value;
    }
    
    void set(const std::string& key, std::shared_ptr<DataElement> value) {
        auto it = cache_.find(key);
        
        if (it != cache_.end()) {
            // Key già presente, aggiorna valore e timestamp
            it->second.value = value;
            it->second.update_timestamp();
        } else {
            // Nuova key - usa batch eviction con threshold
            if (cache_.size() >= max_size_) {
                batch_evict();
            }
            
            cache_.emplace(key, CacheNode(value));
        }
    }
    
    size_t size() const {
        return cache_.size();
    }
    
    size_t max_size() const {
        return max_size_;
    }
    
    bool empty() const {
        return cache_.empty();
    }
    
    void clear() {
        cache_.clear();
    }
};

} // namespace lazyjson