#include "GeograpCoordTrans.h"

#define PI_DIV180   0.0174532925  // PI/180
#define D180_DIVPI 57.2957795785  // 180/PI
#define PI 3.14159265

CGeograpCoordTrans::CGeograpCoordTrans()
{
	m_RadLo = 0.0;
}

CGeograpCoordTrans::~CGeograpCoordTrans()
{
}

// ����λ�þ�γ�ȣ� ����: loDeg ��, loMin ��, loSec ��;  γ��: laDeg ��, laMin ��, laSec��
void CGeograpCoordTrans::InitOrg(short loDeg, short loMin, double loSec, short laDeg, short laMin, double laSec)
{
	m_LoDeg = loDeg;
	m_LoMin = loMin;
	m_LoSec = loSec;
	m_LaDeg = laDeg;
	m_LaMin = laMin;
	m_LaSec = laSec;
	m_Longitude = (double)m_LoDeg + (double)m_LoMin / 60. + m_LoSec / 3600.;
	m_Latitude = (double)m_LaDeg + (double)m_LaMin / 60. + m_LaSec / 3600.;

	m_RadLo = m_Longitude * PI_DIV180; // 3.14159265 / 180.;
	m_RadLa = m_Latitude * PI_DIV180;  // 3.14159265 / 180.;
	Ec = Rj + (Rc - Rj) * (90. - m_Latitude) / 90.;
	Ed = Ec * cos(m_RadLa);
}
// ����λ�þ�γ�ȣ� ����: longitude ��, ;  γ��: latiutde ��
void CGeograpCoordTrans::InitOrg(double longitude, double latitude)
{
	m_LoDeg = short(longitude);
	m_LoMin = short((longitude - m_LoDeg) * 60);
	m_LoSec = (longitude - m_LoDeg - m_LoMin / 60.) * 3600.;

	m_LaDeg = short(latitude);
	m_LaMin = short((latitude - m_LaDeg) * 60);
	m_LaSec = (latitude - m_LaDeg - m_LaMin / 60.) * 3600.;

	m_Longitude = longitude;
	m_Latitude = latitude;
	m_RadLo = longitude * PI_DIV180; // 3.14159265 / 180.;
	m_RadLa = latitude * PI_DIV180;	 // 3.14159265 / 180.;
	Ec = Rj + (Rc - Rj) * (90. - m_Latitude) / 90.;
	Ed = Ec * cos(m_RadLa);
}

// ���뾭γ��λ�ã����������InitOrg��γ�ȵľ���ͷ�λ����λ�ֱ�Ϊ�ס���
double CGeograpCoordTrans::GetDistanceAg(short loDeg, short loMin, double loSec, short laDeg, short laMin, double laSec, double *angle)
{
	double tpdLog = (double)loDeg + (double)loMin / 60. + loSec / 3600.;
	double tpdLat = (double)laDeg + (double)laMin / 60. + laSec / 3600.;

	double RadLo = tpdLog * PI_DIV180; // 3.14159265 / 180.;
	double RadLa = tpdLat * PI_DIV180; // 3.14159265 / 180.;
	// Ec = Rj + (Rc - Rj) * (90. - m_Latitude) / 90.;
	// Ed = Ec * cos(m_RadLa);

	double dx = (RadLo - m_RadLo) * Ed;
	double dy = (RadLa - m_RadLa) * Ec;
	double out = sqrt(dx * dx + dy * dy);

	if (angle != nullptr)
	{
		*angle = atan(fabs(dx / dy)) * D180_DIVPI; // 180. / PI;
		// �ж�����
		double dLo = tpdLog - m_Longitude;
		double dLa = tpdLat - m_Latitude;

		if (dLo > 0 && dLa <= 0)
		{
			*angle = (90. - *angle) + 90.;
		}
		else if (dLo <= 0 && dLa < 0)
		{
			*angle = *angle + 180.;
		}
		else if (dLo < 0 && dLa >= 0)
		{
			*angle = (90. - *angle) + 270;
		}
	}

	return out; // unit: m ;
}
double CGeograpCoordTrans::GetDistanceAg(double longitude, double latitude, double *angle)
{
	double dx = (longitude * PI_DIV180 - m_RadLo) * Ed;
	double dy = (latitude * PI_DIV180 - m_RadLa) * Ec;
	double out = sqrt(dx * dx + dy * dy);

	if (angle != nullptr)
	{
		*angle = atan(fabs(dx / dy)) * D180_DIVPI; // 180. / PI;
		// �ж�����
		double dLo = longitude - m_Longitude;
		double dLa = latitude - m_Latitude;

		if (dLo > 0 && dLa <= 0)
		{
			*angle = (90. - *angle) + 90.;
		}
		else if (dLo <= 0 && dLa < 0)
		{
			*angle = *angle + 180.;
		}
		else if (dLo < 0 && dLa >= 0)
		{
			*angle = (90. - *angle) + 270;
		}
	}
	return out; // unit: m;
}

