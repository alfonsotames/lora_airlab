/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.h to edit this template
 */

/* 
 * File:   lorawan.h
 * Author: mufufu
 *
 * Created on March 17, 2022, 1:12 PM
 */

#ifndef LORAWAN_H
#define LORAWAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lorawan/lorawan.h>
    
    
/* * * * * * * * * * LoRa * * * * * * * * * * */

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
        "No default LoRa radio specified in DT");
#define DEFAULT_RADIO DT_LABEL(DEFAULT_RADIO_NODE)

/* Customize based on network configuration */
#define LORAWAN_DEV_EUI   { 0x00, 0x80, 0xE1, 0x15, 0x05, 0x00, 0xDD, 0x1F }
#define LORAWAN_JOIN_EUI  { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 }
#define LORAWAN_APP_KEY   { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C }



static void dl_callback(uint8_t port, bool data_pending,
        int16_t rssi, int8_t snr,
        uint8_t len, const uint8_t *data);

static void lorwan_datarate_changed(enum lorawan_datarate dr);

#ifdef __cplusplus
}
#endif

#endif /* LORAWAN_H */

