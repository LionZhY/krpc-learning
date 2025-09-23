#pragma once

#include "krpcConfig.h"
#include "krpcChannel.h"
#include "krpcController.h"

#include <mutex>

// rpc基础类，负责框架的一些初始化操作

class krpcApplication
{
public:
    static void Init(int args, char** argv);
    static krpcApplication& GetInstance();
    static void deleteInstance();
    static krpcConfig& GetConfig();

private:
    static krpcConfig m_config;
    static krpcApplication* m_application; // 全局唯一单例访问对象
    static std::mutex m_mutex;

    krpcApplication(){}
    ~krpcApplication(){}

    krpcApplication(const krpcApplication&) = delete;
    krpcApplication(krpcApplication&&) = delete;
};