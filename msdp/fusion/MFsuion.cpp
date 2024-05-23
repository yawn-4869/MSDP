#include <sys/time.h>
#include <time.h>
#include <sstream>
#include "MFusion.h"
#include "Logger.h"
#include "utils.h"
#include "AlgorithmTool.h"

MFusion::MFusion()
{
    for(int i = 0;i<2000;++i) {
        sysTrackNoL.push_back(i+1);
    }
    init();
}

MFusion::~MFusion()
{
}

void MFusion::init() {
    m_sys_time = getNowMs();
}

// unitTrack 单源列表 动态更新，每个周期都会更新最新的数据，没有新数据就外推
void MFusion::updateUnitTrack(std::list<RadarTrack> tempRepList)
{
    for (auto it = tempRepList.begin(); it != tempRepList.end(); ++it)
    {
        if(unitTrack.count(it->id)) {
            if(unitTrack[it->id].count(it->TrackNo)) {
                unitTrack[it->id][it->TrackNo] = *it;
            } else {
                unitTrack[it->id].insert(std::make_pair(it->TrackNo, *it));
            }
        } else {
            std::map<int, RadarTrack> tmp_map;
            tmp_map.insert(std::make_pair(it->TrackNo, *it));
            unitTrack.insert(std::make_pair(it->id, tmp_map));
        }
    }
}

