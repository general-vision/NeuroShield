// comm_brilliant.cpp 
// Copyright 2019 General Vision Inc.
#pragma once

//#include "stdafx.h"
#include <Windows.h>
#include "CyAPI.h"

int platform = 2; //0=Simu,  1=Neuroshield,  2=Brilliant
int maxveclength = 256; // length of the neuron memory on the Brilliant platform

CCyUSBDevice *usbHandle = new CCyUSBDevice(NULL);

#define USB_BUFF_LENGTH 512
long USB_buffLength = USB_BUFF_LENGTH;
byte wbuffer[USB_BUFF_LENGTH], rbuffer[USB_BUFF_LENGTH];

//-----------------------------------------------
// Open USB connection
//-----------------------------------------------
int Connect(int device)
{
	int error = 1;
	int devnum = usbHandle->DeviceCount();
	for (byte d = 0; d < devnum; d++)
	{
		usbHandle->Open(d);
		//printf("\nUSB Vendor = %u", usbHandle->VendorID);
		//printf("\nUSB Product = %u", usbHandle->ProductID);
		if (usbHandle->VendorID == 0x04B4 && usbHandle->ProductID == 0x1003)
		{
			error = 0;
			break;
		}
	}
	return(error);
}

//-----------------------------------------------
// Close USB
//-----------------------------------------------
int Disconnect()
{
	usbHandle->Close();
	delete usbHandle;
	return(0);
}

//---------------------------------------------
// Generic USB Read command
//---------------------------------------------
int Read_Addr(int addr, int length_inByte, byte data[])
{
	// Radical crop which should never occur
	// since the Brillaint can only use the Read_Addr to load 256 COMP

	if (length_inByte > USB_BUFF_LENGTH) length_inByte = USB_BUFF_LENGTH;

	int error = 0;
	int len = length_inByte / 2; // used in the send-buff packet

	memset(wbuffer, 0x00, USB_BUFF_LENGTH);
	memset(rbuffer, 0x00, USB_BUFF_LENGTH);

	wbuffer[0] = 1; // settings from nepes
	wbuffer[1] = (byte)((addr & 0xFF000000) >> 24);
	wbuffer[2] = (byte)((addr & 0x00FF0000) >> 16);
	wbuffer[3] = (byte)((addr & 0x0000FF00) >> 8);
	wbuffer[4] = (byte)(addr & 0x000000FF);
	wbuffer[5] = (byte)((len & 0x00FF0000) >> 16);
	wbuffer[6] = (byte)((len & 0x0000FF00) >> 8);
	wbuffer[7] = (byte)(len & 0x000000FF);

	usbHandle->BulkOutEndPt->XferData(wbuffer, USB_buffLength, NULL, false);
	usbHandle->BulkInEndPt->XferData(rbuffer, USB_buffLength, NULL, true);

	if (USB_buffLength != USB_BUFF_LENGTH)
	{
		error = 1;
	}
	else
	{
		memcpy(data, rbuffer, length_inByte);
	}

	return(error);
}

//-----------------------------------------------------
// Generic USB Write command
//-----------------------------------------------------
int Write_Addr(int addr, int length_inByte, byte data[])
{
	// Radical handling of data[] longer than USB_BUFF_LENGTH:
	// Since the Brilliant can only use the Read_Addr to load 256 COMP
	// the function split the transaction into 1 or 2 Xfer

	int lenB = length_inByte;
	if (length_inByte > USB_BUFF_LENGTH - 8) lenB = USB_BUFF_LENGTH - 8;

	int error = 0;
	int lenW = lenB / 2;

	memset(wbuffer, 0x00, USB_BUFF_LENGTH);

	wbuffer[0] = 1; // settings from nepes
	wbuffer[1] = 0x80 + (byte)((addr & 0xFF000000) >> 24);
	wbuffer[2] = (byte)((addr & 0x00FF0000) >> 16);
	wbuffer[3] = (byte)((addr & 0x0000FF00) >> 8);
	wbuffer[4] = (byte)(addr & 0x000000FF);
	wbuffer[5] = (byte)((lenW & 0x00FF0000) >> 16);
	wbuffer[6] = (byte)((lenW & 0x0000FF00) >> 8);
	wbuffer[7] = (byte)(lenW & 0x000000FF);

	memcpy(wbuffer + 8, data, lenB);
	USB_buffLength = USB_BUFF_LENGTH;
	usbHandle->BulkOutEndPt->XferData(wbuffer, USB_buffLength, NULL, false);
	if (USB_buffLength != USB_BUFF_LENGTH)
	{
		error = 1;
	}
	else if (length_inByte > lenB)
	{
		int lenB2 = length_inByte - lenB;
		lenW = lenB2 / 2;

		memset(wbuffer, 0x00, USB_BUFF_LENGTH);

		wbuffer[0] = 1; // settings from nepes
		wbuffer[1] = 0x80 + (byte)((addr & 0xFF000000) >> 24);
		wbuffer[2] = (byte)((addr & 0x00FF0000) >> 16);
		wbuffer[3] = (byte)((addr & 0x0000FF00) >> 8);
		wbuffer[4] = (byte)(addr & 0x000000FF);
		wbuffer[5] = (byte)((lenW & 0x00FF0000) >> 16);
		wbuffer[6] = (byte)((lenW & 0x0000FF00) >> 8);
		wbuffer[7] = (byte)(lenW & 0x000000FF);

		memcpy(wbuffer + 8, data + lenB, lenB2);
		USB_buffLength = USB_BUFF_LENGTH;
		usbHandle->BulkOutEndPt->XferData(wbuffer, USB_buffLength, NULL, false);
		if (USB_buffLength != USB_BUFF_LENGTH)
		{
			error = 2;
		}
	}

	return(error);
}

// --------------------------------------------------------
// Read the register of a given module (module + reg = addr)
//---------------------------------------------------------
int Read(byte module, byte reg)
{
	memset(wbuffer, 0x00, USB_BUFF_LENGTH);
	memset(rbuffer, 0x00, USB_BUFF_LENGTH);

	wbuffer[0] = 1; // settings from nepes
	wbuffer[1] = module;
	wbuffer[2] = 0;
	wbuffer[3] = 0;
	wbuffer[4] = reg;
	wbuffer[5] = 0;
	wbuffer[6] = 0;
	wbuffer[7] = 1;

	usbHandle->BulkOutEndPt->XferData(wbuffer, USB_buffLength, NULL, false);
	usbHandle->BulkInEndPt->XferData(rbuffer, USB_buffLength, NULL, true);
	int data = (rbuffer[0] << 8) + rbuffer[1];

	return(data);
}
// ---------------------------------------------------------
// Write the register of a given module (module + reg = addr)
// ---------------------------------------------------------
void Write(byte module, byte reg, int data)
{
	memset(wbuffer, 0x00, USB_BUFF_LENGTH);

	wbuffer[0] = 1; // settings from nepes
	wbuffer[1] = module + 0x80;
	wbuffer[2] = 0;
	wbuffer[3] = 0;
	wbuffer[4] = reg;
	wbuffer[5] = 0;
	wbuffer[6] = 0;
	wbuffer[7] = 1;
	wbuffer[8] = (byte)((data >> 8) & 0x00ff);
	wbuffer[9] = (byte)(data & 0x00ff);

	usbHandle->BulkOutEndPt->XferData(wbuffer, USB_buffLength, NULL, false);
}
