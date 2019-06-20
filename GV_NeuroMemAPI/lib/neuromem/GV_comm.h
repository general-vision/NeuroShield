// GV_comm.h
// Copyright General Vision Inc.
//----------------------------------------------------------------
//
// Declaration of the communication functions necessary to access
// NeuroMem_Smart hardware platforms
//
// The protocol implemented in the R/W functions below has been written
// to support a variety of communication drivers including USB_Cypress,
// USB FTDI, SPI, I2C, etc.
// for more details regarding this protocol, refer to
// http://www.general-vision.com/documentation/TM_NeuroMem_Smart_protocol.pdf
//
int Connect(int DeviceID);
int Disconnect();
int Read(unsigned char module, unsigned char reg);
void Write(unsigned char module, unsigned char reg, int value);
int Write_Addr(int addr, int length_inByte, unsigned char data[]);
int Read_Addr(int addr, int length_inByte, unsigned char data[]);

//
// Definition of the NeuroMem neuron registers
//
#define MOD_NM			0x01

#define NM_NCR			0x00
#define NM_COMP			0x01
#define NM_LCOMP		0x02
#define NM_DIST			0x03 
#define NM_INDEXCOMP	0x03 
#define NM_CAT			0x04
#define NM_AIF			0x05
#define NM_MINIF		0x06
#define NM_MAXIF		0x07
#define NM_TESTCOMP		0x08
#define NM_TESTCAT		0x09
#define NM_NID			0x0A
#define NM_GCR			0x0B
#define NM_RESETCHAIN	0x0C
#define NM_NSR			0x0D
#define NM_NCOUNT		0x0F	
#define NM_FORGET		0x0F

#define DEFMAXIF		0x4000
#define DEFMINIF		0x0002
#define DEFGCR			0x01