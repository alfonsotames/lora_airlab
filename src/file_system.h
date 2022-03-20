/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.h to edit this template
 */

/* 
 * File:   files.h
 * Author: mufufu
 *
 * Created on March 16, 2022, 6:55 PM
 */

#ifndef FILES_H
#define FILES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>
#include <device.h>
#include <storage/disk_access.h>
#include <fs/fs.h>
#include <ff.h>
#include <string.h>
    


    

/* * * * * * * * * SD CARD Functions * * * * * * * * * */

 void init_and_mount_sdcard();
 //void save_data(char* data, char* type);
 void save_data();
 int lsdir(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* FILES_H */

