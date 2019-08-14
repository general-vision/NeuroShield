# GVcomm_SPI
# Copyright 2018 General Vision Inc.
#
# For detail  on the SPI protocol refer to
# https://www.general-vision.com/documentation/TM_NeuroMem_Smart_protocol.pdf
#
# Requirement:
# https://github.com/doceme/py-spidev

import RPi.GPIO as GPIO
import spidev
spi = spidev.SpiDev()

deviceID=0x01 #code for NeuroShield selected by nepes
SPI_CS = 24

def Connect():
    error=0
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(SPI_CS, GPIO.OUT, initial=1)
    #GPIO.setwarnings(False)
    spi.open(0,0)
    spi.max_speed_hz = 300000
    spi.mode = 0b00
    Write(1, 15, 0)
    if (Read(1,6)!=2):
        error=1
    return(error)

def Disconnect():
    spi.close()
    GPIO.cleanup()
    
#---------------------------------------
# read the register of a given module
#---------------------------------------
def Read(module, reg):
    GPIO.output(SPI_CS, 0)
    cmdread=[deviceID, module, 0x00, 0x00, reg, 0x00, 0x00, 0x01, 0x00, 0x00]
    data= spi.xfer2(cmdread)
    GPIO.output(SPI_CS, 1)
    return ((data[8] << 8) + data[9]);

#---------------------------------------
# write the register of a given module
#---------------------------------------
def Write(module, reg, data):
    GPIO.output(SPI_CS, 0)
    cmdwrite=[deviceID, module + 0x80, 0x00, 0x00, reg, 0x00, 0x00, 0x01, (data & 0xFF00)>> 8, data & 0x00FF]
    data= spi.xfer2(cmdwrite)
    GPIO.output(SPI_CS, 1)

#---------------------------------------
# read word array at address
#---------------------------------------
def ReadAddr(address, length):
    addr0=(int)(address & 0xFF000000) >> 24
    addr1=(address & 0x00FF0000) >> 16
    addr2=(address & 0x0000FF00) >> 8
    addr3=address & 0x000000FF
    len0=(length & 0x00FF0000) >> 16
    len1=(length & 0x0000FF00) >> 8
    len2=length & 0x000000FF
    GPIO.output(SPI_CS, 0)
    cmdread=[deviceID, addr0, addr1, addr2, addr3, len0, len1, len2]
    databyte=[0 for i in range(0,length*2)]
    databyte2=spi.xfer2(cmdread+databyte)
    data=[0 for i in range(0,length)]
    for i in range(0,length):
        data[i] = (databyte2[8+(i*2)] << 8) + databyte2[8+(i*2)+1];
    GPIO.output(SPI_CS, 1)
    return(data)

#---------------------------------------
# write word array at address
#---------------------------------------
def WriteAddr(address, length, data):
    addr0=(int)(address & 0xFF000000) >> 24
    addr1=(address & 0x00FF0000) >> 16
    addr2=(address & 0x0000FF00) >> 8
    addr3=address & 0x000000FF
    len0=(length & 0x00FF0000) >> 16
    len1=(length & 0x0000FF00) >> 8
    len2=length & 0x000000FF
    GPIO.output(SPI_CS, 0)
    cmdwrite=[deviceID, addr0 + 0x80, addr1, addr2, addr3, len0, len1, len2]
    databyte=[0 for i in range(0,length *2)]
    for i in range(0,length):
           databyte[(i*2)]= data[i] >> 8
           databyte[(i*2)+1]=data[i] & 0x00FF
    spi.xfer2(cmdwrite+databyte)
    GPIO.output(SPI_CS, 1)
