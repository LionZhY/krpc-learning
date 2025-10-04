#include <errno.h>      // 提供错误码errno定义
#include <unistd.h>     // 提供close() read()  write()等系统调用
#include <sys/socket.h> // socket接口
#include <sys/types.h>  // socket类型定义
#include <arpa/inet.h>  // ip 地址与网络字节序的转换函数
#include <memory>

#include "krpcChannel.h"
#include "krpcHeader.pb.h"
#include "zookeeperutil.h"
#include "krpcApplication.h"
#include "krpcController.h"
#include "krpcLogger.h"

// 全局互斥锁
std::mutex g_data_mutx;


// 构造，支持延迟连接
KrpcChannel::KrpcChannel(bool connectNow) : m_clientfd(-1), m_idx(0)
{
    // connectNow - 决定是否在创建对象时，立即连接服务器

    if (!connectNow)    return; // connectNow为false时，延迟连接，连接将在首次调用RPC时再建立

    auto rt = newConnect(m_ip.c_str(), m_port); // connectNow = true 尝试连接服务器，最多重试3次
    int count = 3;
    while (!rt && count--)  rt = newConnect(m_ip.c_str(), m_port);
}



// RPC 调用的核心方法，负责将客户端的请求序列化并发送到服务端，同时接收服务端的响应
void KrpcChannel::CallMethod(const ::google::protobuf::MethodDescriptor* method, // 调用哪个方法
                ::google::protobuf::RpcController* controller, // 上下文控制器
                const ::google::protobuf::Message* request,    // 请求参数
                ::google::protobuf::Message* response,         // 请求响应
                ::google::protobuf::Closure* done)             // 回调
{
    // 检查客户端socket是否建立，如果客户端Socket未初始化
    if (-1 == m_clientfd)
    {
        // 获取服务对象名和方法名
        const google::protobuf::ServiceDescriptor* sd = method->service();
        service_name = sd->name();
        method_name = method->name();

        // 查询zookeeper，找到提供该服务的服务端ip:port
        ZkClient zkCli;
        zkCli.Start(); // TODO 建立与zk集群的连接？？？ 不太理解？？这里连接的是什么？
        std::string host_data = QueryServiceHost(&zkCli, service_name, method_name, m_idx); // 查询服务地址
        m_ip = host_data.substr(0, m_idx); // 提取 ip 
        std::cout << "ip: " << m_ip << std::endl;
        m_port = atoi(host_data.substr(m_idx + 1, host_data.size() - m_idx).c_str()); // 提取 port 
        std::cout << "port: " << m_port << std::endl;

        // 尝试连接服务器，返回的是client的sockfd
        auto rt = newConnect(m_ip.c_str(), m_port);
        if (!rt)
        {
            LOG(ERROR) << "connect server error"; // 连接失败，记录错误日志
            return;
        }
        else 
        {
            LOG(INFO) << "connect server success"; // 连接成功，记录日志
        }
    }

    // 将请求参数 request 序列化为字符串，并计算其长度
    uint32_t args_size{};
    std::string args_str;
    if (request->SerializeToString(&args_str))  // 序列化请求参数
    {
        args_size = args_str.size(); // 获取序列化后的长度
    }
    else 
    {
        controller->SetFailed("serialize request fail"); // 序列化失败，设置错误信息
    }


    // 定义RPC请求的头部消息 header: 服务名 + 方法名 + 参数长度
    krpc::rpcHeader krpcheader;
    krpcheader.set_service_name(service_name);
    krpcheader.set_method_name(method_name);
    krpcheader.set_args_size(args_size);


    // RPC头部消息序列化为字符串，并计算其长度
    uint32_t header_size = 0;
    std::string rpc_header_str;
    if (krpcheader.SerializeToString(&rpc_header_str)) 
    {
        header_size = rpc_header_str.size();
    }
    else {
        controller->SetFailed("serialize rpc header error!");
        return;
    }

    // 拼接完整的RPC请求报文：send_rpc_str = [header_size][rpc_header_str][args_str]
    std::string send_rpc_str;
    {
        google::protobuf::io::StringOutputStream string_output(&send_rpc_str);
        google::protobuf::io::CodedOutputStream coded_output(&string_output);
        coded_output.WriteVarint32(static_cast<uint32_t>(header_size)); // 写入头部长度
        coded_output.WriteString(rpc_header_str); // 写入头部信息
    }
    send_rpc_str += args_str; // 拼接请求参数


    // 发送RPC请求 send_rpc_str 到服务器
    if (-1 == send(m_clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0)) {
        close(m_clientfd); // 发送失败，关闭Socket
        char errtxt[512] = {};
        std::cout << "send error: " << strerror_r(errno, errtxt, sizeof(errtxt)) << std::endl; // 打印错误信息
        controller->SetFailed(errtxt); // 设置错误信息
        return;
    }

    // 发送成功，接收服务器的响应
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if (-1 == (recv_size = recv(m_clientfd, recv_buf, 1024, 0)))
    {
        char errtxt[512] = {};
        std::cout << "recv error" << strerror_r(errno, errtxt, sizeof(errtxt)) << std::endl; // 打印错误信息
        controller->SetFailed(errtxt); // 设置错误信息
        return;
    }

    // 将接收到的响应数据，反序列化为response对象
    if (!response->ParseFromArray(recv_buf, recv_size)) {
        close(m_clientfd); // 反序列化失败，关闭Socket
        char errtxt[512] = {};
        std::cout << "parse error" << strerror_r(errno, errtxt, sizeof(errtxt)) << std::endl;
        controller->SetFailed(errtxt);
        return;
    }


    // close(m_clientfd); // 关闭Socket连接
}




