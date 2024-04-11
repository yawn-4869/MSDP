#ifndef __DECODE_H__
#define __DECODE_H__

#include <list>
#include<iostream>
#include<math.h>
#include "defstruct.h"


class CDecodeAsterix
{
public:
	CDecodeAsterix(void);
	~CDecodeAsterix(void);
	unsigned char DecodeReport(unsigned char* buf, int len);
	std::list <RadarTrack> GetReportList();
private:
	unsigned char GetFRNum(unsigned char* buf);
	unsigned char DecodeTrack(unsigned char* buf, unsigned char FRN, unsigned char & TrackLen);
	unsigned char DecodeService(unsigned char* buf, unsigned char FRN, unsigned char & ServiceLen);

private:
	std::list<RadarTrack> m_Report;
};

class CDecodeCat021
{
public:
	CDecodeCat021();
	~CDecodeCat021();

	unsigned char DecodeReport(unsigned char* buf, int len);
	std::list <RadarTrack> GetReportList();
private:
	unsigned char GetFRNum(unsigned char* buf);
	unsigned char DecodeADSB(unsigned char* buf, unsigned char FRN, unsigned char & TrackLen);
private:
	std::list<RadarTrack> m_Report;
};

class CDecodeCat062
{
public:
	CDecodeCat062();
	~CDecodeCat062();
	// ???
	unsigned int FormatTrkCat062(RadarTrack tk);// , unsigned char *buf);
	std::list <char> GetCat062BufList();
	// ????// ??? ?? 021?????��????
	unsigned char DecodeReport(unsigned char* buf, int len);
	std::list <RadarTrack> GetReportList();
private:
	unsigned char GetFRNum(unsigned char* buf);
	unsigned char DecodeADSB(unsigned char* buf, unsigned char FRN, unsigned char & TrackLen);
private:
	std::list<RadarTrack> m_Report;
	std::list<char>m_Cat062Buf;
};




#endif