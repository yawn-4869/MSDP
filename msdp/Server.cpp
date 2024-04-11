#include "Server.h"
#include "Config.h"
#include "Logger.h"
#include "fusion.pb.h"
#include "utils.h"

Server::Server() {
    // 成员变量初始化
    m_ip = Config::get_instance()->m_server_address;
    m_worker_ip = Config::get_instance()->m_worker_server_address;
    m_next_track_number = -1;

    // 类初始化
    m_coord_trans.InitOrg(Config::get_instance()->m_fusion_center_lon, Config::get_instance()->m_fusion_center_lat);

    // 初始化处理雷达报文接收的netsocket
    for(int i = 0; i < Config::get_instance()->m_radar_count; ++i) {
        CNetSocket* receive_sock = new CNetSocket();
        receive_sock->InitNetSocket('R', (char*)Config::get_instance()->m_recv_address.c_str(), 
                                    Config::get_instance()->get_instance()->m_receive_ports[i]);
        m_recv_socks.push_back(receive_sock);
    }

    // 初始化融合单元接收的netsocket
    CNetSocket* fusion_unit_receive_sock = new CNetSocket();
    fusion_unit_receive_sock->InitNetMultiSocket('R', (char*)Config::get_instance()->m_fusion_unit_group_address.c_str(), 
                                                Config::get_instance()->m_fusion_unit_group_port);
    m_recv_socks.push_back(fusion_unit_receive_sock);

    
    // 初始化融合报文发送的netsocket
    for(int i = 0; i < Config::get_instance()->m_send_port_count; ++i) {
        CNetSocket* send_sock = new CNetSocket();
        send_sock->InitNetSocket('S', (char*)Config::get_instance()->m_send_address.c_str(), 
                                Config::get_instance()->m_send_ports[i]);
        m_send_socks.push_back(send_sock);
    }

    // 初始化融合单元发送的netsocket
    CNetSocket* fusion_unit_send_sock = new CNetSocket();
    fusion_unit_send_sock->InitNetMultiSocket('S', (char*)Config::get_instance()->m_fusion_unit_group_address.c_str(), 
                                            Config::get_instance()->m_fusion_unit_group_port);
    m_send_socks.push_back(fusion_unit_send_sock);

    // 初始化心跳包
    m_hb_handler = new HeartBeat(HB_CONNECTED, Config::get_instance()->m_hb_group_address.c_str(), 
    m_ip.c_str(), Config::get_instance()->m_hb_group_port);

    init();
}

Server::~Server() {
    // 清理指针
    if(m_main_event_loop) {
        delete m_main_event_loop;
        m_main_event_loop = NULL;
    }

    if(m_thread_pool) {
        delete m_thread_pool;
        m_thread_pool = NULL;
    }

    for(int i = 0; i < m_recv_socks.size(); ++i) {
        if(m_recv_socks[i]) {
            delete m_recv_socks[i];
            m_recv_socks[i] = NULL;
        }
    }

    for(int i = 0; i < m_send_socks.size(); ++i) {
        if(m_send_socks[i]) {
            delete m_send_socks[i];
            m_send_socks[i] = NULL;
        }
    }

    if(m_hb_handler) {
        delete m_hb_handler;
        m_hb_handler = NULL;
    }
}

