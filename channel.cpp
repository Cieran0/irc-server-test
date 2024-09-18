#include <channel.hpp>

channel::channel(std::string name) : m_name(name)
{

}

bool channel::empty() 
{
    return m_users.empty();
}

bool channel::is_in(std::string username) 
{
    return m_users.contains(username);
}

void channel::join(std::string username)
{
    m_users.emplace(username);
}

void channel::remove_user(std::string username)
{
    m_users.erase(username);
}

std::unordered_set<std::string> channel::get_users() 
{
    return m_users;
}

channel::~channel()
{

}