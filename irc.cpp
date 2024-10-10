#include <irc.hpp>
#include <iostream>
#include <sstream>

/*
    Parses a string into a irc::client_command.
    Extracts command e.g "PRIVMSG" 
    and arguments e.g {"#channel", "This is a message"}
*/
irc::client_command irc::parse_client_command(std::string clientCommand) {
    //TODO: comment this!
    irc::client_command parsed_command;
    parsed_command.raw = clientCommand;

    size_t command_end = clientCommand.find_first_of(' ');
    if(command_end == std::string::npos) {
        std::cerr << "Failed to parse command [" << clientCommand << "]" << std::endl;
    }

    parsed_command.command = clientCommand.substr(0, command_end);

    size_t after_name = command_end + 1;
    std::string argsString = clientCommand.substr(after_name);
    size_t end_of_space_seperated = argsString.find_first_of(':');

    std::string last_args;
    if (end_of_space_seperated != std::string::npos) {
        last_args = argsString.substr(end_of_space_seperated + 1);
        argsString = argsString.substr(0, end_of_space_seperated);
    }

    std::istringstream iss(argsString);
    std::string arg;
    while (iss >> arg) {
        parsed_command.arguments.push_back(arg);
    }

    if(!last_args.empty()) {
        parsed_command.arguments.push_back(last_args);
    }

    return parsed_command;
}