/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.c to edit this template
 */

// Clock Functions and Data Types



#include <logging/log.h>
LOG_MODULE_REGISTER(time_system);

#include "time_system.h"


const struct device *i2c2_dev;

const struct device *dev_ds3231;



/* * * * * * * * * Time Functions * * * * * * * * * */


static const struct device *get_ds3231_device(void) {
    const struct device *dev = device_get_binding("DS3231");
	
    if (dev == NULL) {
        /* No such node, or the node does not have status "okay". */
        LOG_ERR("\nError: no device DS3231 found.\n");
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

void init_time_system() {
    
    dev_ds3231 = get_ds3231_device();
    if (dev_ds3231 == NULL) {
        return;
    }

    uint32_t i2c2_cfg = I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_MASTER;

    LOG_INF("Initializing I2C 2...");
    i2c2_dev = device_get_binding("I2C_2");
    if (!i2c2_dev) {
        LOG_INF("I2C2: Device driver not found.");
        return;
    }

    if (i2c_configure(i2c2_dev, i2c2_cfg)) {
        LOG_ERR("I2C2 config failed");
    } else {
        LOG_INF("i2c2 configured...");
    }    
    
}

/* GET FORMATTED TIME */

char* get_formatted_time() {
          
    uint32_t syncclock_Hz = maxim_ds3231_syncclock_frequency(dev_ds3231);
    uint32_t syncclock = maxim_ds3231_read_syncclock(dev_ds3231);
    uint32_t now = 0;
    int rc = counter_get_value(dev_ds3231, &now);
    if (rc < 0) {
        LOG_ERR("Can't get the counter value");
    } else {
        //LOG_INF("Counter returns: %d",now);
    }
    struct maxim_ds3231_syncpoint sp = {
        .rtc =
        {
            .tv_sec = now,
            .tv_nsec = (uint64_t) NSEC_PER_SEC * syncclock / syncclock_Hz,
        },
        .syncclock = syncclock,
    };
    //LOG_INF("Counter returns: %d, %d, %d",sp.rtc.tv_sec, sp.rtc.tv_nsec, syncclock);
    return format_time(sp.rtc.tv_sec, sp.rtc.tv_nsec);
}
char *format_time(time_t time, long nsec) {
    static char buf[64];
    char *bp = buf;
    char *const bpe = bp + sizeof (buf);
    struct tm tv;
    struct tm *tp = gmtime_r(&time, &tv);

    bp += strftime(bp, bpe - bp, "%Y-%m-%d %H:%M:%S", tp);
    /*
    if (nsec >= 0) {
        bp += snprintf(bp, bpe - bp, ".%09lu", nsec);
    }
    bp += strftime(bp, bpe - bp, " %a %j", tp);
    */
    return buf;
}

char *format_date(time_t time, long nsec) {
    static char buf[64];
    char *bp = buf;
    char *const bpe = bp + sizeof (buf);
    struct tm tv;
    struct tm *tp = gmtime_r(&time, &tv);
    bp += strftime(bp, bpe - bp, "%Y%m%d", tp);
    return buf;
}

char *get_date() {
    // get time
    uint32_t syncclock_Hz = maxim_ds3231_syncclock_frequency(dev_ds3231);
    uint32_t syncclock = maxim_ds3231_read_syncclock(dev_ds3231);
    uint32_t now = 0;
    int rc = counter_get_value(dev_ds3231, &now);
    if (rc < 0) {
        LOG_ERR("Can't get the counter value");
    }
    struct maxim_ds3231_syncpoint sp = {
        .rtc =
        {
            .tv_sec = now,
            .tv_nsec = (uint64_t) NSEC_PER_SEC * syncclock / syncclock_Hz,
        },
        .syncclock = syncclock,
    };
    return format_date(sp.rtc.tv_sec, sp.rtc.tv_nsec);
}