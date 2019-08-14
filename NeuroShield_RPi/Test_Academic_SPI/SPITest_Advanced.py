##
## Test SPI
##
## Simple Test verifying SPI access to the NeuroShield
##
## Adjust the SPI clock speed in NeuroMem.py
## if necessary for communication stability

import GVcomm_SPI as comm
from GVconstants import *

print ("NeuroShield Test SPI with single and multiple RW\n")
error=comm.Connect()
if (error!=0):
    print ("Cannot connect to NeuroShield\n")
#    sys.exit()

comm.Write(MOD_NM,FORGET,0)
value=comm.Read(MOD_NM,MINIF)
print("Minif=%u" % value)

value=comm.Read(MOD_NM,MAXIF)
print("Maxif=%u" % value)

value=comm.Read(MOD_NM,GCR)
print("GCR=%u" % value)

value1= 1;
value2= 4096;
value3=10;
length=10;
iteration=0;
error=0;

while iteration<5:
    error=0;
    
    ## Single Write to the MINIF and MAXIF registers
    comm.Write(MOD_NM,MINIF, value1);
    comm.Write(MOD_NM,MAXIF, value2);

    ## Multiple Write to the COMP register
    ## after setting the neurons in save and restore mode
    comm.Write(MOD_NM, NSR,16);
    comm.Write(MOD_NM, RESETCHAIN,0);
    data=[0 for i in range(0,length)]
    for i in range(0,length):
        data[i]=value3+i;
    comm.WriteAddr(0x01000001, length, data);
    comm.Read(MOD_NM, CAT) ##to move to next neuron in the chain
    comm.Write(MOD_NM, NSR,0); ##set NeuroMem network in normal mode

    ## Single Read to the MINIF and MAXIF registers
    ## and verification of the expected value
    value=comm.Read(MOD_NM,MINIF);
    if (value!=value1):
        error=1;
        print("ERROR, incorrect Minif=%u" % value);
    
    value=comm.Read(MOD_NM,MAXIF);
    if (value!=value2):
        error=2;
        print("ERROR2, incorrect Maxif=%u" % value);

    ## Multiple Write to the COMP register
    ## after setting the neurons in save and restore mode
    comm.Write(MOD_NM, NSR,16); ##set MOD_NM in save and restore mode
    comm.Write(MOD_NM, RESETCHAIN,0);
    data=comm.ReadAddr(0x01000001, length);
    for i in range(0,length):
        if(data[i]!= value3+i):
            error=3;
            print("ERROR3, incorrect COMP %u = %u" % (i, data[i]));
    comm.Read(MOD_NM, CAT) ##to move to next neuron in the chain
    comm.Write(MOD_NM, NSR,0); ##set MOD_NM in normal mode

    if (error==0):
            print ("\nIteration= %u, Pass" % iteration);

    iteration=iteration+1;       
    value1=value1+1;
    if (value1==256):
        value1=0
    value2=value2-1;
    if (value2==0):
        value2=4096;
    value3=value3+1;
    if (value3==256):
        value3=0;