void Server::init() {
    // eventloop及线程池初始化
    m_main_event_loop = EventLoop::GetCurrentEventloop();
    // 线程池初始化
    // 雷达报文接收线程m_radar_count个+1个融合线程+1个心跳包发送线程+1个心跳包接收线程+1个融合单元接收线程
    m_thread_pool = new IOThreadPool(Config::get_instance()->m_radar_count + 4);

    for(int i = 0; i < m_recv_socks.size(); ++i) {
        IOThread* io_thread = m_thread_pool->getIOThread();
        int listenfd = m_recv_socks[i]->GetReceiveFd();
        int port = m_recv_socks[i]->GetPort();
        FdEvent *event = new FdEvent(listenfd);
        event->listen(FdEvent::IN_EVENT, [this, port, i](){
            memset(data_buf, 0, 1024);
            int len = m_recv_socks[i]->ReceiveData(data_buf);
            // this->repRecv(data_buf, port, len);
            this->repRecvSingle(data_buf, len, port);
        });
        io_thread->getEventLoop()->addEpollEvent(event);
    }

    // 初始化心跳包线程
    IOThread* hb_send_thread = m_thread_pool->getIOThread();
    // m_hb_client.init(HB_CLIENT, Config::get_instance()->m_hb_group_address.c_str(), Config::get_instance()->m_hb_group_port);
    // client
    // 需要先发送一次包含注册信息的心跳包
    // HBMSGTYPE send_type = HB_MSG_REG;
    // const char* content = "register";
    u_int len = 8;
    // m_hb_client.sendMsg(send_type, (char*)m_worker_ip.c_str(), m_next_track_number);

    // 定时发送HB_MSG_RUNNING类型的心跳包
    // 心跳包发送内容: 下一个trackNum, 当前工作机ip
    HBMSGTYPE send_type = HB_MSG_RUNNING;
    const char* content = "running";
    TimerEvent::s_ptr hb_send_event = std::make_shared<TimerEvent>(Config::get_instance()->m_hb_send_interval, true, [send_type, content, len, this] () {
        int next_track_number = -1;
        if(m_ip == m_worker_ip) {
            // 只有工作机才能发送航迹号, 非工作机发送-1的航迹号, 作为区分
            next_track_number = m_next_track_number;
        }
        m_hb_handler->sendMsg(send_type, (char*)m_worker_ip.c_str(), next_track_number);
    });
    hb_send_thread->getEventLoop()->addTimerEvent(hb_send_event);

    IOThread* hb_recv_thread = m_thread_pool->getIOThread();
    // m_hb_server.init(HB_SERVER, Config::get_instance()->m_hb_group_address.c_str(), Config::get_instance()->m_hb_group_port);
    // server: 1. 定时检查节点是否超时 2. 处理接收到的HBMSG
    // 定时检查
    TimerEvent::s_ptr hb_check_event = std::make_shared<TimerEvent>(Config::get_instance()->m_loop_check_interval, true, [this](){
        m_hb_handler->loopCheck();
    });

    // 处理接收到的心跳包
    FdEvent* hb_recv_event = new FdEvent(m_hb_handler->getRecvFd());
    hb_recv_event->listen(FdEvent::IN_EVENT, [this] () {
        int next_track_number = m_hb_handler->recvMsg();
        char* ip = m_hb_handler->getWorkerAddr();
        if(ip != NULL) {
            m_worker_ip = ip;
        }
        if(m_ip != m_worker_ip && next_track_number != -1) {
            // 非工作机, 更新trackNum
            m_next_track_number = next_track_number;
        }
    });
    hb_recv_thread->getEventLoop()->addTimerEvent(hb_check_event);
    hb_recv_thread->getEventLoop()->addEpollEvent(hb_recv_event);
    // 心跳包线程初始化结束

    // 初始化融合线程
    int64_t fusion_time = 0;
    TimerEvent::s_ptr fusion_event = std::make_shared<TimerEvent>(Config::get_instance()->m_fusion_period, true, [this, &fusion_time](){
        this->repProcess(fusion_time);
        fusion_time += Config::get_instance()->m_fusion_period;
        // this->fusion();
    });
    m_main_event_loop->addTimerEvent(fusion_event);
    // 融合线程初始化结束
}

void Server::start() {
    m_thread_pool->start();
    m_main_event_loop->loop();
}

void Server::join() {
    m_thread_pool->join();
}

