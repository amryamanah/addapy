#include "adda.h"

ADBOARDSPEC 	BoardSpec;
ADSMPLREQ		AdSmplConfig;

HANDLE AdDeviceHandle;
HANDLE DaDeviceHandle;
HANDLE flowMeterHandle;
HANDLE adjustLedHandle;

DWORD flowMeterThreadId;
DWORD adjustLedThreadId;


unsigned short	wSmplData[2];
ADBOARDSPEC 	BoardSpec;
ADSMPLREQ		AdSmplConfig;
ADSMPLCHREQ		AdSmplChReq1[1];
ADSMPLCHREQ		AdSmplChReq2[1];
ADSMPLCHREQ		AdSmplChReq3[1];
ADSMPLCHREQ		AdSmplChReq5[1];
ADSMPLCHREQ		AdSmplChReq4[1];

DASMPLCHREQ	DaSmplChReq1[1];
DASMPLCHREQ	DaSmplChReq2[1];

double plDistConstA, plDistConstB, plDistConstC;
double noplDistConstA, noplDistConstB, noplDistConstC;
double plLedConstA, plLedConstB, plLedConstC;
double noplLedConstA, noplLedConstB, noplLedConstC;

int TotalFlowCount = 0;
int FlowMeterFlag = 1;
int AdjustLedFlag = 1;

DWORD WINAPI flow_meter_loop()
{	
	int	sensor;//The current state of the sensor
	int	sensorprev = get_flowmeter_signal();//State of one loop front of the sensor
	int short_sensorcount = 0, bufsensorcount = 0;//0.3 seconds pulse count of

	for (;;)
	{
		if (FlowMeterFlag == 0)
			break;

		sensor = get_flowmeter_signal();//1 return in light blocking enter the signal of the optical sensor (IN17 / OUT17), 0 return in light transmission
										
		if (sensor == 1 && sensorprev == 0) {//Sensor signal rising
			TotalFlowCount++;
			sensorprev = 1;
		}
		if (sensor == 0 && sensorprev == 1) {//Fall sensor signal Standing
			TotalFlowCount++;
			sensorprev = 0;
		}

		TotalFlowCount++;
	}
	return 0;
}

DWORD WINAPI adjust_led_loop()
{	
	double pl_distance = 0;
	double nopl_distance = 0;
	for (;;)
	{
		if (AdjustLedFlag == 0)
		{
			light_call("reset", 0, plLedConstA, plLedConstB, plLedConstC);
			break;
		}
		else
		{
			pl_distance = get_distance("pl", plDistConstA, plDistConstB, plDistConstC);
			nopl_distance = get_distance("nopl", noplDistConstA, noplDistConstB, noplDistConstC);
			light_call("pl", pl_distance, plLedConstA, plLedConstB, plLedConstC);
			light_call("nopl", nopl_distance, noplLedConstA, noplLedConstB, noplLedConstC);
		}
	}
	return 0;
}

void SetHandler()
{
	AdDeviceHandle = AdOpen(AdIdentifier);
	printf("AdOpen Success: %p\n", AdDeviceHandle);

	DaDeviceHandle = DaOpen(DaIdentifier);	// Open a device.
	printf("DaOpen Success: %p\n", DaDeviceHandle);

}

void SetChannel()
{
	// Setup Ad Channel
	AdSmplChReq1[0].ulChNo = 1; // AD Channel 1 PL Filter distance meter
	AdSmplChReq1[0].ulRange = AD_10V; // Maximum to 10 V
	AdSmplChReq2[0].ulChNo = 2; // AD Channel 2 NOPL Filter distance meter
	AdSmplChReq2[0].ulRange = AD_10V; // Maximum to 10 V
	AdSmplChReq3[0].ulChNo = 3; // AD Channel 3 NOPL Filter distance meter
	AdSmplChReq3[0].ulRange = AD_10V;// Maximum to 10 V
	AdSmplChReq4[0].ulChNo = 4; // AD Channel 4 NOPL Filter distance meter
	AdSmplChReq4[0].ulRange = AD_10V; // Maximum to 10 V
	AdSmplChReq5[0].ulChNo = 5; // AD Channel 5 NOPL Filter distance meter
	AdSmplChReq5[0].ulRange = AD_10V;// Maximum to 10 V

									 // Setup Ad Channel
	DaSmplChReq1[0].ulChNo = 1;
	DaSmplChReq1[0].ulRange = AD_10V;
	DaSmplChReq2[0].ulChNo = 2;
	DaSmplChReq2[0].ulRange = AD_10V;
}

