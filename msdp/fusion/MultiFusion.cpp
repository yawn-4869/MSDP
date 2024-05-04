#include "MultiFusion.h"
#include "Config.h"
#include "CFit.h"
#include "Logger.h"

#define GET_ANGLE_NEED_NUM 3
#define INITIALMAXDIS 1000000

void UnikTrk::push(const TrkForFushion &trk) {
    // 接收到终止报文
	// if(trk.radar_trk.sFlagOfBatch==FINAL_REPORT){
	// 	isValid=false;
	// 	TotalCount=1000;
	// 	return;
	// }
	// 离群判断
	if(is_valid){
		double px = x + vx * Config::get_instance()->m_fusion_period / 1000.0;
		double py = y + vy * Config::get_instance()->m_fusion_period / 1000.0;
		double dis = Distance(trk.x, trk.y, 0, px, py, 0);
		if(dis > Config::get_instance()->m_stray_threshold) {
			APPDEBUGLOG(" [UNITRK] Stray track message: in radar: %d, unitrack %lld stray, distance: %f", trk.radar_id, trk.trk_no, dis);
			return;
		}
	}

	updateMovementData(trk); // 更新运动状态数据

	initReportStabilityValue(); // 初始化报告稳定性因子，在其未被初始化的条件下
	fit(); // 拟合
	// 起批：拟合度达到阈值且满足建航标准
	if(batch_fit > Config::get_instance()->m_fit_value_floor_limit && !is_valid && SuitToStart()){
		is_valid = true;
		state = INITTRACK;
	}
}

void UnikTrk::updateMovementData(const TrkForFushion &trk) {
    //正常报文
	// radar_trk = trk.radar_trk;
	recv_timestamp = trk.recv_timestamp;

	is_updated = true;//收到报告
	is_available_period = true;//此周期是有效的

	his_count++;
	Moveparam mp;
	mp.ts = trk.recv_timestamp;
	mp.x = trk.x;
	mp.y = trk.y;
	mp.z = trk.z;
	mp.vx = trk.vx;
	mp.vy = trk.vy;
	mp.vz = trk.vz;
	this->his_mp.push_back(mp);
	if(his_mp.size() > Config::get_instance()->m_fit_data_control){ 
		// 收到数据总量多于控制参数设定数量, his_map中只保留最近四个周期的数据
		his_mp.pop_front(); // 剔除最陈旧数据
	}
}

void UnikTrk::fit() {
    if(his_mp.size() < Config::get_instance()->m_fit_data_control) return;
	czy::Fit fitter;
	std::vector<double> param_t, param_x, param_y, param_z;
	for(auto it=his_mp.begin(); it!=his_mp.end(); it++){
		param_t.push_back(it->ts);
		param_x.push_back(it->x);
		param_y.push_back(it->y);
		param_z.push_back(it->z);
	}
    // 找到最开始的时间戳
	double relative_timestamp = *min_element(param_t.begin(), param_t.end());

    // 时间戳对齐
	for(int i=0; i < param_t.size(); i++){
		param_t[i] -= relative_timestamp;
	}

    // 计算x的线性拟合值
	fitter.polyfit(param_t, param_x, 1, false);
	double xfit = fitter.getR_square(); // 取拟合度
	x = fitter.getFactor(0) + fitter.getFactor(1) * param_t.back();
	ax = 0;
	vx = fitter.getFactor(1) + ax * param_t.back();

    // 计算y的线性拟合值
	fitter.polyfit(param_t, param_y, 1, false);
	double yfit = fitter.getR_square();
	y = fitter.getFactor(0) + fitter.getFactor(1) * param_t.back();
	ay = 0;
	vy = fitter.getFactor(1);

    // 计算z的线性拟合值
	fitter.polyfit(param_t, param_z, 1, false);
	double zfit = fitter.getR_square();
	z = fitter.getFactor(0) + fitter.getFactor(1) * param_t.back();
	az = 0;
	vz = fitter.getFactor(1);

	// if(fabs(radar_trk.sAngleOfPitch)<0.1||zfit<0.8){
    //     // 俯仰角小于0.1或拟合后的z值小于0.8, 判定高度不可用
	// 	is_hei_available=false;
	// }
	// else{
	// 	is_hei_available=true;
	// }
    if(zfit < 0.8) {
		// 拟合后的z值小于0.8, 判定高度不可用
        is_hei_available = false;
    } else {
        is_hei_available = true;
    }
	
	if (Config::get_instance()->m_use_report_stability_value) {	
		getReportStabilityValue();
	} else {
		report_stability_value = Config::get_instance()->m_report_stability_default_value;
	}

    fitter.polyfit(param_x, param_y, 1,false);
	batch_fit = fitter.getR_square();
	fit_value = std::min(xfit, yfit);
}