// 创建新的socket连接 client <---> server(ip:port)
bool KrpcChannel::newConnect(const char *ip, uint16_t port)  // 输入服务端的 ip port
{
    // 1.创建新的 Socket（客户端在本地创建的socketfd）
    int clientfd = socket(AF_INET, SOCK_STREAM, 0); // IPv4 TCP

    if (-1 == clientfd) { // 创建失败
        char errtxt[512] = {0}; // 存储错误信息
        std::cout << "socket error" << strerror_r(errno, errtxt, sizeof(errtxt)) << std::endl;
        /* strerror 把error对应的错误信息写入errtxt*/
        LOG(ERROR) << "socket error: " << errtxt; // 记录错误日志
        return false;
    }

    // 2.设置服务器地址信息: ip-port填充 sockaddr_in 结构
    struct sockaddr_in  server_addr;
    server_addr.sin_family = AF_INET;            // IPv4 地址族
    server_addr.sin_port = htons(port);          // 端口号（主机字节序 -> 网络字节序big-endian）
    server_addr.sin_addr.s_addr = inet_addr(ip); // ip地址（点分十进制字符串 -> 网络序的32位整型）

    // 3.客户端调用connect，连接服务器监听的socket（阻塞式connect，会触发TCP三次握手）
    if (-1 == connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr))) 
    {
        // connect失败：关闭刚创建的socket，并打印记录错误信息
        close(clientfd);
        char errtxt[512] = {0};
        std::cout << "connect error" << strerror_r(errno, errtxt, sizeof(errtxt)) << std::endl;  
        LOG(ERROR) << "connect server error" << errtxt;  
        return false;
    }

    // connect 成功：保存socketfd，后续用 m_clientfd 进行 send/recv
    m_clientfd = clientfd; 
    return true;
}



// 从ZooKeeper查询指定服务方法的服务端地址 ip:port
std::string KrpcChannel::QueryServiceHost(ZkClient* zkclient, std::string service_name, std::string method_name, int& idx)
{
    /* 根据 service_name 和 method_name 在zookeeper中查找登记的服务节点，返回服务端的 ip:port，并把":" 位置写入idx */

    // 构造 method_path (每个服务方法在zk中对应一个节点，节点路径按照 "/ServiceName/MethodName" 格式组织)
    std::string method_path = "/" + service_name + "/" + method_name; 
    std::cout << "method_path: " << method_path << std::endl;

    // 从相应zknode获取数据，即服务端的 ip:port，存入 host_data_1
    std::unique_lock<std::mutex> lock(g_data_mutx);
    std::string host_data_1 = zkclient->GetData(method_path.c_str()); // 取出method_path路径上的节点数据（即服务端的ip:port）
    lock.unlock();

    if (host_data_1 == "") // 返回空字符串，未找到服务器地址
    {
        LOG(ERROR) << method_path + " is not exist!"; // 记录错误日志
        return " ";
    }

    idx = host_data_1.find(":"); // 查找第一次出现":"的位置，返回索引赋给idx
    if (idx == -1) // 分隔符不存在，不合法地址
    {
        LOG(ERROR) << method_path + " address is invalid!"; 
        return " ";
    }

    // 返回服务器地址 ip:port
    return host_data_1; 
}

