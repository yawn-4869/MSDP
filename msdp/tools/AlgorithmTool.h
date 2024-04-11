#pragma once
#include<string.h>
#include <vector>
#include <math.h>

#include <Eigen/Dense>

class Kalman;


//KM算法求解全剧最近邻问题
double KM(std::vector<std::vector<double>> ass,int* match );

Eigen::MatrixXd kinematic_state_transition(int order,double dt);

//拟合二次曲线
void polyfit(int n, double *x, double *y, int poly_n, double p[]);

//返回一个符合牛顿运动学的卡尔曼滤波器
Kalman get_kinematic_kf(int dim, int order, double dt); 

//计算在均值mean，方差cov的多元正态分布概率密度函数下，取x时的函数值
double mmultivariate_normal_logpdf(Eigen::MatrixXd x, Eigen::MatrixXd mean, Eigen::MatrixXd cov);

//判断点是否在一个多边形内
bool pnpoly(int nvert, double *vertx, double *verty, double testx, double testy);

//返回一个高速噪声矩阵
Eigen::MatrixXd Q_discrete_white_noise(int dim, double dt, double var, int block_size);

//根据分速度计算航向
double Head(double vx,double vy);

//根据位置计算航向
double Head(double x1,double y1,double x2,double y2);

//航向转角度
double HeadToAngle(double m);

//角度转航向
double AngleToHead(double m);

//计算航向差
double Dhead(double h1,double h2);

//计算航向和
double Ahead(double h1,double theta);

//计算合速度
double Speed(double vx,double vy,double vz);

//计算分速度
void Separation(double s,double h,double& x,double& y);

//计算分速度
void Separation(double s,double h,double p,double& x,double& y,double&z);

//计算俯仰角
double Pitch(double vx,double vy,double vz);

//计算距离
double Distance(double x1,double y1,double x2,double y2);
double Distance(double x1,double y1,double z1,double x2,double y2,double z2);

//名称：计算俯仰速度
double get_VofPitch(double vx,double vy,double vz,double x,double y,double z);

//名称：计算径向速度
double get_VofRadial(double vx,double vy,double vz,double x,double y,double z);

//名称：计算方向速度
double get_VofOrientation(double vx,double vy,double vz,double x,double y,double z);