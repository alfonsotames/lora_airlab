/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.c to edit this template
 */


#include <logging/log.h>
LOG_MODULE_REGISTER(lora_system);

#include "lora_system.h"



static void dl_callback(uint8_t port, bool data_pending,
        int16_t rssi, int8_t snr,
        uint8_t len, const uint8_t *data) {
    LOG_INF("Downlink Callback: Port %d, Pending %d, RSSI %ddB, SNR %ddBm", port, data_pending, rssi, snr);
    if (data) {
        LOG_INF("IT CONTAINS DATA !!!!!!!!!!!!!!!");
        LOG_HEXDUMP_INF(data, len, "Payload: ");
    }
}

static void lorwan_datarate_changed(enum lorawan_datarate dr) {
    uint8_t unused, max_size;

    lorawan_get_payload_sizes(&unused, &max_size);
    LOG_INF("New Datarate: DR_%d, Max Payload %d", dr, max_size);
}

