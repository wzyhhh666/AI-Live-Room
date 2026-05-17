#ifndef G1Arm_H
#define G1Arm_H
#include <ros/ros.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <L2/G1CtrlInput.h>
#include <unitree/robot/g1/arm/g1_arm_action_api.hpp>
#include <unitree/robot/g1/arm/g1_arm_action_client.hpp>
#include <unitree/robot/g1/arm/g1_arm_action_error.hpp>
#include <std_msgs/Bool.h>

using namespace std;

int time_control_G1Arm=10;


int control_mode=0;

struct control_auto_data_G1Arm
{
    int armbase;
}control_auto_data_G1Arm;


std::string getNetworkInterfaceByIp(const std::string& targetIp) {
    // 读取/proc/net/dev获取所有网络接口
    std::ifstream netDevFile("/proc/net/dev");
    if (!netDevFile.is_open()) {
        return "";
    }

    std::vector<std::string> interfaces;
    std::string line;
    
    // 跳过前两行标题
    std::getline(netDevFile, line);
    std::getline(netDevFile, line);

    // 提取所有网络接口名称
    while (std::getline(netDevFile, line)) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string interface = line.substr(0, colonPos);
            // 去除空格
            interface.erase(std::remove_if(interface.begin(), interface.end(), ::isspace), interface.end());
            if (!interface.empty()) {
                interfaces.push_back(interface);
            }
        }
    }
    netDevFile.close();

    // 检查每个接口的IP地址
    for (const auto& interface : interfaces) {
        std::string command = "ip addr show " + interface + " | grep 'inet ' | awk '{print $2}' | cut -d/ -f1";
        
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            continue;
        }

        char buffer[128];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);

        // 去除换行符和空格
        result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
        result.erase(std::remove(result.begin(), result.end(), ' '), result.end());

        if (result == targetIp) {
            return interface;
        }
    }

    return ""; // 没有找到匹配的接口
}



void control_callback2(const L2::G1CtrlInput::ConstPtr& msg)
{
    control_auto_data_G1Arm.armbase=msg->armbase;
}

#endif
