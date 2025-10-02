#pragma once

#include "krpcConfig.h"
#include "krpcChannel.h"
#include "krpcController.h"

#include <mutex>

// rpc基础类，负责框架初始化、加载配置，并提供KrpcConfig 访问

class KrpcApplication
{
public:
    // 初始化函数，用于解析命令行参数并加载配置文件
    static void Init(int argc, char** argv);

    // 获取单例对象的引用，保证全局只有一个实例
    static KrpcApplication& GetInstance();
    
    // 销毁单例对象，程序退出时自动调用
    static void deleteInstance();
    
    // 获取全局配置对象的应用
    static KrpcConfig& GetConfig();

private:
    /* 静态成员变量 类内声明 类外初始化 */
    static KrpcConfig m_config; // 全局配置对象，用于保存整个 RPC 框架的配置状态
    static KrpcApplication* m_application; // 单例指针，指向全局唯一单例KrpcApplication对象
    static std::mutex m_mutex;

    // 构造和析构都私有化
    KrpcApplication(){}
    ~KrpcApplication(){}

    // 拷贝构造禁用
    KrpcApplication(const KrpcApplication&) = delete;
    KrpcApplication(KrpcApplication&&) = delete;
};