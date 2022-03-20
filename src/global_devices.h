/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.h to edit this template
 */

/* 
 * File:   global_devices.h
 * Author: mufufu
 *
 * Created on March 17, 2022, 10:15 AM
 */

#ifndef GLOBAL_DEVICES_H
#define GLOBAL_DEVICES_H

#ifdef __cplusplus
extern "C" {
#endif

/* * * * * * * * * * Global Devices  * * * * * * * * * * */

extern const struct device *gpiob;
extern const struct device *gpioa;


/* * * * * * * * * * LEDS  * * * * * * * * * * */

//#define LED0_NODE DT_ALIAS(led0)
//#define LED1_NODE DT_ALIAS(led1)
//#define LED2_NODE DT_ALIAS(led2)

//extern const struct gpio_dt_spec led_green;
//extern const struct gpio_dt_spec led_red;
//extern const struct gpio_dt_spec led_blue;


#ifdef __cplusplus
}
#endif

#endif /* GLOBAL_DEVICES_H */