void Server::repRecv(unsigned char* buf, int port, int len) {
    std::list<RadarTrack> tracklist021; // 保存021类数据
    std::list<RadarTrack> tracklist001; // 保存001类数据
    switch (port)
    {
    case 10001: { // adsb 
        m_track021_decode.DecodeReport(buf, len);
        tracklist021 = m_track021_decode.GetReportList();
        std::unique_lock<std::mutex> lck(m_list_mtx);
        for (std::list<RadarTrack>::iterator ite = tracklist021.begin(); ite != tracklist021.end(); ++ite) {
            if (ite->SSR != 0 && ite->vec != 0)
            {
                ite->id = 1; 
                // 坐标转换
                double rho = 0.0, theta = 0.0;
                ite->TrackNo = ite->Address;
                // ite->currTime = currTime;
                rho = m_coord_trans.GetDistanceAg(ite->Lon, ite->Lat, &theta);
                ite->rho = rho;
                ite->theta = theta;
                // 将rho theta转化为fX, fY
                ite->fX = rho * sin(theta * PI / 180.0);
                ite->fY = rho * cos(theta * PI / 180.0);
                std::cout<<"fx = "<<ite->fX <<"   fy = "<<ite->fY<<std::endl;
                m_track_list.push_back(*ite);
            }
        }
        lck.unlock();
        break;
    }
    
    case 10002: { // dhz
        std::unique_lock<std::mutex> lck(m_list_mtx);
        m_track001_decode.DecodeReport(buf, len);
        tracklist001 = m_track001_decode.GetReportList();
        for (std::list<RadarTrack>::iterator ite = tracklist001.begin(); ite != tracklist001.end(); ++ite) {
            ite->id = 2; // dhz雷达
            // ite->currTime = currTime;
            // 计算目标相对于dhz雷达的X-Y坐标
            double target_dhz_x = ite->rho * sin(ite->theta * PI / 180.0);
            double target_dhz_y = ite->rho * cos(ite->theta * PI / 180.0);
            double theta_dhz = 0;
            double rho_dhz = m_coord_trans.GetDistanceAg(113.366667, 23.230556, &theta_dhz);
            double detaX = rho_dhz * sin(theta_dhz * PI / 180.0);
            double detaY = rho_dhz * cos(theta_dhz * PI / 180.0);
            ite->fX = target_dhz_x + detaX;
            ite->fY = target_dhz_y + detaY;
            m_track_list.push_back(*ite);
        }
        lck.unlock();
        break;
    }
    
    case 10003: {
        std::unique_lock<std::mutex> lck(m_list_mtx);
        m_track001_decode.DecodeReport(buf, len);
        tracklist001 = m_track001_decode.GetReportList();
        for (std::list<RadarTrack>::iterator ite = tracklist001.begin(); ite != tracklist001.end(); ++ite) {
            // 因为融合中心为fhs, 因此不需要进行坐标转换
            ite->id = 3; // fhs雷达
            // ite->currTime = currTime;
            m_track_list.push_back(*ite);
        }
        lck.unlock();
        break;
    }

    default:
        break;
    }
}

