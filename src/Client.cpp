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
 * @param packetsPerSecond 按照带宽计算出每秒需要发送多少个包
 * @param testSeconds 测试的总时间
 * @return
 */
int getEndTime(uint64_t packetsPerSecond,int testSeconds){
    const int groupTimeMs = 5;
//    const uint64_t packetsPerSecond = bandwidthValue / packetSize;
    const int packestPerGroup = std::ceil((double)packetsPerSecond * groupTimeMs / 1000);
    const double packetsIntervalMs = (double)1000 / packetsPerSecond;
    int64_t passedTime = 0;
    int totalPkt = 0;
    int64_t startTime = seeker::Time::currentTime();
    int64_t endTime = startTime + ((int64_t)testSeconds * 1000);
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
    TestRequest request = TestRequest(TestType::bandwidth,testSeconds,Message::genMid());
    sendMsg(request);

    //3. 接收服务器的返回的测试确认包
    uint8_t recvBuf[CLIENT_BUF_SIZE]{0};
    int recvLen = conn.recvData(reinterpret_cast<char *>(recvBuf), CLIENT_BUF_SIZE);
    if (recvLen > 0){
        I_LOG("recv test bandwidth confirm");

        int testId;
        TestConfirm confirm(recvBuf);
        testId = confirm.testId;
        memset(recvBuf, 0, recvLen);//清空数组


        //4. 发送数据包

        //确定结束时间
        int64_t startTime = seeker::Time::currentTime();
        int64_t endTime = startTime + ((int64_t)testSeconds * 1000);

        //确定每组发包的数目
        const int groupTimeMs = 5;
        const uint64_t packetsPerSecond = bandwidthValue / packetSize;
        const int packestPerGroup = std::ceil((double)packetsPerSecond * groupTimeMs / 1000);


        //进行发测试包
        int64_t passedTime = 0;
        int totalPkt = 0;//已经发送的包的数目
        uint64_t totalData = 0;//已经发送的数据量
        int64_t lastReportTime = 0;//上次打印报告距离程序开始的的时间
        const double packetsIntervalMs = (double)1000 / packetsPerSecond;//每个

        //测试包的数据结构
        uint8_t sendBuf[CLIENT_BUF_SIZE]{0};
        BandwidthTestMsg msg(packetSize, testId, 0, Message::genMid());
        size_t len = msg.getLength();
        msg.getBinary(sendBuf, MSG_SEND_BUF_SIZE);

        while(seeker::Time::currentTime() < endTime){
            for (int i =0;i<packestPerGroup;i++){
                T_LOG("send a BandwidthTestMsg, totalPkt={}", totalPkt);
                BandwidthTestMsg::update(sendBuf, Message::genMid(), totalPkt, seeker::Time::microTime());
                conn.sendData((char*)sendBuf, len);
                totalData += len;
                totalPkt += 1;
            }
            passedTime = seeker::Time::currentTime() - startTime;//发送totalPkt包实际花费的时间
            //判断是否需要进行打印，根据报告时间间隔
            if (passedTime - lastReportTime > reportInterval * 1000){
                lastReportTime = passedTime;
                I_LOG("[ ID] test time   Data transfer          Packets send");

                I_LOG("[{}]       {}s     {}            {}      ",
                      testId,
                      passedTime/1000,
                      formatTransfer(totalData),
                      totalPkt
                );
            }

            //进行线程休息，以确保发送速率
            //确定休息时间
            int aheadTime = (totalPkt * packetsIntervalMs) - passedTime;
            if (aheadTime > 5) {
                std::this_thread::sleep_for(std::chrono::milliseconds(aheadTime - 2));
            }
        }

        //发送结束测试包，以便接收服务端返回带宽的报告包
        BandwidthFinish finishMsg(testId, totalPkt, Message::genMid());
        sendMsg(finishMsg);
        recvLen = conn.recvData((char*)recvBuf, CLIENT_BUF_SIZE);

        if (recvLen > 0){
            BandwidthReport report(recvBuf);
            I_LOG("bandwidth test report:");
            I_LOG("[ ID] Interval    Transfer    Bandwidth     Jitter   Lost/Total Datagrams");

            double interval = (double)passedTime / 1000;
            int lossPkt = totalPkt - report.receivedPkt;
            I_LOG("[{}] {}s     {}    {}     {}ms   {}/{} ({:.{}f}%)",
                  testId,
                  interval,
                  formatTransfer(totalData),
                  formatBandwidth(totalData / interval),
                  (double)report.jitterMicroSec / 1000,
                  lossPkt,
                  totalPkt,
                  (double)100 * lossPkt / totalPkt,
                  4);
        }
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