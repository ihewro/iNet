/**
  文件注释样例，参考： http://www.edparrish.net/common/cppdoc.html
  socket连接流程封装

  @project netTester
  @author Tao Zhang, Tao, Tao
  @since 2020/4/26
  @version 0.1.3 2020/4/30
*/
#pragma once
#include <string>
#include "seeker/socketUtil.h"


using std::string;


class UdpConnection {
  int localPort{0};
  int remotePort{0};

  string localIp{""};
  string remoteIp{""};

  bool inited{false};
  SOCKET sock{0};


  sockaddr_in localAddr{};

 public:
  sockaddr_in remoteAddr{};
  sockaddr_in lastAddr{};
  socklen_t lastAddrLen = sizeof(lastAddr);

  UdpConnection();
  void updateRemoteAddr(const sockaddr_in& addr);
  void updateRemoteAddr();

  UdpConnection& setLocalIp(const string& ip);
  UdpConnection& setLocalPort(int port);

  UdpConnection& setRemoteIp(const string& ip);
  UdpConnection& setRemotePort(int port);

  UdpConnection& init();
  void close();

  SOCKET getSocket();

  void sendData(char* buf, size_t len);

  void reply(char* buf, size_t len);

  int recvData(char* buf, size_t len);
};