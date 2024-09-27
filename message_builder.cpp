#include <message_builder.hpp>
#include <server.hpp>

std::string pad_code(int code) {
    std::string code_string = std::to_string(code);
    const int expected_length = 3;
    code_string.insert(0,expected_length - code_string.length(),'0');
    return code_string;
}

message_builder::message_builder()
{

}

message_builder::message_builder(std::string prefix) {
    back << prefix;

}


message_builder::~message_builder()
{
}

message_builder& message_builder::hostname(bool add_colon) {
    if(add_colon){
        back << ":";
    }
    back << server::host_name << " ";
    
    return *this;
};
message_builder& message_builder::code(int code) {
    back << pad_code(code) << " ";
    return *this;

};
message_builder& message_builder::raw(std::string str, bool addSpace) {
    back << str;
    if(addSpace){
        back << " ";
    }
    return *this;

};
message_builder& message_builder::text(std::string text, bool add_colon) {
    if(add_colon){
        back << ":";
    }
    back << text;
    
    return *this;
};

std::string message_builder::build() {
    back << "\r\n";
    return back.str();
}

message_builder& message_builder::user_details(client_info info) {
    back << ":" << info.nickname << "!" <<info.username << "@" << info.ip << " "; 
    return *this;
}
