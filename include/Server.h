/**
  * User: hewro
  * Date: 2020/5/21
  * Time: 16:40
  * Description: 
  */
//
// Created by hewro on 2020/5/21.
//

#ifndef INET_SERVER_H
#define INET_SERVER_H

#include "UdpConnection.h"
#include "seeker/socketUtil.h"
#define SERVER_BUF_SIZE 2048

class Server {

public:
    explicit Server(int p);
    void start();
    void close();

private:
    UdpConnection conn{};//服务器端的socket管理
    uint16_t currentTest{0};
    std::atomic<int> testIdGen{1};
//    int genTestId();
    void bandwidthTest(int testSeconds);//带宽测试
};


#endif //INET_SERVER_H