// 删除3个周期还没有更新的点，对本周期未更新的点进行外推
void MFusion::delAndExtra(int64_t fusion_time)
{
    for (auto it = unitTrack.begin(); it != unitTrack.end(); ++it)
    {
        for (auto ite = it->second.begin(); ite != it->second.end();)
        {
            // int64_t curr_time = ite->second.currTime;
            // int64_t detaT = m_sys_time - curr_time;
            // if (PREMIT_EXTRA_COUNT <= ite->second.extraCount) // 删除过期的点
            // {
            //     // 先把融合列表里面对应的 it->first 和 ite->second.trackNo 初始化
            //     iniFusionUnitTrack(it->first, ite->second.TrackNo);
            //     ite = it->second.erase(ite);
            // }else if (detaT >= Config::get_instance()->m_fusion_period) {
            //     // 本周期没有数据更新，需要进行外推
            //     // 外推后的坐标
            //     detaT = Config::get_instance()->m_fusion_period;
            //     ite->second.extraCount++;
            //     ite->second.currTime = curr_time + detaT;
            //     double vx = ite->second.vec * sin(ite->second.cource * PI / 180);
            //     double vy = ite->second.vec * cos(ite->second.cource * PI / 180);
            //     double detaX = detaT / 1000 * vx;
            //     double detaY = detaT / 1000 * vy;
            //     double detaZ = detaT / 1000 * ite->second.vz / 60;
            //     ite->second.fX += detaX;
            //     ite->second.fY += detaY;
            //     ite->second.Hei += detaZ;
            //     ite++;
            // } else {
            //     // 本周期正常更新
            //     // 清空之前的extra
            //     ite->second.extraCount = 0;
            //     ite->second.afterExtraT = 0;
            //     ite++;
            // }
            if (PREMIT_EXTRA_COUNT <= ite->second.extraCount) // 删除过期的点
            {
                // 先把融合列表里面对应的 it->first 和 ite->second.trackNo 初始化
                iniFusionUnitTrack(it->first, ite->second.TrackNo);
                ite = it->second.erase(ite);
                m_associate_map[it->first].erase(ite->second.TrackNo);
            } else {
                // 外推后的坐标
                int64_t curr_time = ite->second.currTime;
                int64_t detaT = m_sys_time - curr_time;
                if(detaT >= Config::get_instance()->m_fusion_period) {
                    // 本周期没有数据更新，需要进行外推
                    ite->second.extraCount++;
                    ite->second.afterExtraT = m_sys_time;
                } else {
                    // 本周期正常更新
                    // 清空之前的extra
                    ite->second.extraCount = 0;
                    ite->second.afterExtraT = 0;
                }

                // 时间戳与系统时间对齐
                // detaT = Config::get_instance()->m_fusion_period;
                ite->second.currTime = m_sys_time;
                double vx = ite->second.vec * sin(ite->second.cource * PI / 180);
                double vy = ite->second.vec * cos(ite->second.cource * PI / 180);
                double detaX = detaT / 1000 * vx;
                double detaY = detaT / 1000 * vy;
                double detaZ = detaT / 1000 * ite->second.vz / 60;
                ite->second.fX += detaX;
                ite->second.fY += detaY;
                ite->second.Hei += detaZ;
                ite++;
            }
            // } else {
            //     // 本周期正常更新
            //     // 清空之前的extra
            //     ite->second.extraCount = 0;
            //     ite->second.afterExtraT = 0;
            //     ite++;
            // }
        }
    }
}
// 删除（初始化）过期的融合单源航迹
void MFusion::iniFusionUnitTrack( int id, int trackNo)
{
    for (auto it = fusionUnits.begin(); it != fusionUnits.end(); ++it)
    {
        if (it->second.assMap[id].unitTrackVec.back().TrackNo == trackNo)
        {
            RadarTrack rt;
            rt.InitInstance();
            it->second.assMap[id].unitTrackVec.push_back(rt);
            it->second.assMap[id].weight = 0;
        }
    }
    // 不能直接删除，还需要根据权重慢慢降下来
    // 外推3个周期，却要在后续10个周期内降权重为0？如果转弯的时候没有收到报告，那按其前面的点进行外推肯定不对？
}
// 单源航迹转存到 unitTrack_data 中，后续按不同的雷达发送到显示程序
void MFusion::saveUnitTrack()
{
    for (auto it = unitTrack.begin(); it != unitTrack.end(); ++it)
    {
        for (auto ite = it->second.begin(); ite != it->second.end(); ++ite)
        {
            unitTrack_data.push_back(ite->second);
        }
    }
}
// 关联+融合
void MFusion::associaFusion()
{
    // 3路雷达报告
    for (int radarNo = 1; radarNo <= RADAR_NO + 1; ++radarNo) {
        for (auto it = unitTrack[radarNo].begin(); it != unitTrack[radarNo].end(); ++it) {
            // 查找系统航迹中是否已经有这个 trackNo
            bool related = false;
            // for (auto ite = fusionUnitVec.begin(); ite != fusionUnitVec.end(); ++ite) {
            for (auto ite = fusionUnits.begin(); ite != fusionUnits.end(); ++ite) {
                // 系统航迹中有雷达 i 的  ite->arrRT[i].TrackNo 号航迹，直接更新此航迹
                // if (it->first == ite->assMap[radarNo].unitTrackVec.back().TrackNo) {
                if (it->first == ite->second.assMap[radarNo].unitTrackVec.back().TrackNo) {
                    // associaTrack(*ite, it->second, radarNo, 1);
                    associaTrack(ite->second, it->second, radarNo, 1);
                    related = true;
                    break;
                // } else if (ite->assMap[radarNo].unitTrackVec.back().TrackNo == -1) {
                } else if (ite->second.assMap[radarNo].unitTrackVec.back().TrackNo == -1) {
                    // 系统航迹中有雷达 i 为空的航迹，可能需要关联
                    double dis = getDis(it->second, ite->second.fRet);
                    // double dis = Distance(it->second.fX, it->second.fY, ite->second.fRet.fX, it->second.fY);
                    if (dis < DIS_THRESHOLD)
                    {
                        // 需要关联
                        // associaTrack(*ite, it->second, radarNo, 0);
                        associaTrack(ite->second, it->second, radarNo, 1);
                        related = true;
                        break;
                    }
                }
            }
            if (!related) {
                // 如果附近有 SSR 相同的航迹，则不要新建系统航迹
                // if (SSREixt(fusionUnitVec, it->second))
                if (SSREixt( it->second))
                    continue;
                else
                    newSysTrack(it->second,  radarNo);
            }
        }
    }

    // 其他服务器的离线报告
    for (auto it = unitTrack[RADAR_NO+1].begin(); it != unitTrack[RADAR_NO+1].end(); ++it) {
        bool related = false;
        for (auto ite = fusionUnits.begin(); ite != fusionUnits.end(); ++ite) {
            // 根据距离计算是否关联
            double dis = getDis(it->second, ite->second.fRet);
            if (dis < DIS_THRESHOLD)
            {
                // 需要关联
                // associaTrack(*ite, it->second, radarNo, 0);
                // associaTrack(ite->second, it->second, RADAR_NO+1, 1);
                related = true;
                break;
            }
        }
        if (!related) {
            // 如果附近有 SSR 相同的航迹，则不要新建系统航迹
            // if (SSREixt(fusionUnitVec, it->second))
            if (SSREixt( it->second)) {
                continue;
            } else {
                // 关联失败, 为新的航迹, 建航
                newSysTrack(it->second,  4);
            }
        }
    }
}

