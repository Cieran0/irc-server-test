#pragma once
#include <string>
#include <sstream>
#include <client.hpp>


class message_builder
{
private:
    std::stringstream back;
public:
    message_builder(std::string prefix);
    message_builder();
    ~message_builder();

    message_builder& hostname(bool add_colon);
    message_builder& code(int code);
    message_builder& raw(std::string str, bool addSpace);
    message_builder& text(std::string text, bool add_colon);
    message_builder& user_details(client_info info);

    std::string build();
};

