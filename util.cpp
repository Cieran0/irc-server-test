#include <util.hpp>

std::vector<std::string> split_string(const std::string& input, const std::string& split_by, bool include_last) {
    std::string buffer;
    std::vector<std::string> split_strings;

    size_t start = 0;
    size_t end = input.find(split_by);

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

std::string decode(std::string to_decode) {
    std::string decoded_string;

    // Loop through each character in the input string
    for (char c : to_decode) {
        if (c == '\n') {
            decoded_string += "\\n";  // Replace newline with literal '\n'
        } else if (c == '\r') {
            decoded_string += "\\r";  // Replace carriage return with literal '\r'
        } else {
            decoded_string += c;      // Otherwise, add the character as it is
        }
    }

    return decoded_string;
}