void MFusion::newSysTrack(RadarTrack rt, int radarNo)
{
    // 新建系统航迹
    FusionUnit fusUnit;
    fusUnit.initInstance();
    fusUnit.time=rt.currTime;
    fusUnit.newTrackNo = sysTrackNoL.front();
    sysTrackNoL.pop_front();
    ++fusUnit.assMap[radarNo].unitTrackTermCount;
    fusUnit.assMap[radarNo].unitTrackVec.back() = rt;
    fusUnit.assMap[radarNo].weight = 1; // 新建的融合航迹只关联了一个单源航迹，将权重赋值为 1

    fusUnit.fRet.id = rt.id;
    fusUnit.fRet.fX = rt.fX;
    fusUnit.fRet.fY = rt.fY;
    fusUnit.fRet.fHei = rt.Hei;
    fusUnit.fRet.fV = rt.vec;
    fusUnit.fRet.fHead = rt.cource;
    fusUnit.fRet.SSR = rt.SSR;
    fusUnit.fRet.currTime = rt.currTime;

    // fusionUnitVec.push_back(fusUnit);
    fusionUnits[fusUnit.newTrackNo] = fusUnit;
    m_associate_map[radarNo][rt.TrackNo] = fusUnit.newTrackNo;
    APPDEBUGLOG(" [Fusion] new systrk create, systrk_no[%lld] id[%d] unitrk_no[%lld] (%.4f, %.4f)", fusUnit.newTrackNo, fusUnit.fRet.id, rt.TrackNo, fusUnit.fRet.fX, fusUnit.fRet.fX);
}
// newRT 加入到 fu
void MFusion::associaTrack(FusionUnit &fu, RadarTrack newRt, int radarNo, int flag)
{
    int detaTime = 0;
    // 更新系统航迹时间
    if (fu.time < newRt.currTime)
        fu.time = newRt.currTime;
    // fu.time = m_sys_time;
    ++fu.assMap[radarNo].unitTrackTermCount;
    // if(newRt.TrackNo == 2781) {
    //     printf("flag: %d, pos(%f, %f)\n", flag, newRt.fX, newRt.fY);
    // }

    if (flag) {
        // 系统航迹中有雷达 i 的  ite->arrRT[i].TrackNo 号航迹，直接更新此航迹
        // 计算不同雷达数据的权重
        fu.assMap[radarNo].unitTrackVec.push_back(newRt);
        int extraCount = fu.assMap[radarNo].unitTrackVec.back().extraCount;
        if (extraCount > 0)  {
            // 外推得到的单源航迹
            fu.assMap[radarNo].weight *= (MAX_HIS_TERM - extraCount) / (MAX_HIS_TERM * 1.0);
        } else {
            int size = fu.assMap[radarNo].unitTrackVec.size();
            RadarTrack lastRT = fu.assMap[radarNo].unitTrackVec[size - 2];
            RadarTrack currRT = fu.assMap[radarNo].unitTrackVec[size - 1];
            // if (currRT.extraCount > 0)
            //     detaTime = Config::get_instance()->m_fusion_period;
            // else if (lastRT.extraCount > 0) {
            //     detaTime = currRT.currTime - lastRT.afterExtraT;
            // } else {
            //     detaTime = currRT.currTime - lastRT.currTime;
            // }

            detaTime = currRT.currTime - lastRT.currTime;
            
            // printf("lastRT.extraCount:%d, detaTime:%lld\n", lastRT.extraCount, detaTime);
            RadarTrack predRT = getPredRT(lastRT, detaTime);
            double dis = detaDis(currRT, predRT);
            // 需要修改 计算最近10个周期的 1/detaR 才对？
            if(dis > 1e-6) {
                // 上面的外推过程可能存在问题，会有重复的报点导致dis为0, 需要跳过这种情况
                fu.assMap[radarNo].weight = (fu.assMap[radarNo].weight * (size - 1) + 1 / dis) / size;
            }
            if(std::isinf(fu.assMap[radarNo].weight)) {
                printf("dis: %f, size: %d, detaTime: %d, currRT.extraCount: %d, lastRT.extraCount: %d, currRT.currTime: %lld, lastRT.afterExtraT: %lld\n", 
                dis, size, detaTime, currRT.extraCount, lastRT.extraCount, currRT.currTime, lastRT.afterExtraT);
            }
        }
    } else {
        // 对系统航迹中雷达 i 的单源航迹为空的情况进行关联
        fu.assMap[radarNo].unitTrackVec.back() = newRt;
        bool isFirst = true;
        for (int radarNo = 1; radarNo <= RADAR_NO + 1; ++radarNo) {
            if (fu.assMap[radarNo].unitTrackVec.size() > 1) {
                isFirst = false;
                break;
            }
        }
        // 针对融合航迹的第一个周期
        //如果本融合航迹是新建的（第一个融合周期），编号为 radarNo 的数据新加入后权重和已有的单源航迹权重相等，同样赋值为 1
        if (isFirst)
            fu.assMap[radarNo].weight = 1;
        else
            fu.assMap[radarNo].weight = 0;
    }

    // 只保留最近 MAX_HIS_TERM （10）个周期的历史
    if (fu.assMap[radarNo].unitTrackVec.size() > MAX_HIS_TERM)
        fu.assMap[radarNo].unitTrackVec.erase(fu.assMap[radarNo].unitTrackVec.begin());
    // prtRTvec(fu.assMap[radarNo].unitTrackVec);

    // 重新计算融合结果
    RadarTrack tmpRt;
    double weight = 0, sumX = 0, sumY = 0, sumHei = 0, sumVec = 0, sumCource = 0, sumW = 0;
    for (int radarNo = 1; radarNo <= RADAR_NO; ++radarNo) {
        weight = fu.assMap[radarNo].weight;
        tmpRt = fu.assMap[radarNo].unitTrackVec.back();
        if(tmpRt.TrackNo == -1) {
            continue;
        }

        sumX += tmpRt.fX * weight;
        sumY += tmpRt.fY * weight;
        sumHei += tmpRt.Hei * weight;
        sumVec += tmpRt.vec * weight;
        sumCource += tmpRt.cource * weight;
        sumW += weight;
    }
    fu.fRet.fX = sumX / sumW;
    fu.fRet.fY = sumY / sumW;
    fu.fRet.fHei = sumHei / sumW;
    fu.fRet.fV = sumVec / sumW;
    fu.fRet.fHead = sumCource / sumW;
    fu.fRet.SSR = newRt.SSR;
    fu.fRet.currTime = newRt.currTime;
}

