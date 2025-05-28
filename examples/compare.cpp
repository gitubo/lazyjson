#include "parser.hpp"
#include "data.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>

#include "nlohmann/json.hpp"
#include <rapidjson/document.h>     // per parsing e manipolazione JSON
#include <rapidjson/writer.h>       // per serializzazione (dump)
#include <rapidjson/stringbuffer.h> // per salvare il JSON come stringa

#define REPETITIONS 100

int main() {
    using namespace std::chrono;

    std::string jsonStr = R"(
        {
            "bool_1":true,
            "num_1":123.456,
            "str_1":"Ciao Mondo!",
            "arr_1":[123,1.020304,3.00],
            "obj_1":{"a":1,"b":2},
            "obj_2":{"obj_nested":{"a":3,"b":4,"obj_nested":{"x":200.2,"y":100.1}},"str_nested":"pippo"}
        }
    )";
    std::string jsonStr1 = R"({
        "bool_1": false, "bool_2": false, "bool_3": true, "bool_4": false, "bool_5": true,
        "bool_6": false, "bool_7": true, "bool_8": false, "bool_9": true, "bool_10": false,
        "bool_11": true, "bool_12": false,

        "str_1": "alpha", "str_2": "beta", "str_3": "gamma", "str_4": "delta",
        "str_5": "epsilon", "str_6": "zeta", "str_7": "eta", "str_8": "theta",
        "str_9": "iota", "str_10": "kappa", "str_11": "lambda", "str_12": "mu",

        "num_1": 1, "num_2": 2, "num_3": 3, "num_4": 4, "num_5": 5, "num_6": 6,
        "num_7": 7, "num_8": 8, "num_9": 9, "num_10": 10, "num_11": 11, "num_12": 12,

        "arr_num_1": [1,2,3], "arr_num_2": [4,5,6], "arr_num_3": [7,8,9], "arr_num_4": [10,11,12],
        "arr_str_1": ["a","b"], "arr_str_2": ["c","d"], "arr_str_3": ["e","f"], "arr_str_4": ["g","h"],

        "arr_obj_1": [{ "a": { "b": { "c": { "d": { "e": { "value": 1 } } } } } }],
        "arr_obj_2": [{ "a": { "b": { "c": { "d": { "e": { "value": 2 } } } } } }],
        "arr_obj_3": [{ "a": { "b": { "c": { "d": { "e": { "value": 3 } } } } } }],
        "arr_obj_4": [{ "a": { "b": { "c": { "d": { "e": { "value": 4 } } } } } }],

        "obj_1": { "num": 1, "str": "one", "arr": [1, 2], "obj": { "a": { "b": { "c": { "d": "deep" } } } } },
        "obj_2": { "num": 2, "str": "two", "arr": [3, 4], "obj": { "a": { "b": { "c": { "d": "deep" } } } } },
        "obj_3": { "num": 3, "str": "three", "arr": [5, 6], "obj": { "a": { "b": { "c": { "d": "deep" } } } } },
        "obj_4": { "num": 4, "str": "four", "arr": [7, 8], "obj": { "a": { "b": { "c": { "d": "deep" } } } } },
        "obj_5": { "num": 5, "str": "five", "arr": [9, 10], "obj": { "a": { "b": { "c": { "d": "deep" } } } } },
        "obj_6": { "num": 6, "str": "six", "arr": [11, 12], "obj": { "a": { "b": { "c": { "d": "deep" } } } } },
        "obj_7": { "num": 7, "str": "seven", "arr": [13, 14], "obj": { "a": { "b": { "c": { "d": "deep" } } } } },
        "obj_8": { "num": 8, "str": "eight", "arr": [15, 16], "obj": { "a": { "b": { "c": { "d": "deep" } } } } },
        "obj_9": { "num": 9, "str": "nine", "arr": [17, 18], "obj": { "a": { "b": { "c": { "d": "deep" } } } } },
        "obj_10": { "num": 10, "str": "ten", "arr": [19, 20], "obj": { "a": { "b": { "c": { "d": "deep" } } } } },
        "obj_11": { "num": 11, "str": "eleven", "arr": [21, 22], "obj": { "a": { "b": { "c": { "d": "deep" } } } } },
        "obj_12": { "num": 12, "str": "twelve", "arr": [23, 24], "obj": { "a": { "b": { "c": { "d": "deep" } } } } }
    })";

    std::vector<std::string> paths = {
        "bool_1"
        ,"arr_1[1]"
        ,"arr_1[0]"
        ,"obj_1.a"
        ,"obj_2.obj_nested.b"
        ,"obj_2.obj_nested.obj_nested.x"
        ,"obj_2.obj_nested.a"
        ,"obj_2.obj_nested.a"
/*
        ,"arr_num_2[1]", "arr_str_4[0]"
        ,"arr_obj_1[0].a.b.c.d.e.value"
        ,"obj_1.str"
        , "obj_10.obj.a.b.c.d"
       , "obj_5.arr[1]"
*/
    };

    std::vector<std::string> nlohmann_paths = {
        "bool_1"
        ,"arr_1/1/"
        ,"arr_1/0/"
        ,"obj_1/a"
        ,"obj_2/obj_nested/b"
        ,"obj_2/obj_nested/obj_nested/x"
        ,"obj_2/obj_nested/a"
        ,"obj_2/obj_nested/a"
/*
        ,"arr_num_2[1]", "arr_str_4[0]"
        ,"arr_obj_1[0].a.b.c.d.e.value"
        ,"obj_1.str"
        , "obj_10.obj.a.b.c.d"
       , "obj_5.arr[1]"
*/
    };


    // lazyjson
    int64_t lazyjson_duration_avg = 0;
    for(int i=0; i<REPETITIONS; i++)
    {
        lazyjson::Parser parser;
        auto t_lasyjson_start = high_resolution_clock::now();
        if (!parser.parse(jsonStr)) {
            std::cerr << "Failed to parse JSON\n";
            return 1;
        }
        for (const auto& path : paths) {
            std::shared_ptr<lazyjson::DataElement> elem;
            auto err = parser.get(path,elem);
            if(err){
                std::cerr << "Error in retrieving " << path << std::endl;
                return 2;
            }
        }
        std::string dumped = parser.dump();
        lazyjson_duration_avg = duration_cast<nanoseconds>(high_resolution_clock::now() - t_lasyjson_start).count();
    }
    std::cout << "lazyjson total avg time: " << lazyjson_duration_avg << " ns\n";

    // nlohmann
    int64_t nlohmann_duration_avg = 0;
    for(int i=0; i<REPETITIONS; i++)
    {
        nlohmann::json j;
        auto t_nlohmann_start = high_resolution_clock::now();
        try {
            j = nlohmann::json::parse(jsonStr).flatten();
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse JSON with nlohmann: " << e.what() << std::endl;
            return 100;
        }
        
        for (const auto& path : nlohmann_paths) {
            auto value = j[path];
    }
        std::string dumped = j.dump();
        nlohmann_duration_avg = duration_cast<nanoseconds>(high_resolution_clock::now() - t_nlohmann_start).count();

    }
    std::cout << "nlohmann total avg time: " << nlohmann_duration_avg << " ns\n";

    int64_t rapidjson_duration_avg = 0;
    for(int i=0; i<REPETITIONS; i++)
    {
        using namespace rapidjson;
        Document doc;
        auto t_rapidjson_start = high_resolution_clock::now();
    
        if (doc.Parse(jsonStr.c_str()).HasParseError()) {
            std::cerr << "RapidJSON failed to parse JSON\n";
            return 200;
        }
    
        // Access paths manually (no built-in flattening)
        std::vector<const Value*> found;
        for (const auto& path : paths) {
            const Value* current = &doc;
            std::istringstream ss(path);
            std::string token;
            bool error = false;
    
            while (std::getline(ss, token, '.')) {
                size_t index = std::string::npos;
                size_t bracket_pos = token.find('[');
                if (bracket_pos != std::string::npos) {
                    index = std::stoul(token.substr(bracket_pos + 1, token.find(']') - bracket_pos - 1));
                    token = token.substr(0, bracket_pos);
                }
    
                if (token != "") {
                    if (!current->IsObject() || !current->HasMember(token.c_str())) {
                        error = true;
                        break;
                    }
                    current = &(*current)[token.c_str()];
                }
    
                if (index != std::string::npos) {
                    if (!current->IsArray() || index >= current->Size()) {
                        error = true;
                        break;
                    }
                    current = &(*current)[index];
                }
            }
    
            if (error) {
                std::cerr << "RapidJSON failed to get: " << path << std::endl;
                return 201;
            }
    
            found.push_back(current); // store or use if needed
        }
    
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        std::string dumped = buffer.GetString();
    
        rapidjson_duration_avg = duration_cast<nanoseconds>(high_resolution_clock::now() - t_rapidjson_start).count();

    }
    std::cout << "rapidjson total avg time: " << rapidjson_duration_avg << " ns\n";

    return 0;
}
