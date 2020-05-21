/**
  * User: hewro
  * Date: 2020/5/21
  * Time: 17:19
  * Description: socket-udp Á¬½Ó
  */

#ifndef INET_UDPCONNECTION_H
#define INET_UDPCONNECTION_H

#include <string>
#include "seeker/socketUtil.h"

class UdpConnection {

    void sendData(char* buf, size_t len);

    void reply(char* buf, size_t len);

    int recvData(char* buf, size_t len);
};


#endif //INET_UDPCONNECTION_H