void Server::repRecvSingle(unsigned char* buf, int len, int port) {
    if(len <= 0) {
        return;
    }
    
    int rep_recv_port = Config::get_instance()->m_receive_ports[0];
    int fusion_unit_port = Config::get_instance()->m_fusion_unit_group_port;
    if(port == rep_recv_port) {
        // unsigned int detaT = (int)buf[len-1] * 16777216 + (int)buf[len-2] * 65536 + (int)buf[len-3] * 256 + (int)buf[len-4]; // us
        // detaT /= 1000; // 转化成毫秒
        len -= 4;
        unsigned char rep_buf[200];
        memset(rep_buf, 0, 200);
        memcpy(rep_buf, buf, len);
        int64_t curr_time = getNowMs();
        if ((int)(rep_buf[0]) == 21) {
            m_track021_decode.DecodeReport(rep_buf, len);
            std::list<RadarTrack> tracklist021 = m_track021_decode.GetReportList();
            std::unique_lock<std::mutex> trk_list_lck(m_list_mtx);
            int size = 0;
            for (std::list<RadarTrack>::iterator ite = tracklist021.begin(); ite != tracklist021.end(); ++ite) {
                ++size;
                if (ite->SSR != 0 && ite->vec != 0) {
                    ite->id = 1; 
                    // 坐标转换
                    ite->TrackNo = ite->Address;
                    ite->currTime = curr_time;
                    ite->rho = m_coord_trans.GetDistanceAg(ite->Lon, ite->Lat, &ite->theta);
                    // 将rho theta转化为fX, fY
                    ite->fX = ite->rho * sin(ite->theta * PI / 180.0);
                    ite->fY = ite->rho * cos(ite->theta * PI / 180.0);
                    APPDEBUGLOG("adsb: receive and decode success, trk_no: %lld, fx: %.4f, fy: %.4f\n", ite->TrackNo, ite->fX, ite->fY);
                    m_track_list.push_back(*ite);
                    // m_tff_vec.push_back(trkRepToTrkForFusion(*ite));
                }
            }
            trk_list_lck.unlock();
        } else if(rep_buf[0] == 0x01) {
            m_track001_decode.DecodeReport(rep_buf, len);
            std::list<RadarTrack> tracklist001 = m_track001_decode.GetReportList();
            std::unique_lock<std::mutex> trk_list_lck(m_list_mtx);
            for (std::list<RadarTrack>::iterator ite = tracklist001.begin(); ite != tracklist001.end(); ++ite) {
                if ((int)(rep_buf[2]) < 50) { // dhz
                    ite->id = 2; // dhz雷达
                    ite->currTime = curr_time;
                    // 计算目标相对于dhz雷达的X-Y坐标
                    double target_dhz_x = ite->rho * sin(ite->theta * PI / 180.0);
                    double target_dhz_y = ite->rho * cos(ite->theta * PI / 180.0);
                    double theta_dhz = 0;
                    double rho_dhz = m_coord_trans.GetDistanceAg(113.366667, 23.230556, &theta_dhz);
                    double detaX = rho_dhz * sin(theta_dhz * PI / 180.0);
                    double detaY = rho_dhz * cos(theta_dhz * PI / 180.0);
                    ite->fX = target_dhz_x + detaX;
                    ite->fY = target_dhz_y + detaY;
                    APPDEBUGLOG("dhz: receive and decode success, trk_no: %lld, fx: %.4f, fy: %.4f", ite->TrackNo, ite->fX, ite->fY);
                    m_track_list.push_back(*ite);
                    // m_tff_vec.push_back(trkRepToTrkForFusion(*ite));
                } else { // fhs
                    ite->id = 3; // fhs雷达
                    ite->currTime = curr_time;
                    APPDEBUGLOG("fhs: receive and decode success, trk_no: %lld, fx: %.4f, fy: %.4f", ite->TrackNo, ite->fX, ite->fY);
                    m_track_list.push_back(*ite);
                    // m_tff_vec.push_back(trkRepToTrkForFusion(*ite));
                }
            }
            trk_list_lck.unlock();
        }
    } else if(port == fusion_unit_port) {
        if(m_ip == m_worker_ip) {
            // 工作机不需要重复接收融合数据
            return;
        }
        std::string fusion_unit_str(reinterpret_cast<const char*>(buf), len);
        std::unique_lock<std::mutex> fusion_list_lck(m_fusion_mtx);
        m_fusion_unit_str_vec.push_back(fusion_unit_str);
        fusion_list_lck.unlock();
    }
}

