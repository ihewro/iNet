#include <iostream>
#include <string>
#include <cxxopts.hpp>
#include "seeker/logger.h"


using std::cout;
using std::endl;
using std::string;
cxxopts::ParseResult parse(int argc, char* argv[]);


int main(int argc, char* argv[]) {
    seeker::Logger::init();
    auto result = parse(argc, argv); //解析命令行参数

    return 0;
}
