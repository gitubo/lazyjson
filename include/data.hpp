#ifndef LAZYJSON_DATA_HPP
#define LAZYJSON_DATA_HPP

#include "tokenizer.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>
#include <functional> 
#include <iostream>
#include <queue>
#include <unordered_set>

namespace lazyjson {

    using DataNull = std::monostate;
    using DataBoolean = bool;
    using DataNumber = double;
    using DataString = std::string_view; 
    using PrimitiveType = std::variant<DataNull, DataBoolean, DataNumber, DataString>;

    enum class ElementType {
        NULL_VALUE,
        BOOLEAN,
        NUMBER,
        STRING,
        OBJECT,
        ARRAY,
        UNDEFINED
    };
    std::ostream& operator<<(std::ostream& os, const ElementType& type);

    class DataElement {
        public:
            
            DataElement() : 
                type_(ElementType::NULL_VALUE),
                materialized_value_(DataNull{}),
                token_index_start_(0),
                token_index_end_(0),
                is_materialized_(false),
                is_modified_(false)
                {}
            
            ~DataElement() {
                destroyRecursively();
            }

            void clear() {
                destroyRecursively();
                materialized_element_list_.clear();
                token_index_list_.clear();
                key_ordered_list_.clear();
                is_materialized_ = false;
                is_modified_ = false;
                type_ = ElementType::NULL_VALUE;
                materialized_value_ = DataNull{};
            }

            inline ElementType getType() const { return type_; }
            inline void setType(ElementType type){ type_ = type; }
            inline size_t getTokenIndexStart() const { return token_index_start_; }
            inline void setTokenStartIndex(size_t token_index_start){ token_index_start_ = token_index_start; }
            inline size_t getTokenIndexEnd() const { return token_index_end_; }
            inline void setTokenEndIndex(size_t token_index_end){ token_index_end_= token_index_end; }    
                        
            inline bool isMaterialized() const { return is_materialized_; }
            inline void setIsMaterialized(bool is_materialized) { is_materialized_ = is_materialized; }
            inline bool isModified() const { return is_modified_; }
            inline void setIsModified(bool is_modified) { is_modified_ = is_modified; }

            inline PrimitiveType& getMaterializedValue() { return materialized_value_; }
            template<typename T> void setMaterializedValue(T value) { materialized_value_ = value; }
            
            bool isNull() const { return std::holds_alternative<DataNull>(materialized_value_); }
            bool isBoolean() const { return std::holds_alternative<DataBoolean>(materialized_value_); }
            bool isNumber() const { return std::holds_alternative<DataNumber>(materialized_value_); }
            bool isString() const { return std::holds_alternative<DataString>(materialized_value_); }
            DataBoolean asBoolean() const { return std::get<DataBoolean>(materialized_value_); }
            DataNumber asNumber() const { return std::get<DataNumber>(materialized_value_); }
            const DataString& asString() const { return std::get<DataString>(materialized_value_); }
            
            inline const std::vector<std::string_view>& getElementKeyList() const { return key_ordered_list_; }
            
            inline bool isTokenIndexRegistered(const std::string_view key) const { return token_index_list_.find(key) != token_index_list_.end(); }
            inline const size_t& getTokenIndex(const std::string_view& key) const { return token_index_list_.at(key); }
            inline const std::unordered_map<std::string_view, size_t>& getTokenIndexList() const { return token_index_list_; }
            inline void addTokenIndex(const std::string_view key, const size_t index) { 
                if(token_index_list_.emplace(key, index).second){
                    key_ordered_list_.push_back(key);
                } 
            }
            inline const std::string_view getTokenStringView(const std::string_view key) { 
                auto it = token_index_list_.find(key);
                if (it != token_index_list_.end()) {
                    return it->first;
                }
                return {};
            }

            inline const bool isMaterializedElement(const std::string_view& key) const { return materialized_element_list_.find(key) != materialized_element_list_.end(); }
            inline const std::shared_ptr<DataElement> getMaterializedElement(const std::string_view& key) const { return materialized_element_list_.at(key); }
            inline const std::unordered_map<std::string_view, std::shared_ptr<lazyjson::DataElement>> getMaterializedElementList() const { return materialized_element_list_; }
            inline void addMaterializedElement(const std::string_view& key, std::shared_ptr<DataElement> value_ptr) { materialized_element_list_.emplace(key, value_ptr); }

        private:

            ElementType type_;
            size_t token_index_start_;
            size_t token_index_end_;

            bool is_materialized_;
            bool is_modified_;

            PrimitiveType materialized_value_;

            std::vector<std::string_view> key_ordered_list_;
            std::unordered_map<std::string_view, size_t> token_index_list_;
            std::unordered_map<std::string_view, std::shared_ptr<DataElement>> materialized_element_list_;

            void destroyRecursively() {
                if (materialized_element_list_.empty()) {
                    return;
                }

                // Usa una coda per attraversare iterativamente la struttura
                std::queue<std::unordered_map<std::string_view, std::shared_ptr<DataElement>>*> toProcess;
                toProcess.push(&materialized_element_list_);

                while (!toProcess.empty()) {
                    auto* currentMap = toProcess.front();
                    toProcess.pop();

                    for (auto& [key, element] : *currentMap) {
                        if (element && element.use_count() == 1) {
                            // Se siamo gli unici proprietari, aggiungi i figli alla coda
                            if (!element->materialized_element_list_.empty()) {
                                toProcess.push(&element->materialized_element_list_);
                            }
                        }
                    }
                    
                    // Svuota la mappa corrente
                    currentMap->clear();
                }
            }
    };

    class DataElementManager {
        public:
            static void safeDestroy(std::shared_ptr<DataElement>& element) {
                if (!element) return;
                
                // Se siamo gli unici proprietari, forziamo la pulizia
                if (element.use_count() == 1) {
                    element->clear();
                }
                element.reset();
            }
    
            static void clearAll(std::vector<std::shared_ptr<DataElement>>& elements) {
                for (auto& element : elements) {
                    safeDestroy(element);
                }
                elements.clear();
            }
    
            // Metodo per verificare se ci sono riferimenti circolari (debugging)
            static bool hasCircularReference(const std::shared_ptr<DataElement>& root) {
                if (!root) return false;
                
                std::unordered_set<const DataElement*> visited;
                std::queue<const DataElement*> toCheck;
                toCheck.push(root.get());
                
                while (!toCheck.empty()) {
                    const DataElement* current = toCheck.front();
                    toCheck.pop();
                    
                    if (visited.count(current)) {
                        return true; // Riferimento circolare trovato
                    }
                    visited.insert(current);
                    
                    for (const auto& [key, child] : current->getMaterializedElementList()) {
                        if (child) {
                            toCheck.push(child.get());
                        }
                    }
                }
                
                return false;
            }
    };
    
} // namespace lazyjson

#endif 