// ���������InitOrg��γ�ȵ�ľ���(m)�ͷ�λ(deg)�����ض�Ӧ��γ�ȣ���λΪ����
void CGeograpCoordTrans::GetTarLLPos(double distance, double angle, double *logitude, double *latitude)
{
	double dx = distance * sin(angle * PI_DIV180); // 3.14159265 / 180.);
	double dy = distance * cos(angle * PI_DIV180); // 3.14159265 / 180.);

	// double dx = (B.m_RadLo - A.m_RadLo) * A.Ed;
	// double dy = (B.m_RadLa - A.m_RadLa) * A.Ec;

	*logitude = (dx / Ed + m_RadLo) * D180_DIVPI;
	*latitude = (dy / Ec + m_RadLa) * D180_DIVPI;
}
// https://www.cnblogs.com/xingzhensun/p/11377963.html
//  bool CGeograpCoordTrans::BL2XY(double lat, double lon, double &x, double &y)
//  {
//      if (meridianLine < -180)
//      {
//          meridianLine = int((lon + 1.5) / 3) * 3;
//      }

//     lat = lat * 0.0174532925199432957692;
//     double dL = (lon - meridianLine) * 0.0174532925199432957692;

//     double X = ellipPmt.a0 * lat - ellipPmt.a2 * sin(2 * lat) / 2 + ellipPmt.a4 * sin(4 * lat) / 4 - ellipPmt.a6 * sin(6 * lat) / 6;
//     double tn = tan(lat);
//     double tn2 = tn * tn;
//     double tn4 = tn2 * tn2;

//     double j2 = (1 / pow(1 - ellipPmt.f, 2) - 1) * pow(cos(lat), 2);
//     double n = ellipPmt.a / sqrt(1.0 - ellipPmt.e2 * sin(lat) * sin(lat));

//     double temp[6] = { 0 };
//     temp[0] = n * sin(lat) * cos(lat) * dL * dL / 2;
//     temp[1] = n * sin(lat) * pow(cos(lat), 3) * (5 - tn2 + 9 * j2 + 4 * j2 * j2) * pow(dL, 4) / 24;
//     temp[2] = n * sin(lat) * pow(cos(lat), 5) * (61 - 58 * tn2 + tn4) * pow(dL, 6) / 720;
//     temp[3] = n * cos(lat) * dL;
//     temp[4] = n * pow(cos(lat), 3) * (1 - tn2 + j2) * pow(dL, 3) / 6;
//     temp[5] = n * pow(cos(lat), 5) * (5 - 18 * tn2 + tn4 + 14 * j2 - 58 * tn2 * j2) * pow(dL, 5) / 120;

//     y = X + temp[0] + temp[1] + temp[2];
//     x = temp[3] + temp[4] + temp[5];

//     if (projType == 'g')
//     {
//         x = x + 500000;
//     }
//     else if (projType == 'u')
//     {
//         x = x * 0.9996 + 500000;
//         y = y * 0.9996;
//     }

//     return true;
// }
// https://blog.csdn.net/pulian1508/article/details/120834185
double CGeograpCoordTrans::B2S(double B1, double B2) // BL2xy84函数的内部一个函数
{
	double a, b, e2, A, B, C, D, E, F, G, S;
	a = 6378137.0; // WGS_84参考椭球参数
	// f=1/298.257223563;%椭球扁率
	b = 6356752.3142; // 短轴
	e2 = (a * a - b * b) / (a * a);
	A = 1 + 3 * e2 / 4 + 45 * e2 * e2 / 64 + 175 * e2 * e2 * e2 / 256 + 11025 * pow(e2, 4) / 16384 + 43659 * pow(e2, 5) / 65536 + 693693 * pow(e2, 6) / 1048576;
	B = A - 1;
	C = 15 * e2 * e2 * e2 / 32 + 175 * e2 * e2 * e2 / 384 + 3675 * pow(e2, 4) / 8192 + 14553 * pow(e2, 5) / 32768 + 231231 * pow(e2, 6) / 524288;
	D = 35 * e2 * e2 * e2 / 96 + 735 * pow(e2, 4) / 2048 + 14553 * pow(e2, 5) / 40960 + 231231 * pow(e2, 6) / 655360;
	E = pow(e2, 4) * 315 / 1024 + pow(e2, 5) * 6237 / 20480 + pow(e2, 6) * 99099 / 327680;
	F = 693 * pow(e2, 5) / 2560 + 11011 * pow(e2, 6) / 40960;
	G = 1001 * pow(e2, 6) / 4096;
	B1 = B1 / 180 * PI;
	B2 = B2 / 180 * PI;
	S = a * (1 - e2) * (A * (B2 - B1) - B * (sin(B2) * cos(B2) - sin(B1) * cos(B1)) - C * (pow(sin(B2), 3) * cos(B2) - pow(sin(B1), 3) * cos(B1)) - D * (pow(sin(B2), 5) * cos(B2) - pow(sin(B1), 5) * cos(B1)) - E * (pow(sin(B2), 7) * cos(B2) - pow(sin(B1), 7) * cos(B1)) - F * (pow(sin(B2), 9) * cos(B2) - pow(sin(B1), 9) * cos(B1)) - G * (pow(sin(B2), 11) * cos(B2) - pow(sin(B1), 11) * cos(B1)));
	return S;
}

