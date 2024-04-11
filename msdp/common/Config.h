#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <string>
#include <stdio.h>
#include <vector>
#include "cJSON.h"

class Config
{
public:
	static Config* get_instance();
	static void setGlobalConfig(const char* jsonfile);
	bool init(const char* jsonfile);

private:
	Config();
	Config(const char* jsonfile);
	~Config();

public:
	// common setting
	int m_close_log; // 0: false 1: true
	std::string m_log_level; // 日志级别
	std::string m_common_log_file; // 程序日志路径
	std::string m_fusion_log_file; // 融合数据日志路径
	std::string m_log_file_name; // 日志文件前缀名
	int m_log_max_file_size; // 单个日志文件最大size
	int m_async_log_interval; // 异步日志写入周期

	// net setting
	int m_server_count; // 服务器数量
	int m_radar_count; // 雷达数量
	int m_send_port_count; // 需要发送的端口数量
	std::string m_server_address; // 服务器ip地址
	std::string m_worker_server_address; // 工作机ip地址
	std::string m_recv_address; // 接收地址
	std::string m_send_address; // 发送地址
	std::string m_fusion_unit_group_address; // 融合单元组播地址
	std::string m_hb_group_address; // 心跳包组播地址
	std::vector<int> m_receive_ports; // 雷达报文接收端口组
	std::vector<int> m_send_ports; // 融合报文发送端口组
	int m_fusion_unit_group_port; // 融合单元组播端口
	int m_hb_group_port; // 心跳包组播端口
	int m_hb_send_interval; // 心跳包发送间隔
	int m_loop_check_interval; // 节点循环检查间隔
	int m_hb_lost_tolerance; // 未接收到心跳包最大忍耐时间
	int m_hb_lost_max_count; // 心跳包最大丢失次数

	// coordinate setting
	double m_radar1_center_hei;
	double m_radar1_center_lon;
	double m_radar1_center_lat;
	double m_radar2_center_hei;
	double m_radar2_center_lon;
	double m_radar2_center_lat;
	double m_radar3_center_hei;
	double m_radar3_center_lon;
	double m_radar3_center_lat;

	// extra setting
	int m_unitrk_max_ex_times; // 单源航迹最大外推周期
	int m_systrk_max_ex_times; // 系统航迹最大外推周期

	// fusion setting
	int m_fusion_period; // 融合周期, ms
	double m_fusion_center_hei; // 融合中心高度, m
	double m_fusion_center_lon; // 融合中心经度, m
	double m_fusion_center_lat; // 融合中心纬度, m
	double m_stray_threshold; // 离群判定阈值, m
	double m_delt_angle_threshold; // 建航航向差阈值
	double m_turn_angle_threshold; // 转向阈值
	double m_associate_threshold; // 关联阈值
	double m_fit_value_floor_limit; // 拟合度最低阈值
	int m_fit_data_control; // 对报告航迹做预处理，至少需要的报点数
	int m_use_report_stability_value; // 是否使用报告稳定值因子
	double m_report_stability_value_upper_limit; // 报告稳定因子上限值
	double m_report_stability_value_lower_limit; // 报告稳定因子下限值
	double m_report_stability_default_value; // 报告稳定值因子默认值
	double m_head_angle_trust_upper_limit; // 报告稳定因子航向值变动上限
	int m_use_frequency_value; // 是否使用频率因子
	double m_frequency_value_upper_limit; // 频率因子上限值
	double m_frequency_value_lower_limit; // 频率因子下限值
	double m_frequency_default_value; // 频率因子默认值
	double m_close_value_upper_limit; // 距离因子上限值
	double m_close_value_lower_limit; // 距离因子下限值
	double m_quality_factor_base; // 质量因子归一化
	int m_unitrk_max_alive_period; // 单源航迹最大存活周期数

	// fliter setting 矢量法 设定的卡尔曼滤波参数
	double m_unisource_x_var; // 统一设定的单源航迹x的方差值
	double m_unisource_y_var; // 统一设定的单源航迹y的方差值
	double m_unisource_z_var; // 统一设定的单源航迹z的方差值
	double m_unisource_vx_var; // 统一设定的单源航迹vx的方差值
	double m_unisource_vy_var; // 统一设定的单源航迹vy的方差值
	double m_unisource_vz_var; // 统一设定的单源航迹vz的方差值
	double m_unisource_kalman_q; // 统一设定的单源航迹过程噪声
	double m_unisource_kalman_r; // 统一设定的单源航迹观测噪声
	double m_fusion_x_var; // 统一设定的系统航迹x的方差值
	double m_fusion_y_var; // 统一设定的系统航迹y的方差值
	double m_fusion_z_var; // 统一设定的系统航迹z的方差值
	double m_fusion_vx_var; // 统一设定的系统航迹vx的方差值
	double m_fusion_vy_var; // 统一设定的系统航迹vy的方差值
	double m_fusion_vz_var; // 统一设定的系统航迹vz的方差值
	double m_fusion_ax_var; // 统一设定的系统航迹ax的方差值
	double m_fusion_ay_var; // 统一设定的系统航迹ay的方差值
	double m_fusion_az_var; // 统一设定的系统航迹az的方差值
	double m_fusion_kalman_q; // 统一设定的系统航迹过程噪声
	double m_fusion_kalman_r; // 统一设定的系统航迹观测噪声
	double m_unitrk_associate_stableSystrk_advance; // 统一设定的系统位置预测偏值
	double m_height_smoothing_alpha;
	double m_v_extra_continuous_reduce;
	double m_v_extra_x_reduce;
	double m_v_extra_y_reduce;
	double m_v_extra_z_reduce;
	double m_v_continuous_lower_limit;

	// vector advance
	double m_vector_advance; // 初始渐进量
	double m_vector_advance_add_step_size; // 渐进值增加量
	double m_vector_advance_dec_step_size; // 渐进值减少量
	double m_vector_advance_lower_value; // 渐进量最小值
	double m_vector_advance_upper_value; // 渐进量最大值
	double m_vector_advance_return_step_size; // 系统航迹渐进量自回归参数
	double m_singleTrk_qty_add;
	double m_singleTrk_qty_floor_limit_bad;
	double m_singleTrk_qty_floor_limit_normal;

	// json file
	FILE* m_json_file; // 指向json配置文件的指针
	cJSON* m_json_data; // json数据存储对象
};

#endif