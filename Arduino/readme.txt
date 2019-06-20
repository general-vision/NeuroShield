/*****************************************/
/*                                       */
/* Using the neurons of a NeuroMem_Smart */
/* device through Arduino IDE            */
/*                                       */
/*****************************************/

Software installation:
----------------------
Copy the NeuroMem folder to MyDocuments\\Arduino\\libraries
Launch the Arduino IDE
Under Tools/Board Manager menu, select your Arduino processor board
Under File/Examples/NeuroMem, you can access
	- generic examples for all NeuroMem-Smart devices
	- hardware specific examples

/*****************************************/
/*             src folder                */
/*****************************************/

Functions to use the NeuroMem neurons are documented in
https://www.general-vision.com/documentation/TM_NeuroMem_API.pdf
- NeuroMemAI.cpp
- NeuroMemAI.h

Functions to communicate with the NeuroMem hardware are documented in http://www.general-vision.com/documentation/TM_NeuroMem_Smart_protocol.pdf 
- NeuroMemSPI.cpp
- NeuroMemSPI.h

Functions to access a Flash memeory through SPI,
dependency to reprogram the FPGA of a NeuroMem hardware,
deployed on the BrainCard
- SPIFlash.cpp
- SPIFlash.h


/*****************************************/
/*                Support                */
/*****************************************/

Refer to http://general-vision.com/support

