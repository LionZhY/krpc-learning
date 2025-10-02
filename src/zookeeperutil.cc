#include "zookeeperutil.h"
#include "krpcApplication.h"
#include "krpcLogger.h"

#include <mutex>
#include <condition_variable>


/* 
zookeeper_init 是异步的，真正连上 ZooKeeper 后，会通过 watcher 回调通知。
这里用 mutex + condition_variable 实现“阻塞等待连接”。
*/

std::mutex cv_mutex;        // 全局锁，保护共享变量的线程安全
std::condition_variable cv; // 条件变量，用于线程间通信
bool is_connected = false;  // 标记 Zookeeper 客户端是否连接成功


// 全局 watcher ，用于接收 Zookeeper 服务器的通知
void global_watcher(zhandle_t* zh,    // 当前会话句柄（哪个客户端会话触发的回调）
                    int type,         // 事件类型（如 ZOO_SESSION_EVENT, ZOO_CREATED_EVENT 等）
                    int status,       // 事件的状态
                    const char* path, // 触发事件关联的znode路径（节点事件时有意义，会话事件一般为nullptr）
                    void* watcherCtx) // 初始化时传入的上下文指针（未用到时为nullptr）
{
    // 只处理会话级事件（与连接/会话状态变化相关的事件），忽略其他类型的 watcher 事件（例如节点创建/删除等）
    if (type == ZOO_SESSION_EVENT) 
    {
        if (status == ZOO_CONNECTED_STATE) // zkclient 和 zkserver 连接成功，当会话状态变为 “已连接” 时执行后续动作
        {
            std::lock_guard<std::mutex> lock(cv_mutex);
            is_connected = true; // 标记连接成功
        }
    }

    cv.notify_all(); // 通知所有等待的线程，唤醒在 Start() 里等待的线程
}



// 构造：初始化zookeeper客户端句柄为空
ZkClient::ZkClient() : m_zhandle(nullptr) {}

// 析构：关闭zookeeper连接
ZkClient::~ZkClient() 
{
    if (m_zhandle != nullptr)   zookeeper_close(m_zhandle);
}


/*
    zookeeper_mt：多线程版本
    ZooKeeper的API客户端程序提供了三个线程：
        1. API调用线程
        2. 网络I/O线程（使用pthread_create和poll）
        3. watcher回调线程（使用pthread_create）
*/


// 启动 zookeeper 客户端，并连接 zookeeper 服务器
void ZkClient::Start()
{
    // 从配置文件中读取zookeeper服务器的 ip 和 端口，并拼接 "ip : port"
    std::string host = KrpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = KrpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port; // zkserver："ip : port"


    // 使用 zookeeper_init() 初始化一个 zookeeper 客户端对象，并连接服务器
    // 返回一个会话句柄，表示这次客户端与服务器的连接
    m_zhandle = zookeeper_init(connstr.c_str(),  // 要连接的服务器地址
                               global_watcher,   // 全局watcher回调
                               6000,             // 会话超时 ms
                               nullptr, 
                               nullptr, 
                               0);
    
    // 初始化失败 退出程序
    if (m_zhandle == nullptr) 
    {
        LOG(ERROR) << "zookeeper_init error";
        exit(EXIT_FAILURE); 
    }


    // 等待连接成功
    std::unique_lock<std::mutex> lock(cv_mutex);
    cv.wait(lock, []{ return is_connected; }); // 阻塞等待，直到 global_watcher 设置了 is_connected=true
    LOG(INFO) << "zookeeper_init success";

    /*  
        条件变量cv 阻塞等待，直到global_watcher收到连接成功事件，把is_connectrd设为true，并调用cv.notify_all()
        这里 cv.wait()被唤醒，继续执行
        这一步确保函数返回时，客户端与服务器已经建立好连接
    */
}



// 创建ZooKeeper节点 zoo_create()
void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128]; // 用于存储创建的节点路径
    int bufferlen = sizeof(path_buffer);

    // 检查节点是否已存在
    int flag = zoo_exists(m_zhandle, path, 0, nullptr);
    if (flag == ZNONODE) // 如果节点不存在，用 zoo_create 创建节点（支持持久 / 临时节点）
    {
        flag = zoo_create(m_zhandle, path, data, datalen, 
                          &ZOO_OPEN_ACL_UNSAFE, // 表示不设权限控制（任何客户端可读写）
                          state, path_buffer, bufferlen);
        if (flag == ZOK) // 创建成功
        {
            LOG(INFO) << "znode create success... path: " << path;
        }
        else // 创建失败
        {
            LOG(ERROR) << "znode create failed... path: " << path;
            exit(EXIT_FAILURE); 
        }
    }
}



// 获取ZooKeeper节点的数据 zoo_get()
std::string ZkClient::GetData(const char *path)
{
    char buf[64]; // 存储节点数据
    int bufferlen = sizeof(buf);

    // 直接调用 zoo_get 从指定路径的节点取数据
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
