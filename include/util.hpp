#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

typedef std::string message;

std::vector<std::string> split_string(const std::string& input, const std::string& split_by, bool include_last);
std::string decode(std::string to_decode);