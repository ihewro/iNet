/**
  * User: hewro
  * Date: 2020/5/21
  * Time: 16:40
  * Description: 
  */
//
// Created by hewro on 2020/5/21.
//

#include "Server.h"
#include "seeker/loggerApi.h"
#include "Message.h"

string formatTransfer(const uint64_t& dataSize) {
    static uint32_t Gbytes = 1024 * 1024 * 1024;
    static uint32_t Mbytes = 1024 * 1024;
    static uint32_t Kbytes = 1024;
    string rst{};
    if (dataSize > Gbytes) {
        rst = fmt::format("{:.{}f}Gbytes", (double)dataSize / Gbytes, 3);
    } else if (dataSize > Mbytes) {
        rst = fmt::format("{:.{}f}Mbytes", (double)dataSize / Mbytes, 3);
    } else if (dataSize > Kbytes) {
        rst = fmt::format("{:.{}f}Kbytes", (double)dataSize / Kbytes, 3);
    } else {
        rst = fmt::format("{}Gbytes", dataSize);
    }
    return rst;
}

string formatBandwidth(const uint32_t& bytesPerSecond) {
    static uint32_t Gbits = 1024 * 1024 * 1024;
    static uint32_t Mbits = 1024 * 1024;
    static uint32_t Kbits = 1024;
    uint64_t bitsPerSecond = (uint64_t)bytesPerSecond * 8;

    string rst{};
    if (bitsPerSecond > Gbits) {
        rst = fmt::format("{:.{}f}Gbits/sec", (double)bitsPerSecond / Gbits, 3);
    } else if (bitsPerSecond > Mbits) {
        rst = fmt::format("{:.{}f}Mbits/sec", (double)bitsPerSecond / Mbits, 3);
    } else if (bitsPerSecond > Kbits) {
        rst = fmt::format("{:.{}f}Kbits/sec", (double)bitsPerSecond / Kbits, 3);
    } else {
        rst = fmt::format("{}bits/sec", bitsPerSecond);
    }
    return rst;
}


Server::Server(int p) {
    //初始化
    D_LOG("init server on port[{}]", p);

    seeker::SocketUtil::startupWSA();
    conn.setLocalIp("0.0.0.0");
    conn.setLocalPort(p);
    conn.init();

    D_LOG("server socket bind port[{}] success: {}", p, conn.getSocket());
}

void Server::start() {
    uint8_t recvBuf[SERVER_BUF_SIZE]{0};

    while (true){
        //接收客户端的数据
        auto recvLen = conn.recvData((char*)recvBuf, SERVER_BUF_SIZE);
        if (recvLen > 0) {
            uint8_t msgType;
            Message::getMsgType(recvBuf, msgType);
            int msgId = 0;
            Message::getMsgId(recvBuf, msgId);
            switch ((MessageType)msgType) {
                case MessageType::testRequest: {
                    TestRequest req(recvBuf);
                    I_LOG("Got TestRequest, msgId={}, testType={}", req.msgId, (int)req.testType);
                    TestConfirm response(1, testIdGen.fetch_add(1), req.msgId, Message::genMid());
                    I_LOG("Reply Msg TestConfirm, msgId={}, testType={}, rst={}", response.msgId,
                          (int)response.msgType, response.result);
                    Message::replyMsg(response, conn);
                    if (req.testType == 2) {//测试带宽
                        currentTest = response.testId;
                        bandwidthTest(req.testTime);
                        currentTest = 0;
                    } else {
                        // nothing to do.
                    }
                    break;
                }
                case MessageType::rttTestMsg: {
                    conn.reply((char*)recvBuf, recvLen);
                    D_LOG("Reply rttTestMsg, msgId={}", msgId);
                    break;
                }
                default:
                    W_LOG("Got unknown msg, ignore it. msgType={}, msgId={}", msgType, msgId);
                    break;
            }
        } else {
            throw std::runtime_error("msg receive error.");
        }
    }


}

