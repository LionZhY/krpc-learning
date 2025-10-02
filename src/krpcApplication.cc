#include "krpcApplication.h"
#include<cstdlib>
#include<unistd.h>

/* 静态成员变量 类内声明 类外初始化 */
KrpcConfig KrpcApplication::m_config; // 全局配置对象
std::mutex KrpcApplication::m_mutex;  // 用于线程安全的互斥锁

KrpcApplication* KrpcApplication::m_application = nullptr;  // 单例对象指针，初始为空


// 初始化：框架启动时调用，解析命令行参数获得配置文件路径 config_file，并加载配置文件
void KrpcApplication::Init(int argc, char** argv)
{
    if (argc < 2) // 如果命令行参数个数少于2个，说明没有指定配置文件，退出程序
    {
        std::cout << "格式: command -i <配置文件路径>" << std::endl;
        exit(EXIT_FAILURE);
    }

    
    int o; // 保存 getopt 的返回值
    std::string config_file; // 存放解析出来的配置文件路径    

    // 使用 getopt 解析命令行参数，获取 -i <配置文件路径>，optarg 会自动指向 -i 后面跟的参数值
    while (-1 != (o = getopt(argc, argv, "i:"))) 
    {
        switch (o) {
            case 'i': // 如果参数是 -i，后面的值就是配置文件的路径，将配置文件路径保存到 config_file
                config_file = optarg; 
                break;
            case '?': // 如果出现未知参数（不是-i），提示正确格式并退出
                std::cout << "格式: command -i <配置文件路径>" << std::endl;
                exit(EXIT_FAILURE);
                break;
            case ':': // 如果-i后面没有跟参数，提示正确格式并退出
                std::cout << "格式: command -i <配置文件路径>" << std::endl;
                exit(EXIT_FAILURE);
                break;
            default:
                break;
        }
    }

    // 调用全局配置对象 m_config 的 LoadConfigFile 方法，真正去解析并加载配置文件内容
    m_config.LoadConfigFile(config_file.c_str());
}


// 获取单例对象的引用，保证全局只有一个实例
KrpcApplication& KrpcApplication::GetInstance()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_application == nullptr) // 如果单例对象还未创建
    {
        m_application = new KrpcApplication(); // 创建单例对象
        atexit(deleteInstance); // 注册atexit函数，程序退出时自动销毁单例对象
        
        /* 注册在正常程序退出（调用 exit() 或 main 返回）时由运行时调用的函数 deleteInstance，用于销毁单例 */
    }

    return *m_application; // 返回单例对象的引用
}


// 销毁单例对象，程序退出时自动调用
void KrpcApplication::deleteInstance()
{
    if (m_application)  delete m_application; // 如果单例对象还存在，销毁
}


// 获取全局配置对象的引用
KrpcConfig& KrpcApplication::GetConfig()
{
    return m_config;
}
