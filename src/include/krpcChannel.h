#pragma once

#include <google/protobuf/service.h>
#include "zookeeperutil.h"


// 客户端调用远程服务时，stub（代理类）会将请求传给 rpcChannel 的 CallMehod()，由其进行实际的发送
// 因此你只要实现 CallMethod()，就能实现完整的RPC客户端调用

class KrpcChannel : public google::protobuf::RpcChannel // 继承google::protobuf::RpcChannel，是 Protobuf 的远程调用通道接口
{
public:
    KrpcChannel(bool connectNow);
    virtual ~KrpcChannel()
    {

    }

    // 重写RpcChannel中的CallMethod()

    // 所有通过 stub 代理对象调用的 rpc 方法，都会走到这里，统一做 rpc 方法调用的数据序列化和网络发送
    void CallMethod(const ::google::protobuf::MethodDescriptor* method, // 调用哪个服务、哪个方法
                    ::google::protobuf::RpcController* controller,      // 上下文控制器
                    const ::google::protobuf::Message* request,         // 请求参数
                    ::google::protobuf::Message* response,              // 请求响应
                    ::google::protobuf::Closure* done) override;        // 回调


private:
    int m_clientfd; // 存放客户端sockfd
    std::string service_name;
    std::string m_ip;
    uint16_t m_port;
    std::string method_name;

    int m_idx; // 划分服务器ip和port的下标
    bool newConnect(const char* ip, uint16_t port);

    std::string QueryServiceHost(ZkClient* zkclient, std::string service_name, std::string method_name, int& idx);
};

