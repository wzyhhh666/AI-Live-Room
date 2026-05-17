#ifndef TALKAUDIO_H
#define TALKAUDIO_H
#include <fstream>
#include <unitree/robot/g1/audio/g1_audio_client.hpp>
#include <L2/TalkAudio/wav.hpp>
#include <ros/ros.h>
#include <stdio.h>
#include <L2/TalkAudioSend.h>
#include <std_msgs/Bool.h>
#include <iostream>
#include <filesystem>
#include <app/AppToL2Ctrl.h>
#include <L2/MicRecv.h>
#include <L2/PCMAudio.h>
#include <vector>
#include <map>
#include<queue>
#define CHUNK_SIZE 4096   // ~0.128s at 16k mono int16

using namespace std;
int time_talk_audio=50;

std::string file_path="";
std::string refile_path="/root/unitree_2025_08_14/src/L1/scripts/zaine.wav";
int daduancount=0;
int reaudio_stop=0;
bool audio_stop_flag=false;
bool audio_stop_flag1=false;
int re_buttonx_status=0;
int re_buttony_status=0;
int re_buttona_status=0;
int re_buttonb_status=0;

// PCM数据相关全局变量
std::map<std::string, std::vector<uint8_t>> pcm_buffer;  // stream_id -> PCM数据缓冲区
std::map<std::string, bool> is_last_chunk_received;      // 标记是否收到最后一个数据块
std::map<std::string, int> pcm_sample_rate;              // 记录每个流的采样率
std::string current_stream_id = "";                     // 当前播放的流ID
std::queue<std::string> stream_queue;                   // 等待播放的stream_id队列

// [NEW] Configurable parameters
double rebuffer_timeout_sec = 3.0;
double silence_padding_sec = 1.0;

// 播放控制flag
bool play_control_flag = true;                          // 控制播放行为的flag，true: 正常播放，false: 停止当前播放并清除队列

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

void talkaudiocallback2(const L2::MicRecv::ConstPtr& msg)
{
    // ROS_INFO("refile_path--->%s",refile_path.data());
    if(msg->micrecv=="小西小西。"&&refile_path!="/root/unitree_2025_08_14/src/L1/scripts/zaine.wav")
    {
        ROS_WARN("Detected wake word '小西小西。', stopping audio! (refile_path=%s)", refile_path.c_str());
        audio_stop_flag1=true;
    }
    // ROS_INFO("audio_stop_flag1--->%d",audio_stop_flag1);
}

void talkaudiocallback(const L2::TalkAudioSend::ConstPtr& msg)
{
    // 不再直接设置file_path，避免TalkAudio节点直接播放文件
    // 让Pcm_Audio节点处理文件路径并通过PCM数据方式播放
    ROS_INFO("Received file path from /L2/TalkAudioSend: %s", msg->file_path.data());
    // 文件路径将由Pcm_Audio节点处理，这里只记录日志
}

// 回调函数，处理接收到的PCM音频数据
void pcmAudioCallback(const L2::PCMAudio::ConstPtr& msg) {
    ROS_INFO("Received PCM chunk: stream_id=%s, size=%zu, is_last=%d, play_control_flag=%d", 
             msg->stream_id.c_str(), msg->pcm_data.size(), msg->is_last_chunk, play_control_flag);
    // 只有在play_control_flag为true时，才处理PCM数据
    if (play_control_flag) {
        // 如果是新流且未入队，立即入队
        if (pcm_buffer.find(msg->stream_id) == pcm_buffer.end()) {
            stream_queue.push(msg->stream_id);
            ROS_INFO("Enqueue stream_id %s for streaming playback, queue size: %zu", msg->stream_id.c_str(), stream_queue.size());
            // 添加延迟，确保播放器有足够时间准备
            ros::Duration(0.05).sleep();
        }
        // 追加PCM数据
        pcm_buffer[msg->stream_id].insert(pcm_buffer[msg->stream_id].end(), msg->pcm_data.begin(), msg->pcm_data.end());
        pcm_sample_rate[msg->stream_id] = msg->sample_rate; // 记录采样率用于播放节奏
        // 标记是否收到最后一个数据块
        if (msg->is_last_chunk) {
            is_last_chunk_received[msg->stream_id] = true;
            ROS_INFO("Mark stream_id %s as last chunk received", msg->stream_id.c_str());
        }
    } else {
        ROS_INFO("Ignoring PCM chunk because play_control_flag is false");
        // 确保关闭期间不会积累旧数据
        if (pcm_buffer.find(msg->stream_id) != pcm_buffer.end()) {
            pcm_buffer.erase(msg->stream_id);
            is_last_chunk_received.erase(msg->stream_id);
            pcm_sample_rate.erase(msg->stream_id);
            ROS_INFO("Dropped buffered PCM for stream_id %s while play_control_flag is false", msg->stream_id.c_str());
        }
    }
}

