#ifndef __MULTI_FUSION_H
#define __MULTI_FUSION_H

#include <map>
#include "defstruct.h"
#include "Kalman.h"
#include "AlgorithmTool.h"
#include "utils.h"

typedef enum STATUS {
    INITTRACK = 0,
    PRE_STABLE_TRACK = 1,
    EXTRATRACK = 2,
    STABLETRACK = 3,
    FINALTRACK = 4
} STATUS;

typedef struct TrkForFushion {
    // 原始报告信息
	// RadarTrack radar_trk;
	// 用于融合的信息
	int radar_id;
	int64_t trk_no;
	int64_t recv_timestamp;
	// 位置信息
	double x;
	double y;
	double z;
	// 速度分量
	double vx;
	double vy;
	double vz;
	double speed; // 合速度
	double heading; // 航向
	double pitching; // 俯仰角
};

typedef struct Moveparam {
	int64_t ts;
	double x;
	double y;
	double z;
	double vx;
	double vy;
	double vz;
	double ax;
	double ay;
	double az;
} Moveparam;

class SystemTrk;

class UnikTrk{
public:
	UnikTrk(){};
	UnikTrk(const TrkForFushion& trk):
	batch_fit(0),
	radar_id(trk.radar_id),
	trk_no(trk.trk_no),
	is_valid(false),
	his_count(0),
	total_count(0),
	ep_count(0),
	report_stability_value(0.0),
	fit_value(0),
	systrk_index(0),
	is_hei_available(false),
	isf_afterpush(false),
	fit_value_ave(0),
	close_value_ave(0),
	q_count(1),
	Q(0),
	direction_confirm_times(0) {
		recv_timestamp=getNowMs();
	};
	//单源航迹信息
	int radar_id;
	int64_t trk_no;
	STATUS state;
	//是否为外推之后的第一个周期
	bool isf_afterpush;
	//与其关联的系统航迹索引
	int systrk_index;
	//本次新航迹接收时间
	int64_t recv_timestamp;
	//本次处理时间
	int64_t proc_timestamp;
    //是否更新
	bool is_updated;
    //是否被使用
	bool is_used;
    //是否外推
	bool is_extra;
    //是否此数据周期有效
	bool is_available_period;
    //是否为有效航迹
	bool is_valid;
    //是否高度信息可用
	bool is_hei_available;
	//当前位置X
	double x;
	//当前位置Y
	double y;
	//当前位置Z
	double z;
	//当前速度VX
	double vx;
	//当前速度VY
	double vy;
	//当前速度VZ
	double vz;
	//当前加速度AX
	double ax;
	//当前加速度AY
	double ay;
	//当前加速度AZ
	double az;
	//当前合速度
	double speed;
	//当前航向
	double heading;
	//当前俯仰
	double pitching;
	//该通道记录的系统X
	double sysx;
	//该通道记录的系统Y
	double sysy;
	//该通道记录的系统Z
	double sysz;
	//上次矢量校准时系统航迹航向
	double sys_lasthead;
	//上次更新中，保存的对准位置
	double beforex;
	double beforey;
	double beforez;
	//上一时刻滤波时间
	int64_t report_timestamp;
	//临时关联表
	std::map<int,double> associate_track;
	//历史报告次数
	int his_count;
	//存活系统周期次数
	int total_count;
	//外推次数
	int ep_count;
	//历史运动信息
	std::list<Moveparam> his_mp;
	//更新频率因子
	double frequency;
	//起批拟合度
	double batch_fit;
	//运动信息拟合度因子
	double fit_value;
	//距离因子
	double close_value;
	//报告稳定性因子
	double report_stability_value;
	//外推因子
	double ex_value;
	//距离因子均值
	double close_value_ave;
	//拟合度因子均值
	double fit_value_ave;
	//求取质量因子次数
	double q_count;
	//滤波器
	Kalman filter;
	//质量因子
	double Q;
	//测向确认次数
	int direction_confirm_times;

	// 更新单源航迹的运动数据
	void push(const TrkForFushion& trk);

