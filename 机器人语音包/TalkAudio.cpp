#include <L2/TalkAudio/TalkAudio.h>

int main(int argc,char** argv)
{
    setlocale(LC_ALL,"");
    ros::init(argc,argv,"TalkAudio");
    ros::NodeHandle n_talk_audio;
    ros::Publisher talk_audio_pub1 =n_talk_audio.advertise<std_msgs::Bool>("/L2/TalkAudioStatus",1);
    ros::Subscriber talk_audio_sub2 = n_talk_audio.subscribe("/L2/TalkAudioSend", 5, talkaudiocallback);
    ros::Subscriber talk_audio_sub3 = n_talk_audio.subscribe("/app/AppToL2Ctrl", 5, talkaudiocallback1);
    ros::Subscriber talk_audio_sub4 = n_talk_audio.subscribe("/L2/MicRecv", 5, talkaudiocallback2);
    // [FIX] 添加PCM音频流订阅 - Increased buffer size from 100 to 2000 to prevent Buffer Overrun
    ros::Subscriber talk_audio_sub5 = n_talk_audio.subscribe("/L2/PCMAudio", 2000, pcmAudioCallback);
    // [FIX] 添加播放控制订阅
    ros::Subscriber talk_audio_sub6 = n_talk_audio.subscribe("/L2/PlayControlFlag", 50, playControlFlagCallback);
    
    std::string interface=getNetworkInterfaceByIp("192.168.123.164");
    unitree::robot::ChannelFactory::Instance()->Init(0,interface);
    unitree::robot::g1::AudioClient client_audio_play;
    client_audio_play.Init();
    client_audio_play.SetTimeout(10.0f);

    // [FIX] Read volume parameters from ROS
    ros::NodeHandle nh_private("~");
    int volume_sdk = 100;
    int volume_sys = -1; // Default -1 means do not change system volume

    nh_private.param("volume", volume_sdk, 100);
    nh_private.param("system_volume", volume_sys, -1);
    
    // [NEW] Read timeout and padding params
    nh_private.param("rebuffer_timeout", rebuffer_timeout_sec, 3.0);
    nh_private.param("silence_padding", silence_padding_sec, 0.2); // Reduced from 1.5 to 0.2
    ROS_INFO("TalkAudio params: rebuffer_timeout=%.2fs, silence_padding=%.2fs", rebuffer_timeout_sec, silence_padding_sec);

    // [HARDCODE] Force Max Volume
    client_audio_play.SetVolume(100);
    ROS_INFO("HARDCODED SDK volume to 100");

    // [HARDCODE] Force System Volume to 100% on Card 2
    std::string cmd1 = "amixer -c 2 set MVC1 100% > /dev/null 2>&1";
    std::string cmd2 = "amixer -c 2 set MVC2 100% > /dev/null 2>&1";
    system(cmd1.c_str());
    system(cmd2.c_str());
    ROS_INFO("HARDCODED System volume (MVC1/MVC2) to 100%");

    /*
    if (volume_sys >= 0 && volume_sys <= 100) {
        // [FIX] Use card 2 (APE) instead of 3
        std::string cmd1 = "amixer -c 2 set MVC1 " + std::to_string(volume_sys) + "% > /dev/null 2>&1";
        std::string cmd2 = "amixer -c 2 set MVC2 " + std::to_string(volume_sys) + "% > /dev/null 2>&1";
        int ret1 = system(cmd1.c_str());
        int ret2 = system(cmd2.c_str());
        ROS_INFO("Set system volume (MVC1/MVC2) to %d%%", volume_sys);
    }
    */

    // unitree::robot::ChannelSubscriber<std_msgs::msg::dds_::String_> subscriber_mic(
    //     AUDIO_SUBSCRIBE_TOPIC);
    // subscriber_mic.InitChannel(asr_handler_talkaudio);

    // [FIX] 系统级预热：在进入主循环前，播放一段不可见的静音流
    // 这能强制唤醒音频硬件和底层服务，避免第一次真实播放时的冷启动卡顿
    ROS_INFO("Performing system-level audio warmup...");
    std::string warmup_id = "sys_warmup_" + std::to_string(unitree::common::GetCurrentTimeMillisecond());
    std::vector<uint8_t> warmup_silence(32000, 0); // 1秒静音 (16000 * 2 bytes)
    // 分块发送以模拟真实流
    for(size_t i=0; i<warmup_silence.size(); i+=1024) {
        std::vector<uint8_t> chunk(warmup_silence.begin()+i, warmup_silence.begin()+std::min(i+1024, warmup_silence.size()));
        client_audio_play.PlayStream("example", warmup_id, chunk);
        ros::Duration(0.02).sleep(); // 稍微慢一点发送
    }
    client_audio_play.PlayStop(warmup_id);
    ROS_INFO("System-level audio warmup completed.");

    ros::Rate rate_loop_talknode(time_talk_audio);
    /*action*/
    while(ros::ok())
	{
        // 注意：audio_stop_flag 和 audio_stop_flag1 是全局变量，在回调中被修改
        // 这里不应无条件重置为 false，否则会覆盖回调中的停止信号
        // audio_stop_flag=false; 
        // audio_stop_flag1=false;
        // 只有在处理完停止逻辑后才重置，或者在开始新播放前重置
        // 但原逻辑似乎是每帧重置？这看起来很危险。
        // 暂时保留原逻辑但注释掉，因为 playPCMData 内部会检查这些标志
        // 如果每循环都重置，打断功能可能失效。
        // 经过分析，原代码在 stream_play 内部循环检查这些标志。
        // 如果外层循环重置它们，那么 stream_play 返回后，标志被重置，这是对的。
        // 但是如果在 stream_play 执行期间，回调设置了标志，stream_play 会退出。
        // 退出后，外层循环重置标志，准备下一次。
        // 所以这里的重置是用于"下一次循环的初始化"。
        // 但是 playPCMData 也是阻塞的，所以逻辑类似。
        
        audio_stop_flag=false;
        audio_stop_flag1=false;

        ros::spinOnce();
        
        // 优先处理文件播放（如果有）
        if(file_path!="")
        {
            std_msgs::Bool now_status_audio;
            now_status_audio.data=false;
            talk_audio_pub1.publish(now_status_audio);
            stream_play(file_path,client_audio_play);
            
            refile_path=file_path;
            if(audio_stop_flag1)
            {
                file_path="/root/unitree_2025_08_14/src/L1/scripts/zaine.wav";
            }else{
                file_path="";
            }
            now_status_audio.data=true;
            talk_audio_pub1.publish(now_status_audio);
        } 
        
        // [FIX] 处理流式PCM播放
        // playPCMData 内部会检查队列，如果有流则播放，没有则立即返回
        playPCMData(client_audio_play, talk_audio_pub1);
           
        rate_loop_talknode.sleep();
	}
    /*action*/

    
    return 0;
}