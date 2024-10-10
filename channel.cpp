#include "channel.hpp"

channel::channel(std::string name) : m_name(name){}
channel::~channel(){}

bool channel::empty(){
    return m_users.empty();
}

bool channel::has_user(std::string nickname){
    return m_users.contains(nickname);
}

void channel::join(std::string nickname){
    m_users.emplace(nickname);
}

void channel::remove_user(std::string nickname){
    m_users.erase(nickname);
}

std::unordered_set<std::string> channel::get_users(){
    return m_users;
}