bool MFusion::SSREixt( RadarTrack rt)
{
    double detaX = 0, detaY = 0, detaH = 0, dis = 0;
    // for (auto it = fusionUnitVec.begin(); it != fusionUnitVec.end(); ++it)
    for (auto it = fusionUnits.begin(); it != fusionUnits.end(); ++it)
    {
        if (rt.SSR == it->second.fRet.SSR)
        {
            detaX = rt.fX - it->second.fRet.fX;
            detaY = rt.fY - it->second.fRet.fY;
            dis = sqrt(detaX * detaX + detaY * detaY);
            if (dis < NEAR_TRACK_THRESHOLD)
            {
                return true;
            }
        }
    }
    return false;
}
void MFusion::getFusionRet()
{
    for (auto it = fusionUnits.begin(); it != fusionUnits.end();)
    {
        bool valid = false;
        for (int radarNo = 1; radarNo <= RADAR_NO; ++radarNo) {
            // if (it->assMap[radarNo].unitTrackVec.back().TrackNo != -1) {
            if (it->second.assMap[radarNo].unitTrackVec.back().TrackNo != -1) {
                valid = true;
                break;
            }
        }
        
        if(!valid) {
            // it = fusionUnitVec.erase(it);
            it = fusionUnits.erase(it);
            continue;
        }

        RadarTrack rt;
        rt.InitInstance();
        rt.TrackNo = it->second.newTrackNo;
        if(m_associate_map[RADAR_NO+1].count(it->second.newTrackNo)) {
            rt.TrackNo = m_associate_map[RADAR_NO+1][it->second.newTrackNo];
        }
        rt.id = it->second.fRet.id;
        rt.fX = it->second.fRet.fX;
        rt.fY = it->second.fRet.fY;
        rt.Hei = it->second.fRet.fHei;
        rt.vec = it->second.fRet.fV;
        rt.cource = it->second.fRet.fHead;
        rt.SSR = it->second.fRet.SSR;
        rt.currTime = it->second.fRet.currTime;
        fusionRetList.push_back(rt);
        ++it;
    }
    // 打印日志
    // for (auto it = fusionUnitVec.begin(); it != fusionUnitVec.end(); ++it)
    for (auto it = fusionUnits.begin(); it != fusionUnits.end(); ++it)
    {
        for (int radarNo = 1; radarNo <= RADAR_NO; ++radarNo)
        // for (int radarNo = 1; radarNo <=  it->assMap.size(); ++radarNo)
        {
            std::stringstream ss;
            // for (auto ite = it->assMap[radarNo].unitTrackVec.begin(); ite != it->assMap[radarNo].unitTrackVec.end(); ++ite)
            for (auto ite = it->second.assMap[radarNo].unitTrackVec.begin(); ite != it->second.assMap[radarNo].unitTrackVec.end(); ++ite)
            {
                ss << ite->TrackNo << " (" << ite->fX << "," << ite->fY << ")";
                if (ite->extraCount > 0)
                    ss << "(extra:" << ite->extraCount << ")   ";
                else
                    ss << "   ";
            }
            // APPINFOLOG("[Fusion] SystemTrack: systrk_no[%d] (%f, %f) assMap[%d] unitTrackTermCount: %d weight: %f unitrackVec: %s", 
            // it->newTrackNo, it->fRet.fX, it->fRet.fY, radarNo, it->assMap[radarNo].unitTrackTermCount, it->assMap[radarNo].weight, ss.str().c_str());
            APPINFOLOG("[Fusion] SystemTrack: systrk_no[%d] (%f, %f) assMap[%d] unitTrackTermCount: %d weight: %f unitrackVec: %s", 
            it->second.newTrackNo, it->second.fRet.fX, it->second.fRet.fY, radarNo, it->second.assMap[radarNo].unitTrackTermCount, 
            it->second.assMap[radarNo].weight, ss.str().c_str());
        }
    }
}
double MFusion::getDis(RadarTrack tk, FusionRet ret)
{
    double detX = tk.fX - ret.fX;
    double detY = tk.fY - ret.fY;
    double detZ = tk.Hei * 0.3048 - ret.fHei * 0.3048;
    // return sqrt(detX * detX + detY * detY + detZ * detZ);
    return sqrt(detX * detX + detY * detY);
}
double MFusion::detaDis(RadarTrack lastRT, RadarTrack RT)
{
    double detX = lastRT.fX - RT.fX;
    double detY = lastRT.fY - RT.fY;
    double detZ = lastRT.Hei * 0.3048 - RT.Hei * 0.3048;
    return sqrt(detX * detX + detY * detY + detZ * detZ);
}
RadarTrack MFusion::getPredRT(RadarTrack lastRT, long detaTime)
{
    double vx = lastRT.vec * sin(lastRT.cource * PI / 180);
    double vy = lastRT.vec * cos(lastRT.cource * PI / 180);
    lastRT.fX += detaTime / 1000 * vx;
    lastRT.fY += detaTime / 1000 * vy;
    lastRT.Hei += detaTime / 1000 * lastRT.vz / 60;
    return lastRT;
}
void MFusion::prtRTvec(std::vector<RadarTrack> unitTrackVec)
{
    std::cout << "currTime :  ";
    for (auto ite = unitTrackVec.begin(); ite != unitTrackVec.end(); ++ite)
    {
        std::cout << ite->currTime << "  ";
    }
    std::cout << std::endl;
}

void MFusion::updateFusionUnits(FusionUnit& fusion_unit) {
    fusionUnits[fusion_unit.newTrackNo] = fusion_unit;
    for(int i = 1; i <= RADAR_NO + 1; ++i) {
        m_associate_map[i][fusion_unit.assMap[i].unitTrackVec.back().TrackNo] = fusion_unit.newTrackNo;
    }
}