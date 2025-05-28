#ifndef LAZYJSON_STRING_BUFFER_HPP
#define LAZYJSON_STRING_BUFFER_HPP

#include <string>
#include <string_view>
#include <vector>
#include <cstring>
#include <stdexcept>

namespace lazyjson {

class StringBuffer {
private:
    struct Block {
        char* data;
        size_t capacity;
        size_t used;
        
        Block(size_t size) : capacity(size), used(0) {
            data = static_cast<char*>(std::malloc(size));
            if (!data) {
                throw std::bad_alloc();
            }
        }
        
        ~Block() {
            if (data) {
                std::free(data);
                data = nullptr;
            }
        }
        
        // Impedisci la copia
        Block(const Block&) = delete;
        Block& operator=(const Block&) = delete;
        
        // Permetti il movimento
        Block(Block&& other) noexcept : data(other.data), capacity(other.capacity), used(other.used) {
            other.data = nullptr;
            other.capacity = 0;
            other.used = 0;
        }
        
        Block& operator=(Block&& other) noexcept {
            if (this != &other) {
                if (data) {
                    std::free(data);
                }
                data = other.data;
                capacity = other.capacity;
                used = other.used;
                
                other.data = nullptr;
                other.capacity = 0;
                other.used = 0;
            }
            return *this;
        }
        
        bool can_fit(size_t size) const {
            return used + size <= capacity;
        }
        
        std::string_view add_string(const std::string& value) {
            size_t size = value.size();
            if (!can_fit(size)) {
                return std::string_view(); // Empty view to indicate failure
            }
            
            std::memcpy(data + used, value.data(), size);
            std::string_view result(data + used, size);
            used += size;
            return result;
        }
        
        void clear() {
            if (data && used > 0) {
                std::memset(data, 0, used);
                used = 0;
            }
        }
    };
    
    // Vettore di blocchi di memoria
    std::vector<Block> blocks;
    
    // Dimensione fissa per i blocchi
    size_t block_size;
    
public:
    // Costruttore con dimensione del blocco (obbligatoria)
    explicit StringBuffer(size_t block_size) : block_size(block_size) {
        // Crea il primo blocco con la dimensione specificata
        blocks.emplace_back(block_size);
    }
    
    // Aggiunge una stringa al buffer
    std::string_view add(const std::string& value) {
        // Cerca un blocco con spazio sufficiente
        for (auto& block : blocks) {
            if (block.can_fit(value.size())) {
                return block.add_string(value);
            }
        }
        
        // Se arriviamo qui, nessun blocco esistente ha spazio sufficiente
        // Crea un nuovo blocco della dimensione fissa
        blocks.emplace_back(block_size);
        
        // Se la stringa è più grande della dimensione del blocco, potrebbe non entrare
        auto result = blocks.back().add_string(value);
        if (result.empty()) {
            // In casi eccezionali, crea un blocco speciale per stringhe molto grandi
            blocks.emplace_back(value.size() + 1);
            return blocks.back().add_string(value);
        }
        
        return result;
    }
    
    // Pulisce tutti i blocchi tranne il primo
    void clear() {
        if (blocks.empty()) {
            return;
        }
        
        // Mantieni solo il primo blocco e puliscilo
        Block first_block = std::move(blocks[0]);
        first_block.clear();
        
        blocks.clear();
        blocks.emplace_back(std::move(first_block));
    }
    
    // Restituisce la capacità totale del buffer
    size_t capacity() const {
        size_t total = 0;
        for (const auto& block : blocks) {
            total += block.capacity;
        }
        return total;
    }
    
    // Restituisce lo spazio totale utilizzato
    size_t used() const {
        size_t total = 0;
        for (const auto& block : blocks) {
            total += block.used;
        }
        return total;
    }
    
    // Restituisce il numero di blocchi di memoria
    size_t block_count() const {
        return blocks.size();
    }
};

} // namespace lazyjson

#endif // LAZYJSON_STRING_BUFFER_HPP