int open_adda() {

	SetHandler();
	SetChannel();

	if (AdDeviceHandle == INVALID_HANDLE_VALUE ||
		DaDeviceHandle == INVALID_HANDLE_VALUE)
	{
		printf("Failed to open the ADC device.");
		return -1;
	}

	return 0;
}

int close_adda() {
	if (AdClose(AdDeviceHandle) == AD_ERROR_SUCCESS &&
		DaClose(DaDeviceHandle) == DA_ERROR_SUCCESS)
		return 0;
	else
	{
		return -1;
	}

}

double get_channel_value(ADSMPLCHREQ ChannelReq[1])
{
	int	NRet = 0;
	unsigned short	SamplingData[2];
	double AdOutput[CONVERT_TIME];
	double AdOutputSum = 0;
	double AdOutputBuf = 0;

	int i = 0;
	for (i = 0; i < CONVERT_TIME; ++i)
	{
		NRet = AdInputAD(AdDeviceHandle, 1, AD_INPUT_SINGLE, ChannelReq, SamplingData);
		if (NRet != AD_ERROR_SUCCESS)
		{
			printf("AdInputAD errr(%lx) \n", NRet);
			return -1;
		}
		AdOutput[i] = (double)(SamplingData[0] - 32768) * 20 / 65536;
		AdOutputSum = AdOutputSum + AdOutput[i];
	}

	AdOutputBuf = AdOutputSum / CONVERT_TIME;
	//printf("Channel1 Voltage = %f \n", AdOutputBuf);
	return AdOutputBuf;
}

int set_da_out(unsigned short voltage, DASMPLCHREQ ChannelReq[1])
{

	WORD		Data[1];			// Output data storage area
	int			nRet;

	Data[0] = voltage;

	nRet = DaOutputDA(DaDeviceHandle, 1, ChannelReq, &voltage);	// Start the analog output.
	if (nRet != DA_ERROR_SUCCESS) {
		printf("DaOutputDA errr(%lx)", nRet);
		return -1;
	}
	else
	{
		return 0;
	}
}

double get_distance(char *kind, double ConstA, double ConstB, double ConstC)
{
	double RawVoltage;
	double ConvertedDistance;


	if (strcmp(kind, "pl") == 0) {
		//printf("%s\n", kind);
		RawVoltage = get_channel_value(AdSmplChReq1);
	}
	else if (strcmp(kind, "nopl") == 0) {
		//printf("%s\n", kind);
		RawVoltage = get_channel_value(AdSmplChReq2);
	}
	else
	{
		return -1;
	}
	//printf("%.4f\n", RawVoltage);

	if (RawVoltage < 0)
	{
		return -1;
	}

	ConvertedDistance = (ConstA * RawVoltage * RawVoltage) + (ConstB * RawVoltage) + ConstC;

	return ConvertedDistance;
}

double get_humidity(double ConstA, double ConstB)
{
	double RawVoltage;
	double humidity;

	RawVoltage = get_channel_value(AdSmplChReq5);
	//printf("%.4f\n", RawVoltage);
	if (RawVoltage < 0)
		return -1;

	humidity = ConstA * RawVoltage + ConstB;

	return humidity;
}

double get_illumination(double ConstA, double ConstB, double ConstC, double ConstD)
{
	double RawVoltage;
	double illumination;

	RawVoltage = get_channel_value(AdSmplChReq5);
	//printf("%.4f\n", RawVoltage);
	if (RawVoltage < 0)
		return -1;

	RawVoltage = get_channel_value(AdSmplChReq4);
	if ((-1.0*RawVoltage)<1.0) {
		illumination = ConstA * (-1 * RawVoltage) + ConstB;
	}
	else {
		illumination = ConstC * (-1 * RawVoltage) + ConstD;
	}

	return illumination;
}

