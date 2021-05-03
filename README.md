# SCADDER 2021 - Satellite Anomaly Detection

This project is based on earlier iterations of this project, which can be found here: https://github.com/scadder-cu-boulder/spacecraft_cyberSec

Satellites are a vital, but underprotected asset. They were long considered immune to cyber attack due to their physical distance from the earth, but this is not the case. The goal of this project is to prototype an ML-enabled anomaly detection system that looks at MIL-STD-1553 bus traffic.

A satellite simulation was created using a Xilinx ZYNC ZC706 FPGA kit, a switch, and Raspberry Pis connected via ethernet as shown below:

![alt text](https://github.com/scadder2021/satellite_cybersec/blob/main/FBD_2021.png?raw=true)

There are three main components of found in this repository:

# MIL-STD-1553 Communication/Flight Software Emulation

The 1553 simulation code is responsible for replicating 1553 communication as defined in the standard (available: https://quicksearch.dla.mil/qsDocDetails.aspx?ident_number=36973) and contains three files:
- MIL-STD-1553_Library.c: This is the core of the emulation code and contains all the functions for constructing 1553 words, sending data between the bus controller and remote terminals, and analyzing incoming words. BC_Simulator.c and RT_simulator.c both call into this library to perform 1553 functions, so this library should be included in the directory with these files when they are compiled.
- BC_Simulator.c: The bus controller simulator initializes a socket to listen for commands. This file is #included in Flight_Software.c and will be initialized by Flight_Software.c. The MACROS at the top of the file may be modified to specify a port on which to listen for and a port on which to send data. The RT Address is set to 0, since the 1553_Library requires an address be specified, but the bus controller does not have/need an address to function.
- RT_Simulator.c: This file initializes the remote terminal and starts listening for data. The MACROS at the top of the file may be modified to specify a port on which to listen for and a port on which to send data, as well as the RT address (in this case, the address of each device is defined in the library and the RT passes the name of its device, i.e. STARTRACKER)
- Flight_Software.c: The flight software file defines a series of commands to be sent to spacecraft systems periodically. It sends these commands via the bus controller and should be run on Peta-Linux. Currently, the flight software repeatedly sends out commands and then waits ten seconds before sending the next, but this can be refined to more closely mirror satellite function.

How to Run:

- Clone the Github repository.
- Modify the RT_Simulator files so the RT addresses defined match the set up. To do this, first make sure the macros within the library are the devices and addresses you would like to use (For example, "#define THRUSTER 0x01" at line 38). Then, make sure each Pi is running as the device you want it to be (in the RT_simulator code, #define RT_ADDRESS THRUSTER)
- Compile each file using: gcc -lpthread <file> -o <output file name> (compile the Flight_software code on a Raspberry Pi and then transfer it to the PetaLinux instance).
- Run the RT_Simulator code on each Raspberry Pi to initialize the 1553 library and begin listening for data on the defined ports.
- Run the compiled flight software on the PetaLinux to begin sending spacecraft commands and receiving responses.

# Attack Code

# Security Monitor
