#pragma once

#include "client.hpp"
#include "map"
#include <unordered_set>


//typedef std::map<std::string,std::vector<std::string>> channels;

class channel
{
private:
    std::string m_name;
    std::unordered_set<std::string> m_users;
public:
    channel() = default;
    channel(std::string name);
    bool empty();
    bool is_in(std::string username);
    void join(std::string username);
    void remove_user(std::string username);
    std::unordered_set<std::string> get_users();
    ~channel();
};

