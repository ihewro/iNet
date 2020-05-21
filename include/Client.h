/**
  * User: hewro
  * Date: 2020/5/21
  * Time: 16:40
  * Description: 
  */
//
// Created by hewro on 2020/5/21.
//

#ifndef INET_CLIENT_H
#define INET_CLIENT_H
#include "UdpConnection.h"
#include "Message.h"
#define CLIENT_BUF_SIZE 2048

class Client {


public:
    Client(const string& serverHost, int serverPort);
    void close();
    void startRtt(int testTimes, int packetSize);
    void startBandwidth(uint32_t bandwidth,
                        char bandwidthUnit,
                        int packetSize,
                        int testSeconds,
                        int reportInterval);

private:
    UdpConnection conn;
    void sendMsg(const Message& msg);
};


#endif //INET_CLIENT_H