void Server::repProcess(int64_t fusion_time) {
    std::list<RadarTrack> tmp_list;
    std::unique_lock<std::mutex> trk_list_lck(m_list_mtx);
    tmp_list.swap(m_track_list);
    trk_list_lck.unlock();
    // INFOLOG("fusion time: %lld", fusion_time);
    printf("next track number: %d, generated by server: %s, this server: %s\n", m_next_track_number, m_worker_ip.c_str(), m_ip.c_str());
    // 不是工作机, 不启动融合程序, 对齐系统航迹列表的trackNo, 更新融合航迹列表
    if(m_ip != m_worker_ip) {
        // 更新下一个新建的航迹号
        while(m_next_track_number > 0 && m_next_track_number != m_fusion.getNextTrackNum()) {
            m_fusion.dropTrackNum();
        }

        // 更新融合航迹列表
        std::vector<std::string> tmp_vec;
        std::unique_lock<std::mutex> fusion_list_lck(m_fusion_mtx);
        tmp_vec.swap(m_fusion_unit_str_vec);
        fusion_list_lck.unlock();
        for(int i = 0; i < tmp_vec.size(); ++i) {
            FusionUnit fusion_unit = bufStringToFusionUnit(tmp_vec[i]);
            m_fusion.updateFusionUnitVec(fusion_unit);
        }
        return;
    }

    if(m_fusion.m_sys_time == 0) {
        m_fusion.m_sys_time = getNowMs();
    } else {
        m_fusion.m_sys_time += Config::get_instance()->m_fusion_period;
    }
    // 装填航迹；新建/更新单元航迹
    m_fusion.updateUnitTrack(tmp_list);
    // 删除3个周期还没有更新的点，对本周期未更新的点进行外推
    m_fusion.delAndExtra(fusion_time);
    // 单源航迹转存到 unitTrack_data 中，后续按不同的雷达发送到显示程序
    m_fusion.saveUnitTrack();
    // 关联+融合
    m_fusion.associaFusion();
    // 融合结果转存到 fusionRetList
    m_fusion.getFusionRet(); 
    // 获取下一个trackNum
    m_next_track_number = m_fusion.getNextTrackNum();

    // 融合单元发送给其他服务器
    for(int i = 0; i < m_fusion.fusionUnitVec.size(); ++i) {
        FusionUnit fusion_unit = m_fusion.fusionUnitVec[i];
        std::string fusion_unit_str = fusionUnitToBufString(fusion_unit);
        m_send_socks[4]->SendData((unsigned char*)fusion_unit_str.c_str(), fusion_unit_str.length());
    }

    // 单源发送到显示程序
    while(!m_fusion.unitTrack_data.empty()) {
        RadarTrack rtData = m_fusion.unitTrack_data.front();
        m_fusion.unitTrack_data.pop_front();

        memset(sdBuf, 0, 2000);
        int flystate = 1;
        double Height = (double)(rtData.Hei * 0.3048);
        memcpy(sdBuf, &(rtData.fX), 8);
        memcpy(sdBuf + 8, &(rtData.fY), 8);
        memcpy(sdBuf + 16, &Height, 8);
        memcpy(sdBuf + 24, &(rtData.vec), 8);
        memcpy(sdBuf + 32, &(rtData.TrackNo), 4);
        memcpy(sdBuf + 36, &(rtData.cource), 8);
        memcpy(sdBuf + 44, &flystate, 4);
        
        APPINFOLOG("[Send] RepUnitrack Info: trkno[%d] (%f, %f)", rtData.TrackNo, rtData.fX, rtData.fY);

        m_send_socks[rtData.id - 1]->SendData(sdBuf, 2000);
    }

    // 融合结果发送到显示程序
    // printf("m_fusion.fusionRetList.size(): %d\n", m_fusion.fusionRetList.size());
    while(!m_fusion.fusionRetList.empty()) {
        RadarTrack repFusion = m_fusion.fusionRetList.front();
        m_fusion.fusionRetList.pop_front();
        memset(sdBuf, 0, 2000);
        int flystate = 1;
        double Height = (double)(repFusion.Hei * 0.3048);
        memcpy(sdBuf, &(repFusion.fX), 8);
        memcpy(sdBuf + 8, &(repFusion.fY), 8);
        memcpy(sdBuf + 16, &Height, 8);
        memcpy(sdBuf + 24, &(repFusion.vec), 8);
        memcpy(sdBuf + 32, &(repFusion.TrackNo), 4);
        memcpy(sdBuf + 36, &(repFusion.cource), 8);
        memcpy(sdBuf + 44, &flystate, 4);

        APPINFOLOG("[Send] RepFusion Info: trkno[%d] (%f, %f)", repFusion.TrackNo, repFusion.fX, repFusion.fY);

        m_send_socks[3]->SendData(sdBuf, 2000);
    }
}

