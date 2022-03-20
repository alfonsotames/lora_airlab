/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.h to edit this template
 */

/* 
 * File:   opc.h
 * Author: mufufu
 *
 * Created on March 17, 2022, 10:05 AM
 */

#ifndef OPC_H
#define OPC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
    


    
    
struct histogram {
    double bin0;
    double bin1;
    double bin2;
    double bin3;
    double bin4;
    double bin5;
    double bin6;
    double bin7;
    double bin8;
    double bin9;
    double bin10;
    double bin11;
    double bin12;
    double bin13;
    double bin14;
    double bin15;

    // Mass Time-of-Flight
    float bin1MToF;
    float bin3MToF;
    float bin5MToF;
    float bin7MToF;

    // Sample Flow Rate
    float sfr;

    // Either the Temperature or Pressure
    unsigned long temp_pressure;

    // Sampling Period
    float period;

    // Checksum
    unsigned int checksum;

    float pm1;
    float pm25;
    float pm10;
};

struct PMData {
  float pm1;
  float pm25;
  float pm10;
};

struct ConfigVars {
    // Bin Boundaries
    int bb0;
    int bb1;
    int bb2;
    int bb3;
    int bb4;
    int bb5;
    int bb6;
    int bb7;
    int bb8;
    int bb9;
    int bb10;
    int bb11;
    int bb12;
    int bb13;
    int bb14;

    // Bin Particle Volume (floats)
    float bpv0;
    float bpv1;
    float bpv2;
    float bpv3;
    float bpv4;
    float bpv5;
    float bpv6;
    float bpv7;
    float bpv8;
    float bpv9;
    float bpv10;
    float bpv11;
    float bpv12;
    float bpv13;
    float bpv14;
    float bpv15;

    // Bin Particle Densities (floats)
    float bpd0;
    float bpd1;
    float bpd2;
    float bpd3;
    float bpd4;
    float bpd5;
    float bpd6;
    float bpd7;
    float bpd8;
    float bpd9;
    float bpd10;
    float bpd11;
    float bpd12;
    float bpd13;
    float bpd14;
    float bpd15;

    // Bin Sample Volume Weightings (floats)
    float bsvw0;
    float bsvw1;
    float bsvw2;
    float bsvw3;
    float bsvw4;
    float bsvw5;
    float bsvw6;
    float bsvw7;
    float bsvw8;
    float bsvw9;
    float bsvw10;
    float bsvw11;
    float bsvw12;
    float bsvw13;
    float bsvw14;
    float bsvw15;

    // Gain Scaling Coefficient
    float gsc;

    // Sample Flow Rate (ml/s)
    float sfr;

    // LaserDAC 8 bit int
    unsigned int laser_dac;
    unsigned int fan_dac;

    // Time of Flight to Sample Flow Rate Conversion Factor
    unsigned int tof_sfr;
};

struct ConfigVars2 {
  int AMSamplingInterval;
  int AMIntervalCount;
  int AMFanOnIdle;
  int AMLaserOnIdle;
  int AMMaxDataArraysInFile;
  int AMOnlySavePMData;
};

void opc_release();
void opc_start();
void opc_stop();
void opc_init();    
struct histogram opc_read_histogram(uint8_t samples);
void opc_read_information_string();


#ifdef __cplusplus
}
#endif

#endif /* OPC_H */