double UnikTrk::get_Q(const SystemTrk &systrk) {
    if(systrk.state==INITTRACK){
		if (Config::get_instance()->m_use_frequency_value) {
            //计算frequency因子，min-max or 1
			frequency = (double) his_count / total_count;
			frequency = frequency>Config::get_instance()->m_frequency_value_upper_limit?Config::get_instance()->m_frequency_value_upper_limit:frequency;
			frequency = frequency<Config::get_instance()->m_frequency_value_lower_limit?Config::get_instance()->m_frequency_value_lower_limit:frequency;
		} else {
			frequency = Config::get_instance()->m_frequency_default_value;
		}
		fit_value_ave = fit_value;
		close_value_ave = 0;
	} else if(systrk.state==STABLETRACK||systrk.state==EXTRATRACK){
		if (Config::get_instance()->m_use_frequency_value) {
			frequency = (double) his_count / total_count;
			frequency = frequency>Config::get_instance()->m_frequency_value_upper_limit?Config::get_instance()->m_frequency_value_upper_limit:frequency;
			frequency = frequency<Config::get_instance()->m_frequency_value_lower_limit?Config::get_instance()->m_frequency_value_lower_limit:frequency;
		} else {
			frequency = Config::get_instance()->m_frequency_default_value;
		}
		fit_value_ave=(fit_value_ave*(q_count-1)+fit_value)/q_count;
	    fit_value=fit_value_ave*fit_value_ave;
	}

	getCloseValue(systrk.X, systrk.Y);

	q_count++;
	Q = frequency + close_value + fit_value + report_stability_value; //Q为各个属性的和

	if(isf_afterpush && systrk.his_num>1)  //对于有两条航迹参与融合的情况 ，其中一条的外推之后的第一次报告q值减半，
		Q *= 0.5;
	if(close_value < 0.3)
		Q = 1;
	return Q;
}

void UnikTrk::getCloseValue(double sys_x, double sys_y) {
    double dis=Distance(x,y,0,sys_x,sys_y,0);
	close_value = Config::get_instance()->m_close_value_upper_limit*(1.0 - 1.0*dis/Config::get_instance()->m_stray_threshold);
	if (close_value < Config::get_instance()->m_close_value_lower_limit)
	{
		close_value = Config::get_instance()->m_close_value_lower_limit;
	}
}


bool UnikTrk::SuitToStart() {
    // 通过计算历史轨迹中各个运动数据航向差是否超过阈值来判定是否满足建航标准
    // TODO: 考虑增加速度稳定性 
	auto it = his_mp.begin();
	double now_head, last_head, delt_head;
	Moveparam lm,nm;
	while(it != (--his_mp.end()))
	{
		lm = *it;
		if(it == his_mp.begin()){
			nm = *(++it);
			last_head = Head(lm.x, lm.y, nm.x, nm.y);
		} else {
			nm = *(++it);
			now_head = Head(lm.x, lm.y, nm.x,nm.y);
			if(fabs(Dhead(now_head, last_head)) > Config::get_instance()->m_delt_angle_threshold)
				return false;
			last_head = now_head;
		}
	}
	return true;
}

void UnikTrk::initReportStabilityValue() {
    if (Config::get_instance()->m_use_report_stability_value && 
		(report_stability_value < Config::get_instance()->m_report_stability_value_lower_limit || 
		report_stability_value > Config::get_instance()->m_report_stability_value_upper_limit)) {
		report_stability_value = Config::get_instance()->m_report_stability_value_lower_limit;
	} else if (!Config::get_instance()->m_use_report_stability_value) {
		report_stability_value =Config::get_instance()->m_report_stability_default_value;
	}
}

float UnikTrk::getReportStabilityValue() {
    double included_angle = getUnitrkReportLatestHeadIncludedAngle();
	report_stability_value = (getReportStabilityValueFromFunc(included_angle) + report_stability_value) / 2;//20191122 redo 直线函数求值

	if(included_angle < Config::get_instance()->m_head_angle_trust_upper_limit){
		// 小角变动
		report_stability_value = std::min(report_stability_value, Config::get_instance()->m_report_stability_value_upper_limit);//最大置信度
	}else {
		// 大角度变动
		report_stability_value = std::max(report_stability_value , Config::get_instance()->m_report_stability_value_lower_limit);//最低置信度
	}
	return report_stability_value;
}

double UnikTrk::getUnitrkReportLatestHeadIncludedAngle() {
    if (his_mp.size() < GET_ANGLE_NEED_NUM)
	{
		return -1;
	}
	//20191008 redo 最近的历史三个点
	Moveparam k_back_1 = his_mp.back();//倒数第一个
	his_mp.pop_back();
	Moveparam k_back_2 = his_mp.back();//倒数第二个
	his_mp.pop_back();
	Moveparam k_back_3 = his_mp.back();//倒数第三个
	his_mp.push_back(k_back_2);
	his_mp.push_back(k_back_1);
	int64_t dt_1_2 = k_back_1.ts - k_back_2.ts;
	double lastVx = (k_back_1.x-k_back_2.x)/dt_1_2;
	double lastVy = (k_back_1.y -k_back_2.y)/dt_1_2;
	//20190930 redo 根据航向角，更新head_value 航向置信度 ，用于控制get_Q；历史上航向变化度越大，置信度越低
	double head_last = Head(lastVx,lastVx);
	double head_new = Head(this->vx,this->vy);
	return fabs(Dhead(head_last,head_new));
}

double UnikTrk::getReportStabilityValueFromFunc(double included_angle) {
    if (included_angle < 0) {
		return Config::get_instance()->m_report_stability_value_lower_limit;
	}
    double k = -1.0 / Config::get_instance()->m_turn_angle_threshold  * (Config::get_instance()->m_report_stability_value_upper_limit - Config::get_instance()->m_report_stability_value_lower_limit);
    double b = Config::get_instance()->m_report_stability_value_upper_limit;
    return (k*included_angle + b);
}

MultiFusion::MultiFusion() {
    init();
}

MultiFusion::~MultiFusion() {

}

void MultiFusion::init() {
    for(int i = 0; i < 2000; ++i) {
        m_tracknum_pool.push_back(i+1);
    }
}

