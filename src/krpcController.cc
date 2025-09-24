#include "krpcController.h"

KrpcController::KrpcController()
{
    m_failed = false; // 初始状态为未失败
    m_errText = "";   // 初始错误信息为空
}   

// 重置控制器状态，失败标志和错误信息清空
void KrpcController::Reaset()
{
    m_failed = false;
    m_errText = "";
}

// 判断RPC调用是否失败
bool KrpcController::Failed() const
{
    return m_failed; // 返回是否失败标志
}

// 获取错误信息
std::string KrpcController::ErrorText() const
{
    return m_errText; 
}

// 设置RPC调用失败，并记录失败原因
void KrpcController::SetFailed(const std::string &reason)
{
    m_failed = true;
    m_errText = reason;
}



// TODO 目前未实现的功能 
void KrpcController::StartChannel() {}     // 开始取消RPC调用
bool KrpcController::IsCanceled() const { return false; } // 判断RPC调用是否被取消
void KrpcController::NotifyOnCancel(google::protobuf::Closure* callback) {} // 注册取消回调函数