/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.h to edit this template
 */

/* 
 * File:   ambient_sensors.h
 * Author: mufufu
 *
 * Created on March 20, 2022, 12:28 PM
 */

#ifndef AMBIENT_SENSORS_H
#define AMBIENT_SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif

    
#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>

    
/* * * * * * * * * * Sensors Connected to I2C2  * * * * * * * * * * */

extern const struct device *dev_sht3xd;
extern const struct device *dev_bme280;




const struct device *get_bme280_device(void);
const struct device *get_sht3xd_device(void);

const struct device *get_bme280_device(void);
const struct device *get_sht3xd_device(void);
void ambient_sensors_read(uint8_t samples, float *ambientsensor);


#ifdef __cplusplus
}
#endif

#endif /* AMBIENT_SENSORS_H */

