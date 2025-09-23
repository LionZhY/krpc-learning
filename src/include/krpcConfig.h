#ifndef KRPCCONFIG_H
#define KRPCCONFIG_H

#include <unordered_map>
#include <string>

class krpcConfig
{
public:
    void LoadConfigFile(const char* config_file); // 加载配置文件
    std::string Load(const std::string& key); // 查找key对应的value

private:
    std::unordered_map<std::string, std::string> config_map; // TODO 存什么？？？
    
    void Trim(std::string& read_buf); // 去掉字符串前后的空格
};

#endif