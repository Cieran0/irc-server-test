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
    };

    irc::client_command parseClientCommand(std::string clientCommand);
    
} // namespace irc
