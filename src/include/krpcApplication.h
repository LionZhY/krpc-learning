#pragma once

#include "krpcConfig.h"
#include "krpcChannel.h"
#include "krpcController.h"

#include <mutex>

// rpc基础类，负责框架的一些初始化操作

class KrpcApplication
{
public:
    static void Init(int args, char** argv);
    static KrpcApplication& GetInstance();
    static void deleteInstance();
    static KrpcConfig& GetConfig();

private:
    static KrpcConfig m_config;
    static KrpcApplication* m_application; // 全局唯一单例访问对象
    static std::mutex m_mutex;

    KrpcApplication(){}
    ~KrpcApplication(){}

    KrpcApplication(const KrpcApplication&) = delete;
    KrpcApplication(KrpcApplication&&) = delete;
};