void MultiFusion::feedTrack(std::vector<TrkForFushion>& trk_vec) {
    for(int i = 0; i < trk_vec.size(); i++){
        int trk_radar_id = trk_vec[i].radar_id;
		int64_t trk_number = trk_vec[i].trk_no;
        if(m_uni_trks.count(trk_radar_id)) {
            // 找到雷达
            if(m_uni_trks[trk_radar_id].count(trk_number)) {
                // 找到航迹
                m_uni_trks[trk_radar_id][trk_number].push(trk_vec[i]);
            } else {
                // 找不到航迹
                UnikTrk uni_trk(trk_vec[i]);
				uni_trk.push(trk_vec[i]);
                m_uni_trks[trk_radar_id][trk_number] = uni_trk;
            }
        } else {
            // 找不到雷达
            std::map<int64_t, UnikTrk> ump;
            UnikTrk uni_trk(trk_vec[i]);
			uni_trk.push(trk_vec[i]);
            ump[trk_number] = uni_trk;
            m_uni_trks[trk_radar_id] = ump;
        }
	}
}

void MultiFusion::process() {
    if(m_sys_time == 0) {
        m_sys_time = getNowMs();
    } else {
        m_sys_time += Config::get_instance()->m_fusion_period;
    }
    deleteSystrk();
	processUnitrk();
	associate();
	createSystrk();
	fusion();
}

void MultiFusion::processUnitrk() {
    for(auto it = m_uni_trks.begin(); it != m_uni_trks.end(); it++){
		std::map<int64_t,UnikTrk> &tmpMap = it->second;
		for(auto itx = tmpMap.begin();itx != tmpMap.end();){
			UnikTrk &trk = itx->second;
			trk.total_count++;
			trk.is_used = false; 
			if(trk.systrk_index != 0){
				trk.is_used = true;
			}
			// 使用统一的外推上限
            if (trk.ep_count > Config::get_instance()->m_unitrk_max_ex_times)//超限
            {
                trk.state = FINALTRACK;
            }
			
			//单源航迹删除
			if((trk.total_count > Config::get_instance()->m_unitrk_max_alive_period && trk.is_valid == false)
                || trk.state == FINALTRACK) {
				// 存在大于60个周期且不可用 || 进入终止态
			  
				if(trk.systrk_index != 0) {
                    m_sys_trks[trk.systrk_index].associate_map.erase(trk.radar_id);
				}
				if (!Logger::GetInstance()->isLogClose()) {
					APPINFOLOG("Systrack associate map delete message: systrk: %d delete unitrk: radar_id: %d, trk_no: %lld", 
					trk.systrk_index, trk.radar_id, trk.trk_no);
				}
				itx = tmpMap.erase(itx);
				continue;
			}

			if(trk.is_valid == false){//不可用，跳过此周期
				itx++;
                trk.proc_timestamp = m_sys_time;
				continue;
			}
			
			//单源航迹外推，外推的单源航迹信息无用
			if(trk.is_updated == false) {
				trk.filter.predict();
				trk.x  = trk.filter.x(0);
				trk.vx = trk.filter.x(1);
				trk.y  = trk.filter.x(2);
				trk.vy = trk.filter.x(3);
				trk.z  = trk.filter.x(4);
				trk.vz = trk.filter.x(5);

				trk.ep_count++;
				trk.is_extra=true;
				trk.state = EXTRATRACK;
			}
			//有更新
			if(trk.is_updated==true){
				if(trk.is_extra){
					auto lt = trk.his_mp.rbegin();
					auto last1 = *lt++;
					auto last2 = *lt++;
					auto last3 = *lt++;
					auto last4 = *lt;
					double p = std::min(last2.ts - last3.ts, last3.ts-last4.ts);
					if(last1.ts - last2.ts > 2 * p)
						trk.isf_afterpush = true;
					else
						trk.isf_afterpush = false;
				}else{
					trk.isf_afterpush = false;
				}

				trk.is_extra = false;
				double dt=m_sys_time - trk.recv_timestamp;
				if (trk.state == PRE_STABLE_TRACK) {
                    //介于init 和 stable的中间态，为初始化的单源航迹提供关联的特殊安排
					trk.state = STABLETRACK;
				}
				Moveparam &mp = trk.his_mp.back();
				//时间对准
				trk.x += trk.vx * dt;
				trk.y += trk.vy * dt;
				trk.z += trk.vz * dt;

				if(trk.state == INITTRACK){
					//初始化滤波器
					trk.filter = get_kinematic_kf(3, 1, Config::get_instance()->m_fusion_period / 1000.0);
					Eigen::VectorXd p(6),inputData(6); //九维向量
					//inputData<<mp.x,mp.vx,0,mp.y,mp.vy,0,mp.z,mp.vz,0; //x vx ax y vy ay z vz az
					inputData << trk.x, trk.vx, trk.y, trk.vy, trk.z, trk.vz;
					p << Config::get_instance()->m_unisource_x_var,Config::get_instance()->m_unisource_vx_var,
						 Config::get_instance()->m_unisource_y_var,Config::get_instance()->m_unisource_vy_var,
						 Config::get_instance()->m_unisource_z_var,Config::get_instance()->m_unisource_vz_var; //方差（x vx ax y vy ay z vz az）
					
                    trk.filter.x=inputData;
					trk.filter.P=p.asDiagonal();//向量转对角阵
                    //设置过程噪声 决定对预测的相信程度 大则不相信
					trk.filter.Q=Q_discrete_white_noise(2, Config::get_instance()->m_fusion_period/1000.0,Config::get_instance()->m_unisource_kalman_q, 3); //10;
					//设置观测噪声 决定对观测值的相信程度 大则不相信
                    trk.filter.R*=Config::get_instance()->m_unisource_kalman_r;//100000;
					//Q,R会决定初始态，在滤波持续后会收敛
					trk.state = PRE_STABLE_TRACK;//参与一次融合之后变成稳定状态
					//初始化该通道下贡献的系统位置，即初始状态认为的系统位置就是时空对齐预测点
					trk.sysx=trk.x;
					trk.sysy=trk.y;
					trk.sysz=trk.z;					
					//保留贡献值
					trk.beforex=trk.x;
					trk.beforey=trk.y;
					trk.beforez=trk.z;
				} else {//稳定态
					
					Eigen::Vector3d inputData;
					//z<<rx,ry,rz;
					//inputData<<mp.x,mp.y,mp.z;
					inputData<<trk.x,trk.y,trk.z;
					
					trk.filter.predict();//状态更新

					double beforex=trk.filter.x(0);
					trk.filter.update(inputData);//数据更新
					//取得滤波输出数据
					trk.x=trk.filter.x(0);
					trk.vx=trk.filter.x(1);
					trk.y=trk.filter.x(2);
					trk.vy=trk.filter.x(3);
					trk.z=trk.filter.x(4);
					trk.vz=trk.filter.x(5);
					//更新该通道下贡献的系统位置
					double advance = m_sys_trks[trk.systrk_index].systrk_vector_advance;//20191114 redo 动态自调整的advance
					if(m_sys_trks[trk.systrk_index].his_num == 1)  //只有一条航迹的情况下应该紧跟
						advance = 0.2;
					//渐进量，大则靠近单源航迹的预测（对齐点）快，越小越接近单源航迹自己的位子
					//对系统位置的估计，是预测其在 单源航迹预测点 和 利用历史系统航迹叠加矢量后的预测点 中间
					trk.sysx = trk.x + advance * (trk.sysx - trk.beforex);//新的系统预测位置=（旧的系统预测位置 + 近两次位移差）*渐进量 + 单源航迹预测位置*（1-渐进量）
					trk.sysy = trk.y + advance * (trk.sysy - trk.beforey);
					trk.sysz = trk.z + advance * (trk.sysz - trk.beforez);
					//保留贡献值
					trk.beforex=trk.x;
					trk.beforey=trk.y;
					trk.beforez=trk.z;
				}
				trk.ep_count = 0;
				if(trk.systrk_index!=0){
					m_sys_trks[trk.systrk_index].is_updated = true;
				}
                trk.proc_timestamp=m_sys_time;
			}
			//恢复标志
			//trk.pro_timestamp=sysTime;
			trk.is_updated = false;
			itx++;
			if (!Logger::GetInstance()->isLogClose()) {
				APPDEBUGLOG(" [RECEIVE] received unitrk data message: radar_id:%d, trkno: %lld", trk.radar_id, trk.trk_no);
			}
		}	
	}
}