// 回调函数，处理播放控制flag
void playControlFlagCallback(const std_msgs::Bool::ConstPtr& msg) {
    bool old_flag = play_control_flag;
    play_control_flag = msg->data;
    ROS_INFO("Play control flag changed from %d to %d", old_flag, play_control_flag);
    
    // 当flag从true变为false时，立即停止当前播放并清除队列
    if (old_flag && !play_control_flag) {
        ROS_INFO("Stopping current playback and clearing queue...");
        
        // 标记需要停止当前播放
        audio_stop_flag = true;
        audio_stop_flag1 = true;
        
        // 清除播放队列
        while (!stream_queue.empty()) {
            stream_queue.pop();
        }
        // [FIX] 不要在回调中直接清除 pcm_buffer，避免 playPCMData 正在访问时导致崩溃
        // 让 playPCMData 循环检测到标志后，在主线程中安全清除
        ROS_INFO("Stop signal received, flagged for cleanup in main loop");
    }
    // 当flag从false变回true时，确保不使用旧数据，清空缓存与队列
    if (!old_flag && play_control_flag) {
        while (!stream_queue.empty()) {
            stream_queue.pop();
        }
        // 同理，不要在这里清除 pcm_buffer，除非我们确定主循环不在访问它
        // 但为了安全，我们只重置队列。playPCMData 会处理剩下的。
        ROS_INFO("Play control re-enabled");
    }
}

