#include <iostream>
#include <float.h>
#include "Kalman.h"
#include "AlgorithmTool.h"

using namespace Eigen;
using namespace std;

Kalman::Kalman(void)
{

}


Kalman::Kalman(int dimx, int dimz)
{
	dim_x = dimx;
	dim_z = dimz;

	x = MatrixXd::Zero(dimx,1);
	P = MatrixXd::Identity(dimx,dimx);
	Q = MatrixXd::Identity(dimx,dimx);
	F = MatrixXd::Identity(dimx,dimx);
	H = MatrixXd::Zero(dimz,dimx);
	R = MatrixXd::Identity(dimz,dimz);
	K = MatrixXd::Zero(dimx,dimz);
	y = MatrixXd::Zero(dimz,1);
	S = MatrixXd::Zero(dimz,dimz);
	SI = MatrixXd::Zero(dimz,dimz);
	z = MatrixXd::Zero(dimz,1);
	I = MatrixXd::Identity(dimx,dimx);
	likelihood = DBL_MIN;
	x_prior = x;
	P_prior = P;
	x_post = x;
	P_post = P;
}


void Kalman::predict()
{
    //x = Fx 
	x = F * x;

    //P = FPF' + Q
    P = F*P*(F.transpose() ) + Q;

    //save prior
    x_prior = x;
    P_prior = P;
}

//z : (dim_z, 1)
void Kalman::update(VectorXd z)
{
	//y = z - Hx,error (residual) between measurement and prediction
	y = z - (H*x);

    //common subexpression for speed
	MatrixXd  PHT = P*(H.transpose() );

    //S = HPH' + R
    //project system uncertainty into measurement space
	S = H*PHT + R;
    SI = S.inverse();

    //K = PH'inv(S)
    //map system uncertainty into kalman gain
    K = PHT*SI;

    //x = x + Ky
    //predict new x with residual scaled by the kalman gain
	x += K*y;
    //P = (I-KH)P(I-KH)' + KRK'
    //This is more numerically stable and works for non-optimal K vs the equation P = (I-KH)P usually seen in the literature.

	MatrixXd I_KH = I - K*H;
	P = I_KH*P*(I_KH.transpose()) + K*R*(K.transpose()); 
	//P = I_KH *P;

	//computer the likelihood of the measurement z
	MatrixXd u = MatrixXd::Zero(dim_z,1);
	double log_li = mmultivariate_normal_logpdf(y,u,S);
	likelihood = exp(log_li);
	if(fabs(likelihood -1) <= 0.0000001)
		likelihood = DBL_MIN;

    //save measurement and posterior state
	this->z = z;
    x_post = x;
	P_post = P;
}

Kalman::~Kalman(void)
{

}
