# SCADDER 2021 - Satellite Anomaly Detection

This project is based on earlier iterations of this project, which can be found here: https://github.com/scadder-cu-boulder/spacecraft_cyberSec

Satellites are a vital, but underprotected asset. They were long considered immune to cyber attack due to their physical distance from the earth, but this is not the case. The goal of this project is to prototype an ML-enabled anomaly detection system to run aboard a satellite that looks at MIL-STD-1553 bus traffic.

A satellite simulation was created using a Xilinx ZYNC ZC706 FPGA kit, a switch, and Raspberry Pis connected via ethernet as shown below:

![alt text](https://github.com/scadder2021/satellite_cybersec/blob/main/FBD_2021.png?raw=true)

There are three main components of found in this repository:

# MIL-STD-1553 Communication/Flight Software Emulation

