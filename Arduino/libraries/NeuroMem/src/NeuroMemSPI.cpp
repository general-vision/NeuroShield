/************************************************************************/
/*																		
 *	NeuroMemSPI.cpp	--	SPI driver common to all NeuroMem hardware				        	
 *	Copyright (c) 2017, General Vision Inc, All rights reserved
 *
 *  Created September 8, 2017
 *  Modified 02/13/2018
 *  Support BrainCard, NeuroShield, NeuroStamp
 *  
 *  Modified May 28, 2019
 *  UNO, 101 and older Arduino boards
 *  might not be compatible with the latest driver
 *  - begin() does not take pinselect input
 *  - possibly replace spi.settings with spi.setclockdivider
 *
 * http://www.general-vision.com/documentation/TM_NeuroMem_Smart_protocol.pdf
 */
/******************************************************************************/

/* ------------------------------------------------------------ */
/*				Include File Definitions						*/
/* ------------------------------------------------------------ */

#include <NeuroMemSPI.h>

using namespace std;
extern "C" {
  #include <stdint.h>
}

// SPI NeuroMem select pin
#define NM_CS_BRAINCARD 10
#define NM_CS_NEUROSHIELD 7
#define NM_CS_NEUROSTAMP 10
#define NM_CS_NEUROTILE 10

// SPI speed
#define CK_BRAINCARD 4000000 // 4Mhz
#define CK_NEUROSHIELD 2000000 // 2 Mhz
#define CK_NEUROSTAMP 2000000 // 2 Mhz
#define CK_NEUROTILE 4000000 // 4Mhz

int SPI_NMSelect;
int SPIClockDiv = SPI_CLOCK_DIV8;
int SPIspeed;

