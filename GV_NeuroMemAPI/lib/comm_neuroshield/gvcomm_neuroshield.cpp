// gvcomm_neuroshield.cpp
// Copyright(c) General Vision Inc.

#include "stdlib.h"	 //for calloc
#include "string.h"  //for memcpy
#include <windows.h>
#include <stdint.h> // to support int8_t and int16_t
#include "stdio.h" // printf

#include "CyUSBSerial.h"
#define NM500_SPI_CLK_DIV	SPI_CLOCK_DIV8	// spi clock : 16MHz / 8 = 2MHz
#define NM500_SPI_CLK		2000000

#define VID 0x04B4
#define PID 0x000A

int platform = 2;
int navail = 576;
int maxveclength = 256;

CY_DEVICE_INFO cyDeviceInfo, cyDeviceInfoList[16];
CY_VID_PID cyVidPid;
CY_HANDLE cyHandle;

uint8_t cyNumDevices;
unsigned char deviceID[16];

unsigned char wbuffer[520], rbuffer[520];
CY_DATA_BUFFER cyDatabufferWrite, cyDatabufferRead;
CY_RETURN_STATUS rStatus;
CY_RETURN_STATUS cyReturnStatus;

int InitDevice(int deviceNumber)
{
	CY_SPI_CONFIG cySPIConfig;
	CY_RETURN_STATUS rStatus;
	int interfaceNum = 0;

	rStatus = CyOpen(deviceNumber, interfaceNum, &cyHandle);

	if (rStatus != CY_SUCCESS) {
		//cout << "SPI Device open failed. " << endl;
		return rStatus;
	}

	rStatus = CyGetSpiConfig(cyHandle, &cySPIConfig);
	if (rStatus != CY_SUCCESS) {
		//cout << "CyGetSpiConfig returned failure code." << endl;
		return rStatus;
	}

	//printf("SPI CONFIG: Frequency Is %d , data width id %d, is continuousmode %d \n", cySPIConfig.frequency, cySPIConfig.dataWidth, cySPIConfig.isContinuousMode);
	return CY_SUCCESS;
}

int FindDeviceAtSCB1()
{
	CY_VID_PID cyVidPid;

	cyVidPid.vid = VID; // Defined as macro
	cyVidPid.pid = PID; // Defined as macro

	// Array size of cyDeviceInfoList is 16 
	cyReturnStatus = CyGetDeviceInfoVidPid(cyVidPid, deviceID, (PCY_DEVICE_INFO)&cyDeviceInfoList, &cyNumDevices, 16);

	int deviceIndexAtSCB1 = -1;
	for (int index = 0; index < cyNumDevices; index++) {
		// Find the device at device index at SCB1
		if (cyDeviceInfoList[index].deviceBlock == SerialBlock_SCB1) {
			deviceIndexAtSCB1 = index;
		}
	}
	return deviceIndexAtSCB1;
}
//-----------------------------------------------
// Open USB connection
//-----------------------------------------------
int Connect(int DeviceID)
{
	// DeviceID is presently ignored

	// Assmumptions:
	// 1. SCB1 is configured as SPI
	int deviceIndexAtSCB1 = FindDeviceAtSCB1();
	// Open the device at index deviceIndexAtSCB1
	if (deviceIndexAtSCB1 >= 0)
	{
		// Assuming that USB-Serial at "deviceIndexAtSCB1" is configured as SPI device
		// Device Open, Close, Configuration, Data operations are handled in the function SPIMaster
		int status = InitDevice(deviceIndexAtSCB1);
		// if (status == 0)
		// {

		// }
		return(0);
	}
	else
	{
		return(1);
	}
}

//-----------------------------------------------
// Close USB
//-----------------------------------------------
int Disconnect()
{
	return(0);
}

