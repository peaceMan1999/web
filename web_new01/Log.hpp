#pragma once

#include <iostream>
#include <string>
#include <ctime>

using std::cout;
using std::endl;
using std::string;

#define INFO    1
#define WARNING 2
#define ERROR   3
#define FATAL   4

#define LOG(level, msg) Log(#level, msg, __FILE__, __LINE__) 

void Log(string level, string msg, string file_name, int line){
    cout << "[" << level << "]"
    << "[" << time(nullptr) << "]"
    << "[" << msg.c_str() << "]"
    << "[" << file_name.c_str() << "]"
    << "[" << line << "]" << endl;
}
