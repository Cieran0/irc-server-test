#pragma once

#include "client.hpp"
#include <map>
#include <unordered_set>

class channel{
    private:
        std::string m_name;
        std::unordered_set<std::string> m_users;
    public:
        channel() = default;
        channel(std::string name);
        bool empty();
        bool is_in(std::string nickname);
        void join(std::string nickname);
        void remove_user(std::string nickname);
        std::unordered_set<std::string> get_users();
        ~channel();
};