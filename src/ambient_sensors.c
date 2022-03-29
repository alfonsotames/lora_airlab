/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.c to edit this template
 */

/* * * * * * * * * * Sensors Connected to I2C2  * * * * * * * * * * */



#include "ambient_sensors.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(ambient_sensors);

const struct device *dev_sht3xd;
const struct device *dev_bme280;




const struct device *get_bme280_device(void) {
     const struct device *dev = device_get_binding("BME280");

    if (dev == NULL) {
        /* No such node, or the node does not have status "okay". */
        LOG_ERR("\nError: no device BME280 found.\n");
        return NULL;
    }

    if (!device_is_ready(dev)) {
        LOG_ERR("\nError: Device \"%s\" is not ready; "
                "check the driver initialization logs for errors.\n",
                dev->name);
        return NULL;
    }

    LOG_INF("Found device \"%s\", getting sensor data", dev->name);
    return dev;
}

const struct device *get_sht3xd_device(void) {
    const struct device *dev = device_get_binding("SHT3XD");

    if (dev == NULL) {
        /* No such node, or the node does not have status "okay". */
        LOG_ERR("\nError: no device SHT3XD found.\n");
        return NULL;
    }

    if (!device_is_ready(dev)) {
        LOG_ERR("\nError: Device \"%s\" is not ready; "
                "check the driver initialization logs for errors.\n",
                dev->name);
        return NULL;
    }

    LOG_INF("Found device \"%s\", getting sensor data", dev->name);
    return dev;
}


void ambient_sensors_read(uint8_t samples, float *ambientsensor) {
    
    struct sensor_value temp, hum, press;
    
    float sum[3];

    for(int i=0; i<samples; i++) {
        int rc=0;
        /*
        k_msleep(20);
        int rc = sensor_sample_fetch(dev_sht3xd);
        k_msleep(20);

        if (rc == 0) {
            rc = sensor_channel_get(dev_sht3xd, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        }
        if (rc == 0) {
            rc = sensor_channel_get(dev_sht3xd, SENSOR_CHAN_HUMIDITY, &hum);
        }
        if (rc != 0) {
            printf("SHT3XD: failed: %d\n", rc);
            return;
        }
         */
        k_msleep(20);
        rc = sensor_sample_fetch(dev_bme280);
        k_msleep(20);

        if (rc == 0) {
            rc = sensor_channel_get(dev_bme280, SENSOR_CHAN_PRESS, &press);
        }

        if (rc == 0) {
            rc = sensor_channel_get(dev_bme280, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        }
        if (rc == 0) {
            rc = sensor_channel_get(dev_bme280, SENSOR_CHAN_HUMIDITY, &hum);
        }        
        
        if (rc != 0) {
            printf("BME280: failed: %d\n", rc);
            return;
        }
        
        sum[0] = sum[0] + sensor_value_to_double(&hum);
        sum[1] = sum[1] + sensor_value_to_double(&temp);
        sum[2] = sum[2] + sensor_value_to_double(&press);

    }
    
    double div = samples;
    ambientsensor[0] = sum[0]/div;
    ambientsensor[1] = sum[1]/div;
    ambientsensor[2] = sum[2]/div;
    LOG_INF("Humidity:\t %0.2f",ambientsensor[0]);
    LOG_INF("Temperature:\t %0.2f",ambientsensor[1]);
    LOG_INF("Pressure:\t %0.2f",ambientsensor[2]);
    
}