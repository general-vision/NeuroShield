# NeuroShield

NeuroShield is a shield board featuring the NM500 neuromorphic chip with
576 neurons ready to learn and recognize stimuli extracted from any type
of sensors including IMU, audio, environmental sensors, bio-signal, video
and more.

- NeuroShield is compatible with Arduino, Raspberry PI, and any microcontrollers which can support SPI
    communication.
- NeuroShield can also be used as a simple USB dongle to empower PC-based applications with access
    to a NeuroMem network through a serial USB communication.


## Arduino library & examples

- **Academic scripts** to understand how easily you can teach the
    neurons and query them for simple recognition status, or a best
    match, or a detailed classification of the K nearest neurons.
- **Motion recognition examples** using the on-board IMU from
    Invensense (MPU6050) and the IMU from the Arduino101.
- **Video recognition examples** using an ArduCAM shield.

Save data files and project files in a format
compatible with our Knowledge Builder tools and SDKs.


## Python library & examples for Raspberry Pi

- **Academic scripts** to understand how easily you can teach the
    neurons and query them for simple recognition status, or a best
    match, or a detailed classification of the K nearest neurons.
- **Video recognition examples** using RaspiCam


## GV_NeuroMem API
- C++ library to access the neurons through the NeuroShield USB-serial port
- Academic script to understand how to teach and query the neurons
- For the latest documentation, refer to https://www.general-vision.com/documentation/TM_NeuroMem_API.pdf

If you have never connected a device on your PC using a Cypress USB serial chip, the NeuroShield will not be detected unless you run the CypressDriverInstaller.exe

Under the Windows Device Manager,the NeuroMem USB dongle should appear as a Universal Serial Bus Controller with the label "USB Composite device"

## NeuroMem Console
Windows-based utility to test the good health of the dongle but also to experience with the neurons using simple Register Transfer Level transactions.

Open the console using Start/General Vision/NeuroMem Console.
The default platform shown in the upper left corner of the panel is "Simu" and the network capacity reported in the listbox to the right is 1024 neurons.
Change the platform to NeuroShield and verify that the network capacity reported in the listbox to the right is 576 neurons, or more if you have stacked some NeuroBrick modules on th NeuroShield.

Fr more information on how to use the NeuroMem Console, click the Help button or refer to https://www.general-vision.com/documentation/TM_NeuroMem_Console.pdf.


## For additional tools compatible with NeuroShield, visit our web site:

- **NeuroMem Knowledge Builder** (https://www.general-vision.com/product/nmkb/)
- **CogniPat SDK** (https://www.general-vision.com/product/cp-sdk/)
- **CogniPat SDK MatLab** (https://www.general-vision.com/product/cp-sdk-ml/)
- **CogniPat SDK LabVIEW** (https://www.general-vision.com/product/cp-sdk-lv/)
- **Hardware Development Kit for Diligent Arty Z7** board (coming soon)


## Other interface

NeuroShield can be interfaced to any device supporting an SPI interface and supporting the protocol
described in https://www.general-
vision.com/documentation/TM_NeuroMem_Smart_protocol.pdf.

Source code of the primitive SPI Read and Write commands can be found in:
- Arduino\Libraries\Src\NeuroMemSPI.cpp
- USB\NeuroMemAPI\lib\comm_neuroshield
- Python ex\NeuroShield.py


## Hardware Specifications

For more details, refer to the nepes NeuroShield Hardware Manual at https://github.com/nepes-ai/neuroshield.


