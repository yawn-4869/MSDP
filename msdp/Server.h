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
    // 报文处理, m_fusion融合类
    void repProcess(int64_t fusion_time);
    // 报文处理, m_multi_fusion融合类
    void fusion();
    // 发送单源和融合后的航迹
    void repSend();

private:
    // 融合单元序列化和反序列化
    std::string fusionUnitToBufString(FusionUnit& fusion_unit);
    FusionUnit bufStringToFusionUnit(std::string fusion_unit_string);
    // 航迹报告序列化和反序列化
    std::string radarTrackToBufstring(RadarTrack& radar_trk);
    RadarTrack bufStringToRadarTrack(std::string& radar_trk_str);
    // 航迹报告转化为融合的航迹结构体 -- m_multi_fusion类
    TrkForFushion trkRepToTrkForFusion(RadarTrack& trk_rep);
private:
    EventLoop* m_main_event_loop;
    IOThreadPool* m_thread_pool; // 线程池
    std::string m_ip; // 本机ip地址
    std::string m_worker_ip; // 工作机ip地址
    std::vector<CNetSocket*> m_recv_socks; // 接收雷达报文的socket
    std::vector<CNetSocket*> m_send_socks; // 发送航迹的socket
    HeartBeat* m_hb_handler; // 心跳包
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
    // 锁
    std::mutex m_list_mtx;
    std::mutex m_fusion_mtx;
    std::mutex m_fusion_lsit_mtx;
    // 航迹号信息
    int m_next_track_number; // 集群下一个新建航迹的航迹号
    int m_last_track_number; // 本机掉线前记录的下一个新建航迹的航迹号
    // 融合航迹列表
    std::vector<FusionUnit> m_fusion_unit_list; // 融合单元列表
    std::vector<std::string> m_fusion_unit_str_vec; // 序列化的融合单元列表
    // 数据缓冲区
    unsigned char data_buf[1024];
    unsigned char sdBuf[2000];
    unsigned char fusion_unit_send_buffer[1024];

};

#endif