void Server::fusion() {
    // printf("next track number: %d, generated by server: %s, this server: %s\n", m_next_track_number, m_worker_ip.c_str(), m_ip.c_str());
    std::unique_lock<std::mutex> lck(m_list_mtx);
    std::vector<TrkForFushion> tmp_vec;
    tmp_vec.swap(m_tff_vec);
    lck.unlock();
    // 装填航迹
    m_multi_fusion.feedTrack(tmp_vec);
    // 融合处理
    m_multi_fusion.process();

    // 单源航迹发送
    for(auto it = m_multi_fusion.m_uni_trks.begin(); it != m_multi_fusion.m_uni_trks.end(); it++) {
        std::map<int64_t, UnikTrk> &tmp_map = it->second;
        for(auto ite = tmp_map.begin(); ite != tmp_map.end(); ite++) {
            if(!ite->second.is_valid || ite->second.is_extra) continue;

            int flystate = 1;
            memset(sdBuf, 0, 2000);

            memcpy(sdBuf, &(ite->second.x), 8);
            memcpy(sdBuf + 8, &(ite->second.y), 8);
            memcpy(sdBuf + 16, &(ite->second.z), 8);
            memcpy(sdBuf + 24, &(ite->second.speed), 8);
            memcpy(sdBuf + 32, &(ite->second.trk_no), 4);
            memcpy(sdBuf + 36, &(ite->second.heading), 8);
            memcpy(sdBuf + 44, &flystate, 4);

            m_send_socks[it->first - 1]->SendData(sdBuf, 2000);
        }
    }

    // 融合航迹发送
    for(auto it = m_multi_fusion.m_sys_trks.begin(); it != m_multi_fusion.m_sys_trks.end(); it++) {
        double heading = Head(it->second.vX, it->second.vY);
        double speed = Speed(it->second.vX, it->second.vY, 0);
        int flystate = 1;

        memset(sdBuf, 0, 2000);

        memcpy(sdBuf, &(it->second.X), 8);
        memcpy(sdBuf + 8, &(it->second.Y), 8);
        memcpy(sdBuf + 16, &(it->second.Z), 8);
        memcpy(sdBuf + 24, &speed, 8);
        memcpy(sdBuf + 32, &(it->second.systrk_no), 4);
        memcpy(sdBuf + 36, &heading, 8);
        memcpy(sdBuf + 44, &flystate, 4);

        m_send_socks[3]->SendData(sdBuf, 2000);
    }
}

void Server::repSend() {

}

