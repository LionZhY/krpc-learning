#pragma once

#include <semaphore.h> // 提供信号量相关接口
#include <zookeeper/zookeeper.h>
#include <string>

/*
zkclient 是 Zookeeper 客户端的封装类，负责：
    - 启动并连接Zookeeper服务
    - 创建Zookeeper的节点 znode
    - 读取节点数据
*/


class ZkClient
{
public:
    ZkClient();
    ~ZkClient();

    // zkclient 启动并连接 Zookeeper 服务器 zkserver
    void Start();

    // 在 zkserver 中根据指定的 path 创建 znode 节点
    void Create(const char* path,   // 节点路径，如 "/service/node1"
                const char* data,   // 节点保存的数据
                int datalen,        // 数据长度
                int state = 0);     // 节点类型，默认是0（持久节点），也可指定临时节点

    // 获取指定路径节点的数据
    std::string GetData(const char* path);

private:
    zhandle_t* m_zhandle; // zk 的客户端会话句柄，ZkClient 用 zhandle_t* 管理 ZooKeeper 会话
};
