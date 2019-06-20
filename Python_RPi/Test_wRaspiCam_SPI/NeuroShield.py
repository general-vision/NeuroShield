# NeuroShield module
# Copyright 2018 General Vision Inc.
#
# For detail  on the SPI protocol refer to
# https://www.general-vision.com/documentation/TM_NeuroMem_Smart_protocol.pdf
#
# For detailts on the basic API function, refer to
# https://www.general-vision.com/documentation/TM_NeuroMem_API.pdf
#
# Requirement:
# https://github.com/doceme/py-spidev

from constants import *

import RPi.GPIO as GPIO

import time
import spidev
spi = spidev.SpiDev()

deviceID=0x01 #code for NeuroShield selected by nepes
SPI_CS = 24

def Connect():
        error=0
        GPIO.setmode(GPIO.BOARD)
        GPIO.setup(SPI_CS, GPIO.OUT, initial=1)
	spi.open(0,0)
	spi.max_speed_hz = 500000
	spi.mode = 0b00
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

#-----------------------------------------------
# clear neurons
#-----------------------------------------------
def ClearNeurons():
    Write(MOD_NM,NSR, 16)
    Write(MOD_NM,TESTCAT,1)
    Write(MOD_NM,NSR, 0)
    for i in range(0,255):
        Write(MOD_NM,INDEXCOMP, i)
        Write(MOD_NM,TESTCOMP, 0)
    Write(MOD_NM,FORGET, 0)

#-----------------------------------------------
# broadcast a vector
#-----------------------------------------------
def Broadcast(vector):
    l=len(vector)
    if l>1:
        for i in range(0,l-1):
            Write(MOD_NM,COMP, vector[i])
        # to test
        #WriteAddr(0x01000001, l-1, vector);
    Write(MOD_NM, LCOMP, vector[l-1])

#-----------------------------------------------
# Learn a vector using the current context value
#-----------------------------------------------
def Learn(vector, category):
    Broadcast(vector)
    Write(MOD_NM, CAT,category)
    return(Read(MOD_NM, NCOUNT))

#----------------------------------------------
# Recognize a vector and return the best match, or the 
# category, distance and identifier of the top firing neuron
#----------------------------------------------
def BestMatch(vector):
    Broadcast(vector)
    distance = Read(MOD_NM, DIST)
    category= Read(MOD_NM, CAT) 
    nid =Read(MOD_NM, NID)
    return(distance,category, nid)

#----------------------------------------------
# Recognize a vector and return the response  of up to K top firing neurons
# The response includes the distance, category and identifier of the neuron
# The Degenerated flag of the category is masked,
# Return the number of firing neurons or K whichever is smaller
#----------------------------------------------
def Classify(vector, K):
    Broadcast(vector)
    results=[[0 for col in range(0,3)] for row in range(0,K)]
    recoNbr=0;
    while recoNbr<K:
        results[recoNbr][0]= Read(MOD_NM, DIST)
        results[recoNbr][1]= Read(MOD_NM, CAT) & 0x7FFF
        results[recoNbr][2]= Read(MOD_NM, NID)
        if results[recoNbr][0]==65535:
            break
        else:
            recoNbr=recoNbr+1
    return(recoNbr,results);
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

#--------------------------------------------------------------------------------------
# Read the contents of the neuron pointed by index in the chain of neurons
# Returns its context, model (256 bytes), active infuence field, minimum influence field, and category
#--------------------------------------------------------------------------------------
def ReviewNeuron(index):
    TempNSR=Read(MOD_NM, NSR); # backup the NSR
    Write(MOD_NM,  NSR, 0x0010); # set the NSR to Save and Restore mode   
    Write(MOD_NM,  RESETCHAIN, 0);
    # move to the neuron pointed by index in the chain
    for i in range(0,index):
        Temp=Read(MOD_NM,  CAT); 
    # read the registers and memory of the neuron in focus
    ncr=Read(MOD_NM, NCR);
    model=[0 for i in range(0,256)]
    for i in range(0,256):
        model[i]=Read(MOD_NM, COMP);
    aif=Read(MOD_NM, AIF);
    minif=Read(MOD_NM, MINIF);
    cat=Read(MOD_NM, CAT);
    Write(1,  NSR, TempNSR); # restore the NSR
    return(ncr, model, aif, minif, cat);