double get_temperature(double ConstA, double ConstB)
{
	double RawVoltage;
	double temperature;

	RawVoltage = get_channel_value(AdSmplChReq3);
	//printf("%.4f\n", RawVoltage);
	if (RawVoltage < 0)
		return -1;

	temperature = ConstA * RawVoltage + ConstB;

	return temperature;
}

int light_call(char *kind, double buf, double ConstA, double ConstB, double ConstC)
{
	double cal_output_buf;
	unsigned int cal_output_int;
	unsigned short cal_output;

	if (strcmp(kind, "reset") == 0)
	{
		return (set_da_out(0, DaSmplChReq2) || set_da_out(0, DaSmplChReq1));
	}


	cal_output_buf = (ConstA * buf* buf) + (ConstB*buf) + ConstC; //1200 lux

																  //printf("%lf\n", cal_output_buf);

	cal_output_int = (unsigned int)((double)cal_output_buf*(double)32768.0 / (double)10.0 + (double)32768.0);

	if (cal_output_int >= 65535) {
		cal_output = (unsigned short)65535;
	}
	else if (cal_output_int<32768) {
		cal_output = (unsigned short)32768;
	}
	else {
		cal_output = (unsigned short)cal_output_int;
	}


	if (strcmp(kind, "pl") == 0)
	{
		return set_da_out(cal_output, DaSmplChReq2);
	}
	else if (strcmp(kind, "nopl") == 0)
	{
		return set_da_out(cal_output, DaSmplChReq1);
	}
	else
	{
		return -1;
	}

}


int device_cleaning()
{
	INT Ret = 0;
	// Water pump
	Ret = AdOutputDO(AdDeviceHandle, CLS_DO_WATERPUMP_ON);
	if (Ret != DA_ERROR_SUCCESS)
		return -1;

	Sleep(10000);
	Ret = AdOutputDO(AdDeviceHandle, CLS_DO_NEUTRAL);
	if (Ret != DA_ERROR_SUCCESS)
		return -1;

	//  Wiper motor
	Ret = AdOutputDO(AdDeviceHandle, CLS_DO_WIPER_ON);
	if (Ret != DA_ERROR_SUCCESS)
		return -1;

	Sleep(3000);
	Ret = AdOutputDO(AdDeviceHandle, CLS_DO_NEUTRAL);
	if (Ret != DA_ERROR_SUCCESS)
		return -1;

	return 0;
}

int set_usb(char *on_off)
{
	INT Ret1 = NULL;
	INT Ret2 = NULL;
	printf("Set USB %s\n", on_off);
	if (strcmp(on_off, "reset") == 0)
	{
		printf("IM RESET\n");
		Ret1 = AdOutputDO(AdDeviceHandle, CLS_DO_NEUTRAL);
		Sleep(2000);
		Ret2 = AdOutputDO(AdDeviceHandle, CLS_DO_USB_ON);
		if (Ret1 == AD_ERROR_SUCCESS && Ret2 == AD_ERROR_SUCCESS)
			return 0;
		else
			return 1;
	}
	else if (strcmp(on_off, "off") == 0)
	{
		printf("IM OFF\n");
		Ret1 = AdOutputDO(AdDeviceHandle, CLS_DO_NEUTRAL);
	}
	else if (strcmp(on_off, "on") == 0)
	{
		printf("IM ON\n");
		Ret1 = AdOutputDO(AdDeviceHandle, CLS_DO_USB_ON);
	}
	else
		return -1;

	if (Ret1 != DA_ERROR_SUCCESS)
		return -1;

	return 0;
}


int get_flowmeter_signal(void)
{
	DWORD d_input[1];
	int Ret;
	Ret = AdInputDI(AdDeviceHandle, d_input);
	if ((d_input[0] & 32) != 32)
		return 1;
	else
		return 0;
}

