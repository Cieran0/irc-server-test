#pragma once

#include <vector>
#include <string>

namespace irc
{
    struct client_command
    {
        std::string name;
        std::vector<std::string> arguments;
        std::string raw;

        bool operator==(const std::string& str) const {
            return name == str;
        }
    };

    irc::client_command parse_client_command(std::string str);
    
} // namespace irc