// 播放PCM数据的函数
void playPCMData(unitree::robot::g1::AudioClient& client_audio, ros::Publisher& status_pub) {
    // 尝试从队列获取下一个流
    if (current_stream_id.empty()) {
        if (!stream_queue.empty()) {
            current_stream_id = stream_queue.front();
            stream_queue.pop();
            ROS_INFO("Popped stream_id %s from queue, starting playback logic", current_stream_id.c_str());
            
            // [NEW] Publish Playing status
            std_msgs::Bool status_msg;
            status_msg.data = false; // False means Playing
            status_pub.publish(status_msg);
        } else {
            return; // 没有流需要播放
        }
    }

    if (pcm_buffer.find(current_stream_id) == pcm_buffer.end()) {
        ROS_WARN("Stream ID %s not found in buffer, skipping", current_stream_id.c_str());
        current_stream_id = "";
        return;
    }

    // 注意：这里引用了 map 中的 vector，如果 map 被 clear，引用将失效
    // 因此 playControlFlagCallback 中必须小心 clear
    auto& pcm_data = pcm_buffer[current_stream_id];
    int sample_rate = 16000;
    if (pcm_sample_rate.find(current_stream_id) != pcm_sample_rate.end()) {
        sample_rate = pcm_sample_rate[current_stream_id];
    }
    // 预缓冲：目标为约120ms或至少1个CHUNK，避免等待过久导致句间明显停顿
    // 声道与字节/样本在后续播放中使用，提前声明以保证作用域
    int channels = 1;
    int bytes_per_sample = 2; // int16
    size_t bytes_per_sample_local = bytes_per_sample * channels;
    // 增加预缓冲以提升播放流畅度（600ms），避免首句卡顿
    size_t target_prefill_ms = 600; // 600ms 初始缓冲
    size_t target_prefill_bytes = static_cast<size_t>((sample_rate * (target_prefill_ms / 1000.0)) * bytes_per_sample_local);
    size_t min_prefill = std::max(static_cast<size_t>(CHUNK_SIZE), target_prefill_bytes);
    ROS_INFO("Starting pre-buffering for stream_id %s, target %zu bytes (sample_rate=%d)", current_stream_id.c_str(), min_prefill, sample_rate);
    ros::Time prefill_start = ros::Time::now();
    ros::Duration prefill_timeout(2.0); // 最多等待2s以避免长时间阻塞
    while (!is_last_chunk_received[current_stream_id] && pcm_data.size() < min_prefill && !audio_stop_flag && !audio_stop_flag1 && play_control_flag) {
        ros::spinOnce();
        if (ros::Time::now() - prefill_start > prefill_timeout) {
            ROS_WARN("Prefill timeout reached for stream_id %s: have %zu bytes, proceeding", current_stream_id.c_str(), pcm_data.size());
            break;
        }
        ros::Duration(0.005).sleep();
    }
    ROS_INFO("Pre-buffering completed for stream_id %s, current buffer size: %zu bytes", current_stream_id.c_str(), pcm_data.size());

    // [MODIFIED] 移除C++侧的静音预热，因为Python侧已经实现了"Merged Pre-warm" (300ms noise)，
    // 避免双重延迟。变量已在上方声明供后续cooldown使用。
    
    // 生成静音（在没有后续流时稍长，若有下一个流则尽量缩短以实现无缝接续）
    double silence_duration_sec = silence_padding_sec; // [MODIFIED] Use global param
    size_t silence_samples = static_cast<size_t>(sample_rate * silence_duration_sec); 
    size_t silence_bytes = silence_samples * channels * bytes_per_sample; 
    
    std::vector<uint8_t> silence(silence_bytes, 0); 
    size_t offset_silence = 0; 

    /* 
    // 预热循环已禁用
    ROS_INFO("Starting audio device warmup for stream_id %s, duration: %.2fms, size: %zu bytes", 
             current_stream_id.c_str(), silence_duration_sec * 1000, silence_bytes);
    while (offset_silence < silence_bytes && !audio_stop_flag && !audio_stop_flag1 && play_control_flag) { 
        size_t chunk_size = std::min(static_cast<size_t>(CHUNK_SIZE), silence_bytes - offset_silence); 
        std::vector<uint8_t> chunk(silence.begin() + offset_silence, silence.begin() + offset_silence + chunk_size); 
        client_audio.PlayStream("example", current_stream_id, chunk); 
        double sleep_sec = static_cast<double>(chunk_size) / (sample_rate * bytes_per_sample); 
        ros::Duration(sleep_sec).sleep(); 
        offset_silence += chunk_size; 
    }
    ROS_INFO("Audio device warmup completed for stream_id %s", current_stream_id.c_str());
    */

    size_t offset = 0;
    // 流式播放：允许边接收边播放，直到收完最后一块
    ROS_INFO("Starting playback for stream_id %s, total size: %zu bytes, sample_rate: %d", 
             current_stream_id.c_str(), pcm_data.size(), sample_rate);
    
    // [FIX] 统计发送的总音频时长，用于最后等待缓冲区排空
    double total_audio_duration_sent = 0.0;
    // [FIX] 使用 Leaky Bucket 算法维护机器人端缓冲区
    ros::Time robot_playback_end_time = ros::Time::now();
    // [MODIFIED] Increased target buffer to 0.4s
    double target_robot_buffer = 0.4; 

    bool is_rebuffering = false;
    size_t rebuffer_threshold = 3 * CHUNK_SIZE; // ~384ms
    ros::Time rebuffer_start_time;

    while ((!is_last_chunk_received[current_stream_id] || offset < pcm_data.size()) && !audio_stop_flag1 && !audio_stop_flag && play_control_flag) {
        ros::spinOnce();
        size_t available = pcm_data.size() - offset;

        // [NEW] Re-buffering logic with hysteresis
        if (!is_last_chunk_received[current_stream_id]) {
            // Low water mark: 1 chunk
            if (available < CHUNK_SIZE && !is_rebuffering) {
                ROS_WARN("Buffer underrun (available=%zu), starting re-buffering...", available);
                is_rebuffering = true;
                rebuffer_start_time = ros::Time::now();
            }
            
            // High water mark: rebuffer_threshold
            if (is_rebuffering) {
                if (available < rebuffer_threshold) {
                    if ((ros::Time::now() - rebuffer_start_time).toSec() > rebuffer_timeout_sec) {
                        ROS_WARN("Re-buffering timed out (%.1fs), assuming end of stream", rebuffer_timeout_sec);
                        is_rebuffering = false;
                        is_last_chunk_received[current_stream_id] = true; // Force finish
                    } else {
                        ros::Duration(0.01).sleep(); // Wait for data
                        continue;
                    }
                } else {
                    ROS_INFO("Re-buffering completed (available=%zu), resuming", available);
                    is_rebuffering = false;
                }
            }
        }

        // 如果可用数据太少且尚未收完，稍等以聚合更多数据，减少断续
        if (available < CHUNK_SIZE / 2 && !is_last_chunk_received[current_stream_id]) {
            ros::Duration(0.005).sleep();
            continue;
        }
        size_t current_chunk_size = std::min(static_cast<size_t>(CHUNK_SIZE), available);
        std::vector<uint8_t> chunk(pcm_data.begin() + offset, pcm_data.begin() + offset + current_chunk_size);
        client_audio.PlayStream("example", current_stream_id, chunk);
        
        // 累加已发送的音频时长
        double chunk_duration = static_cast<double>(current_chunk_size) / (static_cast<double>(sample_rate) * 2.0);
        total_audio_duration_sent += chunk_duration;

        // [OPTIMIZATION] Leaky Bucket Pacing
        // 更新预计播放结束时间。如果发生 Underrun（当前时间超过了预计结束时间），则重置为当前时间。
        if (robot_playback_end_time < ros::Time::now()) {
            robot_playback_end_time = ros::Time::now();
        }
        robot_playback_end_time += ros::Duration(chunk_duration);

        // 计算当前机器人端积压的缓冲时长
        double current_buffer = (robot_playback_end_time - ros::Time::now()).toSec();
        
        // 如果缓冲超过目标值，则休眠等待（维持缓冲水位）
        // 如果缓冲不足（例如刚开始或发生 Underrun），则不休眠，全速发送以填充缓冲
        double effective_target = target_robot_buffer;
        if (is_last_chunk_received[current_stream_id]) {
            effective_target = 5.0; // Flush everything at the end
        }
        if (current_buffer > effective_target) {
            double sleep_time = current_buffer - effective_target;
            double slept = 0;
            while (slept < sleep_time) {
                if (audio_stop_flag || audio_stop_flag1 || !play_control_flag) break;
                double step = 0.005;
                ros::Duration(step).sleep();
                ros::spinOnce();
                slept += step;
            }
        } else {
            // 缓冲不足，全速发送（仅做极短休眠以处理回调）
            ros::spinOnce();
        }
        
        // [RESTORED] Buffer visualization
        int bar_width = 20;
        int filled = (int)((double)offset / pcm_data.size() * bar_width);
        if (filled > bar_width) filled = bar_width;
        std::string bar = "[";
        for (int i=0; i<filled; i++) bar += "|";
        for (int i=filled; i<bar_width; i++) bar += " ";
        bar += "]";
        std::cout << "\rBuffer: " << bar << " " << offset << "/" << pcm_data.size() << " RobotBuf: " << current_buffer << "s" << std::flush;
        
        offset += current_chunk_size;
    }
    std::cout << std::endl; // Newline after progress bar

    // [FIX] 发送静音帧作为结尾，防止尾音被吞
    // 必须在等待播放结束前发送，以保证连续性，防止硬件缓冲区排空导致断音
    if (!stream_queue.empty()) {
        ROS_INFO("Next stream queued, using minimal cooldown for stream_id %s", current_stream_id.c_str());
        double minimal_silence_sec = 0.06; // 60ms minimal padding
        size_t minimal_silence_samples = static_cast<size_t>(sample_rate * minimal_silence_sec);
        size_t minimal_silence_bytes = minimal_silence_samples * channels * bytes_per_sample;
        size_t offset_min = 0;
        while (offset_min < minimal_silence_bytes && !audio_stop_flag && !audio_stop_flag1 && play_control_flag) {
            size_t chunk_size = std::min(static_cast<size_t>(CHUNK_SIZE), minimal_silence_bytes - offset_min);
            std::vector<uint8_t> chunk(silence.begin() + offset_min, silence.begin() + offset_min + chunk_size);
            client_audio.PlayStream("example", current_stream_id, chunk);
            double sleep_sec = static_cast<double>(chunk_size) / (sample_rate * bytes_per_sample);
            ros::Duration(sleep_sec).sleep();
            // 更新预计结束时间，确保等待逻辑包含这段静音
            robot_playback_end_time += ros::Duration(sleep_sec);
            offset_min += chunk_size;
        }
        ROS_INFO("Minimal cooldown completed for stream_id %s", current_stream_id.c_str());
    } else {
        ROS_INFO("Starting audio device cooldown (padding) for stream_id %s", current_stream_id.c_str());
        offset_silence = 0;
        while (offset_silence < silence_bytes && !audio_stop_flag && !audio_stop_flag1 && play_control_flag) {
            size_t chunk_size = std::min(static_cast<size_t>(CHUNK_SIZE), silence_bytes - offset_silence);
            std::vector<uint8_t> chunk(silence.begin() + offset_silence, silence.begin() + offset_silence + chunk_size);
            client_audio.PlayStream("example", current_stream_id, chunk);
            double sleep_sec = static_cast<double>(chunk_size) / (sample_rate * bytes_per_sample);
            ros::Duration(sleep_sec).sleep();
            // 更新预计结束时间，确保等待逻辑包含这段静音
            robot_playback_end_time += ros::Duration(sleep_sec);
            offset_silence += chunk_size;
        }
        ROS_INFO("Audio device cooldown completed for stream_id %s", current_stream_id.c_str());
    }

    // [FIX] 等待累积的缓冲区播放完毕 (Anti-swallow logic)
    // 使用 robot_playback_end_time 准确判断播放结束时刻
    if (!audio_stop_flag && !audio_stop_flag1 && play_control_flag) {
        double remaining_buffer = (robot_playback_end_time - ros::Time::now()).toSec();
        // 加上一点安全余量 (e.g. 100ms -> 1.0s)
        remaining_buffer += 1.0; 
        
        if (remaining_buffer > 0) {
            ROS_INFO("Waiting for buffered audio to finish: %.3fs", remaining_buffer);
            while (ros::Time::now() < robot_playback_end_time + ros::Duration(1.5)) {
                if (audio_stop_flag || audio_stop_flag1 || !play_control_flag) break;
                ros::Duration(0.01).sleep();
                ros::spinOnce();
            }
        }
    }

    client_audio.PlayStop(current_stream_id);
    ROS_INFO("Playback completed for stream_id %s, total bytes played: %zu", current_stream_id.c_str(), offset);
    
    // [FIX] 增强清理逻辑：如果是被打断或停止，清空所有缓冲区，防止残留
    if (audio_stop_flag || audio_stop_flag1 || !play_control_flag) {
        pcm_buffer.clear();
        is_last_chunk_received.clear();
        pcm_sample_rate.clear();
        ROS_INFO("Force cleared ALL PCM buffers due to stop signal");
    } else {
        // 正常结束，只清理当前流
        if (!current_stream_id.empty()) {
            pcm_buffer.erase(current_stream_id);
            is_last_chunk_received.erase(current_stream_id);
            pcm_sample_rate.erase(current_stream_id);
            ROS_INFO("Cleaned up resources for stream_id %s", current_stream_id.c_str());
        }
    }
    
    current_stream_id = "";
    audio_stop_flag = false;
    audio_stop_flag1 = false;

    // [NEW] Publish Idle status
    std_msgs::Bool status_msg;
    status_msg.data = true; // True means Idle
    status_pub.publish(status_msg);
}

