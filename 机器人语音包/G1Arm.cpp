// 基础 ROS/消息
#include <ros/ros.h>
#include "L2/G1CtrlInput.h"
#include <std_msgs/Bool.h>

// Unitree G1 SDK
#include <unitree/robot/channel/channel_factory.hpp>
#include <unitree/robot/g1/arm/g1_arm_action_client.hpp>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// 全局动作数据（简单共享变量）
static int g_armbase = 0;

// 订阅回调：记录上层发布的动作码
static void ctrlCallback(const L2::G1CtrlInput::ConstPtr& msg)
{
    ROS_INFO("Received command - Mode: %d, ArmBase: %d, XV: %f, YV: %f, YawV: %f",
             msg->mode, msg->armbase, msg->xv, msg->yv, msg->yawv);
    if (msg->mode == 1) {
        g_armbase = msg->armbase;
        ROS_INFO("Executing action code: %d", g_armbase);
    }
}

// 根据 IP 查找对应的网卡接口名（来自你的参考实现）
static std::string getNetworkInterfaceByIp(const std::string& targetIp)
{
    std::ifstream netDevFile("/proc/net/dev");
    if (!netDevFile.is_open()) return "";

    std::vector<std::string> interfaces;
    std::string line;
    std::getline(netDevFile, line);
    std::getline(netDevFile, line);
    while (std::getline(netDevFile, line)) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string iface = line.substr(0, colonPos);
            iface.erase(std::remove_if(iface.begin(), iface.end(), ::isspace), iface.end());
            if (!iface.empty()) interfaces.push_back(iface);
        }
    }
    netDevFile.close();

    for (const auto& iface : interfaces) {
        std::string command = std::string("ip addr show ") + iface + " | grep 'inet ' | awk '{print $2}' | cut -d/ -f1";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) continue;
        char buffer[128];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
        result.erase(std::remove(result.begin(), result.end(), ' '), result.end());
        if (result == targetIp) return iface;
    }
    return "";
}

// 打印动作名称，便于日志确认
static const char* actionNameForCode(int code)
{
    switch (code) {
        case 1:  return "Init (release_arm)";         // ExecuteAction(99)
        case 2:  return "Two-hand kiss";              // 11
        case 3:  return "Both hands up";              // 15
        case 4:  return "Clamp";                      // 17
        case 5:  return "High five";                  // 18
        case 6:  return "Hug";                        // 19
        case 7:  return "Refuse";                     // 22
        case 8:  return "Ultraman ray";               // 24
        case 9:  return "Wave above head";            // 26
        case 10: return "Shake hand";                 // 27
        case 11: return "Extend right arm forward";   // 31
        case 12: return "Both hands up deviate right";// 34
        case 13: return "Emphasize";                  // 35
        default: return "Unknown";
    }
}

int main(int argc, char** argv)
{
    setlocale(LC_ALL, "");
    ros::init(argc, argv, "G1Arm");
    ros::NodeHandle nh;

    // 订阅动作话题；可选发布状态
    ros::Subscriber sub = nh.subscribe("/L2/G1CtrlInput", 10, ctrlCallback);
    ros::Publisher status_pub = nh.advertise<std_msgs::Bool>("/L2/G1ArmStatus", 1);

    // 从参数服务器获取机器人 IP（或网卡 IP），默认 192.168.123.164
    std::string robot_ip;
    nh.param<std::string>("g1arm_robot_ip", robot_ip, std::string("192.168.123.164"));
    std::string iface = getNetworkInterfaceByIp(robot_ip);
    if (iface.empty()) {
        ROS_WARN("Network interface for IP %s not found, using default 'eth0'", robot_ip.c_str());
        iface = "eth0"; // 回退
    }

    // 初始化 Unitree 通道
    try {
        unitree::robot::ChannelFactory::Instance()->Init(0, iface);
    } catch (const std::exception& e) {
        ROS_ERROR("ChannelFactory init failed: %s", e.what());
    }

    // G1Arm 客户端
    unitree::robot::g1::G1ArmActionClient client;
    try {
        client.Init();
        client.SetTimeout(10.f);
    } catch (const std::exception& e) {
        ROS_ERROR("G1ArmActionClient init failed: %s", e.what());
    }

    ROS_INFO("G1Arm node started, listening on /L2/G1CtrlInput");

    ros::Rate rate(10); // 10Hz 轮询执行
    int last_executed = 0;
    while (ros::ok()) {
        ros::spinOnce();

        int code = g_armbase;
        if (code != 0) {
            // 打印动作名称映射
            ROS_INFO("Action requested: code=%d, name=%s", code, actionNameForCode(code));

            try {
                switch (code) {
                    case 1:  client.ExecuteAction(99); break; // release_arm
                    case 2:  client.ExecuteAction(11); break; // two-hand kiss
                    case 3:  client.ExecuteAction(15); break; // both hands up
                    case 4:  client.ExecuteAction(17); break; // clamp
                    case 5:  client.ExecuteAction(18); break; // high five
                    case 6:  client.ExecuteAction(19); break; // hug
                    case 7:  client.ExecuteAction(22); break; // refuse
                    case 8:  client.ExecuteAction(24); break; // ultraman ray
                    case 9:  client.ExecuteAction(26); break; // wave above head
                    case 10: client.ExecuteAction(27); break; // shake hand
                    case 11: client.ExecuteAction(31); break; // extend right arm forward
                    case 12: client.ExecuteAction(34); break; // both hands up deviate right
                    case 13: client.ExecuteAction(35); break; // emphasize
                    default:
                        ROS_WARN("Unknown action code: %d", code);
                        break;
                }
                last_executed = code;
                // 执行一次后清零，避免重复触发
                g_armbase = 0;
                std_msgs::Bool st; st.data = true; status_pub.publish(st);
            } catch (const std::exception& e) {
                ROS_ERROR("ExecuteAction failed for code %d: %s", code, e.what());
                std_msgs::Bool st; st.data = false; status_pub.publish(st);
            }
        }

        rate.sleep();
    }
    return 0;
}
