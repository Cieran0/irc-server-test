#pragma once
#include <string>
#include <sstream>


class message_builder
{
private:
    std::stringstream back;
public:
    message_builder(std::string prefix);
    message_builder();
    ~message_builder();

    message_builder& hostname();
    message_builder& code(int code);
    message_builder& raw(std::string str, bool addSpace);
    message_builder& text(std::string text);

    std::string build();
};

