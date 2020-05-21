# iNet

- [x] 带宽 Bandwidth
- [x] 往返时延 rtt
- [x] 抖动 Jitter
- [x] 丢包率


## 使用方法

### server 端

```shell script
# 启动
./iNet -s -p 9999
```

### client 端

```shell script
./iNet -c 127.0.0.1 -p 9999 -i 2 -t 10 -b 1M 
./iNet -c 127.0.0.1 -p 9999 -r # rtt test 默认测试10次，数据包大小为64B
```

```shell
-c：客户端模式，后接服务器ip
-p：后接服务端监听的端口
-i：设置带宽报告的时间间隔，单位为秒
-t：设置测试的时长，单位为秒
-b：设置udp的发送带宽，单位bit/s
```

在UDP模式下，客户端以100Mbps为数据发送速率，测试客户端到服务器上的带宽。




## 项目文件说明

* 客户端与服务器端连接 UdpConnection.h
* 客户端发送数据包工具类 Message.h
* socket 多平台兼容创建和管理 SocketUtil.h


## 主要流程

### server 端

- 绑定本地端口，建立socket
- 接收rtt测试消息
    - 接收客户端传过来的数据包，没接收一个，返回当前数据包的rtt时延
- 接收bandwidth测试消息
    - 接收客户端传过来的数据包，直至bandwidthtestfinish 消息结束，进行统计，并给客户端回复报告
    - 设置了超时时间5s
    

### client 端

- 创建sockets
- 发送rtt测试请求，发送rtt测试包
- 发送bandwidth测试请求，发送bandwidth测试包
    - 测试报告的打印间隔
- 接收服务器端返回的统计信息


## 其他学习记录

### client如何控制速率发送一段时间的数据包的？

例如客户端按照10MB/s 的速率 发送数据包，持续10s中，实际的操作流程如下：

* 每个数据包的大小定为1400B
* 每秒钟需要发送10MB大小，则每秒钟需要发送的数据包个数为  packetsPerSecond = 10*1024*1024/1400
* 则每个包发送的时间间隔为packetsIntervalMs =  1000/ packetsPerSecond

有一种很容易想到的方案是，发一个包，就让线程休息 packetsIntervalMs 这么长时间，看起来可行，实际上如果发送速率过大，
packetsIntervalMs的值就会非常小，再加上代码本身执行需要的时间，导致的误差就会很大。

另一种方案是，按组进行发送，什么意思呢，第一种方案的问题在于两个包间隔时间太短了，但是我们可以人为设置两个组之间的发送间隔，比如为5ms
那一个组中需要包含多少个数据包呢？ 计算如下
packestPerGroup = std::ceil((double)packetsPerSecond * 5 / 1000)

----

为什么是这么计算的呢？可以这样推演出来 :
新的间隔数(=packetsPerSecond/packestPerGroup - 1) * 5 = 1000
即packetsPerSecond = packetsPerSecond*(1000 / 5 + 1)  忽略掉这个1，即上面的结果


---

每发送一个组的数据包后，程序就要休息5ms，但考虑到实际上代码执行的时间，休息的时间并不是这样固定的，而是一种灵活的方式：

发送当前组数据包后本来应该花费的时间 (=发送的数据包数目 * 数据包的时间间隔) - 实际上花费的时间

即通过这样的方式按组进行发送，并且每发一组后休息一段时间，能保证1s中发的包的数据是根据速率得来的数目。

### 一些统计量的计算

* 往返时延 = 客户端（time1） -> 服务器端 ->（time1） 客户端,此时的当前时间 - time1 即为该数据包的往返时间
* 带宽 = 服务器端收到的数据包的总大小 / 总时间（= 服务器端收到的第一个包开始知道收到finish包结束）
* 抖动 = 服务器端统计收到的每个包的到达服务器端所需的时间（当前时间-包携带客户端发送的时间），最后将最大值减去最小值即为平均抖动时间
* 丢包率 = 服务端收到的数据包数据/客户端发送的数据包数目（在一开始发送请求测试的请求包中包含）

## 学习文章

* [NetTester](http://gitea.seekloud.org:50081/Tao/NetTester)
* [Linux编程之UDP SOCKET全攻略](https://www.cnblogs.com/skyfsm/p/6287787.html)
* [UDP编程](https://www.liaoxuefeng.com/wiki/1016959663602400/1017790181885952)
* [网络性能测试工具iperf详细使用图文教程【转载】](https://www.cnblogs.com/yingsong/p/5682080.html)
* [网络性能测试工具Iperf介绍](https://www.sdnlab.com/2961.html)