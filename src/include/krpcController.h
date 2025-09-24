#pragma once

#include <google/protobuf/service.h>
#include <string>


// RpcController 是用于传递调用状态信息的类，属于客户端和服务端通用接口
class KrpcController : public google::protobuf::RpcController
{
public:
    KrpcController();

    // 重置控制器状态
    void Reaset();

    // 判断是否发生错误
    bool Failed() const;

    std::string ErrorText() const;

    void SetFailed(const std::string& reason);

    // TODO 目前未实现的功能 
    void StartChannel();     // 开始取消RPC调用
    bool IsCanceled() const; // 判断RPC调用是否被取消
    void NotifyOnCancel(google::protobuf::Closure* callback); // 注册取消回调函数


private:
    bool m_failed;          // rpc方法执行过程中的状态，是否失败
    std::string m_errText;  // rpc方法执行过程中的错误信息
};
