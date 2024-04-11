#ifndef __SERVER_H__
#define __SERVER_H__

#include "HeartBeat.h"
#include "IOThreadPool.h"
#include "Decode.h"
#include "GeograpCoordTrans.h"
#include "MFusion.h"
#include "MultiFusion.h"



class Server {
public:
    Server();
    ~Server();
    void init();
    void start();
    void join();
    // 接收雷达报文并解码
    void repRecv(unsigned char*buf, int port, int len);
    // 单端口接收
    void repRecvSingle(unsigned char* buf, int len, int port);
    // 本地读取报文
    void localRepRecv();
    // 报文处理
    void repProcess(int64_t fusion_time);
    void fusion();
    // 发送单源和融合后的航迹
    void repSend();

private:
    std::string fusionUnitToBufString(FusionUnit& fusion_unit);
    FusionUnit bufStringToFusionUnit(std::string fusion_unit_string);
    // 航迹报告转化为融合的航迹结构体
    TrkForFushion trkRepToTrkForFusion(RadarTrack& trk_rep);
private:
    EventLoop* m_main_event_loop;
    IOThreadPool* m_thread_pool;
    std::string m_ip; // 本机ip地址
    std::string m_worker_ip; // 工作机ip地址
    std::vector<CNetSocket*> m_recv_socks; // 接收雷达报文的socket
    std::vector<CNetSocket*> m_send_socks; // 发送航迹的socket
    HeartBeat m_hb_server; // 心跳包服务端, 负责心跳包的接收
    HeartBeat m_hb_client; // 心跳包客户端, 负责心跳包的发送
    HeartBeat* m_hb_handler;
    // 解码类
    CDecodeCat021 m_track021_decode;
    CDecodeAsterix m_track001_decode;
    // 坐标转换
    CGeograpCoordTrans m_coord_trans;
    // 解码后的结构体存储列表
    std::list<RadarTrack> m_track_list;
    std::vector<TrkForFushion> m_tff_vec;
    // 融合类
    MFusion m_fusion;
    MultiFusion m_multi_fusion;
    // 时间戳记录
    long long last_time;
    std::mutex m_list_mtx;
    std::mutex m_fusion_mtx;
    std::mutex m_fusion_lsit_mtx;
    // 航迹号信息
    int m_next_track_number;
    // 融合航迹列表
    std::vector<FusionUnit> m_fusion_unit_list;
    std::vector<std::string> m_fusion_unit_str_vec;
    unsigned char data_buf[1024];
    unsigned char sdBuf[2000];
    unsigned char fusion_unit_send_buffer[1024];

};

#endif