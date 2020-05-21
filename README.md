# iNet

- [ ] 带宽 Bandwidth
- [ ] 抖动
- [ ] 丢包率
- [ ] 往返时延 rtt


## 使用方法

### server 端

```shell script
# 启动
./iNet -s -p 9999
```

### client 端

```shell script
./iNet -c 127.0.0.1 -p 9999 -i 2 -t 10 -b 1M 
./iNet -c 127.0.0.1 -p 9999 -i 2 -t 10 -b 1M  -r # rtt test
```



## 代码逻辑

* 客户端与服务器端连接
* 客户端发送数据包
* 服务器端进行统计，发送统计信息
* rtt计算方法
