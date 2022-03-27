/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.c to edit this template
 */


#include "gas_sensors.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(gas_sensors);


const struct device *i2c3_dev;




void gas_sensors_init() {
     
    /* * * * * * * * * * * * * i2c3 Device * * * * * * * * * * * * * */
    uint32_t i2c3_cfg = I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_MASTER;

    LOG_INF("Initializing I2C 3...");
    i2c3_dev = device_get_binding("I2C_3");
    if (!i2c3_dev) {
        LOG_INF("I2C3: Device driver not found.");
        return;
    }

    if (i2c_configure(i2c3_dev, i2c3_cfg)) {
        LOG_ERR("I2C3 config failed");
    } else {
        LOG_INF("i2c3 configured...");
    }

}




void gas_sensors_read(uint16_t *result, int samples) {

    //float voltageConv = 6114 / 32768.0;
    int ret;

    uint8_t cmd[3];
    uint8_t readbuf[2];
    uint16_t reading;
    uint8_t addr;

    /* * * * * * * * * * * Query Alphasensors * * * * * * * * * * * */

    for (int s=0; s < samples; s++) {
        
        addr = 0x48;
        for (int i = 0; i < 4; i++) {
            cmd[0] = 0x01;
            switch (i) {
                case(0):
                    cmd[1] = 0xC1;
                    break;
                case(1):
                    cmd[1] = 0xD1;
                    break;
                case(2):
                    cmd[1] = 0xE1;
                    break;
                case(3):
                    cmd[1] = 0xF1;
                    break;
            }

            cmd[2] = 0x83;
            ret = i2c_write(i2c3_dev, cmd, 3, addr);
            k_msleep(10);

            cmd[0] = 0x00;
            ret = i2c_write(i2c3_dev, cmd, 1, addr);
            k_msleep(10);

            ret = i2c_read(i2c3_dev, readbuf, 2, addr);
            k_msleep(20);

            reading = (readbuf[0] << 8 | readbuf[1]);

            //result[i] = result[i] + (reading * voltageConv);
            if (reading > 10000) {
                reading=0;
            }
            result[i] = reading;
        }
        k_msleep(10);

        addr = 0x49;
        for (int i = 0; i < 4; i++) {
            cmd[0] = 0x01;
            switch (i) {
                case(0):
                    cmd[1] = 0xC1;
                    break;
                case(1):
                    cmd[1] = 0xD1;
                    break;
                case(2):
                    cmd[1] = 0xE1;
                    break;
                case(3):
                    cmd[1] = 0xF1;
                    break;
            }

            cmd[2] = 0x83;
            ret = i2c_write(i2c3_dev, cmd, 3, addr);
            k_msleep(10);

            cmd[0] = 0x00;
            ret = i2c_write(i2c3_dev, cmd, 1, addr);
            k_msleep(10);

            ret = i2c_read(i2c3_dev, readbuf, 2, addr);
            k_msleep(20);

            reading = (readbuf[0] << 8 | readbuf[1]);

            //result[i + 4] = result[i + 4] + (reading * voltageConv);
            if (reading > 10000) {
                reading=0;
            }
            result[i + 4] = reading;

        }
        k_msleep(1000);
    }
    
    for (int i=0; i< 8; i++) {
        result[i] = result[i]/samples;
    }
    
            LOG_INF("Gas Sensors:");
            LOG_INF("SO2   we: %d   ae: %d", result[0], result[1]);
            LOG_INF("O3N2  we: %d   ae: %d", result[2], result[3]);
            LOG_INF("NO2   we: %d   ae: %d", result[4], result[5]);
            LOG_INF("CO    we: %d   ae: %d", result[6], result[7]); 

}