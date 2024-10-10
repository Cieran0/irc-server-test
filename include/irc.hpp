#pragma once

#include <vector>
#include <string>

namespace irc
{
    struct client_command
    {
        std::string command;
        std::vector<std::string> arguments;
        std::string raw;

        bool operator==(const std::string& str) const {
            return command == str;
        }
    };

    irc::client_command parse_client_command(std::string clientCommand);
    
} // namespace irc
