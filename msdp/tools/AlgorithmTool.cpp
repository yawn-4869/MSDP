#include <sys/time.h>
#include "AlgorithmTool.h"
#include "Kalman.h"

using namespace Eigen;
using namespace std;

#define MAXN 310
#define INF 0x3f3f3f3f
#define PI 3.1415926

int nx, ny;
double lx[MAXN], ly[MAXN], slack[MAXN],w[MAXN][MAXN];
int usex[MAXN], usey[MAXN],link[MAXN];

int dfs(int x)
{
	usex[x] = 1;
	for (int y = 0; y < ny; y++)
	{
		if (usey[y])continue;
		double t = lx[x] + ly[y] - w[x][y];
		if (t == 0)
		{
			usey[y] = 1;
			if (link[y] == -1 || dfs(link[y]))
			{
				link[y] = x;
				return 1;
			}
		}
		else if (slack[y]>t)
			slack[y] = t;
	}
	return 0;
}

//二分图最佳匹配 （kuhn munkras 算法 O(m*m*n)).
//邻接矩阵形式 。  返回最佳匹配值（权值最大匹配），传入二分图大小m*n
//m表示目标，n表示量测
//邻接矩阵 mat ，mat[i][j]表示权，表示目标i关联到量测j的权值
//match返回一个最佳匹配,
//match[i] = k,表示目标i匹配量测k
//一定注意m<=n，否则循环无法终止，最小权匹配可将权值取负数
double KM(vector<vector<double>> ass,int* match )
{
	nx = ass.size();
	if (nx == 0)
		return 0;
	ny = ass[0].size();

	for(int i = 0;i < nx; ++i)
		for (int j = 0; j < ny; ++j)
			w[i][j] = ass[i][j];

	int i, j;
	memset(link, -1, sizeof(link));
	memset(ly, 0, sizeof(ly));
	for (i = 0; i < nx; i++)
		for (j = 0, lx[i] = -INF; j < ny; j++)
			if (lx[i]<w[i][j])
				lx[i] = w[i][j];
	for (int x = 0; x < nx; x++)
	{
		for (i = 0; i < ny; i++)
			slack[i] = INF;
		while (1)
		{
			memset(usex, 0, sizeof(usex));
			memset(usey, 0, sizeof(usey));

			if (dfs(x))
				break;
			double d = INF;
			for (i = 0; i < ny; i++)
				if (!usey[i] && d>slack[i])
					d = slack[i];
			for (i = 0; i < nx; i++)
			{
				if (usex[i])
					lx[i] -= d;
			}
			for (i = 0; i < ny; i++)
			{
				if (usey[i])
					ly[i] += d;
				else
					slack[i] -= d;
			}
		}
	}
	memset(match, -1, sizeof(int)*MAXN);
	double ans = 0;
	for (i = 0; i < ny; i++){
		if (link[i] > -1)
		{
			ans += w[link[i]][i];
			match[link[i]] = i;
		}	
	}
	return ans;
}


