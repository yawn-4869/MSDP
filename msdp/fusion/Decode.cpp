#include "Decode.h"
#include <time.h>

#define LL_LSB_RECIP  186413.5111  //2^25/180

CDecodeAsterix::CDecodeAsterix(void)
{
}

CDecodeAsterix::~CDecodeAsterix(void)
{
}

unsigned char CDecodeAsterix::DecodeReport(unsigned char* buf, int len)
{
	if(len < 8)
		return 0;

	unsigned short frameoffset,offset,Length;
	unsigned char FXNum=0; 
	unsigned char Cat = 0;
	unsigned char OneMsgLen = 0;
   
	//TRACE("\n========");
    frameoffset = 0;  // skip SAC SIC
    while( frameoffset < len - 5 ) 
    {
		Cat = buf[frameoffset];
		Length = buf[frameoffset+1] * 256 + buf[frameoffset+2];  // 2 UINT8s
		//TRACE("\ pack Len %d  ", Length);
		if ( Length == 0 ||Length > 250) 
			return 0;      

		//endbuf=buf+frameoffset+Length;
        //Asterix ICD      
        switch( Cat )
		{
		case 1 : // Target Message
			offset = frameoffset + 3;// skip cat ,length(2bytes)
			while( offset < frameoffset + Length )
			{
				if(offset > len - 5) 
					return 0;
				
				FXNum = GetFRNum( buf + offset);
				
				if(FXNum==0) 
					return 0;
				
				if( *(buf +offset+FXNum+2) & 0x80 ) //����
				{
					DecodeTrack(buf + offset,FXNum,OneMsgLen);//解码航迹
					offset += OneMsgLen;					
				}
				else //�㼣
				{		
					break;//解码点迹			
				}

			}   // end while( offset < frameoffset+Length )
			break;
			
		case 2 : 
			offset = frameoffset + 3;    
			FXNum = GetFRNum( buf+offset);
			if(FXNum==0) return 0;
			DecodeService(buf + offset, FXNum,OneMsgLen);

			break;
		default: 
			break;
		} // end switch( Cat )

		frameoffset += Length;
    } // end while( frameoffset< len-2 ) 
	
	return 0;
}

unsigned char CDecodeAsterix::GetFRNum(unsigned char* buf)
{
	unsigned char Num = 0;
	while( (*(buf + Num)) & 0x01 )  
	{
		Num++;

		if(Num >  3) 
			break;
	}
	Num++;
	return Num;
}


// buf from FRN 
unsigned char CDecodeAsterix::DecodeTrack(unsigned char* buf, unsigned char FRN, unsigned char & TrackLen)
{
	unsigned char *pbuf;
	
	RadarTrack Report;

	Report.InitInstance();
	/*
	Report.TrackNo = -1;
	Report.SSR = 0;
	Report.Hei = 0;
	Report.fX = 0.0;
	Report.fY = 0.0;
	Report.xyflg = 0;
	Report.rho = 0.0;
	Report.theta = 0.0;
	Report.rtflg = 0;
	Report.vec = 0.0;
	Report.cource = 0.0;
	Report.HisFlg = 0;
	Report.CourceFlg = 0;
*/
	pbuf = buf + FRN;
	
	//I001/010
	if(buf[0] & 0x80)
		pbuf += 2; 

	// I001/020 Target
	if(buf[0] & 0x40)
	{
		while( *pbuf&0x01 )	
			pbuf++;
		pbuf ++;
	}

	// I001/161 TrackNumber
	if(buf[0] & 0x20)
	{
		Report.TrackNo = pbuf[0] * 256 + pbuf[1];
		pbuf += 2;		
	}

	// I001/040
	if(buf[0] & 0x10)
	{
		// Polar Position
		Report.rho = (double)(pbuf[0] * 256 + pbuf[1]) * 1852.0 / 128.0;   //m
		Report.theta = (double)(pbuf[2] * 256 + pbuf[3]) * 360.0 / 65536.0;  //deg
		// std::cout<<"rho = "<<Report.rho<<"   theta = "<<Report.theta<<std::endl;
		// std::cout<<"rho-theta: fX = "<<Report.rho*cos(Report.theta*PI/180.0)<<"   fY = "<<Report.rho*sin(Report.theta*PI/180.0)<<std::endl;
		// std::cout<<"rho-theta: fX = "<<Report.rho*sin(Report.theta*PI/180.0)<<"   fY = "<<Report.rho*cos(Report.theta*PI/180.0)<<std::endl;
		Report.rtflg = 1;
		pbuf += 4;
	}

	// I001/042
	if(buf[0] & 0x08)
	{
		// x,y
		Report.fX = (double)((short)(pbuf[0] * 256 + pbuf[1])) * 1852.0/64.0;// m
		Report.fY = (double)((short)(pbuf[2] * 256 + pbuf[3])) * 1852.0/64.0;// m
		// std::cout<<"fX = "<<Report.fX<<"   fY = "<<Report.fY<<std::endl;
		Report.xyflg = 1;
		pbuf += 4;
	}

	// I001/200
	if(buf[0] & 0x04)
	{
		// speed
		Report.vec = (double)(pbuf[0] * 256 + pbuf[1]) * 1852.0/16384.0;// m/s
		Report.cource = (double)(pbuf[2] * 256 + pbuf[3]) * 360.0/65536.0;// deg
		pbuf += 4;
	}

	// I001/070
	if(buf[0] & 0x02)
	{
		// SSR 
		Report.SSR = (pbuf[0] & 0x0f) * 256 + pbuf[1];
		pbuf += 2;		
	}

	if(buf[0] & 0x01)
	{
		// I001/090
		if(buf[1] & 0x80)
		{
			// Height
			Report.Hei = ((pbuf[0] & 0x3f) * 256 + pbuf[1] ) * 25;// ft:英尺 1英寸 = 0.3048m
			pbuf += 2;
		}

		// I001/141
		if(buf[1] & 0x40)
		{
			// Time
			pbuf += 2;
		}

		// I001/130
		if(buf[1] & 0x20)
		{
			while( *pbuf&0x01 )	
				pbuf++;
			pbuf ++;
		}

		// I001/131
		if(buf[1] & 0x10)
			pbuf ++;

		// I001/120
		if(buf[1] & 0x08)
			pbuf ++;

		// I001/170
		if(buf[1] & 0x04)
		{
			while( *pbuf&0x01 )	
				pbuf++;
			pbuf ++;
		}

		// I001/210
		if(buf[1] & 0x02)
		{
			while( *pbuf&0x01 )	
				pbuf++;
			pbuf ++;
		}
		
		if(buf[1] & 0x01)
		{
			// I001/050
			if(buf[2] & 0x80)
				pbuf += 2;

			// I001/080
			if(buf[2] & 0x40)
				pbuf += 2;

			// I001/100
			if(buf[2] & 0x20)
				pbuf += 4;

			// I001/060
			if(buf[2] & 0x10)
				pbuf += 2;

			// I001/030
			if(buf[2] & 0x08)
			{
				while( *pbuf&0x01 ) 	
					pbuf++;
				pbuf ++;
			}
		}


	}

	//Report.ShowX = 0;
	//Report.ShowY = 0;
	//Report.offsetX = 0;
	//Report.offsetY = 0;
	//Report.ChangFlag = 0;
	
	
	if(Report.TrackNo != -1)
	{
		if((Report.xyflg == 1) || (Report.rtflg == 1))
			m_Report.push_back(Report);
	}

	TrackLen = (unsigned char ) (pbuf - buf);
	return 0;
}