int flow_check_start(void) 
{
	TotalFlowCount = 0;
	FlowMeterFlag = 1;
	flowMeterHandle = CreateThread(
		NULL, // default security attributes
		0, // use default stack size
		flow_meter_loop, // thread function
		NULL, // argument to thread function
		0, // use default creation flags
		&flowMeterThreadId); // returns the thread identifier
	if (flowMeterHandle == NULL)
	{
		printf("CreateThread() failed, error: %d.\n", GetLastError());
		return -1;
	}
		
	printf("The thread ID: %d.\n", flowMeterThreadId);
	return 0;
}

int adjust_led_start()
{
	AdjustLedFlag = 1;
	adjustLedHandle = CreateThread(
		NULL, // default security attributes
		0, // use default stack size
		adjust_led_loop, // thread function
		NULL, // argument to thread function
		0, // use default creation flags
		&adjustLedThreadId); // returns the thread identifier
	if (adjustLedHandle == NULL)
	{
		printf("CreateThread() failed, error: %d.\n", GetLastError());
		return -1;
	}

	printf("The thread ID: %d.\n", adjustLedThreadId);
	return 0;
}

void set_calibration_value(double ConfigPlDistConstA, double ConfigPlDistConstB, double ConfigPlDistConstC,
	double ConfigNoPlDistConstA, double ConfigNoPlDistConstB, double ConfigNoPlDistConstC,
	double ConfigPlLedConstA, double ConfigPlLedConstB, double ConfigPlLedConstC,
	double ConfigNoPlLedConstA, double ConfigNoPlLedConstB, double ConfigNoPlLedConstC)
{
	plDistConstA = ConfigPlDistConstA;
	plDistConstB = ConfigPlDistConstB;
	plDistConstC = ConfigPlDistConstC;
	noplDistConstA = ConfigNoPlDistConstA;
	noplDistConstB = ConfigNoPlDistConstB;
	noplDistConstC = ConfigNoPlDistConstC;
	plLedConstA = ConfigPlLedConstA;
	plLedConstB = ConfigPlLedConstB;
	plLedConstC = ConfigPlLedConstC;
	noplLedConstA = ConfigNoPlLedConstA;
	noplLedConstB = ConfigNoPlLedConstB;
	noplLedConstC = ConfigNoPlLedConstC;
}

int flow_check_stop(void)
{
	int result = 0;
	FlowMeterFlag = 0;
	if (CloseHandle(flowMeterHandle) != 0)
	{
		printf("Total count = %d", TotalFlowCount);
		result = TotalFlowCount;
		printf("Handle to thread closed successfully.\n");
	}
	return result;
}

int adjust_led_stop(void)
{
	int ret = 0;
	AdjustLedFlag = 0;
	if (CloseHandle(adjustLedHandle) != 0)
	{
		ret = 1;
		printf("Handle to thread closed successfully.\n");
	}
	return ret;
}

int flow_check(void) {
	clock_t t1, t2;//For processing time count
	int	sensor;//The current state of the sensor
	int	sensorprev = get_flowmeter_signal();//State of one loop front of the sensor
	int short_sensorcount = 0, bufsensorcount = 0;//0.3 seconds pulse count of
	int it = 0;
	t1 = clock();
	for (it = 0; it<300;) {//Check whether large flow rate is detected than the threshold value between 300 milliseconds
		t2 = clock();
		if ((t2 - t1)>it) {//The difference between t2 and t1 ( in milliseconds) is I to judge whether more than the number of loops instead of Sleep function
			sensor = get_flowmeter_signal();//1 return in light blocking enter the signal of the optical sensor (IN17 / OUT17), 0 return in light transmission
											//printf("%d,	", sensor);

											//	printf("%d",sensor);
			if (sensor == 1 && sensorprev == 0) {//Sensor signal rising
				short_sensorcount++;
				sensorprev = 1;
			}
			if (sensor == 0 && sensorprev == 1) {//Fall sensor signal Standing
				short_sensorcount++;
				sensorprev = 0;
			}
			//				Sleep(1);//Once per millisecond

			//printf("sensorprev = %d , sensor = %d, short_sensorcount = %d \n", sensorprev, sensor, short_sensorcount);
			it++;
		}
	}
	return short_sensorcount;
}