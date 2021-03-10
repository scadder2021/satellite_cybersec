# SCADDER 2021 - Satellite Anomaly Detection

This project is based on earlier iterations of this project, which can be found here: https://github.com/scadder-cu-boulder/spacecraft_cyberSec

Satellites are a vital, but underprotected asset. They were long considered immune to cyber attack due to their physical distance from the earth, but this is not the case. The goal of this project is to prototype an ML-enabled anomaly detection system to run aboard a satellite that looks at MIL-STD-1553 bus traffic.

A satellite simulation was created using a Xilinx ZYNC ZC706 FPGA kit, a switch, and Raspberry Pis connected via ethernet as shown below:

![alt text](https://github.com/scadder2021/satellite_cybersec/blob/main/FBD_2021.png?raw=true)

There are three main components of found in this repository:

# MIL-STD-1553 Communication/Flight Software Emulation

- MIL-STD-1553_Library.c: This is the core of the emulation code and contains all the functions for constructing 1553 words and sending data between the bus controller and remote terminals. BC_Simulator.c and RT_simulator.c both call into this library to perform 1553 functions, so this library should be included on the bus controller and every remote terminal.
- BC_Simulator.c: The bus controller simulator initializes a bc listener. This file is #included in Flight_Software.c and will be initialized by Flight_Software.c. This file should be stored on the Peta-Linux instance, along with Flight_Software.c (and, of course, the 1553_Library as well).
- RT_Simulator.c: This file initializes the remote terminal and should be run
- Flight_Software.c: The flight software file defines a series of commands to be sent to spacecraft systems periodically. It should be run on Peta-Linux.

# Attack Code

# Security Monitor
