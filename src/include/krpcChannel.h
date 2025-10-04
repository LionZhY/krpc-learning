#pragma once

#include <google/protobuf/service.h>
#include "zookeeperutil.h"


// 客户端调用远程服务时，stub（代理类）会将请求传给 rpcChannel 的 CallMehod()，由其进行实际的发送
// 因此你只要实现 CallMethod()，就能实现完整的RPC客户端调用

class KrpcChannel : public google::protobuf::RpcChannel // 继承google::protobuf::RpcChannel，是 Protobuf 的远程调用通道接口
{
public:
    KrpcChannel(bool connectNow);
    virtual ~KrpcChannel() { }

    /*
    * 核心方法：CallMethod()，实现 Protobuf 框架定义的虚函数 CallMethod()
    * stub 代理类在调用远程方法时，最终都会调用到此函数，统一做 rpc 方法调用的数据序列化和网络发送。
    * 它负责：
    * 1. 从 ZooKeeper 查询服务地址
    * 2. 建立 Socket 连接
    * 3. 将请求序列化，并发送给服务端
    * 4. 接收响应并反序列化，并返回结果
    */

    // RPC 调用的核心方法，负责将客户端的请求序列化并发送到服务端，同时接收服务端的响应
    void CallMethod(const ::google::protobuf::MethodDescriptor* method, // 调用哪个服务、哪个方法
                    ::google::protobuf::RpcController* controller, // 上下文控制器
                    const ::google::protobuf::Message* request,    // 请求参数
                    ::google::protobuf::Message* response,         // 请求响应
                    ::google::protobuf::Closure* done) override;   // 回调


private:
    int m_clientfd;             // 当前客户端sockfd

    std::string service_name;   // 当前调用的RPC服务名称，如："UserService"
    std::string method_name;    // 当前调用的服务方法名称，如："Login" "Register"
    
    std::string m_ip;           // 从zookeeper获取的服务端 ip
    uint16_t m_port;            // 从zookeeper获取的服务端 port
    
    int m_idx; // // 字符串中':'分隔符的位置，划分服务器ip和port的下标

    // 创建新的socket连接
    bool newConnect(const char* ip, uint16_t port);

    // 从ZooKeeper查询指定服务方法的服务端地址 ip:port
    std::string QueryServiceHost(ZkClient* zkclient, std::string service_name, std::string method_name, int& idx);
};

