# NeuroMem.py
# Copyright 2018 General Vision Inc.
#
# For details on the basic API function, refer to
# https://www.general-vision.com/documentation/TM_NeuroMem_API.pdf

from GVconstants import *
import GVcomm_SPI as comm
import ctypes

#-----------------------------------------------
# clear neurons
#-----------------------------------------------
def ClearNeurons():
    comm.Write(MOD_NM,NSR, 16)
    comm.Write(MOD_NM,TESTCAT,1)
    comm.Write(MOD_NM,NSR, 0)
    for i in range(0,255):
        comm.Write(MOD_NM,INDEXCOMP, i)
        comm.Write(MOD_NM,TESTCOMP, 0)
    comm.Write(MOD_NM,FORGET, 0)

#-----------------------------------------------
# broadcast a vector
#-----------------------------------------------
def Broadcast(vector, len):
    if len>1:
        for i in range(0,len-1):
            comm.Write(MOD_NM,COMP, vector[i])
        # to test
        #comm.WriteAddr(0x01000001, l-1, vector);
    comm.Write(MOD_NM, LCOMP, vector[len-1])

#-----------------------------------------------
# Learn a vector using the current context value
#-----------------------------------------------
def Learn(vector, len, category):
    Broadcast(vector, len)
    comm.Write(MOD_NM, CAT,category)
    return(comm.Read(MOD_NM, NCOUNT))

#----------------------------------------------
# Recognize a vector and return the best match, or the 
# category, distance and identifier of the top firing neuron
#----------------------------------------------
def BestMatch(vector, len):
    Broadcast(vector, len)
    distance = comm.Read(MOD_NM, DIST)
    category= comm.Read(MOD_NM, CAT) 
    nid =comm.Read(MOD_NM, NID)
    return(distance,category, nid)

#----------------------------------------------
# Recognize a vector and return the response  of up to K top firing neurons
# The response includes the distance, category and identifier of the neuron
# The Degenerated flag of the category is masked,
# Return the number of firing neurons or K whichever is smaller
#----------------------------------------------
def Classify(vector, len, K):
    Broadcast(vector, len)
    bytearray = ctypes.c_int * K
    dists= bytearray()
    cats= bytearray()
    nids= bytearray()
    recoNbr=0
    while recoNbr<K:
        dists[recoNbr]= comm.Read(MOD_NM, DIST)
        cats[recoNbr]= comm.Read(MOD_NM, CAT) & 0x7FFF
        nids[recoNbr]= comm.Read(MOD_NM, NID)
        if dists[recoNbr]==65535:
            break
        else:
            recoNbr=recoNbr+1
    return(recoNbr,dists, cats,nids)

#--------------------------------------------------------------------------------------
# Read the contents of the neuron pointed by index in the chain of neurons
# Returns its context, model (256 bytes), active infuence field, minimum influence field, and category
#--------------------------------------------------------------------------------------
def ReviewNeuron(index):
    TempNSR=comm.Read(MOD_NM, NSR) # backup the NSR
    comm.Write(MOD_NM,  NSR, 0x0010) # set the NSR to Save and Restore mode   
    comm.Write(MOD_NM,  RESETCHAIN, 0)
    # move to the neuron pointed by index in the chain
    for i in range(0,index):
        Temp=comm.Read(MOD_NM,  CAT) 
    # comm.Read the registers and memory of the neuron in focus
    ncr=comm.Read(MOD_NM, NCR)
    model=[0 for i in range(0,256)]
    for i in range(0,256):
        model[i]=comm.Read(MOD_NM, COMP)
    aif=comm.Read(MOD_NM, AIF)
    minif=comm.Read(MOD_NM, MINIF)
    cat=comm.Read(MOD_NM, CAT)
    comm.Write(1,  NSR, TempNSR) # restore the NSR
    return(ncr, model, aif, minif, cat)

#--------------------------------------------------------------------------------------
# Read the contents of the neuron pointed by index in the chain of neurons
# Returns an array of 264 bytes with the following format
# 2-bytes NCR, 256-bytes COMP, 2-bytes AIF, 2-bytes MINIF, 2-bytes CAT
#--------------------------------------------------------------------------------------
def ReadNeuron(index):
    TempNSR=comm.Read(MOD_NM, NSR) # backup the NSR
    comm.Write(MOD_NM,  NSR, 0x0010) # set the NSR to Save and Restore mode   
    comm.Write(MOD_NM,  RESETCHAIN, 0)
    # move to the neuron pointed by index in the chain
    for i in range(0,index):
        Temp=comm.Read(MOD_NM,  CAT) 
    # comm.Read the registers and memory of the neuron in focus
    neuron=[0 for i in range(0,264)]
    Temp=comm.Read(MOD_NM, NCR) 
    neuron[0]=(Temp & 0xFF00)>>8
    neuron[1]= Temp & 0x00FF
    for i in range(0,256):
        neuron[i+2]= comm.Read(MOD_NM, COMP)
    Temp=comm.Read(MOD_NM, AIF)
    neuron[258]=(Temp & 0xFF00)>>8
    neuron[259]=Temp & 0x00FF
    Temp=comm.Read(MOD_NM, MINIF)
    neuron[260]=(Temp & 0xFF00)>>8
    neuron[261]=Temp & 0x00FF
    Temp=comm.Read(MOD_NM, CAT)
    neuron[262]=(Temp & 0xFF00)>>8
    neuron[263]=Temp & 0x00FF
    comm.Write(1,  NSR, TempNSR) # restore the NSR
    return(neuron)