#--------------------------------------------------------------------------------------
# Read the contents of the neuron pointed by index in the chain of neurons
# Returns an array of 264 bytes with the following format
# 2-bytes NCR, 256-bytes COMP, 2-bytes AIF, 2-bytes MINIF, 2-bytes CAT
#--------------------------------------------------------------------------------------
def ReadNeuron(index):
    TempNSR=Read(MOD_NM, NSR); # backup the NSR
    Write(MOD_NM,  NSR, 0x0010); # set the NSR to Save and Restore mode   
    Write(MOD_NM,  RESETCHAIN, 0);
    # move to the neuron pointed by index in the chain
    for i in range(0,index):
        Temp=Read(MOD_NM,  CAT); 
    # Read the registers and memory of the neuron in focus
    neuron=[0 for i in range(0,264)]
    Temp=Read(MOD_NM, NCR); 
    neuron[0]=(Temp & 0xFF00)>>8;
    neuron[1]= Temp & 0x00FF;
    for i in range(0,256):
        neuron[i+2]= Read(MOD_NM, COMP);
    Temp=Read(MOD_NM, AIF);
    neuron[258]=(Temp & 0xFF00)>>8;
    neuron[259]=Temp & 0x00FF;
    Temp=Read(MOD_NM, MINIF);
    neuron[260]=(Temp & 0xFF00)>>8;
    neuron[261]=Temp & 0x00FF;
    Temp=Read(MOD_NM, CAT);
    neuron[262]=(Temp & 0xFF00)>>8;
    neuron[263]=Temp & 0x00FF;
    Write(1,  NSR, TempNSR); # restore the NSR
    return(neuron);

#-------------------------------------------------------------
# Read the contents of "ncount" neurons, with ncount being less than or equal
# to the number of committed neurons. 
# Returns an array of ncount records of 264 bytes with the following format
# 2-bytes NCR, 256-bytes COMP, 2-bytes AIF, 2-bytes MINIF, 2-bytes CAT
#-------------------------------------------------------------
def ReadNeurons(ncount):
    TempNSR=Read(MOD_NM, NSR); # backup the NSR
    Write(MOD_NM,  NSR, 0x0010); # set the NSR to Save and Restore mode
    Write(MOD_NM,  RESETCHAIN, 0)
    neurons=[0 for i in range(0,ncount*264)]
    for i in range(0,ncount):
        Temp=Read(MOD_NM, NCR);
        neurons[(i*264)]=(Temp & 0xFF00)>>8;
        neurons[(i*264)+1]=Temp & 0x00FF;
        for j in range(0,256):
            neurons[(i*264)+j+2]= Read(MOD_NM, COMP);
        Temp=Read(MOD_NM, AIF);
        neurons[(i*264)+258]=(Temp & 0xFF00)>>8;
        neurons[(i*264)+259]=Temp & 0x00FF;
        Temp=Read(MOD_NM, MINIF);
        neurons[(i*264)+260]=(Temp & 0xFF00)>>8;
        neurons[(i*264)+261]=Temp & 0x00FF;
        Temp=Read(MOD_NM, CAT);
        neurons[(i*264)+262]=(Temp & 0xFF00)>>8;
        neurons[(i*264)+263]=Temp & 0x00FF;
    Write(1, NSR, TempNSR);
    return(neurons);

#-------------------------------------------------------------
# Clear and restore the content of ncount neurons from an array of
# ncount records of 264 bytes with the following format:
# 2-bytes NCR, 256-bytes COMP, 2-bytes AIF, 2-bytes MINIF, 2-bytes CAT
#-------------------------------------------------------------
def WriteNeurons(neurons, ncount):
    TempNSR=Read(MOD_NM, NSR);
    TempGCR=Read(MOD_NM, GCR);
    Write(MOD_NM, FORGET, 0);
    Write(MOD_NM, NSR, 0x0010);
    Write(MOD_NM, RESETCHAIN,0 );
    for i in range(0,ncount):
        Write(MOD_NM, NCR, (neurons[i*264]<<8)+neurons[(i*264)+1]);
        for j in range(0,256):
            Write(MOD_NM, COMP, neurons[(i*264)+2+j]);
        Write(MOD_NM, AIF, (neurons[(i*264)+258]<<8)+neurons[(i*264)+259]);
        Write(MOD_NM, MINIF, (neurons[(i*264)+260]<<8)+neurons[(i*264)+261]);
        Write(MOD_NM, CAT, (neurons[(i*264)+262]<<8)+neurons[(i*264)+263]);
    Write(MOD_NM, NSR, TempNSR);
    Write(MOD_NM, GCR, TempGCR);

#-------------------------------------------------------------
# Display the content of ncount neurons
#-------------------------------------------------------------
def DisplayNeurons(Len):
  print("\nDisplaying the neurons' content")
  ncount=Read(MOD_NM, NCOUNT)
  print ("Committed neurons= " + repr(ncount)) #should return the value 3
  Write(MOD_NM,NSR,16) #Set network to Save and Restore mode
  Write(MOD_NM,RESETCHAIN,0) # Resetchain to point at 1st committed neuron
  for i in range(0,ncount):
    print("Neuron %u: [" % (i+1)),
    for i in range(0,Len):
      print (Read(MOD_NM,COMP)),
    print ("]  AIF= %3u  CAT %3u" % (Read(MOD_NM,AIF),Read(MOD_NM,CAT) & 0x7FFF))
  Write(MOD_NM,NSR,0)