std::string Server::fusionUnitToBufString(FusionUnit& fusion_unit) {
    // 设置设置 fusion_unit_message 的成员变量
    FusionUnitMessage fusion_unit_message;
    fusion_unit_message.set_time(fusion_unit.time);
    fusion_unit_message.set_flag(fusion_unit.flag);
    fusion_unit_message.set_newtrackno(fusion_unit.newTrackNo);

    // 设置assmap
    google::protobuf::Map<int32_t, AssTrackMessage>* assmap_message = fusion_unit_message.mutable_assmap();
    for(auto it = fusion_unit.assMap.begin(); it != fusion_unit.assMap.end(); it++) {
        int radar_id = it->first;
        AssTrack& ass_track = it->second;

        // 设置asstrack
        AssTrackMessage ass_track_message;
        ass_track_message.set_weight(ass_track.weight);
        ass_track_message.set_unittracktermcount(ass_track.unitTrackTermCount); 
        // printf("fusionUnitToBufString, assmp: trkno: %d, radar_id: %d, weight: %f, unitTrackTermCount: %d\n", 
        // fusion_unit_message.newtrackno(), radar_id, ass_track_message.weight(), ass_track_message.unittracktermcount());
        // printf("fusionUnitToBufString, assmp: trkno: %d, radar_id: %d, weight: %f, unitTrackTermCount: %d\n", 
        // fusion_unit.newTrackNo, radar_id, ass_track.weight, ass_track.unitTrackTermCount);
        for(auto ite = ass_track.unitTrackVec.begin(); ite != ass_track.unitTrackVec.end(); ite++) {
            RadarTrackMessage* radar_trk_message = ass_track_message.add_unittrackvec();
            radar_trk_message->set_id(ite->id);
            radar_trk_message->set_address(ite->Address);
            radar_trk_message->set_trackno(ite->TrackNo);
            radar_trk_message->set_ssr(ite->SSR);
            radar_trk_message->set_callno(ite->callNo);
            radar_trk_message->set_fx(ite->fX);
            radar_trk_message->set_fy(ite->fY);
            radar_trk_message->set_xyflg(ite->xyflg);
            radar_trk_message->set_rho(ite->rho);
            radar_trk_message->set_theta(ite->theta);
            radar_trk_message->set_rtflg(ite->rtflg);
            radar_trk_message->set_hei(ite->Hei);
            radar_trk_message->set_lon(ite->Lon);
            radar_trk_message->set_lat(ite->Lat);
            radar_trk_message->set_vec(ite->vec);
            radar_trk_message->set_cource(ite->cource);
            radar_trk_message->set_vz(ite->vz);
            radar_trk_message->set_time(ite->Time);
            radar_trk_message->set_currtime(ite->currTime);
            radar_trk_message->set_extracount(ite->extraCount);
            radar_trk_message->set_afterextrat(ite->afterExtraT);
        }
        (*assmap_message)[radar_id] = ass_track_message;
    }

    // 设置fret
    FusionRetMessage* fusion_ret_message = fusion_unit_message.mutable_fret();
    fusion_ret_message->set_fx(fusion_unit.fRet.fX);
    fusion_ret_message->set_fy(fusion_unit.fRet.fY);
    fusion_ret_message->set_fhead(fusion_unit.fRet.fHead);
    fusion_ret_message->set_fhei(fusion_unit.fRet.fHei);
    fusion_ret_message->set_fv(fusion_unit.fRet.fV);
    fusion_ret_message->set_state(fusion_unit.fRet.state);
    fusion_ret_message->set_ssr(fusion_unit.fRet.SSR);
    fusion_ret_message->set_id(fusion_unit.fRet.id);
    fusion_ret_message->set_address(fusion_unit.fRet.Address);
    fusion_ret_message->set_currtime(fusion_unit.fRet.currTime);
    fusion_ret_message->set_callno(fusion_unit.fRet.callNo);

    std::string fusion_unit_string;
    fusion_unit_message.SerializeToString(&fusion_unit_string);
    return fusion_unit_string;
}