unsigned char CDecodeAsterix::DecodeService(unsigned char* buf, unsigned char FRN, unsigned char & ServiceLen)
{
	unsigned char *pbuf;
	pbuf = buf + FRN;

	unsigned char Sector;
	//1   I002/010  
	if( *buf & 0x80 ) 
		pbuf+=2;		
	
	int type=0;
	// 2   I002/000
	if( *buf & 0x40 ) 
		type = *pbuf++;	
	    //1-North��2-Sector
	
	// 3   I002/020  ������             1
	// ��16����������ʱ��λ8-5λ�����ţ�λ4-1Ϊȫ0
	// ��32����������ʱ��λ8-4λ�����ţ�λ3-1Ϊȫ0
	//������˵��������Ϊ32������
	if( *buf & 0x20 )     //�����ţ�1�ֽ�
	{	
		Sector = *pbuf++;	
		Sector = Sector/8;
	}
	// 4   I002/030
	if( *buf& 0x10 ) 
		pbuf+=3;  

	// 5   I002/041 
	if( *buf & 0x08 )
		pbuf+=2;
	
	// 6   I002/050 
	if ( *buf & 0x04 ) 
	{		
		while(*pbuf& 0x01)
			pbuf++; 
		pbuf++;   
	}

	// 7   I002/060 
	if ( *buf & 0x02 )    
	{
		while(*pbuf& 0x01)
			pbuf++; 
		pbuf++;   
	}	
	
	ServiceLen = (int)(pbuf - buf);

	return 0;
}

std::list <RadarTrack> CDecodeAsterix::GetReportList()
{
	std::list <RadarTrack> RepList;
	RepList.swap(m_Report);
	return RepList;
}

CDecodeCat021::CDecodeCat021()
{
}


CDecodeCat021::~CDecodeCat021()
{
}

std::list <RadarTrack> CDecodeCat021::GetReportList()
{
	std::list <RadarTrack> RepList;
	RepList.swap(m_Report);
	return RepList;
}

unsigned char CDecodeCat021::DecodeReport(unsigned char* buf, int len)
{
	int repLen = (int)((buf[1] << 8) + buf[2]);
	if (len != repLen)
	{
		printf("**** Atten 1: ADSB rep len err %d , %d     \n", len , repLen);
		return 0;
	}
	
	if (0x15 != buf[0])
	{
		printf("**** Atten 2: ADSB type err %x  \n", buf[0]);// , repLen);
		return 0;
	}

	unsigned short frameoffset, offset, Length;
	unsigned char FXNum = 0;
	unsigned char Cat = 0;
	unsigned char OneMsgLen = 0;

	//TRACE("\n========");
	frameoffset = 0;  
	while (frameoffset < len - 5) // 
	{
		Cat = buf[frameoffset];
		Length = (buf[frameoffset + 1] << 8) + buf[frameoffset + 2];  // 2 UINT8s
	
		if (Length == 0 || Length > 250)
		{
			printf("**** Atten 3: ADSB rep len err %d      \n", Length);
			return 0;
		}


		//endbuf=buf+frameoffset+Length;
		//Asterix ICD  ADS-B CAT 021  
		
		offset = frameoffset + 3;// skip cat ,length(2bytes)
		while (offset < frameoffset + Length)
		{
			if (offset > len - 5)
			{
				//printf("**** Atten 4: ADSB rep len err %d   %d   \n", offset, len-5);
				return 0;
			}


			FXNum = GetFRNum(buf + offset);

			if (FXNum == 0)
			{
				//printf("**** Atten 5: ADSB rep len err %d     \n", FXNum);
				return 0;
			}

			
			DecodeADSB(buf + offset, FXNum, OneMsgLen);
			offset += OneMsgLen;
					
		}   // end while( offset < frameoffset+Length )
		

		frameoffset += Length;
	} // end while( frameoffset< len-2 )
	return 0;
}


unsigned char CDecodeCat021::GetFRNum(unsigned char* buf)
{
	unsigned char Num = 0;
	while ((*(buf + Num)) & 0x01)
	{
		Num++;

		if (Num >  7)  // cat 021 -v2.1 FRN ���Ϊ7
			break;
	}
	Num++;
	return Num;
}

