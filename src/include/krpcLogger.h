#ifndef KRPC_LOG_H
#define KRPC_LOG_H

#include <glog/logging.h> // Google提供的开源日志库glog的核心接口
#include <string>

// 采用RAII思想
class krpcLogger
{
public:
    explicit krpcLogger(const char* argv0) // 避免隐式转换，确保只能用 krpcLogger logger(argv[0]); 的形式
    {
        google::InitGoogleLogging(argv0);   // 初始化glog
        fLB::FLAGS_colorlogtostderr = true; // 启用彩色日志
        fLB::FLAGS_logtostderr = true;      // 默认输出到标准错误流，而不是写入文件
    }
    // 这里体现 RAII 思想：在对象构造时自动完成资源初始化

    ~krpcLogger()
    {
        google::ShutdownGoogleLogging(); // 调用 glog 的清理方法，释放日志系统占用的资源
    }


    // 提供静态日志方法 （日志是全局服务，使用静态函数可以避免频繁创建对象，如果方法是静态的，直接调用即可）
    static void Info(const std::string& message)    // 普通信息 
    {
        LOG(INFO)<<message;
    }

    static void Warning(const std::string& message) // 警告信息 
    {
        LOG(WARNING)<<message;
    }

    static void ERROR(const std::string& message)   // 错误信息，但程序还能运行
    {
        LOG(ERROR)<<message;
    }
    
    static void Fatal(const std::string& message)   // 致命错误，输出日志后程序会终止
    {
        LOG(FATAL)<<message;
    }


private:
    // 禁用拷贝 防止 KrpcLogger 被错误地复制（确保日志系统只有一个有效实例（单例思路的一种实现））
    krpcLogger(const krpcLogger&) = delete;
    krpcLogger& operator = (const krpcLogger&) = delete;
};


#endif