void talkaudiocallback1(const app::AppToL2Ctrl::ConstPtr& msg)
{
    if(reaudio_stop==0&&msg->audio_stop==1)
    {
        ROS_WARN("Received audio_stop signal from AppToL2Ctrl!");
        audio_stop_flag=true;
    }
    reaudio_stop=msg->audio_stop;
    if(msg->mode==0)
    {
        if(msg->buttonx_status==1&&re_buttonx_status==0) file_path="/root/unitree_2025_08_14/src/L2/scripts/dianwei/nihao.wav";
        if(msg->buttony_status==1&&re_buttony_status==0) file_path="/root/unitree_2025_08_14/src/L2/scripts/dianwei/1.wav";
        if(msg->buttona_status==1&&re_buttona_status==0) file_path="/root/unitree_2025_08_14/src/L2/scripts/dianwei/2.wav";
        if(msg->buttonb_status==1&&re_buttonb_status==0) file_path="/root/unitree_2025_08_14/src/L2/scripts/dianwei/3.wav";
    }
    re_buttonx_status=msg->buttonx_status;
    re_buttony_status=msg->buttony_status;
    re_buttona_status=msg->buttona_status;
    re_buttonb_status=msg->buttonb_status;
}

/*
 * stream_play函数：播放音频文件并实现WAV到PCM的转换过程
 * 
 * 参数说明：
 *   - file_pathaa: 音频文件路径
 *   - client_audio: 音频客户端实例，用于播放音频流
 * 
 * 功能说明：
 *   1. 调用ReadWave函数将WAV文件转换为PCM数据
 *   2. 检查音频格式（只支持16kHz采样率、单声道的WAV文件）
 *   3. 将PCM数据分块播放
 */