void MultiFusion::deleteSystrk() {
    for(auto it = m_sys_trks.begin(); it!=m_sys_trks.end(); ){
		if(it->second.state == FINALTRACK) {
			m_tracknum_pool.push_back(it->first);
			for (auto associate_it = it->second.associate_map.begin() ; associate_it != it->second.associate_map.end(); ++associate_it) {
                //清理关联表
				UnikTrk &utrk = m_uni_trks[associate_it->second.first][associate_it->second.second];
				utrk.systrk_index = 0;
				utrk.is_used = false;
				utrk.is_valid = false;
				utrk.associate_track.erase(it->first);
			}
			if (!Logger::GetInstance()->isLogClose()) {
				APPINFOLOG(" [SYSTRK] Systrack deleted message: delete systrack, trk_no: %d", it->first);
			}
			it = m_sys_trks.erase(it);
		} else {
			it++;
		}
	}
}

void MultiFusion::associate() {
    for(auto it = m_uni_trks.begin(); it != m_uni_trks.end(); it++){
		std::map<int64_t,UnikTrk> &tmpMap = it->second;
		for(auto itx = tmpMap.begin(); itx != tmpMap.end(); itx++){
			UnikTrk &trk = itx->second;
			
			if(trk.is_used || !trk.is_valid || trk.is_extra) continue; // 已关联、不可用、外推状态，不参与关联

			// 对于该单源航迹，将关联门限内的系统航迹加入待关联链表
			for(auto itxx = m_sys_trks.begin(); itxx != m_sys_trks.end(); itxx++){
				// 对系统航迹筛选
				SystemTrk &systrk = itxx->second;
				if(systrk.associate_map.count(trk.radar_id)) continue; // 已经关联了相同通道的其他航迹
				double dis = Distance(trk.x, trk.y, 0, systrk.preX, systrk.preY, 0); // 不考虑z轴方向
			    // 检验是否满足关联条件
				if(dis < systrk.relation_limit && isHeadingIncludedAngleAcceptable(trk.vx, trk.vy, 0, systrk.vX, systrk.vY, 0)) {
					// 距离在门限内且航向差符合关联条件
					trk.associate_track[systrk.systrk_no] = dis;
					trk.is_used = true;
				}
			}
			// 选择距离最近的系统航迹关联
			if (trk.associate_track.size()) {
				// 已有关联备选
				int candidateNO = NearestNeighbourAlgorithm(trk.associate_track); // 在单源航迹的备选系统航迹里找最近邻
				for(auto itxx = m_sys_trks.begin();itxx != m_sys_trks.end(); itxx++) {
					if(itxx->second.systrk_no == candidateNO){
						SystemTrk& track = itxx->second;
						track.associate_map[trk.radar_id] = std::make_pair(trk.radar_id, trk.trk_no);
						track.is_updated = true;
						track.ep_count = 0;

						trk.systrk_index = track.systrk_no;

                        vectorAdvanceDec(track,1);
						//20191107 redo 对于PRE_STABLE_TRACK更新其预测系统位置
						if (trk.state == PRE_STABLE_TRACK) {
							double unitrk_associate_stableSystrk_advance = Config::get_instance()->m_unitrk_associate_stableSystrk_advance;
                            trk.sysx = track.preX*unitrk_associate_stableSystrk_advance + trk.x*(1-unitrk_associate_stableSystrk_advance);
							trk.sysy = track.preY*unitrk_associate_stableSystrk_advance + trk.y*(1-unitrk_associate_stableSystrk_advance);
							trk.sysz = track.preZ*unitrk_associate_stableSystrk_advance + trk.z*(1-unitrk_associate_stableSystrk_advance);
							trk.sys_lasthead = Head(track.vX,track.vY);
						}

						if (!Logger::GetInstance()->isLogClose()) {
							APPINFOLOG(" [ASSOCIATE] systrk: %d radar_id: %d trkno: %d processtime: %lld", 
										itxx->first, trk.radar_id, trk.trk_no, m_sys_time);
						}
					}
				}
                trk.associate_track.clear(); // 清除单源航迹的系统航迹关联备选
			}
		}
	}
}

