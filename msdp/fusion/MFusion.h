#pragma once

#include <map>
#include <list>
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <fstream>
#include <mutex>
#include <math.h>
#include "defstruct.h"
#include "Config.h"
#define DIS_THRESHOLD 3000  // m
#define MAX_HIS_TERM 10 //计算权重误差的最大历史周期数
#define NEAR_TRACK_THRESHOLD 8000 // m
// #define PREMIT_EXTRA_COUNT 3       // 允许的最大外推周期数
#define PREMIT_EXTRA_COUNT 4       // 允许的最大外推周期数
#define RADAR_NO 3
#define WEI_CAL_CYC 10 // 权重计算周期

struct FusionRet
{
    double fX;
    double fY;
    double fHead;
    double fHei;
    double fV;
    int state;
    int SSR;
    int id;
    long Address;
    long currTime;
    std::string callNo;

public:
    void initInstance()
    {
        fX = 0.0;
        fY = 0.0;
        fHead = 0.0;
        fHei = 0.0;
        fV = 0.0;
        state = 0;
        SSR = 0;
        id = 0;
        Address = 0;
        callNo = "";
    }
};
struct AssTrack
{
    std::vector<RadarTrack> unitTrackVec;
    double weight;
    int unitTrackTermCount;
};

class FusionUnit
{
public:
    void initInstance()
    {
        time = 0;
        flag = 0;
        newTrackNo = 0;
        RadarTrack rt;
        rt.InitInstance();
        for (int radarNo = 1; radarNo <= RADAR_NO; ++radarNo)
        {
            assMap[radarNo].unitTrackVec.push_back(rt);
            assMap[radarNo].weight = 0;
            assMap[radarNo].unitTrackTermCount = 0;
        }
        fRet.initInstance();
    }

public:
    long time;
    int flag; // 为0代表融合列表
    int newTrackNo;
    std::map<int, AssTrack> assMap;//存储关联航迹以及权重
    FusionRet fRet;
};

class MFusion
{
public:
    MFusion();
    ~MFusion();
    void init();
    // 新建/更新单元航迹
    void updateUnitTrack(std::list<RadarTrack> tempRepList);
    // 删除过期的航迹
    void delAndExtra(int64_t fusion_time);
    //  初始化过期的融合单源航迹
    void iniFusionUnitTrack(int id, int trackNo);
    // 单源航迹转存到 unitTrack_data 中，后续按不同的雷达发送到显示程序
    void saveUnitTrack();
    // 关联+融合
    void associaFusion();
    void newSysTrack(RadarTrack rt,  int radarNo);
    void associaTrack(FusionUnit &fu, RadarTrack newRt, int radarNo, int flag);
    // 如果附近有 SSR 相同的航迹存在，就不能新建航迹
    bool SSREixt(RadarTrack rt);
    void getFusionRet();
    double getDis(RadarTrack tk, FusionRet ret);
    double detaDis(RadarTrack lastRT, RadarTrack RT);
    RadarTrack getPredRT(RadarTrack lastRT, long detaTime);
    void prtRTvec(std::vector<RadarTrack> unitTrackVec);
    // void updateFusionUnitVec(FusionUnit& fusion_unit);
    void updateFusionUnits(FusionUnit& fusion_unit);
    int getNextTrackNum() {
        return sysTrackNoL.front();
    }

public:
    std::list<int> sysTrackNoL;
    std::ofstream ofs_fusionRet;

	std::map<int, std::map<int, RadarTrack>> unitTrack; // 单元航迹 第一个int存储雷达雷达编号,第2个map的int存储trackNo
	std::list<RadarTrack> unitTrack_data;
	// std::vector<FusionUnit> fusionUnitVec;
    std::map<int, FusionUnit> fusionUnits;
	std::list<RadarTrack> fusionRetList; // 存储系统航迹(融合航迹)
    int64_t m_sys_time{ 0 };
    
    // // 上一周期位置
    // double lastX;
    // double lastY;
    // double lastZ;
    // // 上一周期速度
    // double lastVx;
    // double lastVy;
    // double lastVz;
    // // 本周期位置
    // double currX;
    // double currY;
    // double currZ;

};