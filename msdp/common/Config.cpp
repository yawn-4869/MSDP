#include <string.h>
#include "Config.h"

Config::Config()
{
	m_log_level="DEBUG";
}

Config::Config(const char* jsonfile) {
	init(jsonfile);
}

Config::~Config()
{
	
}

static Config* g_config = NULL;
Config* Config::get_instance() {
	return g_config;
}

void Config::setGlobalConfig(const char* jsonfile) {
	if(g_config) {
		return;
	}

	if(jsonfile != NULL) {
		g_config = new Config(jsonfile);
	} else {
		g_config = new Config();
	}
}

bool Config::init(const char* jsonfile)
{
	// 读取配置文件
	m_json_file = fopen(jsonfile, "r");
	if (m_json_file == NULL)
	{
		printf("open json file failed!\n");
		return false;
	}
	fseek(m_json_file, 0, SEEK_END);
	long file_length = ftell(m_json_file);
	fseek(m_json_file, 0, SEEK_SET);
	char* file_buf = (char*) malloc(sizeof(char) * file_length + 1);
	// 初始化buffer
	memset(file_buf, 0, file_length + 1);
	fread(file_buf, sizeof(char), file_length, m_json_file);
	// 读取配置文件完毕
	fclose(m_json_file);
	file_buf[file_length] = '\0';
	// printf("file buf = %s\n", file_buf);
	m_json_data = cJSON_Parse(file_buf);
	free(file_buf);

	if(m_json_data == NULL) 
	{
		printf("parse json data failed!\n");
		return false;
	}

	// 解析json
	// common setting
	m_close_log = cJSON_GetObjectItem(m_json_data, "close_log")->valueint;
	m_common_log_file = cJSON_GetObjectItem(m_json_data, "common_log_file_path")->valuestring;
	m_fusion_log_file = cJSON_GetObjectItem(m_json_data, "fusion_log_file_path")->valuestring;
	m_log_file_name = cJSON_GetObjectItem(m_json_data, "log_file_name")->valuestring;
	m_async_log_interval = cJSON_GetObjectItem(m_json_data, "async_log_interval")->valueint;
	m_log_max_file_size = cJSON_GetObjectItem(m_json_data, "log_max_file_size")->valueint;
	m_log_level = cJSON_GetObjectItem(m_json_data, "log_level")->valuestring;

	printf("LOG -- CONFIG LEVEL[%s], LOG_FILE_NAME[%s],COMMON_FILE_PATH[%s], FUSION_FILE_PATH[%s], MAX_FILE_SIZE[%d B], ASYNC_INTEVAL[%d ms]\n", 
    m_log_level.c_str(), m_log_file_name.c_str(), m_common_log_file.c_str(), m_fusion_log_file.c_str(), m_log_max_file_size, m_async_log_interval);

	// net setting
	m_server_count = cJSON_GetObjectItem(m_json_data, "server_count")->valueint;
	m_radar_count = cJSON_GetObjectItem(m_json_data, "radar_count")->valueint;
	m_send_port_count = cJSON_GetObjectItem(m_json_data, "send_port_count")->valueint;
	m_server_address = cJSON_GetObjectItem(m_json_data, "server_address")->valuestring;
	m_worker_server_address = cJSON_GetObjectItem(m_json_data, "worker_server_address")->valuestring;
	m_recv_address = cJSON_GetObjectItem(m_json_data, "recv_address")->valuestring;
	m_send_address = cJSON_GetObjectItem(m_json_data, "send_address")->valuestring;
	m_fusion_unit_group_address = cJSON_GetObjectItem(m_json_data, "fusion_unit_group_address")->valuestring;
	m_hb_group_address = cJSON_GetObjectItem(m_json_data, "hb_group_address")->valuestring;
	m_receive_ports.resize(m_radar_count);
	for(int i = 0; i < m_radar_count; ++i) {
		std::string port_name = "receive_port_" + std::to_string(i+1);
		m_receive_ports[i] = cJSON_GetObjectItem(m_json_data, port_name.c_str())->valueint;
	}
	m_send_ports.resize(m_send_port_count);
	for(int i = 0; i < m_send_port_count; ++i) {
		std::string port_name = "send_port_" + std::to_string(i+1);
		m_send_ports[i] = cJSON_GetObjectItem(m_json_data, port_name.c_str())->valueint;
	}
	m_fusion_unit_group_port = cJSON_GetObjectItem(m_json_data, "fusion_unit_group_port")->valueint;
	m_hb_group_port = cJSON_GetObjectItem(m_json_data, "hb_group_port")->valueint;
	m_hb_send_interval = cJSON_GetObjectItem(m_json_data, "hb_send_interval")->valueint;
	m_loop_check_interval = cJSON_GetObjectItem(m_json_data, "loop_check_interval")->valueint;
	m_hb_lost_tolerance = cJSON_GetObjectItem(m_json_data, "hb_lost_tolerance")->valueint;
	m_hb_lost_max_count = cJSON_GetObjectItem(m_json_data, "hb_lost_max_count")->valueint;

	// coordinate setting
	m_radar1_center_hei = cJSON_GetObjectItem(m_json_data, "radar1_center_hei")->valuedouble;
	m_radar1_center_lon = cJSON_GetObjectItem(m_json_data, "radar1_center_lon")->valuedouble;
	m_radar1_center_lat = cJSON_GetObjectItem(m_json_data, "radar1_center_lat")->valuedouble;
	m_radar2_center_hei = cJSON_GetObjectItem(m_json_data, "radar2_center_hei")->valuedouble;
	m_radar2_center_lon = cJSON_GetObjectItem(m_json_data, "radar2_center_lon")->valuedouble;
	m_radar2_center_lat = cJSON_GetObjectItem(m_json_data, "radar2_center_lat")->valuedouble;
	m_radar3_center_hei = cJSON_GetObjectItem(m_json_data, "radar3_center_hei")->valuedouble;
	m_radar3_center_lon = cJSON_GetObjectItem(m_json_data, "radar3_center_lon")->valuedouble;
	m_radar3_center_lat = cJSON_GetObjectItem(m_json_data, "radar3_center_lat")->valuedouble;

	// extra setting
	m_unitrk_max_ex_times = cJSON_GetObjectItem(m_json_data, "unitrk_max_ex_times")->valueint;
	m_systrk_max_ex_times = cJSON_GetObjectItem(m_json_data, "systrk_max_ex_times")->valueint;
	
	// fusion setting
	m_fusion_period = cJSON_GetObjectItem(m_json_data, "fusion_period")->valueint;
	m_fusion_center_hei = cJSON_GetObjectItem(m_json_data, "fusion_center_hei")->valuedouble;
	m_fusion_center_lon = cJSON_GetObjectItem(m_json_data, "fusion_center_lon")->valuedouble;
	m_fusion_center_lat = cJSON_GetObjectItem(m_json_data, "fusion_center_lat")->valuedouble;
	m_stray_threshold = cJSON_GetObjectItem(m_json_data, "stary_threshold")->valuedouble;
	m_delt_angle_threshold = cJSON_GetObjectItem(m_json_data, "delt_angle_threshold")->valuedouble;
	m_turn_angle_threshold = cJSON_GetObjectItem(m_json_data, "turn_angle_threshold")->valuedouble;
	m_associate_threshold = cJSON_GetObjectItem(m_json_data, "associate_threshold")->valuedouble;
	m_fit_value_floor_limit = cJSON_GetObjectItem(m_json_data, "fit_value_floor_limit")->valuedouble;
	m_fit_data_control = cJSON_GetObjectItem(m_json_data, "fit_data_control")->valueint;
	m_use_report_stability_value = cJSON_GetObjectItem(m_json_data, "use_report_stability_value")->valueint;
	m_report_stability_value_upper_limit = cJSON_GetObjectItem(m_json_data, "report_stability_value_upper_limit")->valuedouble;
	m_report_stability_value_lower_limit = cJSON_GetObjectItem(m_json_data, "report_stability_value_lower_limit")->valuedouble;
	m_report_stability_default_value = cJSON_GetObjectItem(m_json_data, "report_stability_default_value")->valuedouble;
	m_head_angle_trust_upper_limit = cJSON_GetObjectItem(m_json_data, "head_angle_trust_upper_limit")->valuedouble;
	m_use_frequency_value = cJSON_GetObjectItem(m_json_data, "use_frequency_value")->valueint;
	m_frequency_value_upper_limit = cJSON_GetObjectItem(m_json_data, "frequency_value_upper_limit")->valuedouble;
	m_frequency_value_lower_limit = cJSON_GetObjectItem(m_json_data, "frequency_value_lower_limit")->valuedouble;
	m_frequency_default_value = cJSON_GetObjectItem(m_json_data, "frequency_default_value")->valuedouble;
	m_close_value_upper_limit = cJSON_GetObjectItem(m_json_data, "close_value_upper_limit")->valuedouble;
	m_close_value_lower_limit = cJSON_GetObjectItem(m_json_data, "close_value_lower_limit")->valuedouble;
	m_unitrk_max_alive_period = cJSON_GetObjectItem(m_json_data, "unitrk_max_alive_period")->valueint;

	// fliter setting
	m_unisource_x_var = cJSON_GetObjectItem(m_json_data, "unisource_x_var")->valuedouble;
	m_unisource_y_var = cJSON_GetObjectItem(m_json_data, "unisource_y_var")->valuedouble;
	m_unisource_z_var = cJSON_GetObjectItem(m_json_data, "unisource_z_var")->valuedouble;
	m_unisource_vx_var = cJSON_GetObjectItem(m_json_data, "unisource_vx_var")->valuedouble;
	m_unisource_vy_var = cJSON_GetObjectItem(m_json_data, "unisource_vy_var")->valuedouble;
	m_unisource_vz_var = cJSON_GetObjectItem(m_json_data, "unisource_vz_var")->valuedouble;
	m_unisource_kalman_q = cJSON_GetObjectItem(m_json_data, "unisource_kalman_q")->valuedouble;
	m_unisource_kalman_r = cJSON_GetObjectItem(m_json_data, "unisource_kalman_r")->valuedouble;
	m_fusion_x_var = cJSON_GetObjectItem(m_json_data, "fusion_x_var")->valuedouble;
	m_fusion_y_var = cJSON_GetObjectItem(m_json_data, "fusion_y_var")->valuedouble;
	m_fusion_z_var = cJSON_GetObjectItem(m_json_data, "fusion_z_var")->valuedouble;
	m_fusion_vx_var = cJSON_GetObjectItem(m_json_data, "fusion_vx_var")->valuedouble;
	m_fusion_vy_var = cJSON_GetObjectItem(m_json_data, "fusion_vy_var")->valuedouble;
	m_fusion_vz_var = cJSON_GetObjectItem(m_json_data, "fusion_vz_var")->valuedouble;
	m_fusion_ax_var = cJSON_GetObjectItem(m_json_data, "fusion_ax_var")->valuedouble;
	m_fusion_ay_var = cJSON_GetObjectItem(m_json_data, "fusion_ay_var")->valuedouble;
	m_fusion_az_var = cJSON_GetObjectItem(m_json_data, "fusion_az_var")->valuedouble;
	m_fusion_kalman_q = cJSON_GetObjectItem(m_json_data, "fusion_kalman_q")->valuedouble;
	m_fusion_kalman_r = cJSON_GetObjectItem(m_json_data, "fusion_kalman_r")->valuedouble;
	m_unitrk_associate_stableSystrk_advance = cJSON_GetObjectItem(m_json_data, "unitrk_associate_stableSystrk_advance")->valuedouble;
	m_height_smoothing_alpha = cJSON_GetObjectItem(m_json_data, "height_smoothing_alpha")->valuedouble;
	m_v_extra_continuous_reduce = cJSON_GetObjectItem(m_json_data, "v_extra_continuous_reduce")->valuedouble;
	m_v_extra_x_reduce = cJSON_GetObjectItem(m_json_data, "v_extra_x_reduce")->valuedouble;
	m_v_extra_y_reduce = cJSON_GetObjectItem(m_json_data, "v_extra_y_reduce")->valuedouble;
	m_v_extra_z_reduce = cJSON_GetObjectItem(m_json_data, "v_extra_z_reduce")->valuedouble;
	m_v_continuous_lower_limit = cJSON_GetObjectItem(m_json_data, "v_continuous_lower_limit")->valuedouble;

	// vector advance
	m_vector_advance = cJSON_GetObjectItem(m_json_data, "vector_advance")->valuedouble;
	m_vector_advance_add_step_size = cJSON_GetObjectItem(m_json_data, "vector_advance_add_step_size")->valuedouble;
	m_vector_advance_dec_step_size = cJSON_GetObjectItem(m_json_data, "vector_advance_dec_step_size")->valuedouble;
	m_vector_advance_lower_value = cJSON_GetObjectItem(m_json_data, "vector_advance_lower_value")->valuedouble;
	m_vector_advance_upper_value = cJSON_GetObjectItem(m_json_data, "vector_advance_upper_value")->valuedouble;
	m_vector_advance_return_step_size = cJSON_GetObjectItem(m_json_data, "vector_advance_return_step_size")->valuedouble;
	m_singleTrk_qty_add = cJSON_GetObjectItem(m_json_data, "singleTrk_qty_add")->valuedouble;
	m_singleTrk_qty_floor_limit_bad = cJSON_GetObjectItem(m_json_data, "singleTrk_qty_floor_limit_bad")->valuedouble;
	m_singleTrk_qty_floor_limit_normal = cJSON_GetObjectItem(m_json_data, "singleTrk_qty_floor_limit_normal")->valuedouble;

	// 解析完毕
	cJSON_Delete(m_json_data);
	m_json_data = NULL;

	m_quality_factor_base = 0;
	m_quality_factor_base += m_use_report_stability_value ? m_report_stability_value_upper_limit : m_report_stability_default_value;
	m_quality_factor_base += m_use_frequency_value ? m_frequency_value_upper_limit : m_frequency_default_value;
}


