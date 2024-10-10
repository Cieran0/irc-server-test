#include <util.hpp>

/*
    Splits a string, by split_by. 
    If include_last, includes string after final split_by, even if empty.
*/
std::vector<std::string> split_string(const std::string& input, const std::string& split_by, bool include_last) {
    std::string buffer;
    std::vector<std::string> split_strings;

    size_t start = 0;
    size_t end = input.find(split_by);

    //Loop through input until all split_by are found.
    while (end != std::string::npos) {
        buffer = input.substr(start, end - start);
        split_strings.push_back(buffer);
        start = end + split_by.length();
        end = input.find(split_by, start);
    }
    
    if(include_last) {
        buffer = input.substr(start, end);
        split_strings.push_back(buffer);
    }
    return split_strings;
}

/*
    Escapes newline and carriage return characters.
*/
std::string encode_escapes(std::string to_encode) {
    std::stringstream encoded_string_stream;

    for (char c : to_encode) {
        switch(c) {
            case '\n':
                encoded_string_stream << "\\n";
                break;
            case '\r':
                encoded_string_stream << "\\r";
                break;
            default:
                encoded_string_stream << c;
                break;
        }
    }

    return encoded_string_stream.str();
}