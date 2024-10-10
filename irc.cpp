#include <irc.hpp>
#include <iostream>
#include <sstream>
#include <string>

/*
    Parses a string into a irc::client_command.
    Extracts command e.g "PRIVMSG" 
    and arguments e.g {"#channel", "This is a message"}
*/
irc::client_command irc::parse_client_command(std::string str) {
    irc::client_command parsed_command;
    parsed_command.raw = str;

    //Name ends at first space
    size_t command_end = str.find_first_of(' ');
    if(command_end == std::string::npos) {
        std::cerr << "Failed to parse command [" << str << "]" << std::endl;
        //If name never ends it returns an empty command
        return parsed_command;
    }

    parsed_command.name = str.substr(0, command_end);

    size_t after_name = command_end + 1;
    std::string args_string = str.substr(after_name);

    //If it has text e.g :No Topic Set, set it as the last argument
    size_t end_of_space_seperated = args_string.find_first_of(':');

    std::string last_args;
    if (end_of_space_seperated != std::string::npos) {
        last_args = args_string.substr(end_of_space_seperated + 1);
        args_string = args_string.substr(0, end_of_space_seperated);
    }

    std::istringstream iss(args_string);
    std::string arg;

    //Get arguments seperated by spaces.
    while (iss >> arg) {
        parsed_command.arguments.push_back(arg);
    }

    if(!last_args.empty()) {
        parsed_command.arguments.push_back(last_args);
    }

    return parsed_command;
}