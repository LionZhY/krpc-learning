#include <iostream>
#include <string>

#include "../user.pb.h"
#include "krpcApplication.h"
#include "krpcProvider.h"


// 基于 rpc 框架实现一个简单的用户登录注册RPC服务

// service 提供两个本地方法 ： Login 和 Register，通过 RPC 可以被远程调用

class UserService : public kuser::UserServiceRpc // 继承自user.proto生成的RPC 服务基类，这里具体实现Login和Register
{
public:
    // 本地登录方法，用于处理实际的业务逻辑
    bool Login(std::string name, std::string pwd)
    {
        std::cout << "Doing local service: Login" << std::endl;
        std::cout << "name: " << name << "pwd: " << pwd << std::endl;
        return true; // 模拟登录成功
    }

    // 本地注册方法
    bool Register(uint32_t id, std::string name, std::string pwd)
    {
        std::cout << "Doing local service: Register" << std::endl;
        std::cout << "id: " << id << "name: " << name << "pwd: " << pwd << std::endl;
        return true; // 模拟注册成功
    }


    /*
        重写基类 UserServiceRpc的虚函数 Login()  Register() ，下面这些方法都是框架直接调用的
        1. caller  ==>  RPC 框架  ==>  调用 Login(LoginRequest) 
        2. callee  ==>  接收调用请求 Login(LoginRequest)  ==> 调用下面重写的Login方法
    */

    // callee 重写 UserServiceRpc 的Login函数
    void Login( ::google::protobuf::RpcController* controller,
                const ::kuser::LoginRequest* request,
                ::kuser::LoginResponse* response,
                ::google::protobuf::Closure* done)
    {
        // 1. 从请求中获取参数：用户名和密码
        std::string username = request->name();
        std::string userpwd = request->pwd();

        // 2. 调用本地业务逻辑处理
        bool login_result = Login(username, userpwd);

        // 3. 响应结果写入 response 对象
        kuser::ResultCode* code = response->mutable_result(); // 获取resultcode字段可写指针
        code->set_errcode(0); // TODO 错误码设为0，表示成功，为什么直接设置0了？不用判断吗？
        code->set_errmsg(""); // 错误信息为空
        response->set_success(login_result); // 设置登录结果

        // 4. 执行回调操作，执行响应对象数据的序列化和网络发送 (都是由框架来完成的)
        done->Run();
    }

    
    // callee 重写 UserServiceRpc 的 Register 函数
    void Register(::google::protobuf::RpcController* controller,
                  const ::kuser::RegisterRequst* request,
                  ::kuser::RegisterResponse* response,
                  ::google::protobuf::Closure* done)
    {
        // 1. 从请求中获取参数
        uint32_t userid = request->id();
        std::string username = request->name();
        std::string userpwd = request->pwd();

        // 2. 调用本地业务处理注册
        bool register_result = Register(userid, username, userpwd);

        // 3. 响应结果放进 response 对象
        kuser::ResultCode* code = response->mutable_result();
        code->set_errcode(0); // TODO 为什么直接设置0了？不用判断吗？
        code->set_errmsg(""); 
        response->set_success(register_result);

        // 4. 执行回调操作，执行响应对象数据的序列化和网络发送 (都是由框架来完成的)
        done->Run();
    }

};



int main(int argc, char** argv)
{
    // 调用框架的初始操作，解析命令行参数并加载配置文件
    KrpcApplication::Init(argc, argv);

    // 创建一个 RPC 服务提供者对象
    krpcProvider provider;

    // 将 UserService 对象发布到 RPC 节点上，使其可以被远程调用
    provider.NotifyService(new UserService());

    // 启动 RPC 服务节点，进入阻塞状态，等待远程的 RPC 调用请求
    provider.run();

    return 0;

}
