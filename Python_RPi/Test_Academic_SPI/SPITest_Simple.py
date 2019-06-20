             ##
## Test SPI
##
## Simple Test verifying SPI access to the NeuroShield
##
## Adjust the SPI clock speed in NeuroMem.py
## if necessary for communication stability

import GVcomm_SPI as comm
from GVconstants import *

print ("NeuroShield Test SPI with single RW\n")

comm.Connect()
comm.Write(MOD_NM,FORGET,0)

value=comm.Read(MOD_NM,MINIF)
print("Minif=%u" % value)

value=comm.Read(MOD_NM,MAXIF)
print("Maxif=%u" % value)

value=comm.Read(MOD_NM,GCR)
print("GCR=%u" % value)

value1= 1
value2= 4096
iteration=0
error=0

while iteration<5:
#while True:
    error=0;
    
    ## Single Write to the MINIF and MAXIF registers
    comm.Write(MOD_NM,MINIF, value1)
    comm.Write(MOD_NM,MAXIF, value2)

    ## Single Read to the MINIF and MAXIF registers
    ## and verification of the expected value
    value=comm.Read(MOD_NM,MINIF)
    if (value!=value1):
        error=1
        print("\nERROR, incorrect Minif=%u" % value)
    
    value=comm.Read(MOD_NM,MAXIF)
    if (value!=value2):
        error=2
        print("\nERROR2, incorrect Maxif=%u" % value)

    if (error==0):
            print ("\nIteration= %u, Pass" % iteration)

    iteration=iteration+1       
    value1=value1+1
    if (value1==256):
        value1=0
    value2=value2-1
    if (value2==0):
        value2=4096



