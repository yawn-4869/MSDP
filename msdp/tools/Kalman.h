#pragma once
#include <Eigen/Dense>

/*
Parameters
----------
dim_x : int
Number of state variables for the Kalman filter. For example, if
you are tracking the position and velocity of an object in two
dimensions, dim_x would be 4.
This is used to set the default size of P, Q, and u

dim_z : int
Number of of measurement inputs. For example, if the sensor
provides you with position in (x,y), dim_z would be 2.

Attributes
----------
x : numpy.array(dim_x, 1)
Current state estimate. Any call to update() or predict() updates
this variable.

P : numpy.array(dim_x, dim_x)
Current state covariance matrix. Any call to update() or predict()
updates this variable.

x_prior : numpy.array(dim_x, 1)
Prior (predicted) state estimate. The *_prior and *_post attributes
are for convienence; they store the  prior and posterior of the
current epoch. Read Only.

P_prior : numpy.array(dim_x, dim_x)
Prior (predicted) state covariance matrix. Read Only.

x_post : numpy.array(dim_x, 1)
Posterior (updated) state estimate. Read Only.

P_post : numpy.array(dim_x, dim_x)
Posterior (updated) state covariance matrix. Read Only.

z : numpy.array
Last measurement used in update(). Read only.

R : numpy.array(dim_z, dim_z)
Measurement noise matrix

Q : numpy.array(dim_x, dim_x)
Process noise matrix

F : numpy.array()
State Transition matrix

H : numpy.array(dim_z, dim_x)
Measurement function

y : numpy.array
Residual of the update step. Read only.

K : numpy.array(dim_x, dim_z)
Kalman gain of the update step. Read only.

S :  numpy.array
System uncertainty (P projected to measurement space). Read only.

SI :  numpy.array
Inverse system uncertainty. Read only.

likelihood : float
likelihood of last measurement. Read only.
*/

class Kalman
{
public:
	Kalman(int dimx,int dimz);
	//根据状态转换矩阵F来预测下一个时刻的状态
	void predict();
	//根据量测信息z，进行合并，给出最优估计
	void update(Eigen::VectorXd z);   
	
	int dim_x;          //状态转移矩阵的维数
	int dim_z;          //量测矩阵的维数

	Eigen::VectorXd x;         //状态矩阵
	Eigen::MatrixXd P;         //状态估计协方差
	Eigen::MatrixXd Q;         //过程噪声
	Eigen::MatrixXd F;         //状态转移矩阵
	Eigen::MatrixXd H;         //量测转换矩阵
	Eigen::MatrixXd R;         //观测噪声

	double likelihood;   //上次量测的可能性


	Kalman(void);
	~Kalman(void);
private:
	Eigen::MatrixXd K;         //增益矩阵
	Eigen::MatrixXd y;         //新息
	Eigen::MatrixXd S;
	Eigen::MatrixXd SI;
	Eigen::VectorXd z;         //上次的测量值

	Eigen::MatrixXd I;         //单位阵

    //these will always be a copy of x,P after predict() is called，used for checked
    Eigen::MatrixXd x_prior; 
    Eigen::MatrixXd P_prior;

    //these will always be a copy of x,P after update() is called，used for checked
    Eigen::MatrixXd x_post;
	Eigen::MatrixXd P_post;

};

