
#pragma once
#include <list>
#include <string>

#define PI 3.14159265

typedef struct tagTrackHisPt
{
	double HisLon;
	double HisLat;
} TrackHis;

/*
typedef struct tagFilterPara
{
	unsigned char ManeuverIndex;
	double Exk;
	double Eyk;
	double vx;
	double vy;
	double px;
	double py;
}FilterPara;
*/

typedef struct tagTrack
{
	int id; // data source 1--021 2--001
	long Address;
	// int TrackNo;
	long TrackNo;

	int SSR;
	std::string callNo;
	double fX;	  // m
	double fY;	  // m
	bool xyflg;	  // 0 -- no xy pos, 1-- xy pos
	double rho;	  // m
	double theta; // deg
	bool rtflg;	  //  0 --- no rho theta pos, 1-- rho,theta pos
	int Hei;	  // ft
	double Lon;
	double Lat;	   // deg
	double vec;	   // m/s
	double cource; // deg
	double vz;	   // vertical speed   ft/min
	long Time; // Unit : s


	long currTime;	  // us
	int extraCount;	  // 外推次数
	long afterExtraT; // 外推后时间

public:
	void InitInstance()
	{
		id = 0;
		Address = 0xffffff;
		TrackNo = -1;
		SSR = 0;
		callNo = "";
		fX = 0.0;
		fY = 0.0;
		xyflg = 0;
		rho = 0.0;
		theta = 0.0;
		rtflg = 0;
		Lon = 0.0;
		Lat = 0.0;
		Hei = 0;
		vec = 0.0;
		cource = 0.0;
		vz = 0.0;
		Time = 0;
		extraCount = 0;
		afterExtraT = 0;
	}
} RadarTrack;

/*
typedef struct tagReport
{
	BYTE Hour;
	BYTE Min;
	BYTE Second;
	BYTE MSec;  // *10ms

	int TrackNo;
	int SSR;
	double fX;      //m
	double fY;      //m
	char xyflg;     // 0 -- no xy pos, 1-- xy pos
	double rho;     //m
	double theta;   // deg
	char rtflg;    //  0 --- no rho theta pos, 1-- rho,theta pos
	int Hei;        // ft
	double vec;     // m/s
	double cource;  //deg
	unsigned char state;

}ReportInfo;

  */
