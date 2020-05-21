#include <iostream>
#include <string>
#include <cxxopts.hpp>
#include "seeker/logger.h"


using std::cout;
using std::endl;
using std::string;
cxxopts::ParseResult parse(int argc, char* argv[]);
extern void startClientRtt(const string& serverHost, int serverPort, int timeInSeconds);

extern void startClientBandwidth(const string& serverHost,
                          int serverPort,
                          int testSeconds,
                          uint32_t bandwidth,
                          char bandwidthUnit,
                          int packetSize,
                          int reportInterval);

extern void startServer(int port);

int main(int argc, char* argv[]) {
    seeker::Logger::init();
    auto result = parse(argc, argv); //解析命令行参数

    //解析命令行参数，根据参数，决定操作
    try {
        if (result.count("s")) {//启动服务端
            int port = result["p"].as<int>();
            startServer(port);
        } else if (result.count("c")) {//启动客户端
            string host = result["c"].as<string>();
            int port = result["p"].as<int>();//端口号

            if (result.count("b")) {//发送带宽
                int time = result["t"].as<int>();//测试时间
                int reportInterval = result["i"].as<int>();
                string bandwidth = result["b"].as<string>();
                char bandwidthUnit = bandwidth.at(bandwidth.size() - 1);
                auto bandwidthValue = std::stoi(bandwidth.substr(0, bandwidth.size() - 1));
                switch (bandwidthUnit) {
                    case 'b':
                    case 'B':
                    case 'k':
                    case 'K':
                    case 'm':
                    case 'M':
                    case 'g':
                    case 'G': {
                        int packetSize = 1400;//? 1400 有什么特殊含义吗
                        startClientBandwidth(
                                host, port, time, bandwidthValue, bandwidthUnit, packetSize, reportInterval);
                        break;
                    }
                    default:
                        E_LOG("params error, bandwidth unit should only be B, K, M or G");
                        return -1;
                }
            } else {
                startClientRtt(host, port, 10);
            }

        } else {
            E_LOG("params error, it should not happen.");
        }


    } catch (std::runtime_error ex) {
        cout << "runtime_error: " << ex.what() << endl;
    } catch (...) {
        cout << "unknown error." << endl;
    }


    return 0;
}
