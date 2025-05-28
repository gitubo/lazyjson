#include "data.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdio>

namespace lazyjson {

    std::string escapeString(std::string_view str) {
        std::string escaped;
        escaped.reserve(str.length());
        
        for (char c : str) {
            switch (c) {
                case '\"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\b': escaped += "\\b"; break;
                case '\f': escaped += "\\f"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 32) {
                        // Control character, use hex escape
                        char hex[7];
                        std::snprintf(hex, sizeof(hex), "\\u%04x", static_cast<unsigned char>(c));
                        escaped += hex;
                    } else {
                        escaped += c;
                    }
                    break;
            }
        }
        
        return escaped;
    }

    std::ostream& operator<<(std::ostream& os, const ElementType& type) {
        switch (type) {
            case ElementType::NULL_VALUE:
                os << "NULL_VALUE";
                break;
            case ElementType::BOOLEAN:
                os << "BOOLEAN";
                break;
            case ElementType::NUMBER:
                os << "NUMBER";
                break;
            case ElementType::STRING:
                os << "STRING";
                break;
            case ElementType::OBJECT:
                os << "OBJECT";
                break;
            case ElementType::ARRAY:
                os << "ARRAY";
                break;
            default:
                os << "UNDEFINED";
                break;
        }
        return os;
    }

} // namespace lazyjson