	// 更新运动轨迹数据
	void updateMovementData(const TrkForFushion &trk);

	// 利用最近历史信息，拟合轨迹曲线并得到一些基本信息, 得到初态fitValue，0-1
	void fit();

	// 计算航迹质量因子
	double get_Q(const SystemTrk& systrk);

	// 生成距离因子
	void getCloseValue(double sys_x, double sys_y);

	// 检查是否满足建航标准
	bool SuitToStart();

	// 初始化报告稳定性因子
	void initReportStabilityValue();

	// 生成新的报告稳定因子
	float getReportStabilityValue();

	// 得出最近三报告数据间航向角夹角
	double getUnitrkReportLatestHeadIncludedAngle();

	// 从函数映射中取得新的报告稳定因子数值
    double getReportStabilityValueFromFunc(double included_angle);
};

class SystemTrk{
public:
	SystemTrk(){};
	SystemTrk(const UnikTrk&utrk):ep_count(0),is_updated(true),update_continuous_count(0),is_output(false),
	preX(utrk.x),preY(utrk.y),preZ(utrk.z),state(INITTRACK),his_num(0),is_hei_available(false),is_hei_available_ever(false){

	}
	//融合航迹号
	int systrk_no;
	//输出结果,用于存储框架之外的融合结果
	RadarTrack fushion_trk;
	//关联表, 通过<radar_id, trk_no>确定一个单源航迹
	std::map<int,std::pair<int, int64_t>>associate_map;
    //最近雷达报告时间
	int64_t lastRadarReportTime;
	//建立时间
	int64_t create_ts;
	//确认时间
	int64_t confirm_ts;
	//上次处理时间
	int64_t process_ts;
	//外推次数
	unsigned int ep_count;
	//连续更新次数
	unsigned int update_continuous_count;
	bool is_updated;
	bool is_hei_available;
	bool is_hei_available_ever;
	double relation_limit;
	//当前位置
	double X;
	double Y;
	double Z;
	//上次最优估计位置
	//double LastX;
	//double LastY;
	//double LastZ;
	//预测位置
	double preX;
	double preY;
	double preZ;
	//当前速度
	double vX;
	double vY;
	double vZ;
	//当前加速度
	double aX;
	double aY;
	double aZ;
	STATUS state;
	Kalman filter;

    double systrk_vector_advance;
	//是否输出
	bool is_output;
	//list<Moveparam> HisPM;
	//到现在为止被同时探测到的最多雷达源个数
	int his_num;
};

class MultiFusion
{
public:
    MultiFusion();
    ~MultiFusion();

	// 初始化
	void init();

	// 装填航迹
	void feedTrack(std::vector<TrkForFushion>& trk_vec);
    void process();
	void processUnitrk();
	void deleteSystrk();
	void associate();
    void createSystrk();
	void fusion();

public:
	std::map<int, SystemTrk> m_sys_trks;
	std::map<int, std::map<int64_t, UnikTrk>> m_uni_trks;
	int64_t m_sys_time{ 0 };
	std::list<int> m_tracknum_pool;

private: 
	// 判断航向夹角是否符合要求
	bool isHeadingIncludedAngleAcceptable(double vx1,double vy1,double vz1,double vx2,double vy2,double vz2);

	// 系统航迹渐进量自回归，回归终点为系统配置默认值
	void systemTrack_VD_Return(SystemTrk &systrk);

	// 系统航迹渐进量递增
	void vectorAdvanceAdd(SystemTrk &systrk, double power);

	// 系统航迹渐进量递减
	void vectorAdvanceDec(SystemTrk &track, double power);

	// 更新系统航迹最末收到的 雷达报告的时间 记录
	void updateRadrReportTime(SystemTrk & systrk, UnikTrk & utrk);

	// 计算系统航迹高度, 暂时无用
	void getOrAdjustingSystrkHei(SystemTrk &systrk);

	// 最近邻算法
	int  NearestNeighbourAlgorithm(std::map<int,double>& associate_track);
};

#endif