void CGeograpCoordTrans::BL2xy84(double lat, double lon, double L0, double &px, double &py) // 经纬度转换成平面坐标
{
	// double[] xy = new double[2];
	double X, B, L, a, b, e, e1, V, c, M, N, t, n, l, x, y;
	X = B2S(0, lon);
	B = lon / 180 * PI; // 纬度
	L = lat / 180 * PI; // 精度
	L0 = L0 / 180 * PI;
	a = 6378137.0; // WGS_84参考椭球参数
	// f=1/298.257223563;//椭球扁率
	b = 6356752.3142; // 短轴
	e = (sqrt(a * a - b * b)) / a;
	e1 = (sqrt(a * a - b * b)) / b;
	V = sqrt(1 + (e1 * e1) * (cos(B)) * (cos(B)));
	c = (a * a) / b;
	M = c / (V * V * V);
	N = c / V;
	t = tan(B);
	n = sqrt((e1 * e1) * (cos(B)) * (cos(B)));
	l = L - L0;
	px = X + N * t * cos(B) * cos(B) * l * l * (0.5 + (1 / 24) * (5 - t * t + 9 * n * n + 4 * n * n * n * n) * cos(B) * cos(B) * l * l + 1 / 720 * (61 - 58 * t * t + t * t * t * t) * pow((cos(B)), 4) * l * l * l * l);
	py = N * cos(B) * l * (1 + 1 / 6 * (1 - t * t + n * n) * cos(B) * cos(B) * l * l + 1 / 120 * (5 - 18 * t * t + t * t * t * t + 14 * n * n - 58 * t * t * n * n) * pow((cos(B)), 4) * l * l * l * l);
}

void CGeograpCoordTrans::LoLaHtoDxSpaceXYZ(double L, double B, double &X, double &Y)
{
	// double a = 6378137.0;
	// double e2 = 0.00669438002290;	//转换为弧度
	// L = L * PI/180;
	// B = B * PI/180;

	// double fac1 = 1- e2*sin(B)*sin(B);
	// double N = a/sqrt(fac1); //卯酉圈曲率半径
	// double Daita_h = 0;      //高程异常,默认为0

	// X = (N)*cos(B)*cos(L);
	// Y = (N)*cos(B)*sin(L);

	// double l = 6381372 * PI * 2;//地球周长
	//  double W=l;// 平面展开后，x轴等于周长
	//  double H=l/2;// y轴约等于周长一半
	//  double mill=2.3;// 米勒投影中的一个常数，范围大约在正负2.3之间
	//  double x = l * PI / 180;// 将经度从度数转换为弧度
	//  double y = B *PI / 180;// 将纬度从度数转换为弧度
	//  y=1.25 * log( tan( 0.25 *PI + 0.4 * y ) );// 米勒投影的转换
	//  // 弧度转为实际距离
	//  X = ( W / 2 ) + ( W / (2 * PI) ) * x;
	//  Y = ( H / 2 ) - ( H / ( 2 * mill ) ) * y;

	L = L * PI / 180;
	B = B * PI / 180;

	double B0 = 30 * PI / 180;
	double N = 0, e = 0, a = 0, b = 0, e2 = 0, K = 0;
	a = 6378137;
	b = 6356752.3142;
	e = sqrt(1 - (b / a) * (b / a));
	e2 = sqrt((a / b) * (a / b) - 1);
	double CosB0 = cos(B0);
	N = (a * a / b) / sqrt(1 + e2 * e2 * CosB0 * CosB0);
	K = N * CosB0;

	double Pi = PI;
	double SinB = sin(B);

	double tanV = tan(Pi / 4 + B / 2);
	double E2 = pow((1 - e * SinB) / (1 + e * SinB), e / 2);
	double xx = tanV * E2;

	X = K * log(xx);
	Y = K * L;
	
}
void CGeograpCoordTrans::xy2RhoTheta(double x, double y, double &rho, double &theta)
{
	rho = sqrt(x*x+y*y);
	double tan = y/x;
	theta = atan2(x,y)*D180_DIVPI;//deg
	// cout<<"x = "<<x<<"   y = "<<y<<"   theta = "<<theta<<endl;

}