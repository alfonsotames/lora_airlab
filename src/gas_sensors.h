/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.h to edit this template
 */

/* 
 * File:   gas_sensors.h
 * Author: mufufu
 *
 * Created on March 20, 2022, 1:19 PM
 */

#ifndef GAS_SENSORS_H
#define GAS_SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>
#include <zephyr.h>
#include <drivers/i2c.h>
    
    
/* * * * * * * * I2C3 (ADS Converters to GAS Sensors  * * * * * * * */

extern const struct device *i2c3_dev;


void gas_sensors_init();
void gas_sensors_read(uint16_t *result, int samples);

#ifdef __cplusplus
}
#endif

#endif /* GAS_SENSORS_H */

