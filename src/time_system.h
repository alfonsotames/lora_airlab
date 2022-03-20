/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.h to edit this template
 */

/* 
 * File:   time_system.h
 * Author: mufufu
 *
 * Created on March 16, 2022, 7:06 PM
 */

#ifndef TIME_SYSTEM_H
#define TIME_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>
#include <device.h>
#include <drivers/rtc/maxim_ds3231.h>



// Time device DS3231 Precision Clock    
extern const struct device *dev_ds3231;

void init_time_system();
char* get_formatted_time();
char *format_time(time_t time, long nsec);
char *format_date(time_t time, long nsec);
char *get_date();


#ifdef __cplusplus
}
#endif

#endif /* TIME_SYSTEM_H */

