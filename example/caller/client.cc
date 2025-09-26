#include "krpcApplication.h"
#include "../user.pb.h"
#include "krpcController.h"
#include "krpcLogger.h"

#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>


// 发送 RPC 请求，模拟客户端调用远程服务
void send_request(int thread_id, 
                  std::atomic<int>& success_count, 
                  std::atomic<int>& fail_count, 
                  int request_per_thread)
{
    // 创建一个 UserServiceRpc_Stub 对象，用于调用远程的 RPC 方法
    kuser::UserServiceRpc_Stub stub(new KrpcChannel(false));

    // 设置 RPC 方法 (Login) 的请求参数 
    kuser::LoginRequest request;
    request.set_name("zhangsan");
    request.set_pwd("123456");

    // 定义 RPC 方法的响应
    kuser::LoginResponse response;
    KrpcController controller; // 创建控制器对象，用于处理 RPC 调用过程中的错误
    for (int i = 0; i < request_per_thread; ++i)
    {
        // 调用远程方法 Login
        stub.Login(&controller, &request, &response, nullptr);

        // 检查 RPC 是否调用成功
        if (controller.Failed())
        {
            std::cout << controller.ErrorText() << std::endl;
            fail_count++; // 失败计数 + 1
        }
        else
        {
            if (int{} == response.result().errcode()) // 检查响应中的错误码
            {
                std::cout << "rpc login response success: " << response.success() << std::endl; // 打印成功信息
                success_count++; // 成功计数 + 1
            }
            else // 如果响应中有错误
            {
                std::cout << "rpc login response error: " << response.result().errmsg() << std::endl; // 打印错误信息
                fail_count++; // 失败计数 + 1
            }
        }
    }

    
}


int main(int argc, char** argv)
{
    // 初始化 RPC 框架，解析命令行参数并加载配置文件
    KrpcApplication::Init(argc, argv);

    // 创建日志对象
    KrpcLogger logger("MyRpc");

    const int thread_count = 5000; // 并发线程数
    const int request_per_thread = 1000; // 每个线程发送的请求数

    std::vector<std::thread> threads; // 存储线程对象
    std::atomic<int> success_count(0);// 成功请求的计数
    std::atomic<int> fail_count(0);   // 失败请求的计数

    auto start_time = std::chrono::high_resolution_clock::now(); // 记录测试开始时间

    // 启动多线程进行并发测试
    for (int i = 0; i < thread_count; i++)
    {
        threads.emplace_back([argc, argv, i, &success_count, &fail_count, request_per_thread]() 
            {
                send_request(i, success_count, fail_count, request_per_thread); // 每个线程发送指定数量的请求
            }
        );
        
    }

    // 等待所有线程执行完毕
    for (auto& t : threads) t.join();

    auto end_time = std::chrono::high_resolution_clock::now(); // 记录测试结束时间
    std::chrono::duration<double> elapsed_time = end_time - start_time; // 计算测试耗时

    
    // 输出统计结果
    LOG(INFO) << "Total requests: " << thread_count * request_per_thread; // 总请求数
    LOG(INFO) << "Success count: "  << success_count; // 成功请求数
    LOG(INFO) << "Fail count: "     << fail_count;    // 失败请求数
    LOG(INFO) << "Elapsed time: "   << elapsed_time.count() << "seconds";  // 测试耗时
    LOG(INFO) << "QPS: " << (thread_count * request_per_thread) / elapsed_time.count(); // 计算 QPS （每秒请求数）

    
    return 0;
}