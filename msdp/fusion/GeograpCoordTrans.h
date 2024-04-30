#pragma once

#include <math.h>
#include<iostream>

static double Rc = 6378137;
static double Rj = 6356725;

class CGeograpCoordTrans
{
public:
	CGeograpCoordTrans();
	~CGeograpCoordTrans();

private:
	short m_LoDeg, m_LoMin; double m_LoSec;  
	short m_LaDeg, m_LaMin; double m_LaSec;  
	double m_Longitude, m_Latitude;    
	double m_RadLo, m_RadLa;
	double Ec;
	double Ed;

public:
	void InitOrg(short loDeg, short loMin, double loSec, short laDeg, short laMin, double laSec);

	void InitOrg(double longitude, double latitude);

	double GetDistanceAg(short loDeg, short loMin, double loSec, short laDeg, short laMin, double laSec, double *angle);
	double GetDistanceAg(double longitude, double latitude, double *angle);

	void GetTarLLPos(double distance, double angle, double *logitude, double*latitude);

	bool BL2XY(double lat, double lon, double &x, double &y);
	double B2S(double B1, double B2);
	void BL2xy84(double lon,double lat, double L0,double &px,double &py);
	void LoLaHtoDxSpaceXYZ(double L,double B,double &X,double &Y);
	void xy2RhoTheta(double x,double y,double &rho,double &theta);
};