//-----------------------------------------------------
// Generic USB Write command
//-----------------------------------------------------
int Write_Addr(int addr, int length_inByte, byte data[])
{
	//The USB controller in the FPGA expect data length in word
	int len = length_inByte / 2;	
	uint16_t total_size = 8 + length_inByte;

	cyDatabufferWrite.buffer = wbuffer;
	cyDatabufferWrite.length = total_size;

	cyDatabufferRead.buffer = rbuffer;
	cyDatabufferRead.length = total_size;

	memset(wbuffer, 0x00, total_size);
	memset(rbuffer, 0x00, total_size);

	wbuffer[0] = 1; //reserved for a board number
	int module = (byte)((addr & 0xFF000000) >> 24);
	wbuffer[1] = module + 0x80;
	wbuffer[2] = (byte)((addr & 0x00FF0000) >> 16);
	wbuffer[3] = (byte)((addr & 0x0000FF00) >> 8);
	wbuffer[4] = (byte)(addr & 0x000000FF);
	wbuffer[5] = (byte)((len & 0x00FF0000) >> 16);
	wbuffer[6] = (byte)((len & 0x0000FF00) >> 8);
	wbuffer[7] = (byte)(len & 0x000000FF);
	for (int i = 0; i < length_inByte; i++) {
		wbuffer[8 + i] = ((uint8_t)(data[i]));
	}

	//printf("\nWriteBuffer ");
	//for (int i = 0; i < 14; i++) printf("%u, ", wbuffer[i]);

	rStatus = CySpiReadWrite(cyHandle, &cyDatabufferRead, &cyDatabufferWrite, 5000);
	int error = 0;
	if (rStatus != CY_SUCCESS)
	{
		error = 1;
	}
	return(error);
	
}

//---------------------------------------------
// Generic USB Read command
//---------------------------------------------
int Read_Addr(int addr, int length_inByte, byte data[])
{
	int len = length_inByte / 2;
	uint16_t total_size = 8 + length_inByte;

	cyDatabufferWrite.buffer = wbuffer;
	cyDatabufferWrite.length = total_size;

	cyDatabufferRead.buffer = rbuffer;
	cyDatabufferRead.length = total_size;

	memset(wbuffer, 0x00, total_size);
	memset(rbuffer, 0x00, total_size);

	wbuffer[0] = 1; //reserved for a board number
	int module = (byte)((addr & 0xFF000000) >> 24);
	wbuffer[1] = module;
	wbuffer[2] = (byte)((addr & 0x00FF0000) >> 16);
	wbuffer[3] = (byte)((addr & 0x0000FF00) >> 8);
	wbuffer[4] = (byte)(addr & 0x000000FF);
	wbuffer[5] = (byte)((len & 0x00FF0000) >> 16);
	wbuffer[6] = (byte)((len & 0x0000FF00) >> 8);
	wbuffer[7] = (byte)(len & 0x000000FF);
	
	for (int i = 0; i < length_inByte; i++) {
		wbuffer[8 + i] = 0;
	}

	//printf("\nReadBuffer ");
	//for (int i = 0; i < 14; i++) printf("%u, ", wbuffer[i]);

	rStatus = CySpiReadWrite(cyHandle, &cyDatabufferRead, &cyDatabufferWrite, 5000);
	int error = 0;
	if (rStatus != CY_SUCCESS)
	{
		error = 1;
	}
	else
	{
		for (int i = 0; i < length_inByte; i++) data[i] = rbuffer[8 + i];
	}
	return(error);
}

// --------------------------------------------------------
// Read the register of a given module (module + reg = addr)
//---------------------------------------------------------
int Read(byte module, byte reg)
{
	int data = 0xFFFF;
	int addr = (module << 24) + reg;
	byte databyte[2];
	int error = Read_Addr(addr, 2, databyte);
	if (error == 0) data = (databyte[0] << 8) + databyte[1];
	return(data);
}
// ---------------------------------------------------------
// Write the register of a given module (module + reg = addr)
// ---------------------------------------------------------
void Write(byte module, byte reg, int data)
{
	byte databyte[2];
	databyte[1] = (byte)(data & 0x00FF);
	databyte[0] = (byte)((data & 0xFF00) >> 8);
	int addr = (module << 24) + reg;
	int error = Write_Addr(addr, 2, databyte);
}
