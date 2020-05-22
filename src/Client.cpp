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
    //1. 发送请求测试
    uint8_t recvBuf[CLIENT_BUF_SIZE]{0};
    assert(packetSize >= 24);
    TestRequest req(TestType::rtt, 0, Message::genMid());
    sendMsg(req);
    I_LOG("send TestRequest, msgId={} testType={}", req.msgId, req.testType);

    //2. 等待回复
    auto testId = 0;
    auto recvLen = conn.recvData((char *) recvBuf, CLIENT_BUF_SIZE);
    if (recvLen > 0) {
        TestConfirm confirm(recvBuf);
        I_LOG("receive TestConfirm, result={} reMsgId={} testId={}",
              confirm.result,
              confirm.reMsgId,
              req.testId);
        testId = confirm.testId;
        memset(recvBuf, 0, recvLen);
    } else {
        throw std::runtime_error("TestConfirm receive error. recvLen=" + std::to_string(recvLen));
    }

    //3. 发送数据包进行测试
    while (testTimes > 0) {
        testTimes--;
        RttTestMsg msg(packetSize, testId, Message::genMid());
        sendMsg(msg);
        T_LOG("send RttTestMsg, msgId={} testId={} time={}", msg.msgId, msg.testId, msg.timestamp);

        recvLen = conn.recvData((char*)recvBuf, CLIENT_BUF_SIZE);
        if (recvLen > 0) {
            RttTestMsg rttResponse(recvBuf);
            memset(recvBuf, 0, recvLen);
            auto diffTime = seeker::Time::microTime() - rttResponse.timestamp;
            I_LOG("receive RttTestMsg, msgId={} testId={} time={} diff={}ms",
                  rttResponse.msgId,
                  rttResponse.testId,
                  rttResponse.timestamp,
                  (double)diffTime / 1000);
        } else {
            throw std::runtime_error("RttTestMsg receive error. recvLen=" + std::to_string(recvLen));
        }
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

    //1. 单位换算
    uint64_t bandwidthValue = 0;//发送的数据包的大小，单位是bit
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

    assert(testSeconds <= 100);
    assert(packetSize >= 24);
    assert(packetSize <= 1500);
    assert(bandwidthValue < (uint64_t)10 * 1024 * 1024 * 1024 + 1);
    std::cout << "bandwidthValue in bit:" << bandwidthValue << std::endl;
    bandwidthValue = bandwidthValue / 8;//单位转换成字节
    std::cout << "bandwidthValue in byte:" << bandwidthValue << std::endl;

    //2. 带宽测试请求
    uint8_t recvBuf[CLIENT_BUF_SIZE]{0};
    TestRequest req(TestType::bandwidth, testSeconds, Message::genMid());
    sendMsg(req);
    I_LOG("send TestRequest, msgId={} testType={}", req.msgId, req.testType);


    //3. 收到请求确认
    int testId = 0;
    auto recvLen = conn.recvData((char*)recvBuf, CLIENT_BUF_SIZE);
    if (recvLen > 0) {
        TestConfirm confirm(recvBuf);
        I_LOG("receive TestConfirm, result={} reMsgId={} testId={}",
              confirm.result,
              confirm.reMsgId,
              req.testId);
        testId = confirm.testId;
        memset(recvBuf, 0, recvLen);
    } else {
        throw std::runtime_error("TestConfirm receive error. recvLen=" + std::to_string(recvLen));
    }

    //4. 发送测试数据包
    uint8_t sendBuf[CLIENT_BUF_SIZE]{0};

    BandwidthTestMsg msg(packetSize, testId, 0, Message::genMid());
    size_t len = msg.getLength();
    msg.getBinary(sendBuf, MSG_SEND_BUF_SIZE);


    const int groupTimeMs = 5;//? 每个包的发送时间估计值
    const uint64_t packetsPerSecond = bandwidthValue / packetSize;//每秒需要发送的数据包的数目
    const int packestPerGroup = std::ceil((double)packetsPerSecond * groupTimeMs / 1000);//发送完所有数据包的发送的秒数
    const double packetsIntervalMs = (double)1000 / packetsPerSecond;//每个包的发送间隔


    I_LOG("bandwidthValue={}, packetsPerSecond={}, packestPerGroup={}, packetsIntervalMs={}",
          bandwidthValue,
          packetsPerSecond,
          packestPerGroup,
          packetsIntervalMs);


    int64_t passedTime = 0;
    int totalPkt = 0;//已经发送的包数目
    int64_t startTime = seeker::Time::currentTime();
    int64_t endTime = startTime + ((int64_t)testSeconds * 1000);

    int64_t lastReportTime = 0; //上次报告时间

    uint64_t totalData = 0;
    while (seeker::Time::currentTime() < endTime) {
        for (int i = 0; i < packestPerGroup; i++) {
            T_LOG("send a BandwidthTestMsg, totalPkt={}", totalPkt);
            BandwidthTestMsg::update(sendBuf, Message::genMid(), totalPkt, seeker::Time::microTime());
            conn.sendData((char*)sendBuf, len);
            totalData += len;
            totalPkt += 1;
        }

        passedTime = seeker::Time::currentTime() - startTime;

        if(passedTime - lastReportTime > (int64_t)reportInterval * 1000) {


            lastReportTime = passedTime;
            I_LOG("[ ID] test time   Data transfer          Packets send");

            I_LOG("[{}]       {}s     {}            {}      ",
                  testId,
                  passedTime/1000,
                  formatTransfer(totalData),
                  totalPkt
            );
        }


        int aheadTime = (totalPkt * packetsIntervalMs) - passedTime;
        if (aheadTime > 5) {//控制发送速率
            std::this_thread::sleep_for(std::chrono::milliseconds(aheadTime - 2));
        }
    }

    BandwidthFinish finishMsg(testId, totalPkt, Message::genMid());
    sendMsg(finishMsg);
    T_LOG("waiting report.");
    recvLen = conn.recvData((char*)recvBuf, CLIENT_BUF_SIZE);

    //5. 接收最终的测试报告
    if (recvLen > 0) {
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
    } else {
        throw std::runtime_error("TestConfirm receive error. recvLen=" + std::to_string(recvLen));
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