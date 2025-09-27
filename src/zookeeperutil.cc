#include "zookeeperutil.h"
#include "krpcApplication.h"
#include "krpcLogger.h"

#include <mutex>
#include <condition_variable>

std::mutex cv_mutex;        // 全局锁，保护共享变量的线程安全
std::condition_variable cv; // 条件变量，用于线程间通信
bool is_connected = false;  // 标记 Zookeeper 客户端是否连接成功


// 全局的 watcher 观察，用于接收 Zookeeper 服务器的通知
void global_watcher(zhandle_t* zh, int type, int status, const char* path, void* watcherCtx)
{
    if (type == ZOO_SESSION_EVENT) // 回调消息类型和会话相关的事件
    {
        if (status == ZOO_CONNECTED_STATE) // zookeeper 客户端和服务器连接成功
        {
            std::lock_guard<std::mutex> lock(cv_mutex); // 加锁保护
            is_connected = true; // 标记连接成功
        }
    }
}


// 构造：初始化zookeeper客户端句柄为空
ZkClient::ZkClient() : m_zhandle(nullptr) {}

// 析构：关闭zookeeper连接
ZkClient::~ZkClient() 
{
    if (m_zhandle != nullptr)   zookeeper_close(m_zhandle);
}


// 启动 zookeeper 客户端，连接 zookeeper 服务器
void ZkClient::Start()
{
    // 从配置文件中读取zookeeper服务器的 ip 和 端口
    std::string host = KrpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = KrpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port; // 拼接连接字符串

    /*
        zookeeper_mt：多线程版本
        ZooKeeper的API客户端程序提供了三个线程：
            1. API调用线程
            2. 网络I/O线程（使用pthread_create和poll）
            3. watcher回调线程（使用pthread_create）
    */

    // 使用zookeeper_init初始化一个zookeeper客户端对象，异步建立与服务器的连接
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 6000, nullptr, nullptr, 0);
    if (m_zhandle == nullptr)   // 初始化失败
    {
        LOG(ERROR) << "zookeeper_init error";
        exit(EXIT_FAILURE); // 退出程序
    }


    // 等待连接成功
    std::unique_lock<std::mutex> lock(cv_mutex);
    cv.wait(lock, []{ return is_connected; }); // 阻塞等待，直到连接成功
    LOG(INFO) << "zookeeper_init success";
}



// 创建ZooKeeper节点
void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128]; // 用于存储创建的节点路径
    int bufferlen = sizeof(path_buffer);

    // 检查节点是否已存在
    int flag = 
}



// 获取ZooKeeper节点的数据
std::string ZkClient::GetData(const char *path)
{
    char buf[64]; // 存储节点数据
    int bufferlen = sizeof(buf);

    // 获取指定节点数据，直接调用 zookeeper 提供的zoo_get()
    int flag = zoo_get(m_zhandle, path, 0, buf, &bufferlen, nullptr);
    if (flag != ZOK) // 获取失败，返回空字符串
    {
        LOG(ERROR) << "zoo_get error";
        return "";
    }
    else // 获取成功，返回节点数据
    {
        return buf; 
    }

    return ""; // 默认返回空字符串
}

























