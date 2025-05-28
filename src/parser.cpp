#include "parser.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdio>
#include <string>
#include <chrono>
using namespace std::chrono;

namespace lazyjson {

// Parser implementation
Parser::Parser() : string_buffer_(4096) {
    tokenizer_ = Tokenizer();
}

// Helper function to skip a value during lazy parsing
void Parser::skipValue(const std::vector<Token>& tokens, size_t& currentIndex) {
    if (currentIndex >= tokens.size()) {
        throw std::runtime_error("Unexpected end of tokens");
    }
    
    const Token& token = tokens[currentIndex++];
    
    switch (token.type) {
        case TokenType::TOKEN_OBJECT_START: {
            int depth = 1;
            while (depth > 0 && currentIndex < tokens.size()) {
                TokenType type = tokens[currentIndex++].type;
                if (type == TokenType::TOKEN_OBJECT_START) {
                    depth++;
                } else if (type == TokenType::TOKEN_OBJECT_END) {
                    depth--;
                }
            }
            break;
        }
        case TokenType::TOKEN_ARRAY_START: {
            int depth = 1;
            while (depth > 0 && currentIndex < tokens.size()) {
                TokenType type = tokens[currentIndex++].type;
                if (type == TokenType::TOKEN_ARRAY_START) {
                    depth++;
                } else if (type == TokenType::TOKEN_ARRAY_END) {
                    depth--;
                }
            }
            break;
        }
        case TokenType::TOKEN_STRING:
        case TokenType::TOKEN_NUMBER:
        case TokenType::TOKEN_BOOLEAN:
        case TokenType::TOKEN_NULL:
            // Already consumed by currentIndex++
            break;
        default:
            throw std::runtime_error("Unexpected token type");
    }
}

bool Parser::parse(std::string& jsonString) {

    TokenizerError error = TokenizerError::NONE;
        if (tokenizer_.tokenize(jsonString, error) != 0) {
        std::cerr << "Tokenization error: " << static_cast<int>(error) << std::endl;
        return false;
    }
    //std::cout << "TOKENS : \n" << tokenizer_.toString() << std::endl;
    tokens_ = tokenizer_.getTokens();

    // Parse the root value
    try {
        size_t currentIndex = 1;  // Skipping <SOF> START_OF_FILE
        
        auto err = parseElement(root_, currentIndex);
        const size_t lastValidTokenIndex = tokens_.size()-2;
        const auto lastValidToken = tokens_[tokens_.size()-2];
        if(
            (root_->getType() == ElementType::OBJECT && lastValidToken.type == TokenType::TOKEN_OBJECT_END) ||
            (root_->getType() == ElementType::ARRAY && lastValidToken.type == TokenType::TOKEN_ARRAY_END)
        ){
            root_->setTokenEndIndex(lastValidTokenIndex);
        } else {
            throw std::runtime_error("Expected '}' or ']' as last valid token");
        }

        materializeElement(*root_);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        return false;
    }
}

int Parser::materializeElement(DataElement& element) {
    if(element.isMaterialized()) return 0;
    
    if (element.getTokenIndexStart() >= tokens_.size()) {
        throw std::runtime_error("Unexpected end of tokens");
    }
    
    const auto& token_value = tokens_[element.getTokenIndexStart()].value;

    switch (element.getType()) {
        case ElementType::OBJECT:
        case ElementType::ARRAY:
            {

                for(const auto& [token_name, token_index] : element.getTokenIndexList()){
                    if(token_index >= tokens_.size())
                        throw std::runtime_error("Out of range");
                    std::shared_ptr<DataElement> object = std::make_shared<DataElement>();
                    // Parsing all the token in the list
                    auto currentIndex = token_index;
                    parseElement(object, currentIndex);
                    element.addMaterializedElement(token_name, object);
                }
            }
            break;
        case ElementType::STRING:
            element.setMaterializedValue(token_value);
            break;
        case ElementType::NUMBER:
            element.setMaterializedValue(std::stod(std::string(token_value)));
            break;
        case ElementType::BOOLEAN:
            element.setMaterializedValue((token_value == "true" ? true : false));
            break;
        case ElementType::NULL_VALUE:
            element.setMaterializedValue(DataNull{});
            break;
        default:
            throw std::runtime_error("Unexpected token type");
    }
    element.setIsMaterialized(true);
    return 0;
}


int Parser::parseElement(std::shared_ptr<DataElement> element, size_t& currentIndex){
    // Check 
    if (currentIndex >= tokens_.size()) {
        throw std::runtime_error("Out of index");
    }

    element->setTokenStartIndex(currentIndex);
    element->setTokenEndIndex(currentIndex);
    switch(tokens_[currentIndex].type){
        case TokenType::TOKEN_NULL:
            element->setType(ElementType::NULL_VALUE);
            break;
        case TokenType::TOKEN_BOOLEAN:
            element->setType(ElementType::BOOLEAN);
            break;
        case TokenType::TOKEN_NUMBER:
            element->setType(ElementType::NUMBER);
            break;
        case TokenType::TOKEN_STRING:
            element->setType(ElementType::STRING);
            break;
        case TokenType::TOKEN_OBJECT_START:
            {
                element->setType(ElementType::OBJECT);
                currentIndex++; // Skip '{'
                int depth = 1;
                while (depth > 0 && currentIndex < tokens_.size()) {
                    auto token_type = tokens_[currentIndex].type;
                    switch(token_type){
                        case TokenType::TOKEN_OBJECT_START: depth++; break;
                        case TokenType::TOKEN_OBJECT_END: depth--; break;
                        case TokenType::TOKEN_COMMA: currentIndex++; break;
                    }
                    if(depth <= 0) break; // Stop in case the object ends '}'
                    std::string_view token_key = tokens_[currentIndex].value;
                    currentIndex++; // Consume key
                    if (currentIndex >= tokens_.size() || tokens_[currentIndex].type != TokenType::TOKEN_COLON) {
                        throw std::runtime_error("Expected ':' after object key");
                    }
                    currentIndex++; // Consume ':'
                    element->addTokenIndex(token_key, currentIndex);
                    // Skip value for lazy parsing
                    skipValue(tokens_, currentIndex);
                }
                element->setTokenEndIndex(currentIndex);
            }
            break;
        case TokenType::TOKEN_ARRAY_START:
            {
                element->setType(ElementType::ARRAY);
                currentIndex++; // Skip '['
                size_t array_index = 0; // Use a counter as key
                int depth = 1;
                while (depth > 0 && currentIndex < tokens_.size()) {
                    auto token_type = tokens_[currentIndex].type;
                    switch(token_type){
                        case TokenType::TOKEN_ARRAY_START: depth++; break;
                        case TokenType::TOKEN_ARRAY_END: depth--; break;
                        case TokenType::TOKEN_COMMA: currentIndex++; break;
                    }
                    if(depth <= 0) break; // Stop in case the object ends ']'
                    std::string_view token_key = tokens_[currentIndex].value;
                    auto stableStringView = string_buffer_.add(std::to_string(array_index++));
                    element->addTokenIndex(stableStringView, currentIndex);
                    // Skip value for lazy parsing
                    skipValue(tokens_, currentIndex);
                }
                element->setTokenEndIndex(currentIndex);
            }
            break;
        default:
            throw std::runtime_error("Expected a value or '{' or '['");
    }
    return 0;
}

int Parser::set(const std::string& path, const std::shared_ptr<DataElement> element) {
    /*
    const auto& pathComponents = splitPath(path);
    auto currentElement = root_;
    auto previousElement = root_;
    bool pathComponentMatch = true;
    for (const auto& component : pathComponents) {
        switch (currentElement->getType()) {
            case ElementType::NULL_VALUE:
            case ElementType::BOOLEAN:
            case ElementType::NUMBER:
            case ElementType::STRING:
                std::cout << "[set] Destroying the key (primitive)" << std::endl;
                DataElementManager::safeDestroy(currentElement);
                std::cout << "[set] Adding element to previous element (TBD!)" << std::endl;
                previousElement->addMaterializedElement(component, element);
                auto stableStringView = string_buffer_.add(std::string(component));
                previousElement->addTokenIndex(stableStringView, 0);
                break;
            case ElementType::OBJECT:
            case ElementType::ARRAY:
                // Check if the component has already been materialized
                // In case, destroy the materilized element and replace it
                if (currentElement->isMaterializedElement(component)) {
                    std::cout << "[set] Destroying the element of the object/array" << std::endl;
                    DataElementManager::safeDestroy(currentElement);
                    previousElement->addMaterializedElement(component, element);
                    auto stableStringView = string_buffer_.add(std::string(component));
                    previousElement->addTokenIndex(stableStringView, 0);
                    return 0;
                }
                // Check if the component is already registered as a token of the object/array
                // If not, add the new element to the previous component
                if(!currentElement->get getElementKeyList(). (component)){
                    std::cout << "[set] Adding element to previous element" << std::endl;
                    currentElement->addMaterializedElement(component, element);
                    auto stableStringView = string_buffer_.add(std::string(component));
                    currentElement->addTokenIndex(stableStringView, 0);
                    return 0;
                }

                // Otherwise, the component already exists into the object/array 
                // and the search keeps going on

                // Materialize untill the element if found
                if (currentElement->isMaterializedElement(component)) {
                    //std::cout << "[get] get already materialized element" << std::endl;
                    currentElement = currentElement->getMaterializedElement(component);
                } else if (currentElement->isTokenIndexRegistered(component)) {
                        //std::cout << "[get] key/index exists in the object/array" << std::endl;
                        auto tokenIndex = currentElement->getTokenIndex(component);
                        const std::string_view tokenKey = currentElement->getTokenStringView(component);
                        std::shared_ptr<DataElement> child = std::make_shared<DataElement>();
                        auto err = parseElement(child, tokenIndex);
                        if(err){
                            std::string errMsg = "Parsing Element returned error: "; errMsg.append(std::to_string(err));
                            throw std::runtime_error(errMsg);
                        }
                        err = materializeElement(*child);
                        if(err){
                            std::string errMsg = "Materialize Element returned error: "; errMsg.append(std::to_string(err));
                            throw std::runtime_error(errMsg);
                        }
                        currentElement->addMaterializedElement(tokenKey, child);
                        currentElement = child;
                } else {
                        std::string errMsg = "Key/index <";
                        errMsg.append(component).append("> does not exist in the provided object/array");
                        std::cerr << errMsg << std::endl;
                        throw std::runtime_error(errMsg);
                }
                break;
            default:
                throw std::runtime_error("Unsupported type");
        }
    }
        */
    return 0;
}

int Parser::get(const std::string& path, std::shared_ptr<DataElement>& element) {
    
    // Split path according to the standard format
    const auto& pathComponents = splitPath(path);
    /*
    // Check if the DataElement has already been analyzed and cached into the radix tree
    auto elementDirectPointer = radix_tree_.get(pathComponents);
    if(elementDirectPointer) {
        element = elementDirectPointer;
        return 0;
    }
    */

    element = root_;  // Start from the root
    /*
    std::vector<std::string_view> shortcuts_list;

    auto elementParentPointer = radix_tree_.find(pathComponents);
    std::vector<std::string_view> shortPathComponents;
    if(elementParentPointer.second) {
        //std::cout << "Reassign" << std::endl;
        shortPathComponents.assign(pathComponents.begin() + elementParentPointer.first.size(), pathComponents.end());
        element = elementParentPointer.second;
        shortcuts_list = elementParentPointer.first;
    } else {
        shortcuts_list.reserve(pathComponents.size());
    }
    */

    //std::cout << "[get] Number of components: " << pathComponents.size() << std::endl;
//    const std::vector<std::string_view>* selected = elementParentPointer.second ? &shortPathComponents : &pathComponents; 
//    for (const auto& component : *selected) {
//        shortcuts_list.emplace_back(component);
    for (const auto& component : pathComponents) {
        //std::cout << "[get] Analysing component: " << component << ", in type: " << element->getType() << std::endl;
        switch (element->getType()) {
            case ElementType::NULL_VALUE:
            case ElementType::BOOLEAN:
            case ElementType::NUMBER:
            case ElementType::STRING:
                if(!element->isMaterialized()) materializeElement(*element);
                //std::cout << "[get] Found primitive type, returning materialized element" << std::endl;
                return 0;
            case ElementType::OBJECT:
            case ElementType::ARRAY:
                if (element->isMaterializedElement(component)) {
                    //std::cout << "[get] get already materialized element" << std::endl;
                    element = element->getMaterializedElement(component);
                } else {
                    if (element->isTokenIndexRegistered(component)) {
                        //std::cout << "[get] key/index exists in the object/array" << std::endl;
                        auto tokenIndex = element->getTokenIndex(component);
                        const std::string_view tokenKey = element->getTokenStringView(component);
                        std::shared_ptr<DataElement> child = std::make_shared<DataElement>();
                        auto err = parseElement(child, tokenIndex);
                        if(err){
                            std::string errMsg = "Parsing Element returned error: "; errMsg.append(std::to_string(err));
                            throw std::runtime_error(errMsg);
                        }
                        err = materializeElement(*child);
                        if(err){
                            std::string errMsg = "Materialize Element returned error: "; errMsg.append(std::to_string(err));
                            throw std::runtime_error(errMsg);
                        }
                        element->addMaterializedElement(tokenKey, child);
                        element = child;
                    } else {
                        std::string errMsg = "Key/index <";
                        errMsg.append(component).append("> does not exist in the provided object/array");
                        std::cerr << errMsg << std::endl;
                        throw std::runtime_error(errMsg);
                    }
                }
                break; 
            default:
                throw std::runtime_error("Unsupported type");
        }   
        //radix_tree_.insert(shortcuts_list, element);
    }
    return 0;
}

std::vector<std::string_view> Parser::splitPath(const std::string& path) const {
    std::vector<std::string_view> components;
    
    if (path.empty()) {
        return components;
    }
    
    components.reserve(10);
    
    size_t start = 0;
    size_t pos = 0;
    
    while (pos < path.length()) {
        if (path[pos] == '.') {
            // Add component if not empty
            if (pos > start) {
                components.push_back(std::string_view(path.data() + start, pos - start));
                //components.push_back(path.substr(start, pos - start));
            }
            start = pos + 1;
        } else if (path[pos] == '[') {
            // Add component before bracket if not empty
            if (pos > start) {
                components.push_back(std::string_view(path.data() + start, pos - start));
//                components.push_back(path.substr(start, pos - start));
            }
            
            // Find closing bracket
            size_t closeBracket = path.find(']', pos);
            if (closeBracket == std::string_view::npos) {
                throw std::runtime_error("Unterminated '[' in path");
            }
            
            // Extract index between brackets - manteniamo come string_view
            components.push_back(std::string_view(path.data() + pos+1, closeBracket - pos - 1));
 //           components.push_back(path.substr(pos + 1, closeBracket - pos - 1));
            
            // Move past closing bracket
            pos = closeBracket;
            start = pos + 1;
        }
        
        pos++;
    }
    
    // Add final component if not empty
    if (start < path.length()) {
        components.push_back(std::string_view(path.data() + start));
//        components.push_back(path.substr(start));
    }
    
    return components;
}


std::string Parser::dump() const {
    std::ostringstream oss;
    dumpElement(root_, oss, tokens_);
    return oss.str();
}

void Parser::dumpElement(const std::shared_ptr<lazyjson::DataElement> element, std::ostringstream& oss, const auto& tokens) const {
    if(!element)
        throw std::runtime_error("Element points to null object");
    switch(element->getType()){
        case ElementType::NULL_VALUE:
            oss << "null";
            break;
        case ElementType::BOOLEAN:
//            oss << (element->isMaterialized()
            oss << (element->isModified()
                    ? (element->asBoolean() ? "true" : "false") 
                    : tokens.at(element->getTokenIndexStart()).value);
            break;
        case ElementType::NUMBER:
//            oss << (element->isMaterialized()
            oss << (element->isModified()
                    ? std::to_string(element->asNumber())
                    : tokens.at(element->getTokenIndexStart()).value);
            break;
        case ElementType::STRING:
            oss << "\"" 
//                << (element->isMaterialized()
                << (element->isModified()
                    ? element->asString() 
                    : tokens.at(element->getTokenIndexStart()).value)
                << "\"";
            break;
        case ElementType::OBJECT:
        case ElementType::ARRAY:
            {
//                if(element->isMaterialized()){
                if(element->isModified()){
                    oss << (element->getType()==ElementType::OBJECT ? "{ " : "[ ");
                    bool first = true;
                    for(const auto& key : element->getElementKeyList()){
                        if (!first) {
                            oss << ", ";
                        }
                        if (element->getType()==ElementType::OBJECT) oss << "\"" << key << "\": ";
                        if (element->isMaterializedElement(key)) {
                            const auto data = element->getMaterializedElement(key);
                            dumpElement(data, oss, tokens);
                        } else {
                            tokens.at(element->getTokenIndex(key)).dump(oss);
                        }
                        first = false;
                    }
                    oss << (element->getType()==ElementType::OBJECT ? " }" : " ]");
                    break;
                }

                // Dump the entire object or array using the position of the start/end tokens
                const auto& t1 = tokens.at(element->getTokenIndexStart()).value.data();
                const auto& t2 = tokens.at(element->getTokenIndexEnd()).value.data();
                oss << std::string_view{t1, static_cast<size_t>(t2 - t1 + 1)};
 
            }
            break;
        default:
            throw std::runtime_error("Trying to dump an invalid data type object");
    }
}

std::string Parser::elementToString(std::shared_ptr<DataElement> element) const {
    std::ostringstream oss;
    dumpElement(element, oss, tokens_);
    return oss.str();
}



} // namespace lazyjson