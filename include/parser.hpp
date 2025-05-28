#ifndef LAZYJSON_PARSER_HPP
#define LAZYJSON_PARSER_HPP

#include "tokenizer.hpp"
#include "data.hpp"
#include "string_buffer.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace lazyjson {

    // JSON parser
    class Parser {
    public:
        Parser();
        
        // Parse a JSON string
        bool parse(std::string& jsonString);
        
        // Get/Set a value using a path expression
        int get(const std::string&, std::shared_ptr<DataElement>&);
        int set(const std::string&, std::shared_ptr<DataElement>);
        
        // Generate a JSON string from the parsed structure
        std::string dump() const;
        std::string elementToString(std::shared_ptr<DataElement>) const;

    private:

        // Parse a JSON object/array (first level only)
        int parseElement(std::shared_ptr<DataElement> element, size_t& currentIndex);

        //std::shared_ptr<DataElement> materializeToken(const std::vector<Token>& tokens, size_t& currentIndex);
        int materializeElement(DataElement&);
        
        void dumpElement(const std::shared_ptr<lazyjson::DataElement>, std::ostringstream&, const auto&) const;

        // Parse a path expression
        std::vector<std::string_view> splitPath(const std::string& path) const;
        void skipValue(const std::vector<Token>& tokens, size_t& currentIndex);

        // Tokenizer
        Tokenizer tokenizer_;
        
        // Tokens
        std::vector<Token> tokens_;
        
        // Root value
        std::shared_ptr<DataElement> root_ = std::make_shared<DataElement>();
        
        // String buffer
        StringBuffer string_buffer_;
    };

} // namespace lazyjson

#endif // LAZYJSON_PARSER_HPP