void Server::close() {
    I_LOG("Server finish.");
    conn.close();
}

/**
 * 测试带宽
 * 接收客户端发来的数据包，并进行统计
 * @param testSeconds 测试时长
 */
void Server::bandwidthTest(int testSeconds) {
    uint8_t recvBuf[SERVER_BUF_SIZE]{0};

    uint64_t totalRecvByte = 0;
    int64_t maxDelay = INT_MIN;
    int64_t minDelay = INT_MAX;
    // std::set<int> mayMissedTestNum;
    // int expectTestNum = 0;
    int64_t startTimeMs = -1;
    int64_t lastArrivalTimeMs = -1;

    int pktCount = 0;


    //循环接收客户端的测试包
    while (true) {
        T_LOG("bandwidthTest Waiting msg...");
        auto recvLen = conn.recvData((char*)recvBuf, SERVER_BUF_SIZE);

        if (startTimeMs > 0 &&
            seeker::Time::currentTime() - startTimeMs > testSeconds * 1000 + 5000) {
            W_LOG("Test timeout. testId={}", currentTest);
            break;
        }

        int64_t delay;
        if (recvLen > 0) {
            uint8_t msgType;
            int msgId;
            uint16_t testId;
            int testNum;
            int64_t ts = 0;
            Message::getMsgType(recvBuf, msgType);
            Message::getMsgId(recvBuf, msgId);
            Message::getTestId(recvBuf, testId);
            Message::getTimestamp(recvBuf, ts);
            BandwidthTestMsg::getTestNum(recvBuf, testNum);
            //1. 普通的数据测试包
            if (msgType == (uint8_t)MessageType::bandwidthTestMsg && testId == currentTest) {
                T_LOG("receive a BandwidthTestMsg, testNum={}", testNum);
                lastArrivalTimeMs = seeker::Time::currentTime();
                if (startTimeMs < 0) {
                    startTimeMs = lastArrivalTimeMs;//接收到第一个数据包的时间
                }
                totalRecvByte += recvLen;
                delay = seeker::Time::microTime() - ts;
                if (delay < minDelay) minDelay = delay;
                if (delay > maxDelay) maxDelay = delay;

                pktCount += 1;

                if (testNum % 100 == 0) {
                    T_LOG("BandwidthTestMsg testNum={} pktCount={}", testNum, pktCount);
                }


            } else if (msgType == (uint8_t)MessageType::bandwidthFinish && testId == currentTest) {

                //2. 测试终止包
                auto passedTimeInMs = lastArrivalTimeMs - startTimeMs;
                auto jitter = maxDelay - minDelay;
                int totalPkt;
                BandwidthFinish::getTotalPkt(recvBuf, totalPkt);
                int lossPkt = totalPkt - pktCount;
                I_LOG("bandwidth test report:");
                I_LOG("[ ID] Interval    Transfer    Bandwidth      Jitter   totlaReceivePkt");
                I_LOG("[{}]   {}s   {}     {}     {}ms   {}", testId, (double)passedTimeInMs / 1000,
                      formatTransfer(totalRecvByte),
                      formatBandwidth(totalRecvByte * 1000 / passedTimeInMs), (double)jitter / 1000,
                      pktCount);


                BandwidthReport report(jitter, pktCount, totalRecvByte, testId, Message::genMid());
                Message::replyMsg(report, conn);

                break;

            } else {
                // ignore. nothing to od.
                W_LOG("Got a unexpected msg. msgId={} msgType={} testId={} currentTest={}", msgId,
                      msgType, testId, currentTest);
            }
        } else {
            throw std::runtime_error("msg receive error.");
        }
    }
    I_LOG("bandwidthTest finished.");
}


//启动服务端
void startServer(int port) {
    I_LOG("Starting server on port={}", port);
    Server server{port};
    server.start();
    server.close();
}