unsigned char CDecodeCat021::DecodeADSB(unsigned char* buf, unsigned char FRN, unsigned char & TrackLen)
{
	unsigned char *pbuf;
	RadarTrack Report;
	Report.InitInstance();

	pbuf = buf + FRN;

	// ADS-B CAT021 -V2.1 ICD UAP

	// FRN #1
	//I021/010 SAC +SIC  2B
	if (buf[0] & 0x80)
		pbuf += 2;

	// I021/040 TargetReport Des 1+
	if (buf[0] & 0x40)
	{
		while (*pbuf & 0x01)
			pbuf++;
		pbuf++;
	}

	// I021/161 TrackNumber 2B
	if (buf[0] & 0x20)
	{
		Report.TrackNo = (pbuf[0] << 8) + pbuf[1];
		pbuf += 2;
		//printf("**** ADSB TAR TNO  %d     \n", Report.TrackNo);
	}

	// I021/015 Service Id 1B
	if (buf[0] & 0x10)
	{
		pbuf += 1;
	}

	// I021/071 Time of A for Pos 3B
	if (buf[0] & 0x08)
	{
		Report.Time = ((pbuf[0] << 16) + (pbuf[1]<<8) + pbuf[2]) >> 7;  // LSB :1/128s  
		pbuf += 3;
	}
	// I021/130 Pos WGS-84, 6B
	if (buf[0] & 0x04)
	{
		Report.Lat = ((pbuf[0] << 16) + (pbuf[1] << 8) + pbuf[2]) * 0.00002145767;  // LSB :180/2(23) deg  
		Report.Lon = ((pbuf[3] << 16) + (pbuf[4] << 8) + pbuf[5]) * 0.00002145767;  // LSB :180/2(23) deg 
		pbuf += 6;
		//printf("**** ADSB130 TAR WGS-84: Lat  %f  Lon  %f   \n", Report.Lat, Report.Lon);
	}
	// I021/131 Pos WGS-84 High Res, 8B
	if (buf[0] & 0x02)
	{
		Report.Lat = ((pbuf[0] << 24) + (pbuf[1] << 16) + (pbuf[2] << 8) + pbuf[3]) * 0.00000016764;  // LSB :180/2(30) deg  
		Report.Lon = ((pbuf[4] << 24) + (pbuf[5] << 16) + (pbuf[6] << 8) + pbuf[7]) * 0.00000016764;  // LSB :180/2(30) deg 
		pbuf += 8;
		//printf("**** ADSB131 TAR WGS-84 High Res: Lat  %f  Lon  %f   \n", Report.Lat, Report.Lon);
	}
	// FRN #2
	if (buf[0] & 0x01)
	{
		// I021/072 Time of App for V, 3B
		if (buf[1] & 0x80) // skip
		{
			pbuf += 3;
		}
		// I021/150 AirSpeed, 2B
		if (buf[1] & 0x40) // skip
		{
			pbuf += 2;
		}
		// I021/151 TrueAirSpeed, 2B
		if (buf[1] & 0x20) // skip
		{
			pbuf += 2;
		}
		// I021/080 TargetAdd, 3B
		if (buf[1] & 0x10) 
		{
			Report.Address = (pbuf[0] << 16) + (pbuf[1] << 8) + pbuf[2];
			pbuf += 3;

			//printf("***&&&& ADSB TAR Add  0x%x     \n", Report.Address);
		}
		//printf("***&&&& ADSB TAR Add  0x%x     \n", Report.Address);
		// I021/073 Time of Recp for Pos, 071/073������һ��, 3B 
		if ((buf[1] & 0x08) && ((buf[0] & 0x08)!=1))
		{
			Report.Time = ((pbuf[0] << 16) + (pbuf[1] << 8) + pbuf[2]) >> 7;  // LSB :1/128s 
			pbuf += 3;
		}
		// I021/074 Time of Recp for Pos - High Pre, 4B
		if (buf[1] & 0x04) // skip
		{
			pbuf += 4;
		}
		// I021/075 Time of Recp for Velocity, 3B
		if (buf[1] & 0x02) // skip
		{
			pbuf += 3;
		}

		//FRN #3
		if (buf[1] & 0x01)
		{
			// I021/076 Time of Recp for Velocity - High Pre, 4B
			if (buf[2] & 0x80)// skip
			{
				pbuf += 4;
			}
			// I021/140 GeoHeight, 2B
			if (buf[2] & 0x40) // skip
			{
				pbuf += 2;
			}
			// I021/090 Quan Ind, 1+B
			if (buf[2] & 0x20) // skip
			{
				while (*pbuf & 0x01)
					pbuf++;
				pbuf++;
			}
			// I021/210 MOPS Ver, 1B
			if (buf[2] & 0x10) // skip
			{
				pbuf += 1;
			}
			// I021/070 SSR Code, 2B
			if (buf[2] & 0x08) 
			{
				Report.SSR = (pbuf[0] << 8) + pbuf[1];
				pbuf += 2;
			}
			// I021/230 Roll Ang, 2B
			if (buf[2] & 0x04) // skip
			{
				pbuf += 2;
			}
			// I021/145 FL, 2B
			if (buf[2] & 0x02)
			{
				Report.Hei = ((pbuf[0] << 8) + pbuf[1]) * 25;   // LSB : 1/4 FL = 25ft
				pbuf += 2;
			}

			//FRN #4
			if (buf[2] & 0x01)
			{
				// I021/152 Meg Heading, 2B
				if (buf[3] & 0x80)// skip
				{
					pbuf += 2;
				}
				// I021/200 Target Status, 1B
				if (buf[3] & 0x40)// skip
				{
					pbuf += 1;
				}
				// I021/155 Bar Vz, 2B
				if (buf[3] & 0x20)// skip
				{
					pbuf += 2;
				}
				// I021/157 Geo Vz, 2B
				if (buf[3] & 0x10)//
				{
					Report.vz = ((pbuf[0] << 8) + pbuf[1]) * 6.25;   // LSB : 6.25 ft/min
					pbuf += 2;
				}
				// I021/160 Ground Speed, 4B
				if (buf[3] & 0x08)//
				{
					Report.vec = ((pbuf[0] << 8) + pbuf[1]) * 0.113;   // LSB : 2(-14) NM/s = 1852 * 2(-14) m/s = 0.113 m/s
					Report.cource = ((pbuf[2] << 8) + pbuf[3]) * 0.0055;   // LSB : 360/2(16) deg = 0.0055deg
					pbuf += 4;
				}
				// I021/165 Heading Angle Rate, 2B
				if (buf[3] & 0x04)// skip
				{
					pbuf += 2;
				}
				// I021/077 Time of Rep Trans, 3B
				if (buf[3] & 0x02)// skip
				{
					pbuf += 3;
				}

				//FRN #5
				if (buf[3] & 0x01)
				{
					// I021/170 CallNum, 6B
					if (buf[4] & 0x80)// 
					{
						Report.callNo = "";
						//std::string s1; 
						unsigned char SixBit;
						SixBit = (pbuf[0] & 0xFC) >> 2; //CHAR 1
						if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
							SixBit += 0x40; //6bit的ASCII 对， 字符需要加上0x40
						if (((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
							|| ((SixBit >= 0x41) && (SixBit <= 0x5A)))
							Report.callNo += SixBit;
						SixBit = ((pbuf[0] & 0x03) << 4) + ((pbuf[1] & 0xF0) >> 4); // CHAR 2
						if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
							SixBit += 0x40; //6bit的ASCII 对， 字符需要加上0x40
						if (((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
							|| ((SixBit >= 0x41) && (SixBit <= 0x5A)))
							Report.callNo += SixBit;
						SixBit = ((pbuf[1] & 0x0F) << 2) + ((pbuf[2] & 0xC0) >> 6); //CHAR 3
						if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
							SixBit += 0x40; //6bit的ASCII 对， 字符需要加上0x40
						if (((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
							|| ((SixBit >= 0x41) && (SixBit <= 0x5A)))
							Report.callNo += SixBit;
						SixBit = pbuf[2] & 0x3F; //CHAR 4
						if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
							SixBit += 0x40; //6bit的ASCII 对， 字符需要加上0x40
						if (((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
							|| ((SixBit >= 0x41) && (SixBit <= 0x5A)))
							Report.callNo += SixBit;
						
						pbuf += 3;

						SixBit = (pbuf[0] & 0xFC) >> 2; //CHAR 5
						if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
							SixBit += 0x40; //6bit的ASCII 对， 字符需要加上0x40
						if (((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
							|| ((SixBit >= 0x41) && (SixBit <= 0x5A)))
							Report.callNo += SixBit;
						SixBit = ((pbuf[0] & 0x03) << 4) + ((pbuf[1] & 0xF0) >> 4); // CHAR 6
						if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
							SixBit += 0x40; //6bit的ASCII 对， 字符需要加上0x40
						if (((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
							|| ((SixBit >= 0x41) && (SixBit <= 0x5A)))
							Report.callNo += SixBit;
						SixBit = ((pbuf[1] & 0x0F) << 2) + ((pbuf[2] & 0xC0) >> 6); //CHAR 7
						if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
							SixBit += 0x40; //6bit的ASCII 对， 字符需要加上0x40
						if (((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
							|| ((SixBit >= 0x41) && (SixBit <= 0x5A)))
							Report.callNo += SixBit;
						SixBit = pbuf[2] & 0x3F; //CHAR 8
						if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
							SixBit += 0x40; //6bit的ASCII 对， 字符需要加上0x40
						if (((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
							|| ((SixBit >= 0x41) && (SixBit <= 0x5A)))
							Report.callNo += SixBit;
						pbuf += 3;
					}

					// I021/020 EmitCat , 1B
					if (buf[4] & 0x40)// skip
					{
						pbuf += 1;
					}
					// I021/220 Met Info, 1+B
					if (buf[4] & 0x20) // skip
					{
						while (*pbuf & 0x01)
							pbuf++;
						pbuf++;
					}
					// I021/146 SelectAlt, 2B
					if (buf[4] & 0x10)// skip
					{
						pbuf += 2;
					}
					// I021/148 Final state SelectAlt, 2B
					if (buf[4] & 0x08)// skip
					{
						pbuf += 2;
					}
					// I021/110 Track Intent, 1+B
					if (buf[4] & 0x04) // skip
					{
						while (*pbuf & 0x01)
							pbuf++;
						pbuf++;
					}
					// I021/016 Service Manage, 1B
					if (buf[4] & 0x02)// skip
					{
						pbuf += 1;
					}

					//FRN #6
					if (buf[4] & 0x01)
					{
						// I021/008 Operate Status, 1B
						if (buf[5] & 0x80)// skip
						{
							pbuf += 1;
						}
						// I021/271 Surface Cap, 1+B
						if (buf[5] & 0x40) // skip
						{
							while (*pbuf & 0x01)
								pbuf++;
							pbuf++;
						}
						// I021/132 MessageAmp, 1B
						if (buf[5] & 0x20)// skip
						{
							pbuf += 1;
						}
						// I021/250 ModeS MB, 1+N*8B
						if (buf[5] & 0x10)// skip
						{
							pbuf += pbuf[0] * 8;
							pbuf += 1;
						}

						// I021/260 ACRS RA, 7B
						if (buf[5] & 0x08)// skip
						{
							pbuf += 7;
						}
						// I021/400 Recive ID, 1B
						if (buf[5] & 0x04)// skip
						{
							pbuf += 1;
						}
						// I021/295,DataAge 1+B
						if (buf[5] & 0x02)// skip
						{
							while (*pbuf & 0x01)
								pbuf++;
							pbuf += 1;
						}
					}
				}
			}
		}
	}

	if (Report.TrackNo != -1)
	{
		m_Report.push_back(Report);
//		printf("push back succeed!\n");
	}
	TrackLen = (unsigned char)(pbuf - buf);
	return 0;
}

CDecodeCat062::CDecodeCat062()
{
}


CDecodeCat062::~CDecodeCat062()
{
}

// ���������ݰ�CAT062�������
// input tk: ���鱨�ĺ�����Ϣ
// output buf: ��cat062�����Ļ�����������CAT, LEN, FRN, DATA,����m_Cat062Buf�б��У���GetCat062BufList����
// output len : buf�����ݳ���
// ������ݴ��ģʽ�����ݵĸ�λ�ŵ͵�ַ�����ݵĵ�λ���ڸߵ�ַ
unsigned int CDecodeCat062::FormatTrkCat062(RadarTrack tk)//, unsigned char *buf)
{
	
	// ����frn
	std::list <unsigned char> frnLst;
	frnLst.clear();
	unsigned char frn = 0;

	// �������յ�buf�б�
	m_Cat062Buf.clear();
	char buf;

	int tmp = 0; // �м�ת��ʹ�õ���ʱ����
	//buf[0] = 62;// �̶�CAT��ʶ
	//F1-8 I062/010 Data Source Id, SAC/SIC 2B , M
	buf = 0;
	m_Cat062Buf.push_back(buf);
	buf = 0;
	m_Cat062Buf.push_back(buf);
	frn |= 0x80;

	//F1-7 Reserve
	//F1-6 I062/015 Service Id, 1B, O,����д
	//F1-5 I062/070 LocalTime, 3B,M
	time_t curtime, midtime;
	struct tm *localtm_p;	
	curtime = time(NULL);
	localtm_p = localtime(&curtime);
	localtm_p->tm_hour = 0;
	localtm_p->tm_min = 0;
	localtm_p->tm_sec = 0;
	midtime = mktime(localtm_p);
	tmp = (int)(difftime(curtime, midtime) *128);  // uint : 1/128s
	// ������ݴ�ţ���λ�ŵ͵�ַ
	buf = (tmp & 0xff0000) >> 16;
	m_Cat062Buf.push_back(buf);
	buf = (tmp & 0x00ff00) >> 8;
	m_Cat062Buf.push_back(buf);
	buf = tmp & 0x0000ff;
	m_Cat062Buf.push_back(buf);
	frn |= 0x10;
	//F1-4 I062/105,WGS-84 POS, 8B, O���������д,LSB : 180/2^25, �����ò����ʾ
	if ((tk.Lon > 0.0) || (tk.Lat > 0.0))
	{
		// ά��
		tmp = (int)(tk.Lat * LL_LSB_RECIP);
		buf = (tmp & 0xff000000) >> 24;
		m_Cat062Buf.push_back(buf);
		buf = (tmp & 0x00ff0000) >> 16;
		m_Cat062Buf.push_back(buf);
		buf = (tmp & 0x0000ff00) >> 8;
		m_Cat062Buf.push_back(buf);
		buf = tmp & 0x000000ff;
		m_Cat062Buf.push_back(buf);
		
		// ����
		tmp = (int)(tk.Lon * LL_LSB_RECIP);
		buf = (tmp & 0xff000000) >> 24;
		m_Cat062Buf.push_back(buf);
		buf = (tmp & 0x00ff0000) >> 16;
		m_Cat062Buf.push_back(buf);
		buf = (tmp & 0x0000ff00) >> 8;
		m_Cat062Buf.push_back(buf);
		buf = tmp & 0x000000ff;
		m_Cat062Buf.push_back(buf);
		
		frn |= 0x08;
	}

	//F1-3 I062/100,Cartesian POS, 6B, O
	if (tk.xyflg == 1)
	{
		tmp = (int)(tk.fX * 2.0); // LSB 0.5m
		buf = (tmp & 0xff0000) >> 16;
		m_Cat062Buf.push_back(buf);
		buf = (tmp & 0x00ff00) >> 8;
		m_Cat062Buf.push_back(buf);
		buf = tmp & 0x0000ff;
		m_Cat062Buf.push_back(buf);
		

		tmp = (int)(tk.fY * 2.0); // LSB 0.5m
		buf = (tmp & 0xff0000) >> 16;
		m_Cat062Buf.push_back(buf);
		buf = (tmp & 0x00ff00) >> 8;
		m_Cat062Buf.push_back(buf);
		buf = tmp & 0x0000ff;
		m_Cat062Buf.push_back(buf);

		frn |= 0x04;
	}
	//F1-2 I062/185,Cartesian Spd, 4B, O
	if (tk.vec > 0.0) // m/s
	{
		double hd = tk.cource * 0.01745329; // rad����,����������ʼ��˳ʱ�뷽��
		// Vx
		double partv = tk.vec * sin(hd);
		tmp = (int)(partv * 4.0); // LSB 0.25m/s
		buf = (tmp & 0xff00) >> 8;
		m_Cat062Buf.push_back(buf);
		buf = tmp & 0x00ff;
		m_Cat062Buf.push_back(buf);
		
		//Vy
		partv = tk.vec * cos(hd);
		tmp = (int)(partv * 4.0); // LSB 0.25m/s
		buf = (tmp & 0xff00) >> 8;
		m_Cat062Buf.push_back(buf);
		buf = tmp & 0x00ff;
		m_Cat062Buf.push_back(buf);
		
		frn |= 0x02;
	}

	frn |= 0x01;  //(F2 - 3 I062 / 080, State)�Ǳش�����FRN ������2����������չ
	frnLst.push_back(frn);  // First FRN

	frn = 0; // ׼����2��frn

	//F2-8 I062/210,Cartesian Acceleration, 2B, O
	//F2-7 I062/060,3/A code, 2B, O
	if (tk.SSR != 0)
	{
		tmp = tk.SSR;
		buf = (tmp & 0x0f00) >> 8;
		m_Cat062Buf.push_back(buf);
		buf = tmp & 0x00ff;
		m_Cat062Buf.push_back(buf);
		
		frn |= 0x40;
	}
	//F2-6 I062/245,ID-Callsign(0x41-0x5a,0x61-7a,0x31-3a), 7B, O
	tmp = tk.callNo.size();
	int i = 0;
	if ((tmp >= 5) && (tmp <= 8)) // ���ų���5-8���ַ�
	{		
		for (i = 0; i < tmp; i++)
		{
			if ((tk.callNo[i] >= 0x41) && (tk.callNo[i] <= 0x5A)) // A-Z
				tk.callNo[i] -= 0x40;  // ��A-Z�ַ�����8bit  ת��Ϊ  6bit   ASCII����			
		}		
		
		buf = 0;
		m_Cat062Buf.push_back(buf);
		// callNo[i] ��Ϊ6bit����  
		buf = (tk.callNo[0] << 2) + ((tk.callNo[1]&0x30) >> 4);
		m_Cat062Buf.push_back(buf);
		buf = ((tk.callNo[1] & 0x0f)  << 4) + ((tk.callNo[2] & 0x3c )>>2);
		m_Cat062Buf.push_back(buf);
		buf = ((tk.callNo[2] & 0x03 ) << 6) + tk.callNo[3] ;
		m_Cat062Buf.push_back(buf);
		if (tmp == 6)
		{
			buf = (tk.callNo[4] << 2) + ((tk.callNo[5] & 0x30) >> 4);
			m_Cat062Buf.push_back(buf);
			buf = ((tk.callNo[5] & 0x0f) << 4);
			m_Cat062Buf.push_back(buf);
			buf = 0;
			m_Cat062Buf.push_back(buf);
		}			
		else if (tmp == 7)
		{
			buf = (tk.callNo[4] << 2) + ((tk.callNo[5] & 0x30) >> 4);
			m_Cat062Buf.push_back(buf);
			buf = ((tk.callNo[5] & 0x0f) << 4) + ((tk.callNo[6] & 0x3c) >> 2);
			m_Cat062Buf.push_back(buf);
			buf = ((tk.callNo[6] & 0x03) << 6);
			m_Cat062Buf.push_back(buf);
		}
		else if (tmp == 8)
		{
			buf = (tk.callNo[4] << 2) + ((tk.callNo[5] & 0x30) >> 4);
			m_Cat062Buf.push_back(buf);
			buf = ((tk.callNo[5] & 0x0f) << 4) + ((tk.callNo[6] & 0x3c) >> 2);
			m_Cat062Buf.push_back(buf);
			buf = ((tk.callNo[6] & 0x03) << 6) + tk.callNo[7];
			m_Cat062Buf.push_back(buf);
		}
	
		frn |= 0x20;
	}
	//F2-5 I062/380,Aircraft Derived Data, 1B+, O // �ܶ�����

	//F2-4 I062/040,Tno, 2B, O
	if (tk.TrackNo != 0xffff)
	{
		buf = (tk.TrackNo & 0xff00) >> 8;
		m_Cat062Buf.push_back(buf);
		buf = (tk.TrackNo & 0xff);
		m_Cat062Buf.push_back(buf);
		
		frn |= 0x08;
	}
	//F2-3 I062/080,State, 1B+, M, �ɽ϶���д����䣬��ʱ�ȸ�0
	buf = 0;
	m_Cat062Buf.push_back(buf);
	frn |= 0x04;
	//F2-2 I062/290,trk upd age, 1B+, O
	// ������ʷ��¼����Ϣ����ʱ����

	if (tk.Hei > 0 || tk.vz > 0.0001 /*|| tk.His.size() > 0*/)  // ��д��3��---��5��frnʱ 23/03/10 modify
	{
		frn |= 0x01;
		frnLst.push_back(frn);  // Second FRN

		frn = 0; // ׼����3��frn
		//F3-8 I062/200,Mode of Movement, 1B, O���Ƿ�ת�䡢�����½��ȣ��ݲ���

		//F3-7 I062/295,Trk data age, 1B+, O�� ��290���ƣ��ݲ���

		//F3-6 I062/136,Measure FL, 2B, O�� �����߶�  0.25FL(25ft)Ϊ��λ���ݲ���
		//F3-5 I062/130,Geo Alt, 2B, O   �������ƽ�漸�θ߶ȣ�6.25ftΪ��λ
		if (tk.Hei > 0)
		{
			tmp = (int)((double)tk.Hei / 6.25);
			buf = (tmp & 0xff00) >> 8;
			m_Cat062Buf.push_back(buf);
			buf = (tmp & 0xff);
			m_Cat062Buf.push_back(buf);

			frn |= 0x10;
		}
		//F3-4 I062/135,Bar Alt, 2B, O   �� ��ѹ�߶ȣ�0.25FL(25ft)Ϊ��λ���ݲ����ݲ���

		//F3-3 I062/220,Rate of C/D, 2B, O,  ������vz�� �ݲ��� 
		//F3-2 I062/390,Fligth Plan , 1B+, O�� ���мƻ���أ��ݲ���

		frnLst.push_back(frn);  // third FRN,������չ
		// F4 - �ԣ��ݲ���
	}
	else
		frnLst.push_back(frn);  // Second FRN���޺���FRN
	
	// ��frn����frnLst�������ȷ���m_Cat062Buf,��֤����m_Cat062Buf����Ȼ��ԭ����frn˳��
	for (auto fit = frnLst.rbegin(); fit != frnLst.rend(); fit++)
	{
		buf = *fit; 
		m_Cat062Buf.push_front(buf);
	}
	int bufLen = m_Cat062Buf.size();
	bufLen += 3;// ����CAT ,Len �ĳ���

	buf = bufLen & 0x00ff;// ���ģʽ�����ݵ�λ�Ÿߵ�ַ
	m_Cat062Buf.push_front(buf);
	buf = (bufLen & 0xff00) >> 8;
	m_Cat062Buf.push_front(buf);

	buf = 0x3E;// CAT62 FLG
	m_Cat062Buf.push_front(buf);

	return bufLen;
}

// ������������֪����ð��ĳ��ȣ�Ϊ������������ʹ�ã�ֱ�ӷ�������б��������������б�����ȡ����
std::list <char> CDecodeCat062::GetCat062BufList()
{
	std::list <char> bufList;
	bufList.swap(m_Cat062Buf);
	return bufList;
}


//���ؽ��뱨�溽��
std::list <RadarTrack> CDecodeCat062::GetReportList()
{
	std::list <RadarTrack> RepList;
	RepList.swap(m_Report);
	return RepList;
}

// �������ݣ�copy cat021��ADS-B�Ľ��룬cat062�Ľ���Ҫ��д
unsigned char CDecodeCat062::DecodeReport(unsigned char* buf, int len)
{
	int repLen = (int)((buf[1] << 8) + buf[2]);
	if (len != repLen)
		return 0;
	
	if (0x15 != buf[0])
		return 0;

	unsigned short frameoffset, offset, Length;
	unsigned char FXNum = 0;
	unsigned char Cat = 0;
	unsigned char OneMsgLen = 0;

	//TRACE("\n========");
	frameoffset = 0;  
	while (frameoffset < len - 5) // 
	{
		Cat = buf[frameoffset];
		Length = (buf[frameoffset + 1] << 8) + buf[frameoffset + 2];  // 2 UINT8s
	
		if (Length == 0 || Length > 250)
			return 0;

		//endbuf=buf+frameoffset+Length;
		//Asterix ICD  ADS-B CAT 021  
		
		offset = frameoffset + 3;// skip cat ,length(2bytes)
		while (offset < frameoffset + Length)
		{
			if (offset > len - 5)
				return 0;

			FXNum = GetFRNum(buf + offset);

			if (FXNum == 0)
				return 0;
			
			DecodeADSB(buf + offset, FXNum, OneMsgLen);
			offset += OneMsgLen;
					
		}   // end while( offset < frameoffset+Length )
		

		frameoffset += Length;
	} // end while( frameoffset< len-2 )
	return 0;
}


unsigned char CDecodeCat062::GetFRNum(unsigned char* buf)
{
	unsigned char Num = 0;
	while ((*(buf + Num)) & 0x01)
	{
		Num++;

		if (Num >  7)  // cat 021 -v2.1 FRN ���Ϊ7
			break;
	}
	Num++;
	return Num;
}

unsigned char CDecodeCat062::DecodeADSB(unsigned char* buf, unsigned char FRN, unsigned char & TrackLen)
{
	unsigned char *pbuf;
	RadarTrack Report;
	Report.InitInstance();

	pbuf = buf + FRN;

	// ADS-B CAT021 -V2.1 ICD UAP

	// FRN #1
	//I021/010 SAC +SIC  2B
	if (buf[0] & 0x80)
		pbuf += 2;

	// I021/040 TargetReport Des 1+
	if (buf[0] & 0x40)
	{
		while (*pbuf & 0x01)
			pbuf++;
		pbuf++;
	}

	// I021/161 TrackNumber 2B
	if (buf[0] & 0x20)
	{
		Report.TrackNo = (pbuf[0] << 8) + pbuf[1];
		pbuf += 2;
	}

	// I021/015 Service Id 1B
	if (buf[0] & 0x10)
	{
		pbuf += 1;
	}

	// I021/071 Time of A for Pos 3B
	if (buf[0] & 0x08)
	{
		Report.Time = ((pbuf[0] << 16) + (pbuf[1]<<8) + pbuf[2]) >> 7;  // LSB :1/128s  
		pbuf += 3;
	}
	// I021/130 Pos WGS-84, 6B
	if (buf[0] & 0x04)
	{
		Report.Lat = ((pbuf[0] << 16) + (pbuf[1] << 8) + pbuf[2]) * 0.00002145767;  // LSB :180/2(23) deg  
		Report.Lon = ((pbuf[3] << 16) + (pbuf[4] << 8) + pbuf[5]) * 0.00002145767;  // LSB :180/2(23) deg 
		pbuf += 6;
	}
	// I021/131 Pos WGS-84 High Res, 8B
	if (buf[0] & 0x02)
	{
		Report.Lat = ((pbuf[0] << 24) + (pbuf[1] << 16) + (pbuf[2] << 8) + pbuf[3]) * 0.00000016764;  // LSB :180/2(30) deg  
		Report.Lon = ((pbuf[4] << 24) + (pbuf[5] << 16) + (pbuf[6] << 8) + pbuf[7]) * 0.00000016764;  // LSB :180/2(30) deg 
		pbuf += 8;
	}
	// FRN #2
	if (buf[0] & 0x01)
	{
		// I021/072 Time of App for V, 3B
		if (buf[1] & 0x80) // skip
		{
			pbuf += 3;
		}
		// I021/150 AirSpeed, 2B
		if (buf[1] & 0x40) // skip
		{
			pbuf += 2;
		}
		// I021/151 TrueAirSpeed, 2B
		if (buf[1] & 0x20) // skip
		{
			pbuf += 2;
		}
		// I021/080 TargetAdd, 3B
		if (buf[1] & 0x10) 
		{
			Report.Address = (pbuf[0] << 16) + (pbuf[1] << 8) + pbuf[2];
			pbuf += 3;
		}
		// I021/073 Time of Recp for Pos, 071/073������һ��, 3B 
		if ((buf[1] & 0x08) && ((buf[0] & 0x08)!=1))
		{
			Report.Time = ((pbuf[0] << 16) + (pbuf[1] << 8) + pbuf[2]) >> 7;  // LSB :1/128s 
			pbuf += 3;
		}
		// I021/074 Time of Recp for Pos - High Pre, 4B
		if (buf[1] & 0x04) // skip
		{
			pbuf += 4;
		}
		// I021/075 Time of Recp for Velocity, 3B
		if (buf[1] & 0x02) // skip
		{
			pbuf += 3;
		}

		//FRN #3
		if (buf[1] & 0x01)
		{
			// I021/076 Time of Recp for Velocity - High Pre, 4B
			if (buf[2] & 0x80)// skip
			{
				pbuf += 4;
			}
			// I021/140 GeoHeight, 2B
			if (buf[2] & 0x40) // skip
			{
				pbuf += 2;
			}
			// I021/090 Quan Ind, 1+B
			if (buf[2] & 0x20) // skip
			{
				while (*pbuf & 0x01)
					pbuf++;
				pbuf++;
			}
			// I021/210 MOPS Ver, 1B
			if (buf[2] & 0x10) // skip
			{
				pbuf += 1;
			}
			// I021/070 SSR Code, 2B
			if (buf[2] & 0x08) 
			{
				Report.SSR = (pbuf[0] << 8) + pbuf[1];
				pbuf += 2;
			}
			// I021/230 Roll Ang, 2B
			if (buf[2] & 0x04) // skip
			{
				pbuf += 2;
			}
			// I021/145 FL, 2B
			if (buf[2] & 0x02)
			{
				Report.Hei = ((pbuf[0] << 8) + pbuf[1]) * 25;   // LSB : 1/4 FL = 25ft
				pbuf += 2;
			}

			//FRN #4
			if (buf[2] & 0x01)
			{
				// I021/152 Meg Heading, 2B
				if (buf[3] & 0x80)// skip
				{
					pbuf += 2;
				}
				// I021/200 Target Status, 1B
				if (buf[3] & 0x40)// skip
				{
					pbuf += 1;
				}
				// I021/155 Bar Vz, 2B
				if (buf[3] & 0x20)// skip
				{
					pbuf += 2;
				}
				// I021/157 Geo Vz, 2B
				if (buf[3] & 0x10)//
				{
					Report.vz = ((pbuf[0] << 8) + pbuf[1]) * 6.25;   // LSB : 6.25 ft/min
					pbuf += 2;
				}
				// I021/160 Ground Speed, 4B
				if (buf[3] & 0x08)//
				{
					Report.vec = ((pbuf[0] << 8) + pbuf[1]) * 0.113;   // LSB : 2(-14) NM/s = 1852 * 2(-14) m/s = 0.113 m/s
					Report.cource = ((pbuf[2] << 8) + pbuf[3]) * 0.0055;   // LSB : 360/2(16) deg = 0.0055deg
					pbuf += 4;
				}
				// I021/165 Heading Angle Rate, 2B
				if (buf[3] & 0x04)// skip
				{
					pbuf += 2;
				}
				// I021/077 Time of Rep Trans, 3B
				if (buf[3] & 0x02)// skip
				{
					pbuf += 3;
				}

				//FRN #5
				if (buf[3] & 0x01)
				{
					// I021/170 CallNum, 6B
					if (buf[4] & 0x80)// 
					{
						Report.callNo = "";
						//std::string s1; 
						unsigned char SixBit;
						SixBit = (pbuf[0] & 0xFC) >> 2; //CHAR 1
						if ((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
						{
							if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
								SixBit += 0x40; //6bit��ASCII �ԣ� �ַ���Ҫ����0x40

							Report.callNo += SixBit;
						}
						SixBit = ((pbuf[0] & 0x03) << 4) + ((pbuf[1] & 0xF0) >> 4); // CHAR 2
						if ((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
						{
							if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
								SixBit += 0x40; //6bit��ASCII �ԣ� �ַ���Ҫ����0x40

							Report.callNo += SixBit;
						}
						SixBit = ((pbuf[1] & 0x0F) << 2) + ((pbuf[2] & 0xC0) >> 6); //CHAR 3
						if ((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
						{
							if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
								SixBit += 0x40; //6bit��ASCII �ԣ� �ַ���Ҫ����0x40

							Report.callNo += SixBit;
						}
						SixBit = pbuf[2] & 0x3F; //CHAR 4
						if ((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
						{
							if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
								SixBit += 0x40; //6bit��ASCII �ԣ� �ַ���Ҫ����0x40

							Report.callNo += SixBit;
						}
						
						pbuf += 3;

						SixBit = (pbuf[0] & 0xFC) >> 2; //CHAR 5
						if ((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
						{
							if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
								SixBit += 0x40; //6bit��ASCII �ԣ� �ַ���Ҫ����0x40

							Report.callNo += SixBit;
						}
						SixBit = ((pbuf[0] & 0x03) << 4) + ((pbuf[1] & 0xF0) >> 4); // CHAR 6
						if ((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
						{
							if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
								SixBit += 0x40; //6bit��ASCII �ԣ� �ַ���Ҫ����0x40

							Report.callNo += SixBit;
						}
						SixBit = ((pbuf[1] & 0x0F) << 2) + ((pbuf[2] & 0xC0) >> 6); //CHAR 7
						if ((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
						{
							if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
								SixBit += 0x40; //6bit��ASCII �ԣ� �ַ���Ҫ����0x40

							Report.callNo += SixBit;
						}
						SixBit = pbuf[2] & 0x3F; //CHAR 8
						if ((SixBit >= 0x30) && (SixBit <= 0x39))  // 0-9
						{
							if (((SixBit >= 0x1) && (SixBit <= 0x1A)))  // A-Z
								SixBit += 0x40; //6bit��ASCII �ԣ� �ַ���Ҫ����0x40

							Report.callNo += SixBit;
						}
						pbuf += 3;
					}

					// I021/020 EmitCat , 1B
					if (buf[4] & 0x40)// skip
					{
						pbuf += 1;
					}
					// I021/220 Met Info, 1+B
					if (buf[4] & 0x20) // skip
					{
						while (*pbuf & 0x01)
							pbuf++;
						pbuf++;
					}
					// I021/146 SelectAlt, 2B
					if (buf[4] & 0x10)// skip
					{
						pbuf += 2;
					}
					// I021/148 Final state SelectAlt, 2B
					if (buf[4] & 0x08)// skip
					{
						pbuf += 2;
					}
					// I021/110 Track Intent, 1+B
					if (buf[4] & 0x04) // skip
					{
						while (*pbuf & 0x01)
							pbuf++;
						pbuf++;
					}
					// I021/016 Service Manage, 1B
					if (buf[4] & 0x02)// skip
					{
						pbuf += 1;
					}

					//FRN #6
					if (buf[4] & 0x01)
					{
						// I021/008 Operate Status, 1B
						if (buf[5] & 0x80)// skip
						{
							pbuf += 1;
						}
						// I021/271 Surface Cap, 1+B
						if (buf[5] & 0x40) // skip
						{
							while (*pbuf & 0x01)
								pbuf++;
							pbuf++;
						}
						// I021/132 MessageAmp, 1B
						if (buf[5] & 0x20)// skip
						{
							pbuf += 1;
						}
						// I021/250 ModeS MB, 1+N*8B
						if (buf[5] & 0x10)// skip
						{
							pbuf += pbuf[0] * 8;
							pbuf += 1;
						}

						// I021/260 ACRS RA, 7B
						if (buf[5] & 0x08)// skip
						{
							pbuf += 7;
						}
						// I021/400 Recive ID, 1B
						if (buf[5] & 0x04)// skip
						{
							pbuf += 1;
						}
						// I021/295,DataAge 1+B
						if (buf[5] & 0x02)// skip
						{
							while (*pbuf & 0x01)
								pbuf++;
							pbuf += 1;
						}
					}
				}
			}
		}
	}

	//if (Report.Address != 0xffffff)
	if (Report.TrackNo != 0)//0xffffff)
	{
		m_Report.push_back(Report);
	}
	TrackLen = (unsigned char)(pbuf - buf);
	return 0;
}