/*==================polyfit(n,x,y,poly_n,a)===================*/
/*=======拟合y=a0+a1*x+a2*x^2+……+apoly_n*x^poly_n========*/
/*=====n是数据个数 xy是数据值 poly_n是多项式的项数======*/
/*===返回a0,a1,a2,……a[poly_n]，系数比项数多一（常数项）=====*/
void gauss_solve(int n,double A[],double x[],double b[]);
void polyfit(int n, double x[], double y[], int poly_n, double p[])
{
	int i, j;
	double *tempx, *tempy, *sumxx, *sumxy, *ata;

	tempx = (double *)calloc(n, sizeof(double));
	sumxx = (double *)calloc((poly_n * 2 + 1), sizeof(double));
	tempy = (double *)calloc(n, sizeof(double));
	sumxy = (double *)calloc((poly_n + 1), sizeof(double));
	ata = (double *)calloc((poly_n + 1)*(poly_n + 1), sizeof(double));
	for (i = 0; i<n; i++)
	{
		tempx[i] = 1;
		tempy[i] = y[i];
	}
	for (i = 0; i<2 * poly_n + 1; i++)
	{
		for (sumxx[i] = 0, j = 0; j<n; j++)
		{
			sumxx[i] += tempx[j];
			tempx[j] *= x[j];
		}
	}
	for (i = 0; i<poly_n + 1; i++)
	{
		for (sumxy[i] = 0, j = 0; j<n; j++)
		{
			sumxy[i] += tempy[j];
			tempy[j] *= x[j];
		}
	}
	for (i = 0; i<poly_n + 1; i++)
	{
		for (j = 0; j<poly_n + 1; j++)
		{
			ata[i*(poly_n + 1) + j] = sumxx[i + j];
		}
	}
	gauss_solve(poly_n + 1, ata, p, sumxy);

	free(tempx);
	free(sumxx);
	free(tempy);
	free(sumxy);
	free(ata);
}
/*============================================================*/
////	高斯消元法计算得到	n 次多项式的系数
////	n: 系数的个数
////	ata: 线性矩阵
////	sumxy: 线性方程组的Y值
////	p: 返回拟合的结果
/*============================================================*/
void gauss_solve(int n, double A[], double x[], double b[])
{
	int i, j, k, r;
	double max;
	for (k = 0; k<n - 1; k++)
	{
		max = fabs(A[k*n + k]);					// find maxmum 
		r = k;
		for (i = k + 1; i<n - 1; i++)
		{
			if (max<fabs(A[i*n + i]))
			{
				max = fabs(A[i*n + i]);
				r = i;
			}
		}
		if (r != k)
		{
			for (i = 0; i<n; i++)		//change array:A[k]&A[r]
			{
				max = A[k*n + i];
				A[k*n + i] = A[r*n + i];
				A[r*n + i] = max;
			}
			max = b[k];                    //change array:b[k]&b[r]
			b[k] = b[r];
			b[r] = max;
		}

		for (i = k + 1; i<n; i++)
		{
			for (j = k + 1; j<n; j++)
				A[i*n + j] -= A[i*n + k] * A[k*n + j] / A[k*n + k];
			b[i] -= A[i*n + k] * b[k] / A[k*n + k];
		}
	}

	for (i = n - 1; i >= 0; x[i] /= A[i*n + i], i--)
	{
		for (j = i + 1, x[i] = b[i]; j<n; j++)
			x[i] -= A[i*n + j] * x[j];
	}
}

//求阶乘
int factorial(int n)
{
	int sum=1;
	for(int i = 1; i <= n; ++i)
	{
		sum *= i;
	}
	return sum;
}

//create a state transition matrix of a given order for a given time step `dt`.
MatrixXd kinematic_state_transition(int order,double dt)
{
	//hard code common cases for computational efficiency
	if(order == 0)
	{
		MatrixXd rt(1,1);
		rt<<1;
		return rt;
	}
	if(order == 1)
	{
		MatrixXd rt(2,2);
		rt<<1,dt,
			0,1;
		return rt;
	}
	if(order == 2)
	{
		MatrixXd rt(3,3);
		rt<<1,dt,0.5*dt*dt,
			0,1,dt,
			0,0,1;
		return rt;
	}

	//grind it out computationally....
	int N = order+1;
	MatrixXd rt = MatrixXd::Zero(N,N);
	for(int i = 0; i < N; i++)
	{
		rt(0,i) = pow(dt,i) / factorial(i);
	}
    //copy with a shift to get lower order rows
	for(int i = 1; i < N; i++)
	{
		for(int j = i; j < N; j++)
		{
			rt(i,j) = rt(i-1,j-1);
		}
	}
	return rt;
}


/*dim:表示维数。例如：dim=2，表示平面上的运动
order:表示阶数。例如：order =1.表示匀速运动，
order=2，表示匀加速度运动，order=3，表示匀加加速度运动
dt：表示周期*/
Kalman get_kinematic_kf(int dim, int order, double dt)
{
	
	int dim_x = order + 1;
	int N = dim*dim_x;
	Kalman rt(dim*dim_x, dim);
	MatrixXd F = kinematic_state_transition(order, dt);
	for(int i = 0; i < dim; ++i)
	{
		rt.F.block(i*dim_x,i*dim_x,dim_x,dim_x) = F;
		rt.H(i,i*dim_x) = 1;
	}
	return rt;
}

/*
size：   x,mean:(dim,1)     cov:(dim,dim)
out： return the log of the probability density function evaluated at `x`
*/
double mmultivariate_normal_logpdf(MatrixXd x,MatrixXd mean,MatrixXd cov)
{
	int k = x.rows();
	MatrixXd tmp = x-mean;
	double zs = ((tmp.transpose())*(cov.inverse())*tmp)(0,0);
	double rt = log(exp(-0.5*zs) / sqrt(pow(2*3.141592653589,k) * cov.determinant()));
	return rt;
}