void MultiFusion::createSystrk() {
	/*
	对每一个单元航迹，遍历所有的系统航迹，如果发现与某初建的系统航迹在关联圈内，则加入
	候选列表，最后选出最近的系统航迹，保证了相近的区域不重复多建系统航迹，即为那些初建
	的系统航迹增长了关联表
	*/
	for(auto it = m_uni_trks.begin(); it != m_uni_trks.end(); it++){
		std::map<int64_t,UnikTrk> &tmpMap = it->second;
		for(auto itx = tmpMap.begin(); itx != tmpMap.end(); itx++){
			UnikTrk &trk = itx->second;
			
			if(trk.is_used || !trk.is_valid || trk.is_extra) continue; // 已关联、不可用、外推状态，不参与关联

			// 对于该单源航迹，将关联门限内的系统航迹加入待关联链表
			for(auto itxx = m_sys_trks.begin(); itxx != m_sys_trks.end(); itxx++){
				SystemTrk &systrk = itxx->second;
				if(systrk.associate_map.count(trk.radar_id)) continue; // 已经关联了相同通道的其他航迹
				double dis = Distance(trk.x, trk.y, 0, systrk.preX, systrk.preY, 0); // 不考虑z轴方向
			    // 检验是否满足关联条件
				if(dis < systrk.relation_limit && isHeadingIncludedAngleAcceptable(trk.vx, trk.vy, 0, systrk.vX, systrk.vY, 0)) {
					// 距离在门限内且航向差符合关联条件
					trk.associate_track[systrk.systrk_no] = dis;
					trk.is_used = true;
				}
			}
			
			if (trk.is_used) {
				// 单源航迹二次关联成功
				int candidateNO = NearestNeighbourAlgorithm(trk.associate_track); // 在单源航迹的备选系统航迹里找最近邻
				for(auto itxx = m_sys_trks.begin();itxx != m_sys_trks.end(); itxx++) {
					if(itxx->second.systrk_no == candidateNO){
						SystemTrk& track = itxx->second;
						track.associate_map[trk.radar_id] = std::make_pair(trk.radar_id, trk.trk_no);
						track.is_updated = true;
						track.ep_count = 0;

						trk.systrk_index = track.systrk_no;

                        vectorAdvanceDec(track,1);
						//20191107 redo 对于PRE_STABLE_TRACK更新其预测系统位置
						if (trk.state == PRE_STABLE_TRACK) {
							double unitrk_associate_stableSystrk_advance = Config::get_instance()->m_unitrk_associate_stableSystrk_advance;
                            trk.sysx = track.preX*unitrk_associate_stableSystrk_advance + trk.x*(1-unitrk_associate_stableSystrk_advance);
							trk.sysy = track.preY*unitrk_associate_stableSystrk_advance + trk.y*(1-unitrk_associate_stableSystrk_advance);
							trk.sysz = track.preZ*unitrk_associate_stableSystrk_advance + trk.z*(1-unitrk_associate_stableSystrk_advance);
							trk.sys_lasthead = Head(track.vX,track.vY);
						}

						if (!Logger::GetInstance()->isLogClose()) {
							APPINFOLOG(" [ASSOCIATE] systrk: %d radar_id: %d trkno: %d processtime: %lld", 
										itxx->first, trk.radar_id, trk.trk_no, m_sys_time);
						}
					}
				}
                trk.associate_track.clear(); // 清除单源航迹的系统航迹关联备选
			} else if (!trk.is_used && !trk.is_extra && trk.is_valid) {
				// 单源航迹未关联到已有系统航迹，且自己是可用的非外推等待的
				// 新建
				SystemTrk new_systrk(trk);
				new_systrk.systrk_no = m_tracknum_pool.front();
				m_tracknum_pool.pop_front();
				new_systrk.relation_limit = Config::get_instance()->m_associate_threshold;

				new_systrk.systrk_vector_advance = Config::get_instance()->m_vector_advance;//20191114 redo 动态自调整渐进量

				new_systrk.lastRadarReportTime = 0;
				new_systrk.create_ts = m_sys_time;
				new_systrk.associate_map[trk.radar_id] = std::make_pair(trk.radar_id, trk.trk_no);
				trk.systrk_index = new_systrk.systrk_no;
			
				//对刚刚建立的系统航迹，提供一些基本信息（因为还没进入fusion()）
				new_systrk.X = trk.x;
				new_systrk.Y = trk.y;
				new_systrk.Z = trk.z;

				m_sys_trks[new_systrk.systrk_no] = new_systrk;
				if (!Logger::GetInstance()->isLogClose())
				{
					APPINFOLOG(" [SYSTRK] create systrack, trkno: %d, associate map: %lld, %lld, %lld", 
					new_systrk.systrk_no, new_systrk.associate_map[1].second, new_systrk.associate_map[2].second, new_systrk.associate_map[3].second);
				}
			}
		}
	}
}