#-------------------------------------------------------------
# comm.Read the contents of "ncount" neurons, with ncount being less than or equal
# to the number of committed neurons. 
# Returns an array of ncount records of 264 bytes with the following format
# 2-bytes NCR, 256-bytes COMP, 2-bytes AIF, 2-bytes MINIF, 2-bytes CAT
#-------------------------------------------------------------
def ReadNeurons(ncount):
    TempNSR=comm.Read(MOD_NM, NSR) # backup the NSR
    comm.Write(MOD_NM,  NSR, 0x0010) # set the NSR to Save and Restore mode
    comm.Write(MOD_NM,  RESETCHAIN, 0)
    neurons=[0 for i in range(0,ncount*264)]
    for i in range(0,ncount):
        Temp=comm.Read(MOD_NM, NCR)
        neurons[(i*264)]=(Temp & 0xFF00)>>8
        neurons[(i*264)+1]=Temp & 0x00FF
        for j in range(0,256):
            neurons[(i*264)+j+2]= comm.Read(MOD_NM, COMP)
        Temp=comm.Read(MOD_NM, AIF)
        neurons[(i*264)+258]=(Temp & 0xFF00)>>8
        neurons[(i*264)+259]=Temp & 0x00FF
        Temp=comm.Read(MOD_NM, MINIF)
        neurons[(i*264)+260]=(Temp & 0xFF00)>>8
        neurons[(i*264)+261]=Temp & 0x00FF
        Temp=comm.Read(MOD_NM, CAT)
        neurons[(i*264)+262]=(Temp & 0xFF00)>>8
        neurons[(i*264)+263]=Temp & 0x00FF
    comm.Write(1, NSR, TempNSR)
    return(neurons)

#-------------------------------------------------------------
# Clear and restore the content of ncount neurons from an array of
# ncount records of 264 bytes with the following format:
# 2-bytes NCR, 256-bytes COMP, 2-bytes AIF, 2-bytes MINIF, 2-bytes CAT
#-------------------------------------------------------------
def WriteNeurons(neurons, ncount):
    TempNSR=comm.Read(MOD_NM, NSR)
    TempGCR=comm.Read(MOD_NM, GCR)
    comm.Write(MOD_NM, FORGET, 0)
    comm.Write(MOD_NM, NSR, 0x0010)
    comm.Write(MOD_NM, RESETCHAIN,0 )
    for i in range(0,ncount):
        comm.Write(MOD_NM, NCR, (neurons[i*264]<<8)+neurons[(i*264)+1])
        for j in range(0,256):
            comm.Write(MOD_NM, COMP, neurons[(i*264)+2+j])
        comm.Write(MOD_NM, AIF, (neurons[(i*264)+258]<<8)+neurons[(i*264)+259])
        comm.Write(MOD_NM, MINIF, (neurons[(i*264)+260]<<8)+neurons[(i*264)+261])
        comm.Write(MOD_NM, CAT, (neurons[(i*264)+262]<<8)+neurons[(i*264)+263])
    comm.Write(MOD_NM, NSR, TempNSR)
    comm.Write(MOD_NM, GCR, TempGCR)

#-------------------------------------------------------------
# Display the content of ncount neurons
#-------------------------------------------------------------
def DisplayNeurons(len):
  print("\nDisplaying the neurons' content")
  ncount=comm.Read(MOD_NM, NCOUNT)
  print ("Committed neurons= " + repr(ncount)) #should return the value 3
  comm.Write(MOD_NM,NSR,16) #Set network to Save and Restore mode
  comm.Write(MOD_NM,RESETCHAIN,0) # Resetchain to point at 1st committed neuron
  for i in range(0,ncount):
    print("Neuron %u: [" % (i+1), end=" ")
    for i in range(0,len):
      print (comm.Read(MOD_NM,COMP), end=" ")
    print ("]  AIF= %3u  CAT %3u" % (comm.Read(MOD_NM,AIF),comm.Read(MOD_NM,CAT) & 0x7FFF))
  comm.Write(MOD_NM,NSR,0)
