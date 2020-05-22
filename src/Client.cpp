/**
  * User: hewro
  * Date: 2020/5/21
  * Time: 16:40
  * Description: 
  */
//
// Created by hewro on 2020/5/21.
//

#include <seeker/loggerApi.h>
#include "Client.h"

extern string formatTransfer(const uint64_t& dataSize);
extern string formatBandwidth(const uint32_t& bytesPerSecond);

Client::Client(const string &serverHost, int serverPort) {
    seeker::SocketUtil::startupWSA();

    conn.setRemoteIp(serverHost);
    conn.setRemotePort(serverPort);
    conn.init();
}

void Client::close() {
    I_LOG("client finish.");
    conn.close();
}

/**
 * 发送rtt测试请求
 * @param testTimes  发送的包的次数，默认10次
 * @param packetSize 包的数据大小，默认64
 */
void Client::startRtt(int testTimes, int packetSize) {
    //1. 发送测试请求
    TestRequest request = TestRequest(TestType::rtt,10,Message::genMid());
    sendMsg(request);

    //2. 接收服务端的返回
    uint8_t recvBuf[CLIENT_BUF_SIZE]{0};
    int recvLen = conn.recvData((char *)(recvBuf), CLIENT_BUF_SIZE);
    int testId = 0;
    if (recvLen > 0 ){
        TestConfirm confirm = TestConfirm(recvBuf);
        memset(recvBuf, 0, recvLen);//数组置空
        testId = confirm.testId;
        //3. 进入发送测试包
        while (testTimes > 0){
            testTimes --;
            RttTestMsg msg = RttTestMsg(packetSize,testId,Message::genMid());
            sendMsg(msg);

            int recvLen = conn.recvData((char *)(recvBuf), CLIENT_BUF_SIZE);
            RttTestMsg response = RttTestMsg(recvBuf);
            auto diffTimeMs =(double) (seeker::Time::microTime() - response.timestamp)/1000;
            memset(recvBuf, 0, recvLen);//数组置空
            if (recvLen > 0){
                //打印输出结果
                I_LOG("64字节往返时间：消息序号={} testid={} rtt={}ms",
                      response.msgId,
                      response.testId,
                      diffTimeMs);
            } else{
                E_LOG("收到服务端的消息错误");
                break;
            }

        }
    }else{
        E_LOG("收到服务端的消息错误");
    }

}

/**
 *
 * @param bandwidth 发送带宽大小
 * @param bandwidthUnit 发送带宽的单位
 * @param packetSize 每个测试包的大小，单位字节
 * @param testSeconds 测试的时间
 * @param reportInterval 打印报告的间隔
 */
void
Client::startBandwidth(uint32_t bandwidth, char bandwidthUnit, int packetSize, int testSeconds, int reportInterval) {
    //1. 计算带宽，换算成标准单位字节
    int bandwidthValue = 0;
    switch (bandwidthUnit) {
        case 'b':
        case 'B':
            bandwidthValue = bandwidth;
            break;
        case 'k':
        case 'K':
            bandwidthValue = 1024 * (uint64_t)bandwidth;
            break;
        case 'm':
        case 'M':
            bandwidthValue = 1024 * 1024 * (uint64_t)bandwidth;
            break;
        case 'g':
        case 'G':
            bandwidthValue = 1024 * 1024 * 1024 * (uint64_t)bandwidth;
            break;
        default:
            throw std::runtime_error("bandwidthUnit error: " + std::to_string(bandwidthUnit));
    }
    bandwidthValue = bandwidthValue/8;

    //2.发送测试请求
    TestRequest request = TestRequest(TestType::bandwidth,0,Message::genMid());
    sendMsg(request);

    //3. 接收服务器的返回
    uint8_t recvBuf[CLIENT_BUF_SIZE]{0};
    int recvLen = conn.recvData(reinterpret_cast<char *>(recvBuf), CLIENT_BUF_SIZE);
    if (recvLen > 0){



        //4. 发送数据包
    }


}

void Client::sendMsg(const Message& msg) {
    Message::sendMsg(msg, conn);
    // T_LOG("send Message, msgType={} msgId={}", msg.msgType, msg.msgId);
}


/**
 * rtt测试
 * @param serverHost
 * @param serverPort
 * @param testSeconds 测试时间
 */
void startClientRtt(const string& serverHost, int serverPort, int testSeconds) {
    I_LOG("Starting send data to {}:{}", serverHost, serverPort);
    Client client(serverHost, serverPort);
    client.startRtt(10, 64);
    client.close();
}

/**
 * bandwidth 测试
 * @param serverHost
 * @param serverPort
 * @param testSeconds
 * @param bandwidth
 * @param bandwidthUnit
 * @param packetSize
 * @param reportInterval
 */
void startClientBandwidth(const string& serverHost,
                          int serverPort,
                          int testSeconds,
                          uint32_t bandwidth,
                          char bandwidthUnit,
                          int packetSize,
                          int reportInterval) {
    I_LOG("Starting send data to {}:{}", serverHost, serverPort);
    Client client(serverHost, serverPort);

    client.startBandwidth(bandwidth, bandwidthUnit, packetSize, testSeconds, reportInterval);

    client.close();
}