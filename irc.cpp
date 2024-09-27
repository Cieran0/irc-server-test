#include <irc.hpp>
#include <iostream>
#include <sstream>

irc::client_command irc::parseClientCommand(std::string clientCommand) {

    irc::client_command parsedCommand;
    parsedCommand.raw = clientCommand;

    size_t command_end = clientCommand.find_first_of(' ');
    if(command_end == std::string::npos) {
        //FIXME handle error reading client command
        std::cout << "Failed to parse command [" << clientCommand << "]" << std::endl;
    }

    parsedCommand.command = clientCommand.substr(0, command_end);

    size_t after_name = command_end + 1;
    std::string argsString = clientCommand.substr(after_name);
    size_t endOfSpaceSeperated = argsString.find_first_of(':');

    std::string last_args;
    if (endOfSpaceSeperated != std::string::npos) {
        last_args = argsString.substr(endOfSpaceSeperated + 1);
        argsString = argsString.substr(0, endOfSpaceSeperated);
    }

    std::istringstream iss(argsString);
    std::string arg;
    while (iss >> arg) {
        parsedCommand.arguments.push_back(arg);
    }



    if(!last_args.empty()) {
        parsedCommand.arguments.push_back(last_args);
    }

    return parsedCommand;
}