void stream_play(std::string file_pathaa, unitree::robot::g1::AudioClient client_audio)
{
    int32_t ret;
    int32_t sample_rate = -1;        // 音频采样率
    int8_t num_channels = 0;         // 音频通道数
    bool filestate = false;          // 文件状态标志
    
    /*
     * 核心步骤：WAV转PCM
     * ReadWave函数执行以下操作：
     *   1. 打开WAV文件
     *   2. 解析WAV文件头（RIFF格式）
     *   3. 验证文件格式（RIFF和WAVE标识符）
     *   4. 读取音频格式信息（采样率、通道数等）
     *   5. 跳过WAV文件头，提取原始PCM数据
     *   6. 返回PCM数据数组
     */
    std::vector<uint8_t> pcm = ReadWave(file_pathaa, &sample_rate, &num_channels, &filestate);

    // 输出音频文件信息
    std::cout << "wav file sample_rate = " << sample_rate
                << " num_channels =  " << std::to_string(num_channels)
                << " filestate =" << filestate << "filesize = " << pcm.size() 
                << std::endl;

    // 验证音频格式：只支持16kHz采样率和单声道
    if (filestate && sample_rate == 16000 && num_channels == 1) {
        size_t total_size = pcm.size();  // PCM数据总大小
        size_t offset = 0;               // 当前读取偏移量
        int chunk_index = 0;             // 块索引
        // 生成唯一的流ID（使用当前时间戳）
        std::string stream_id = std::to_string(unitree::common::GetCurrentTimeMillisecond());
        
        // 添加延迟，确保播放器有足够时间准备
        ros::Duration(0.05).sleep();
        
        // [FIX] 发送静音帧预热音频设备，解决首字吞音问题
        int sample_rate = 16000; // 或从 msg/sample_rate 获取 
        int channels = 1; 
        int bytes_per_sample = 2; // int16 
        
        // 生成 500ms 静音（更保险） 
        double silence_duration_sec = 0.5; // 500ms 
        size_t silence_samples = static_cast<size_t>(sample_rate * silence_duration_sec); 
        size_t silence_bytes = silence_samples * channels * bytes_per_sample; 
        
        std::vector<uint8_t> silence(silence_bytes, 0); 
        
        // 分块发送（避免单次数据过大） 
        size_t offset_silence = 0; 
        ROS_INFO("Starting file playback warmup for stream_id %s, duration: %.2fms, size: %zu bytes", 
                 stream_id.c_str(), silence_duration_sec * 1000, silence_bytes);
        while (offset_silence < silence_bytes && !audio_stop_flag && !audio_stop_flag1 && play_control_flag) { 
            size_t chunk_size = std::min(static_cast<size_t>(CHUNK_SIZE), silence_bytes - offset_silence); 
            std::vector<uint8_t> chunk(silence.begin() + offset_silence, silence.begin() + offset_silence + chunk_size); 
            client_audio.PlayStream("example", stream_id, chunk); 
            double sleep_sec = static_cast<double>(chunk_size) / (sample_rate * bytes_per_sample); 
            ros::Duration(sleep_sec).sleep(); 
            offset_silence += chunk_size; 
        }
        ROS_INFO("File playback warmup completed for stream_id %s", stream_id.c_str());

        // 分块播放PCM数据
        ROS_INFO("Starting file playback for stream_id %s, total size: %zu bytes, file_path: %s", 
                 stream_id.c_str(), total_size, file_pathaa.c_str());
        while (offset < total_size && play_control_flag && !audio_stop_flag && !audio_stop_flag1) {
            ros::spinOnce();  // 处理ROS回调

            // 检查停止/打断标志或外部暂停
            if (audio_stop_flag1) { audio_stop_flag1 = false; break; }
            if (audio_stop_flag || !play_control_flag) break;

            // 计算当前块大小（不超过CHUNK_SIZE）
            size_t remaining = total_size - offset;
            size_t current_chunk_size = std::min(static_cast<size_t>(CHUNK_SIZE), remaining);

            // 创建当前块的PCM数据
            std::vector<uint8_t> chunk(pcm.begin() + offset, pcm.begin() + offset + current_chunk_size);

            // 播放音频流块
            client_audio.PlayStream("example", stream_id, chunk);

            // 按块时长休眠，避免固定延迟导致卡顿
            double seconds = static_cast<double>(current_chunk_size) / (static_cast<double>(sample_rate) * 2.0);
            ros::Duration(seconds).sleep();
            std::cout << "Playing size: " << offset << std::endl;

            // 更新偏移量，准备下一块
            offset += current_chunk_size;
        }

        // [FIX] 发送静音帧作为结尾，防止尾音被吞。若后面有下一个流则缩短冷却。
        ROS_INFO("Starting file playback cooldown (padding) for stream_id %s", stream_id.c_str());
        offset_silence = 0;
        double file_cooldown_default = 0.4; // [MODIFIED] 120ms -> 400ms
        double file_cooldown_sec = file_cooldown_default;
        // 如果有后续流，缩短文件冷却以减少句间停顿
        if (!stream_queue.empty()) {
            file_cooldown_sec = 0.06; // 60ms when next queued
        }
        size_t file_cooldown_samples = static_cast<size_t>(sample_rate * file_cooldown_sec);
        size_t file_cooldown_bytes = file_cooldown_samples * channels * bytes_per_sample;
        while (offset_silence < file_cooldown_bytes && play_control_flag && !audio_stop_flag && !audio_stop_flag1) {
            size_t chunk_size = std::min(static_cast<size_t>(CHUNK_SIZE), file_cooldown_bytes - offset_silence);
            std::vector<uint8_t> chunk(silence.begin() + offset_silence, silence.begin() + offset_silence + chunk_size);
            client_audio.PlayStream("example", stream_id, chunk);
            double sleep_sec = static_cast<double>(chunk_size) / (sample_rate * bytes_per_sample);
            ros::Duration(sleep_sec).sleep();
            offset_silence += chunk_size;
        }
        ROS_INFO("File playback cooldown completed for stream_id %s", stream_id.c_str());

        // 播放结束后停止流
        ret = client_audio.PlayStop(stream_id);
        ROS_INFO("File playback completed for stream_id %s, total bytes played: %zu, result: %d", 
                 stream_id.c_str(), offset, ret);
        
    } else {
        // 音频格式不符合要求
        std::cout << "audio file format error, please check!" << std::endl;
    }
}

#endif