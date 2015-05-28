#ifndef _adda_h
#define _adda_h

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

#ifdef UNICODE
typedef LPCWSTR LPCTSTR;
#else
typedef LPCSTR LPCTSTR;
#endif

#include <Gpcad.h>
#include <Gpcda.h>
#include <Fbiad.h>
#include <Fbida.h>
#include <FbiPenc.h>

#define CONVERT_TIME			300 
#define AdIdentifier			"FBIAD1"
#define DaIdentifier			"FBIDA1"

#define CLS_DO_NEUTRAL			0x00	//0x00=0000 0000 ALL LOW	
#define CLS_DO_WATERPUMP_ON		0x0c	//0x0c=0000 1100 IO3=H IO4=H	
#define CLS_DO_WIPER_ON			0x03	//0x0c=1100 0011 IO1=H IO2=H	
#define CLS_DO_USB_ON			0x10    //0x10=0001 0000 IO5=H

int open_adda(void);
int close_adda(void);

int light_call(char *kind, double distance, double ConstA, double ConstB, double ConstC);
int device_cleaning(void);
int set_usb(char *on_off);

double get_distance(char *kind, double ConstA, double ConstB, double ConstC);
double get_humidity(double ConstA, double ConstB);
double get_temperature(double ConstA, double ConstB);
double get_illumination(double ConstA, double ConstB, double ConstC, double ConstD);

int get_flowmeter_signal(void);

#endif