FusionUnit Server::bufStringToFusionUnit(std::string fusion_unit_string) {
    FusionUnitMessage fusion_unit_message;
    fusion_unit_message.ParseFromString(fusion_unit_string);

    FusionUnit fusion_unit;
    fusion_unit.initInstance();
    fusion_unit.time = fusion_unit_message.time();
    fusion_unit.flag = fusion_unit_message.flag();
    fusion_unit.newTrackNo = fusion_unit_message.newtrackno();

    const google::protobuf::Map<int32_t, AssTrackMessage>& assmap_message = fusion_unit_message.assmap();
    for(auto it = assmap_message.begin(); it != assmap_message.end(); it++) {
        int radar_id = it->first;
        const AssTrackMessage& ass_track_message = it->second;

        // 设置asstrack
        AssTrack ass_track;
        ass_track.weight = ass_track_message.weight();
        ass_track.unitTrackTermCount = ass_track_message.unittracktermcount();
        // printf("bufStringToFusionUnit, assmp: trkno: %d, radar_id: %d, weight: %f, unitTrackTermCount: %d\n", 
        // fusion_unit.newTrackNo, it->first, ass_track.weight, ass_track.unitTrackTermCount);
        const google::protobuf::RepeatedPtrField<RadarTrackMessage>& unit_trackvec_message = ass_track_message.unittrackvec();
        for(auto ite = unit_trackvec_message.begin(); ite != unit_trackvec_message.end(); ite++) {
            RadarTrack radar_trk;
            radar_trk.id = ite->id();
            radar_trk.Address = ite->address();
            radar_trk.TrackNo = ite->trackno();
            radar_trk.SSR = ite->ssr();
            radar_trk.callNo = ite->callno();
            radar_trk.fX = ite->fx();
            radar_trk.fY = ite->fy();
            radar_trk.xyflg = ite->xyflg();
            radar_trk.rho = ite->rho();
            radar_trk.theta = ite->theta();
            radar_trk.rtflg = ite->rtflg();
            radar_trk.Hei = ite->hei();
            radar_trk.Lon = ite->lon();
            radar_trk.Lat = ite->lat();
            radar_trk.vec = ite->vec();
            radar_trk.cource = ite->cource();
            radar_trk.vz = ite->vz();
            radar_trk.Time = ite->time();
            radar_trk.currTime = ite->currtime();
            radar_trk.extraCount = ite->extracount();
            radar_trk.afterExtraT = ite->afterextrat();
            ass_track.unitTrackVec.push_back(radar_trk);
        }
        fusion_unit.assMap[radar_id] = ass_track;
    }

    // 设置fret
    const FusionRetMessage &fusion_ret_message = fusion_unit_message.fret();
    fusion_unit.fRet.fX = fusion_ret_message.fx();
    fusion_unit.fRet.fY = fusion_ret_message.fy();
    fusion_unit.fRet.fHead = fusion_ret_message.fhead();
    fusion_unit.fRet.fHei = fusion_ret_message.fhei();
    fusion_unit.fRet.fV = fusion_ret_message.fv();
    fusion_unit.fRet.state = fusion_ret_message.state();
    fusion_unit.fRet.SSR = fusion_ret_message.ssr();
    fusion_unit.fRet.id = fusion_ret_message.id();
    fusion_unit.fRet.Address = fusion_ret_message.address();
    fusion_unit.fRet.currTime = fusion_ret_message.currtime();
    fusion_unit.fRet.callNo = fusion_ret_message.callno();

    return fusion_unit;
}

TrkForFushion Server::trkRepToTrkForFusion(RadarTrack& trk_rep) {
    TrkForFushion tff;
    // tff.radar_trk = trk_rep;
    tff.radar_id = trk_rep.id;
    tff.trk_no = trk_rep.TrackNo;
    tff.recv_timestamp = trk_rep.currTime;
    tff.x = trk_rep.fX;
    tff.y = trk_rep.fY;
    tff.z = trk_rep.Hei;
    tff.vx = trk_rep.vec * sin(trk_rep.cource * PI / 180);
    tff.vy = trk_rep.vec * cos(trk_rep.cource * PI / 180);
    tff.vz = trk_rep.vz;
    tff.speed = trk_rep.vec;
    tff.heading = trk_rep.cource;
    tff.pitching = Pitch(tff.vx, tff.vy, tff.vz);
    return tff;
}