void MultiFusion::fusion() {
	for(auto it=m_sys_trks.begin(); it!=m_sys_trks.end(); it++){
		SystemTrk &systrk = it->second;

		systrk.process_ts = m_sys_time;
		systrk.his_num = std::max(systrk.his_num,(int)systrk.associate_map.size());  
		if(!systrk.is_updated){
			//未更新，进入外推
			systrk.state = EXTRATRACK;
		}
		if(systrk.associate_map.size() <= 0 || systrk.ep_count > Config::get_instance()->m_systrk_max_ex_times) {
			//无关联单源航迹，终止
			systrk.state = FINALTRACK;
		}
		if(systrk.state != INITTRACK && systrk.is_updated){
			//非初始且更新了，进入稳定态
			systrk.state = STABLETRACK;
		}
        if (systrk.state != FINALTRACK) {
			//20191114 redo 动态自适应的渐进量 , 回归
			systemTrack_VD_Return(systrk);
		}
		systrk.is_updated = false; // 重置是否更新的状态
		double dt = Config::get_instance()->m_fusion_period / 1000;
		if(systrk.state == INITTRACK){
			//初始态的系统航迹
			double TQ = 0;
			double MX = 0, VX = 0;
			double MY = 0, VY = 0;
			double MZ = 0, VZ = 0;

			//计算Q
			for(auto mapit=systrk.associate_map.begin();mapit!=systrk.associate_map.end();mapit++) {
				// 通过雷达+航迹号确定一个航迹
				UnikTrk &utrk = m_uni_trks[mapit->second.first][mapit->second.second];
				if(utrk.is_extra) 
				    continue;
				TQ += utrk.get_Q(systrk);
				
				if (!Logger::GetInstance()->isLogClose()) {
					// TODO: 日志输出融合过程中的单源航迹信息
				}

				// 根据权重Q更新系统航迹
				MX += utrk.Q * utrk.sysx;
				MY += utrk.Q * utrk.sysy;
				MZ += utrk.Q * utrk.sysz;
				
				VX += utrk.Q*utrk.vx;
				VY += utrk.Q*utrk.vy;
				VZ += utrk.Q*utrk.vz;
                updateRadrReportTime(systrk,utrk);//更新雷达报告时间
			}
			MX /= TQ; MY /= TQ; MZ /= TQ;
			VX /= TQ; VY /= TQ; VZ /= TQ;
			// 更新系统航迹位置,直接使用融合位置
			systrk.X=MX;
			systrk.Y=MY;
			systrk.Z=MZ;
			systrk.vX=VX;
			systrk.vY=VY;
			systrk.vZ=VZ;
			systrk.aX=0;
			systrk.aY=0;
			systrk.aZ=0;
			//初始化滤波器
			systrk.filter=get_kinematic_kf(3,1,dt);
			Eigen::VectorXd p(6),inputData(6);
			inputData<<MX,VX,MY,VY,MZ,VZ;
			p<< Config::get_instance()->m_fusion_x_var,
				Config::get_instance()->m_fusion_vx_var,
				Config::get_instance()->m_fusion_y_var,
				Config::get_instance()->m_fusion_vy_var,
				Config::get_instance()->m_fusion_z_var,
				Config::get_instance()->m_fusion_vz_var;//系统误差
			systrk.filter.x=inputData;
			systrk.filter.P=p.asDiagonal();
			systrk.filter.Q*=Q_discrete_white_noise(2,dt,Config::get_instance()->m_fusion_kalman_q,3);//过程噪声
			systrk.filter.R*=Config::get_instance()->m_fusion_kalman_r;//观测噪声
			systrk.state=STABLETRACK;//转入稳定态
			//记录连续更新次数
			systrk.update_continuous_count++;
		} else if(systrk.state==STABLETRACK){
			//稳定态系统航迹
			systrk.ep_count=0;//重置外推次数
			double TQ=0;
			double MX=0;
			double MY=0;
			double MZ=0;
			double NQ =0;//此次质量因子的基底和
			int effiective_count =0;//此次有效的单源航迹数目
			auto mapit = systrk.associate_map.begin();
			while (mapit != systrk.associate_map.end())
			{
				UnikTrk &utrk=m_uni_trks[mapit->second.first][mapit->second.second];
				if(!utrk.is_valid) {
					//单源航迹不可用的，不参加
					mapit++;
					continue;
				}
				if(utrk.is_extra) {
					//外推单源航迹，不脱钩，不参加，若其外推等待超限，此单元航迹会在process_unitrk()中被置final
					vectorAdvanceAdd(systrk,0.8);//20191114 redo 动态自调整渐进量，源减少，advance+
					mapit++;
					continue;
				}			
				//补充内容：
				// 1.如果该单源航迹所在位置偏离系统位置过多
				// 2.如果该单源航迹航向与系统航向偏差过大
				// 则取消关联关系 20191029 redo 提前防止离群
				double dis = Distance(utrk.x, utrk.y, 0, systrk.preX, systrk.preY, 0);  
				double uni_head = Head(utrk.vx, utrk.vy);
				double sys_head = Head(systrk.vX, systrk.vY);
				double include_angle = fabs(Dhead(uni_head, sys_head));
				//由单源航迹建成的系统航迹不考虑脱钩
				if((systrk.associate_map.size() > 1)
					&& (dis > Config::get_instance()->m_stray_threshold || include_angle > Config::get_instance()->m_turn_angle_threshold)) {
					//单源航迹离群，即此单源航迹位置距离系统预测位置过远，可信度很低
					utrk.is_used = false;//关联状态复位为无关联
					auto tmp_it = systrk.associate_map.erase(mapit);//删除时元遍历器会销毁，需要接收返回的指向下一个元素的遍历器
					utrk.systrk_index = 0;//直接关联清空
					utrk.associate_track.erase(systrk.systrk_no);//关联备选缓存清空
					if (!Config::get_instance()->m_close_log)
					{
						APPINFOLOG("Unitrk_unassociate_System: %d %d %lld %f %f", 
						systrk.systrk_no, utrk.radar_id, utrk.trk_no, dis, include_angle)
					}
                    vectorAdvanceAdd(systrk, 1);//20191114 redo 动态的自适应渐进量，源减少，advance+
					// if (Config::get_instance()->m_target_num_mode == TARGET_MODE_ALONE)//单目标模式，防止脱钩航迹迅速重新建航
					// {
					// 	utrk.state = FINALTRACK;
					// }
					//多目标模式下，认为存在复合目标，认可航迹由一分多？

					utrk.state = FINALTRACK;

					if (systrk.associate_map.empty() || tmp_it == systrk.associate_map.end()) {
						//删除最末一个元素,关联表已遍历完
						break;
					} else {
						//删除了除最末元素以外的一个元素，仍有元素未遍历
						mapit = tmp_it;
					}
					continue;//此离群单源航迹不参与融合
				}
				mapit++;
				effiective_count++;//有效数目++
				//end redo

				//针对转弯时矢量差的方向问题，对utrk做一次角度变换
				double delt_h = Dhead(utrk.sys_lasthead, Head(systrk.vX,systrk.vY))*PI/180;
				double newx = (utrk.sysx-utrk.x)*cos(delt_h) + (utrk.sysy-utrk.y)*sin(delt_h) + utrk.x;
				double newy = (utrk.sysy-utrk.y)*cos(delt_h) - (utrk.sysx-utrk.x)*sin(delt_h) + utrk.y;

				utrk.sysx = newx;
				utrk.sysy = newy;

				TQ+=utrk.get_Q(systrk);
				if (!Config::get_instance()->m_close_log)
				{
					APPINFOLOG("Unitrk_getQ: %d %lld %d %f %f %f %f %f", utrk.radar_id, utrk.trk_no, utrk.state, utrk.frequency, 
							utrk.fit_value, utrk.close_value, utrk.report_stability_value, utrk.Q)
				}
				MX+=utrk.Q*utrk.sysx;
				MY+=utrk.Q*utrk.sysy;
				MZ+=utrk.Q*utrk.sysz;//z暂时不考虑
			}
		
			Eigen::Vector3d inputData; 
			if (TQ)//有 有效的单源航迹
			{
				MX/=TQ;MY/=TQ;MZ/=TQ;//加权融合的位置
                //无差别的与系统预测进行基于质量因子（和）的再平衡
				{
					//质量因子（和）愈高，约接近于加权平均位置
					double qty = TQ/Config::get_instance()->m_quality_factor_base/effiective_count;
					if(systrk.associate_map.size() == 1){  
						qty += Config::get_instance()->m_singleTrk_qty_add; //对于只有一条单源航迹的系统航迹，增大其比例				
						//if(TQ == 1)    //并且如果有存在转弯的情况，更快追上
						if(TQ < (Config::get_instance()->m_quality_factor_base/3)){
							//并且如果有存在转弯的情况，更快追上 20191226 redo
							qty = std::max(Config::get_instance()->m_singleTrk_qty_floor_limit_bad, qty);
						}else{
							qty = std::max(Config::get_instance()->m_singleTrk_qty_floor_limit_normal, qty);
						}
					}
					MX = MX*qty + systrk.preX*(1-qty);
					MY = MY*qty + systrk.preY*(1-qty);
					MZ = MZ*qty + systrk.preZ*(1-qty);
				}
				MZ = systrk.Z * Config::get_instance()->m_height_smoothing_alpha + MZ*(1-Config::get_instance()->m_height_smoothing_alpha);//简单的alpha滤波

				inputData<<MX,MY,MZ;//各关联单源航迹的预测系统点加权融合
			}else{//没有 有效的单源航迹 即TQ=0 不能作为除数
				//使用预测和现在位置的平均位置
				MX = systrk.preX*0.5 + systrk.X*0.5;
				MY = systrk.preY*0.5+systrk.Y*0.5;
				MZ =systrk.preZ*0.5+systrk.Z*0.5;
				inputData <<MX,MY,MZ;
			}


			systrk.filter.predict();
			systrk.filter.update(inputData);
			systrk.X=systrk.filter.x(0);
			systrk.vX=systrk.filter.x(1);
			systrk.Y=systrk.filter.x(2);
			systrk.vY=systrk.filter.x(3);
			systrk.Z=systrk.filter.x(4);
			systrk.vZ=systrk.filter.x(5);
			systrk.update_continuous_count ++;
			//end redo
		}
		else if(systrk.state==EXTRATRACK){

			if(systrk.ep_count > Config::get_instance()->m_systrk_max_ex_times){
				//外推次数大于阈值
				//保持位置不动，不作外推
				//20191025 redo 外推超限
				printf("systrk.ep_count > Config::get_instance()->m_systrk_max_ex_times");
				systrk.state = FINALTRACK;
				it++;
				continue;
				//end redo
			}

			if(systrk.ep_count < Config::get_instance()->m_systrk_max_ex_times){ 
				//为了防止推得过远，推12次之后不继续外推
				//20191029 redo 保守的外推
				systrk.ep_count++;
				systrk.update_continuous_count *= Config::get_instance()->m_v_extra_continuous_reduce;
				if (systrk.update_continuous_count < Config::get_instance()->m_v_continuous_lower_limit)
				{
					systrk.preX = systrk.X * Config::get_instance()->m_v_extra_x_reduce + systrk.preX * (1-Config::get_instance()->m_v_extra_x_reduce);
					systrk.preY = systrk.Y * Config::get_instance()->m_v_extra_y_reduce + systrk.preY * (1-Config::get_instance()->m_v_extra_y_reduce);
					systrk.preZ = systrk.Z * Config::get_instance()->m_v_extra_z_reduce + systrk.preZ * (1-Config::get_instance()->m_v_extra_z_reduce);
				}
				//end redo
				Eigen::Vector3d inputData;
				inputData<<systrk.preX,systrk.preY,systrk.preZ;//用系统航迹的预测来外推
				systrk.filter.predict();
				systrk.filter.update(inputData);
				systrk.X=systrk.filter.x(0);
				systrk.Y=systrk.filter.x(2);
				systrk.Z=systrk.filter.x(4);
				systrk.vX=systrk.filter.x(1);
				systrk.vY=systrk.filter.x(3);
				systrk.vZ=systrk.filter.x(5);
			}
		}

		//计算预测位置dt
		systrk.preX=systrk.X+systrk.vX*dt;
		systrk.preY=systrk.Y+systrk.vY*dt;
		systrk.preZ=systrk.Z+systrk.vZ*dt;	
		if (!Logger::GetInstance()->isLogClose()) {
			APPINFOLOG("Systrk info: trkno: %d state: %d ep_count: %d pos: (%f, %f) pre_pos(%f, %f) vx: %f vy: %f", 
						systrk.systrk_no, systrk.state, systrk.ep_count, systrk.X, systrk.Y, systrk.preX, systrk.preY, systrk.vX, systrk.vY);
		}
		//系统位置信息回填给单源信息，以便更新贡献的系统位置
		for(auto mapit=systrk.associate_map.begin();mapit!=systrk.associate_map.end();mapit++){
			UnikTrk &utrk=m_uni_trks[mapit->second.first][mapit->second.second];
			//本周期没参与融合的不需要回填买,还是用之前的矢量差（存疑）
			if(utrk.is_extra || !utrk.is_valid)
				continue;
			utrk.sysx=systrk.X;utrk.sysy=systrk.Y;utrk.sysz=systrk.Z;//？
			utrk.sys_lasthead = Head(systrk.vX,systrk.vY);
		}
	}
}