//Parameters
//	-----------
//
//dim : must int (2, 3, or 4)
//	  dimension for Q, where the final dimension is (dim x dim)
//
//dt : float, default=1.0
//	 time step in whatever units your filter is using for time. i.e. the
//	 amount of time between innovations
//
//var : float, default=1.0
//	  variance in the noise
//
//block_size : int >= 1
//			 If your state variable contains more than one dimension, such as
//			 a 3d constant velocity model [x x' y y' z z']^T, then Q must be
//			 a block diagonal matrix.
//
MatrixXd Q_discrete_white_noise(int dim, double dt, double var, int block_size)
{
	MatrixXd Q(dim,dim);
	if(dim == 2)
	{
		Q<<0.25*pow(dt,4),0.5*pow(dt,3),
		   0.5*pow(dt,3) ,pow(dt,2) ;
	}
	if(dim == 3)
	{
		Q<<0.25*pow(dt,4),0.5*pow(dt,3),0.5*pow(dt,2),
			0.5*pow(dt,3) ,pow(dt,2)    ,dt,
			0.5*pow(dt,2) ,dt           ,1;
	}
	if(dim == 4)
	{
		Q<<pow(dt,6)/36,pow(dt,5)/12,pow(dt,4)/6,pow(dt,3)/6,
		   pow(dt,5)/12,pow(dt,4)/4 ,pow(dt,3)/2,pow(dt,2),
		   pow(dt,4)/6 ,pow(dt,3)/2 ,pow(dt,2)  ,dt,
		   pow(dt,3)/6 ,pow(dt,2)/2 ,dt         ,1;
	}
	int d = dim*block_size;
	MatrixXd rt = MatrixXd::Zero(d,d);;
	for(int i = 0;i < d;i += dim)
	{
		rt.block(i,i,dim,dim) = Q;
	}
	rt *= var;
	return rt;
}

 // Thefunction will return YES if the point x,y is inside the polygon, or
 // NOif it is not.  If the point is exactly on the edge of the polygon,
 //then the function may return YES or NO.

 //Note that division by zero is avoided because the division is protected
 // bythe "if" clause which surrounds it.
bool pnpoly(int nvert, double *vertx, double *verty, double testx, double testy)
{
	int i, j;
	bool c = 0;
	for (i = 0, j = nvert-1; i < nvert; j = i++) {
		if ( ((verty[i]>testy) != (verty[j]>testy)) && 
			(testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / 
			(verty[j]-verty[i]) + vertx[i]) )
			c = !c;
	}
	return c;
}

double Head(double vx,double vy){
	double m=atan2(vy,vx);
	return AngleToHead(m);
}

double Head(double x1,double y1,double x2,double y2){
	double dx=x2-x1,dy=y2-y1;
	return Head(dx,dy);
}

double HeadToAngle(double m){
	m-=90;
	if(m>=-90&&m<=180) m=-m;
	else m=360-m;
	m=m/180*PI;
	return m;
}

double AngleToHead(double h){
	h=h/PI*180;
	h=-(h-90);
	if(h<0) h+=360;
	return h;
}

double Dhead(double h1,double h2){
	double dh=h2-h1;
	if(dh>180) dh-=360;
	if(dh<-180) dh+=360;
	return dh;
}

double Ahead(double h1,double theta){
	double h=h1+theta;
	if(h>360) h-=360;
	return h;
}

double Speed(double vx,double vy,double vz){
	return sqrt(vx*vx+vy*vy+vz*vz);
}

void Separation(double s,double h,double& x,double& y){
	double m=HeadToAngle(h);
	x=s*cos(m);
	y=s*sin(m);
}

void Separation(double s,double h,double p,double& x,double& y,double&z){
	p=p/180*PI;
	double m=HeadToAngle(h);
	x=s*cos(m)*cos(p);
	y=s*sin(m)*cos(p);
	z=s*sin(p);
}

double Pitch(double vx,double vy,double vz){
	double xy_speed=Speed(vx,vy,0);
	double m=atan2(vz,xy_speed);
	return m/PI*180;
}

double Distance(double x1,double y1,double x2,double y2){
	double dx=x2-x1,dy=y2-y1;
	return Speed(dx,dy,0);
}

double Distance(double x1,double y1,double z1,double x2,double y2,double z2){
	double dx=x2-x1,dy=y2-y1,dz=z2-z1;
	return Speed(dx,dy,dz);
}

double get_VofPitch(double vx,double vy,double vz,double x,double y,double z){
	double r = sqrt(x*x + y*y + z*z);
	double a = -x*z;
	double b = -y*z;
	double c = y*y + x*x;
	return (a*vx + b*vy + c*vz)/(sqrt(a*a + b*b + c*c) * r);
}

double get_VofRadial(double vx,double vy,double vz,double x,double y,double z){
	return (x*vx + y*vy + z*vz)/sqrt(x*x + y*y + z*z);
}

double get_VofOrientation(double vx,double vy,double vz,double x,double y,double z){
	return (y*vx - x*vy)/(y*y + x*x);
}