#include "krpcConfig.h"
#include <memory>

#include "krpcConfig.h"
#include <memory>

// 加载配置文件，解析配置文件中的键值对，存入config_map
void krpcConfig::LoadConfigFile(const char *config_file)
{
    // 使用智能指针管理文件指针，确保文件在退出时自动关闭
    std::unique_ptr<FILE, decltype(&fclose)> pf(fopen(config_file, "r"), // 打开配置森建
                                                &fclose); // 文件关闭函数

    // 如果文件打开失败，退出程序
    if (pf == nullptr)  exit(EXIT_FAILURE) ;

    char buf[1024]; // 存储从文件中读取的每行内容

    // 使用pf.get()方法获取原始指针，逐行读取文件内容
    while (fgets(buf, 1024, pf.get()) != nullptr)
    {
        std::string read_buf(buf); // 将读取的内容转为字符串
        Trim(read_buf);// 去掉字符串前后的空格

        // 忽略注释行（#开头）和空行
        if (read_buf[0] == '#' || read_buf.empty()) continue;

        // 查找键值对的分隔符'='
        int index = read_buf.find('=');
        if (index == -1)    continue; // 没找到'='，跳过
        
        // 提取key
        std::string key = read_buf.substr(0, index);
        Trim(key); // 去掉前后空格

        
        // 提取value，并去掉换行符
        int endindex = read_buf.find('\n', index); // 查找行尾的换行符
        std::string value = read_buf.substr(index + 1, endindex - index - 1);
        Trim(value);

        // 将键值对存入配置map
        config_map.insert({key, value});
    }


}



// 根据key查找value
std::string krpcConfig::Load(const std::string &key)
{
    auto it = config_map.find(key);
    if (it == config_map.end()) return ""; // 未找到，返回空字符串

    return it->second; // 找到，返回对应的value
}



// 去掉字符串前后的空格
void krpcConfig::Trim(std::string& read_buf)
{
    // 去掉前面的空格
    int index = read_buf.find_first_not_of(' ');
    if (index != -1)    read_buf = read_buf.substr(index, read_buf.size() - index); // 截取空格后的字符串

    // 去掉后面的空格
    index = read_buf.find_last_not_of(' ');
    if (index != -1)    read_buf = read_buf.substr(0, index + 1); // 截取空格前的字符串
}