bool MultiFusion::isHeadingIncludedAngleAcceptable(double vx1,double vy1,double vz1,double vx2,double vy2,double vz2) {
		double h1=Head(vx1,vy1);
		double h2=Head(vx2,vy2);

		double dh=fabs(Dhead(h1,h2));
		return dh<=Config::get_instance()->m_turn_angle_threshold;
}

void MultiFusion::systemTrack_VD_Return(SystemTrk &systrk) {
	if (systrk.systrk_vector_advance > Config::get_instance()->m_vector_advance) {
		double new_systrk_vector_advance = systrk.systrk_vector_advance - Config::get_instance()->m_vector_advance_return_step_size; 
		systrk.systrk_vector_advance = std::max(new_systrk_vector_advance, Config::get_instance()->m_vector_advance);
	}else if (systrk.systrk_vector_advance < Config::get_instance()->m_vector_advance) {
		double new_systrk_vector_advance = systrk.systrk_vector_advance + Config::get_instance()->m_vector_advance_return_step_size; 
		systrk.systrk_vector_advance = std::min(new_systrk_vector_advance, Config::get_instance()->m_vector_advance);
	}
}

void MultiFusion::vectorAdvanceAdd(SystemTrk &systrk,double power) {
	double new_systrk_vector_advance = systrk.systrk_vector_advance + Config::get_instance()->m_vector_advance_add_step_size*power;
	systrk.systrk_vector_advance = std::min((double)new_systrk_vector_advance, Config::get_instance()->m_vector_advance_upper_value);
}

void MultiFusion::vectorAdvanceDec(SystemTrk &track,double power) {
	double new_systrk_vector_advance = track.systrk_vector_advance - Config::get_instance()->m_vector_advance_dec_step_size*power;
	track.systrk_vector_advance = std::max(new_systrk_vector_advance, Config::get_instance()->m_vector_advance_lower_value);
}

void MultiFusion::updateRadrReportTime(SystemTrk & systrk, UnikTrk & utrk) {

}

int  MultiFusion::NearestNeighbourAlgorithm(std::map<int,double>& associate_track) {
    std::map<int ,double> ::iterator it=associate_track.begin();
	double mindis=INITIALMAXDIS;
	int minno=-1;
	while(it!=associate_track.end()){
		if(it->second<=mindis){
			mindis=it->second;
			minno=it->first;
		}
        it++;
	}
	return minno;
}
