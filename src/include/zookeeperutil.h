#ifndef ZOOKEEPERUTIL_H
#define ZOOKEEPERUTIL_H

#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include <string>

// 封装的zk客户端
class ZkClient
{
public:
    ZkClient();
    ~ZkClient();

    // zkclient 启动连接 zksever
    void Start();

    // 在 zkserver 中创建一个节点，根据指定的path
    void Create(const char* path, const char* data, int datalen, int state = 0);

    // 根据参数指定的znode节点路径，或者znode节点
    std::string GetData(const char* path);

private:
    zhandle_t* m_zhandle; // zk的客户端句柄
};


#endif