NeuroMemSPI::NeuroMemSPI(){	
}
// ------------------------------------------------------------ 
// Initialize the SPI communication and verify proper interface
// to the NeuroMem network by reading the default Minif value of 2-bytes
// return an error=1 otherwise
// ------------------------------------------------------------ 
int NeuroMemSPI::connect(int Platform)
{
	platform=Platform;
	switch(platform)
	{
		case HW_NEUROSHIELD:
			SPI_NMSelect = NM_CS_NEUROSHIELD;
			SPIspeed= CK_NEUROSHIELD;		
			pinMode (5, OUTPUT); //pin Arduino_CON
			digitalWrite(5, LOW); // pin Arduino_CON
			pinMode (6, OUTPUT); //pin Arduino_SD_CS
			digitalWrite(6,HIGH); // pin Arduino_SD_CS
			delay(100);
			break;
		case HW_NEUROSTAMP:
			SPI_NMSelect = NM_CS_NEUROSTAMP;
			SPIspeed= CK_NEUROSTAMP;
			SPIClockDiv=SPI_CLOCK_DIV16;			
			break;
		case HW_BRAINCARD:
			SPI_NMSelect = NM_CS_BRAINCARD;
			SPIspeed= CK_BRAINCARD;
			pinMode (8, OUTPUT); // pin Flash
			digitalWrite(8, HIGH);
			// SPI_NMSelect and FPGAFlashPin might need reset by pulling both CSn low and returning to high
			break;
		case HW_NEUROTILE: 
			SPI_NMSelect = NM_CS_NEUROTILE;
			SPIspeed= CK_NEUROTILE;
			// SPIselect pin might need reset by pulling both CSn low and returning to high
			break;
	}
	SPI.begin(SPI_NMSelect);
	//SPI.setClockDivider(SPIClockDiv);
	pinMode (SPI_NMSelect, OUTPUT);
	digitalWrite(SPI_NMSelect, HIGH);
	delay(100);
	// Read MINIF (reg 6) and verify that it is equal to 2
	if(read(1, 6)==2)return(0);else return(1); 
}
// --------------------------------------------------------
// Read the FPGA_version
//---------------------------------------------------------
int NeuroMemSPI::FPGArev()
{
	int FPGArev=0;
	switch(platform)
	{
		case HW_BRAINCARD: FPGArev=read(1, 0x0E); break;
		case HW_NEUROSHIELD: FPGArev=read(2, 1); break;
		case HW_NEUROSTAMP: FPGArev=0; break;
		case HW_NEUROTILE: FPGArev=0; break;
	}
	return(FPGArev);
}
// --------------------------------------------------------
// SPI Read a register of the NeuroMem network
//---------------------------------------------------------
int NeuroMemSPI::read(unsigned char mod, unsigned char reg)
{
	SPI.beginTransaction(SPISettings(SPIspeed, MSBFIRST, SPI_MODE0));
	digitalWrite(SPI_NMSelect, LOW);
	SPI.transfer(1);  // Dummy for ID
	SPI.transfer(mod);
	SPI.transfer(0);
	SPI.transfer(0);
	SPI.transfer(reg);
	SPI.transfer(0); // length[23-16]
	SPI.transfer(0); // length [15-8]
	SPI.transfer(1); // length [7-0]
	int data = SPI.transfer(0); // Send 0 to push upper data out
	data = (data << 8) + SPI.transfer(0); // Send 0 to push lower data out
	digitalWrite(SPI_NMSelect, HIGH);
	SPI.endTransaction();
	return(data);
}
// ---------------------------------------------------------
// SPI Write a register of the NeuroMem network
// ---------------------------------------------------------
void NeuroMemSPI::write(unsigned char mod, unsigned char reg, int data)
{
	SPI.beginTransaction(SPISettings(SPIspeed, MSBFIRST, SPI_MODE0));
	digitalWrite(SPI_NMSelect, LOW);
	SPI.transfer(1);  // Dummy for ID
	SPI.transfer(mod + 0x80); // module and write flag
	SPI.transfer(0);
	SPI.transfer(0);
	SPI.transfer(reg);
	SPI.transfer(0); // length[23-16]
	SPI.transfer(0); // length[15-8]
	SPI.transfer(1); // length[7-0]
	SPI.transfer((unsigned char)(data >> 8)); // upper data
	SPI.transfer((unsigned char)(data & 0x00FF)); // lower data
	digitalWrite(SPI_NMSelect, HIGH);
	SPI.endTransaction();
}
// ---------------------------------------------------------
// SPI Write_Addr command
// multiple write of data in word format
// length is expressed in words
// ---------------------------------------------------------
void NeuroMemSPI::writeAddr(long addr, int length, int data[])
{
	SPI.beginTransaction(SPISettings(SPIspeed, MSBFIRST, SPI_MODE0));
	digitalWrite(SPI_NMSelect, LOW);
	SPI.transfer(1);  // Dummy for ID
	SPI.transfer((byte)(((addr & 0xFF000000) >> 24) + 0x80)); // Addr3 and write flag
	SPI.transfer((byte)((addr & 0x00FF0000) >> 16)); // Addr2
	SPI.transfer((byte)((addr & 0x0000FF00) >> 8)); // Addr1
	SPI.transfer((byte)(addr & 0x000000FF)); // Addr0
	SPI.transfer((byte)((length & 0x00FF0000) >> 16)); // Length2
	SPI.transfer((byte)(length & 0x0000FF00) >> 8); // Length1
	SPI.transfer((byte)(length & 0x000000FF)); // Length 0
	for (int i = 0; i < length; i++)
	{
		SPI.transfer((data[i] & 0xFF00)>> 8);
		SPI.transfer(data[i] & 0x00FF);
	}
	digitalWrite(SPI_NMSelect, HIGH);
	SPI.endTransaction();
} 
//---------------------------------------------
// SPI Read_Addr command
// multiple read of data in word format
// length is expressed in words
//---------------------------------------------
void NeuroMemSPI::readAddr(long addr, int length, int data[])
{
	SPI.beginTransaction(SPISettings(SPIspeed, MSBFIRST, SPI_MODE0));
	digitalWrite(SPI_NMSelect, LOW);
	SPI.transfer(1);  // Dummy for ID
	SPI.transfer((byte)((addr & 0xFF000000) >> 24)); // Addr3 and write flag
	SPI.transfer((byte)((addr & 0x00FF0000) >> 16)); // Addr2
	SPI.transfer((byte)((addr & 0x0000FF00) >> 8)); // Addr1
	SPI.transfer((byte)(addr & 0x000000FF)); // Addr0
	SPI.transfer((byte)((length & 0x00FF0000) >> 16)); // Length2
	SPI.transfer((byte)(length & 0x0000FF00) >> 8); // Length1
	SPI.transfer((byte)(length & 0x000000FF)); // Lenght0
	for (int i = 0; i < length; i++)
	{
		data[i] = SPI.transfer(0); // Send 0 to push upper data out
		data[i] = (data[i] << 8) + SPI.transfer(0); // Send 0 to push lower data out
	}
	digitalWrite(SPI_NMSelect, HIGH);
	SPI.endTransaction();
}

