/*
 * Class A LoRaWAN sample application
 *
 * Copyright (c) 2020 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/sensor.h>
#include <lorawan/lorawan.h>
#include <zephyr.h>
#include <drivers/i2c.h>
#include <drivers/display.h>
#include <drivers/rtc/maxim_ds3231.h>
#include <lvgl.h>
#include <storage/disk_access.h>
#include <fs/fs.h>
#include <ff.h>


#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
        "No default LoRa radio specified in DT");
#define DEFAULT_RADIO DT_LABEL(DEFAULT_RADIO_NODE)

/* Customize based on network configuration */
#define LORAWAN_DEV_EUI   { 0x00, 0x80, 0xE1, 0x15, 0x05, 0x00, 0xDD, 0x1F }
#define LORAWAN_JOIN_EUI  { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 }
#define LORAWAN_APP_KEY   { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C }


/* size of stack area used by each thread */
#define STACKSIZE 2048


#define DELAY K_MSEC(5000)

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(lora_airlab);

/* * * * * * * * * SD CARD * * * * * * * * * */
static int lsdir(const char *path);

static FATFS fat_fs;
/* mounting info */
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

/*
*  Note the fatfs library is able to mount only strings inside _VOLUME_STRS
*  in ffconf.h
*/
static const char *disk_mount_pt = "/SD:";

/* * * * * * * * * * LoRa * * * * * * * * * * */

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


/* * * * * * * * * GLOBAL VARIABLES * * * * * * * * * * * * */

const float voltageConv = 6114 / 32768.0;


const struct device *i2c3_dev;

const struct device *dev_sht3xd;
const struct device *dev_bme280;
const struct device *dev_ds3231;

float alphasensor[8];
float voltages[8];
float ambientsensor[3];

#define readings 60


float voltages_table[8][readings];
float ambientsensor_table[3][readings];

// 7 16bit floats
// 8 16bit bytes
/*
 * 
 *  * * * * * *           Data Format          * * * * * * * 
 * 
 *                     10 x 4 = 40 bytes
 *         |4 bytes|
 * |-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
 *    cmd     time   sens1   sens2   sens3   sens4   sens5   sens6   sens7   sens8
 * 
 * 
 * |-----------------------------40 bytes total------------------------------------|
 * 
*/ 
  
// (40)
char results_data[8 * sizeof (float) + 8];
char voltages_data[8 * sizeof (float) + 8];
char arr[4];

struct sensor_value temp, hum, press;


lv_obj_t *title_label;
lv_obj_t *table;

/*
lv_obj_t *humidity_label;
lv_obj_t *temperat_label;
lv_obj_t *press_label;
lv_obj_t *no2_label;
lv_obj_t *so2_label;
lv_obj_t *o3_label;
*/


// Clock Functions and Data Types


static const char *format_time(time_t time, long nsec) {
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

static const char *format_date(time_t time, long nsec) {
    static char buf[64];
    char *bp = buf;
    char *const bpe = bp + sizeof (buf);
    struct tm tv;
    struct tm *tp = gmtime_r(&time, &tv);
    bp += strftime(bp, bpe - bp, "%Y%m%d", tp);
    return buf;
}

static const char *get_date() {
    // get time
    uint32_t syncclock_Hz = maxim_ds3231_syncclock_frequency(dev_ds3231);
    uint32_t syncclock = maxim_ds3231_read_syncclock(dev_ds3231);
    uint32_t now = 0;
    int rc = counter_get_value(dev_ds3231, &now);

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

/* * * * * * * * Save data to SD CARD * * * * * * * * * */
static void save_data(char* data) {
    char fname[50];
    snprintfcb(fname,50, "/SD:/%s.lab", get_date());
    
    struct fs_file_t file;

    fs_file_t_init(&file);

    int rc = fs_open(&file, fname, FS_O_CREATE | FS_O_APPEND | FS_O_RDWR);
    if (rc < 0) {
            printk("FAIL: open %s: %d\n", fname, rc);
    }
    //rc = fs_write(&file, data, sizeof(data));
    rc = fs_write(&file, data, 40);
    if (rc < 0) {
            printk("FAIL: cannot write to %s: %d\n", fname, rc);
    }
    
    printk(" **** fs_write wrote %d bytes **** \n",rc);
    
    fs_close(&file);
}


void convertToBytes(char* array, float f) {
    //printk("Converting: %x\n",f);
    unsigned char *b = (unsigned char *) &f;
    for (int x = 0; x < 4; x++) {
        //printk("%x:", b[x]);
        array[x] = b[x];
    }
}



void query_ambient_sensors() {


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

    k_msleep(20);
    rc = sensor_sample_fetch(dev_bme280);
    k_msleep(20);

    if (rc == 0) {
        rc = sensor_channel_get(dev_bme280, SENSOR_CHAN_PRESS, &press);
    }
    if (rc != 0) {
        printf("BME280: failed: %d\n", rc);
        return;
    }

    ambientsensor[0] = sensor_value_to_double(&hum);
    ambientsensor[1] = sensor_value_to_double(&temp);
    ambientsensor[2] = sensor_value_to_double(&press);

}

void query_alphasensors() {

    int ret;

    uint8_t cmd[3];
    uint8_t readbuf[2];
    uint16_t reading;
    uint8_t addr;



    /* * * * * * * * * * * Query Alphasensors * * * * * * * * * * * */

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
        //printk("0,1: %x,%x   :   ",readbuf[0], readbuf[1]);

        alphasensor[i] = reading * voltageConv;

        //printk("Reading Alphasensor[%d]: %.2f\n", i, alphasensor[i]);
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
        //printk("0,1: %x,%x   :   ",readbuf[0], readbuf[1]);

        alphasensor[i + 4] = reading * voltageConv;

        //printk("Reading Alphasensor[%d]: %.2f\n", i + 4, alphasensor[i + 4]);
    }



}




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
static const struct device *get_bme280_device(void) {
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

static const struct device *get_sht3xd_device(void) {
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



K_THREAD_STACK_DEFINE(take_a_reading_stack_area, STACKSIZE);
struct k_thread take_a_reading_thread_data;
extern void take_a_reading() {
    LOG_INF("Sensor Reading Thread Spawn!");
    char humidity_str[40] = {0};
    char temperat_str[40] = {0};
    char press_str[40] = {0};
    char no2_str[40] = {0};
    char so2_str[40] = {0};
    char o3_str[40] = {0};
    char co_str[40] = {0};

    int c = 0;

    while (1) {
        //printk("Reading sensors...%d\n", c);
        //k_msleep(200);

        query_alphasensors();
        query_ambient_sensors();
        
        // guarda los resultados
        /*
        float no2 = ((alphasensor[0])-(alphasensor[1])) / .165;
        float so2 = ((alphasensor[2])-(alphasensor[3])) / .240;
        float o3no2 = ((alphasensor[4])-(alphasensor[5])) / 216;
        float co = ((alphasensor[6])-(alphasensor[7])) / .165;
        */
        float no2 = ((alphasensor[0])-(alphasensor[1])) / 5;
        if (no2 < 0) no2=0;
        float so2 = ((alphasensor[2])-(alphasensor[3])) / 100;
        if (so2 < 0) so2=0;
        float o3no2 = ((alphasensor[4])-(alphasensor[5])) / 100;
        if (o3no2 < 0) o3no2=0;
        float co = ((alphasensor[6])-(alphasensor[7])) / 100;
        if (co < 0) co=0;
        
        float o3 = o3no2-no2;
        if (o3 < 0) o3=0;
        
        float f;
        
        f = co;
        snprintfcb(co_str, 40, "%0.2f", f);
        lv_table_set_cell_value(table, 6, 1, co_str);       

        f = no2;
        snprintfcb(no2_str, 40, "%0.2f", f);
        lv_table_set_cell_value(table, 3, 1, no2_str);


        f = so2;
        snprintfcb(so2_str, 40, "%0.2f", f);
        lv_table_set_cell_value(table, 4, 1, so2_str);

        f = o3;
        snprintfcb(o3_str, 40, "%0.2f", f);
        lv_table_set_cell_value(table, 5, 1, o3_str);
        
        snprintfcb(temperat_str, 40, "%0.2f", ambientsensor[1]);
        lv_table_set_cell_value(table, 0, 1, temperat_str);
        
        snprintfcb(humidity_str, 40, "%0.2f", ambientsensor[0]);
        lv_table_set_cell_value(table, 1, 1, humidity_str);

        snprintfcb(press_str, 40, "%0.2f", ambientsensor[2]);
        lv_table_set_cell_value(table, 2, 1, press_str);           
        
        

        for (int n = 0; n < 8; n++) {
            voltages_table[n][c] = alphasensor[n]; // ojo aqui
        }
        for (int n = 0; n < 3; n++) {
            ambientsensor_table[n][c] = ambientsensor[n];
        }


 
        c++;
        if (c == readings) {
            //printk("Doing calculations...\n");
            c = 0;

            for (int r = 0; r < readings - 1; r++) {
                for (int i = 0; i < 8; i++) {
                    alphasensor[i] = alphasensor[i] + voltages_table[i][r];
                }
                for (int i = 0; i < 3; i++) {
                    ambientsensor[i] = ambientsensor[i] + ambientsensor_table[i][r];
                }
            }

            // saca promedios
            for (int i = 0; i < 8; i++) {
                alphasensor[i] = (alphasensor[i] / readings);
            }
            for (int i = 0; i < 3; i++) {
                ambientsensor[i] = (ambientsensor[i] / readings);
            }
            char volter[40];
            for (int i=0; i<8; i++) {
                snprintfcb(volter, 40, "%0.2f", alphasensor[i]);
                printk("alphasensor[%d]: %s\n",i,volter);
            }

            // guarda los resultados
            
        float no2 = ((alphasensor[0])-(alphasensor[1])) / 5;
        if (no2 < 0) no2=0;
        float so2 = ((alphasensor[2])-(alphasensor[3])) / 100;
        if (so2 < 0) so2=0;
        float o3no2 = ((alphasensor[4])-(alphasensor[5])) / 100;
        if (o3no2 < 0) o3no2=0;
        float co = ((alphasensor[6])-(alphasensor[7])) / 100;
        if (co < 0) co=0;
        
        float o3 = o3no2-no2;
               if (o3 < 0) o3=0;

            float f;

            f = co;
            snprintfcb(co_str, 40, "%0.2f", f);
            lv_table_set_cell_value(table, 6, 2, co_str);
            
            f = no2;
            snprintfcb(no2_str, 40, "%0.2f", f);
            lv_table_set_cell_value(table, 3, 2, no2_str);
            

            f = so2;
            snprintfcb(so2_str, 40, "%0.2f", f);
            lv_table_set_cell_value(table, 4, 2, so2_str);

            f = o3;
            snprintfcb(o3_str, 40, "%0.2f", f);
            lv_table_set_cell_value(table, 5, 2, o3_str);

            snprintfcb(humidity_str, 40, "%0.2f", ambientsensor[0]);
            lv_table_set_cell_value(table, 1, 2, humidity_str);

            snprintfcb(temperat_str, 40, "%0.2f", ambientsensor[1]);
            lv_table_set_cell_value(table, 0, 2, temperat_str);

            snprintfcb(press_str, 40, "%0.2f", ambientsensor[2]);
            lv_table_set_cell_value(table, 2, 2, press_str);
            
            
            /* GET TIME */

            uint32_t syncclock_Hz = maxim_ds3231_syncclock_frequency(dev_ds3231);
            uint32_t syncclock = maxim_ds3231_read_syncclock(dev_ds3231);
            uint32_t now = 0;
            int rc = counter_get_value(dev_ds3231, &now);

            struct maxim_ds3231_syncpoint sp = {
                .rtc =
                {
                    .tv_sec = now,
                    .tv_nsec = (uint64_t) NSEC_PER_SEC * syncclock / syncclock_Hz,
                },
                .syncclock = syncclock,
            };
            

            // Result processing...
            
            char array[4] = {0}; // temp array storage
            
            // results_data
            
            results_data[0] = 0x00; // results_data command
            
            convertToBytes(array,now);
            memcpy(results_data+4,array,4);
            
            convertToBytes(array,no2);
            memcpy(results_data+8,array,4);
            
            convertToBytes(array,so2);
            memcpy(results_data+12,array,4);
            
            convertToBytes(array,o3);
            memcpy(results_data+16,array,4);
            
            convertToBytes(array,co);
            memcpy(results_data+20,array,4);
            
            convertToBytes(array,ambientsensor[0]);
            memcpy(results_data+24,array,4);
            
            convertToBytes(array,ambientsensor[1]);
            memcpy(results_data+28,array,4);
            
            convertToBytes(array,ambientsensor[2]);
            memcpy(results_data+32,array,4);
            
            /*
            // imprime voltajes
            char mv[40] = {0};
            for (int i = 0; i < 8; i++) {
                snprintfcb(mv, 40, "%0.2f", alphasensor[i]);
                printk("alphasensor[%d]: %s\n", i, mv);
            }
            */
            
            // pure voltage_data in mv
            
            voltages_data[0] = 0x01; // voltages_data command
            
            convertToBytes(array,now);
            memcpy(voltages_data+4,array,4);
            
            int offset=8;
            for (int i=0; i < 8; i++) {
                convertToBytes(array,alphasensor[i]);
                memcpy(voltages_data+offset,array,4);
                offset=offset+4;
            }

            
            printk("%s at %u ms past: %d\n", format_time(sp.rtc.tv_sec, sp.rtc.tv_nsec), syncclock, rc);
            
            printk("\n*-*-*- Sensor Reading Interpreted Data *-*-*-*-\n");
            for (int i = 0; i < sizeof (results_data); i++) {
                printk("%x:", results_data[i]);
            }
            
            printk("\n*-*-*-*-*-*-*-*-*-*-*\n");
            printk("\n*-*-*-     Raw sensor Voltage Data     *-*-*-*-\n");
            for (int i = 0; i < sizeof (voltages_data); i++) {
                printk("%x:", voltages_data[i]);
            }
            printk("\n*-*-*-*-*-*-*-*-*-*-*\n");            
            save_data(results_data);
            save_data(voltages_data);

        }
    }
}

K_THREAD_STACK_DEFINE(update_results_stack_area, STACKSIZE);
struct k_thread update_results_thread_data;
void update_results() {
    LOG_INF("LoRaWAN Update Results Thread Spawn!");
    int ret;
    /* * * * * * * * * * * * * LORA * * * * * * * * * * * * * */

    const struct device *lora_dev;
    struct lorawan_join_config join_cfg;
    uint8_t dev_eui[] = LORAWAN_DEV_EUI;
    uint8_t join_eui[] = LORAWAN_JOIN_EUI;
    uint8_t app_key[] = LORAWAN_APP_KEY;


    struct lorawan_downlink_cb downlink_cb = {
        .port = LW_RECV_PORT_ANY,
        .cb = dl_callback
    };

    lora_dev = device_get_binding(DEFAULT_RADIO);
    if (!lora_dev) {
        LOG_ERR("%s Device not found", DEFAULT_RADIO);
        return;
    } else {
        LOG_INF("LoRaWAN Device Found...");
    }


    ret = lorawan_start();
    if (ret < 0) {
        LOG_ERR("lorawan_start failed: %d", ret);
        return;
    } else {
        LOG_INF("LoRaWAN Started...");
    }

    lorawan_register_downlink_callback(&downlink_cb);
    lorawan_register_dr_changed_callback(lorwan_datarate_changed);

    join_cfg.mode = LORAWAN_ACT_OTAA;
    join_cfg.dev_eui = dev_eui;
    join_cfg.otaa.join_eui = join_eui;
    join_cfg.otaa.app_key = app_key;
    join_cfg.otaa.nwk_key = app_key;

    LOG_INF("Joining network over OTAA");
    ret = -1;
    while (ret < 0) {
        ret = lorawan_join(&join_cfg);
        if (ret < 0) {
            LOG_ERR("lorawan_join_network failed: %d", ret);
            k_sleep(K_MSEC(5000));
        } else {
            LOG_INF("Connected to network over OTAA!");
        }
    }

    LOG_INF("Entering Loop...");
    
    while(1) {
        
        ret = lorawan_send(1, results_data, sizeof (results_data), LORAWAN_MSG_CONFIRMED);

        /*
         * Note: The stack may return -EAGAIN if the provided data
         * length exceeds the maximum possible one for the region and
         * datarate. But since we are just sending the same data here,
         * we'll just continue.
         */
        if (ret == -EAGAIN) {
            LOG_ERR("lorawan_send failed: %d. Continuing...", ret);
            k_sleep(DELAY);
        }

        if (ret < 0) {
            LOG_ERR("lorawan_send failed: %d", ret);
            //return;
            k_sleep(K_MSEC(5000));
        }
        if (ret == 0) {
            LOG_INF("Sensor Data sent!");
        }

        k_sleep(K_MSEC(60000));


        ret = lorawan_send(2, voltages_data, sizeof (voltages_data), LORAWAN_MSG_CONFIRMED);

        /*
         * Note: The stack may return -EAGAIN if the provided data
         * length exceeds the maximum possible one for the region and
         * datarate. But since we are just sending the same data here,
         * we'll just continue.
         */
        if (ret == -EAGAIN) {
            LOG_ERR("lorawan_send failed: %d. Continuing...", ret);
            k_sleep(DELAY);

        }

        if (ret < 0) {
            LOG_ERR("lorawan_send failed: %d", ret);
            //return;
            k_sleep(K_MSEC(60000));
        }

        if (ret == 0) {
            LOG_INF("Voltage Data sent!");
        }
    }
    k_sleep(K_MSEC(60000));

}


static int lsdir(const char *path)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	fs_dir_t_init(&dirp);

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res) {
		printk("Error opening dir %s [%d]\n", path, res);
		return res;
	}

	printk("\nListing dir %s ...\n", path);
	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			printk("[DIR ] %s\n", entry.name);
		} else {
			printk("[FILE] %s (size = %zu)\n",
				entry.name, entry.size);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return res;
}



void main(void) {

    
    results_data[0] = 0;
    voltages_data[0] = 0;


    dev_sht3xd = get_sht3xd_device();
    if (dev_sht3xd == NULL) {
        return;
    }

    dev_bme280 = get_bme280_device();
    if (dev_bme280 == NULL) {
        return;
    }
    
    dev_ds3231 = get_ds3231_device();
    if (dev_ds3231 == NULL) {
        return;
    }
    
    /* * * * * * * FILESYSTEM INIT FOR SD CARD * * * * * * * * * * */
    
    do {
            static const char *disk_pdrv = "SD";
            uint64_t memory_size_mb;
            uint32_t block_count;
            uint32_t block_size;

            if (disk_access_init(disk_pdrv) != 0) {
                    LOG_ERR("Storage init ERROR!");
                    break;
            }

            if (disk_access_ioctl(disk_pdrv,
                            DISK_IOCTL_GET_SECTOR_COUNT, &block_count)) {
                    LOG_ERR("Unable to get sector count");
                    break;
            }
            LOG_INF("Block count %u", block_count);

            if (disk_access_ioctl(disk_pdrv,
                            DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
                    LOG_ERR("Unable to get sector size");
                    break;
            }
            printk("Sector size %u\n", block_size);

            memory_size_mb = (uint64_t)block_count * block_size;
            printk("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));
    } while (0);
    
    
    /* * * * * * * Mount Filesystem * * * * * * * * */
    
    mp.mnt_point = disk_mount_pt;
    int res = fs_mount(&mp);

    if (res == FR_OK) {
            printk("Disk mounted.\n");
            lsdir(disk_mount_pt);
    } else {
            printk("Error mounting disk.\n");
    }
 
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
        LOG_ERR("i2c3 configured...");
    }

    k_sleep(K_MSEC(3000));

    /* * * * * * * * * * * * * Display * * * * * * * * * * * * * */

    const struct device *display_dev;
    display_dev = device_get_binding(CONFIG_LVGL_DISPLAY_DEV_NAME);
    LOG_INF(CONFIG_LVGL_DISPLAY_DEV_NAME);
    if (display_dev == NULL) {
        LOG_ERR("device not found.  Aborting test.");
        return;
    }

    static lv_style_t screenStyle1;
    static lv_style_t style1;
    static lv_style_t chiqui;

    lv_style_init(&screenStyle1);
    lv_style_init(&style1);
    lv_style_init(&chiqui);

    lv_style_set_bg_color(&screenStyle1, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_text_font(&style1, LV_STATE_DEFAULT, &lv_font_montserrat_24);
    lv_style_set_text_color(&style1, LV_STATE_DEFAULT, LV_COLOR_WHITE);

    

    lv_obj_t *screen1;
    screen1 = lv_obj_create(NULL, NULL);
    lv_obj_add_style(screen1, LV_LABEL_PART_MAIN, &screenStyle1);

    title_label = lv_label_create(screen1, NULL);
    lv_obj_add_style(title_label, LV_OBJ_PART_MAIN, &style1);

    lv_label_set_text(title_label, "AirLab");
    lv_obj_align(title_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);    
    
    /*Create a Table */
    
    table = lv_table_create(screen1, NULL);
        
    lv_table_set_col_cnt(table, 3);
    lv_table_set_row_cnt(table, 7);
    
    static lv_style_t table_style;
    static lv_style_t style2;
    
    lv_style_init(&table_style);
    lv_style_init(&style2);

    //lv_style_set_bg_opa(&style2, LV_STATE_DEFAULT, LV_OPA_COVER);
    
    lv_style_set_pad_top(&table_style, LV_TABLE_PART_BG, 0);
    lv_style_set_pad_bottom(&table_style, LV_TABLE_PART_BG, 0);
    lv_style_set_text_line_space(&table_style, LV_TABLE_PART_BG, 0);
    lv_style_set_border_width(&table_style, LV_TABLE_PART_BG, 0);
    lv_style_set_pad_left(&table_style, LV_TABLE_PART_BG, 0);
    lv_style_set_pad_right(&table_style, LV_TABLE_PART_BG, 0);
    lv_style_set_pad_inner(&table_style, LV_TABLE_PART_BG, 0);
    lv_style_set_text_letter_space(&table_style, LV_TABLE_PART_BG, 0);
    
    //lv_style_set_text_color(&table_style, LV_TABLE_PART_BG, LV_COLOR_RED);
    
    lv_style_set_text_color(&table_style, LV_TABLE_PART_BG, LV_COLOR_WHITE);
    lv_style_set_text_color(&table_style, LV_TABLE_PART_CELL1, LV_COLOR_BLUE);
    
    lv_style_set_text_color(&style2, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_pad_top(&style2, LV_STATE_DEFAULT, 0);
    lv_style_set_pad_bottom(&style2, LV_STATE_DEFAULT, 0);
    lv_style_set_text_line_space(&style2, LV_STATE_DEFAULT, 0);
    lv_style_set_border_width(&style2, LV_STATE_DEFAULT, 1);
    lv_style_set_border_color(&style2, LV_STATE_DEFAULT, LV_COLOR_RED);
    lv_style_set_pad_left(&style2, LV_STATE_DEFAULT, 0);
    lv_style_set_pad_right(&style2, LV_STATE_DEFAULT, 0);
    lv_style_set_pad_inner(&style2, LV_STATE_DEFAULT, 0);
    lv_style_set_text_letter_space(&style2, LV_TABLE_PART_BG, 0);
    lv_style_set_text_font(&style2, LV_STATE_DEFAULT, &lv_font_montserrat_20);
    
    lv_obj_add_style(table, LV_TABLE_PART_CELL2, &style2);
    
    lv_style_set_text_color(&table_style, LV_TABLE_PART_CELL3, LV_COLOR_YELLOW);
    lv_style_set_text_color(&table_style, LV_TABLE_PART_CELL4, LV_COLOR_CYAN);
    
    lv_style_set_bg_color(&table_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    
    lv_obj_add_style(table, LV_STATE_DEFAULT, &table_style);

    lv_style_copy(&chiqui, &style2);
    lv_style_set_text_font(&chiqui, LV_STATE_DEFAULT, &lv_font_montserrat_12);
    lv_obj_add_style(table, LV_TABLE_PART_CELL3, &chiqui);

    lv_table_set_cell_type(table, 0, 0, 3);
    lv_table_set_cell_type(table, 0, 1, 2);
    lv_table_set_cell_type(table, 0, 2, 2);
    lv_table_set_cell_type(table, 1, 0, 3);
    lv_table_set_cell_type(table, 1, 1, 2);
    lv_table_set_cell_type(table, 1, 2, 2);
    lv_table_set_cell_type(table, 2, 0, 3);
    lv_table_set_cell_type(table, 2, 1, 2);
    lv_table_set_cell_type(table, 2, 2, 2);
    lv_table_set_cell_type(table, 3, 0, 3);
    lv_table_set_cell_type(table, 3, 1, 2);
    lv_table_set_cell_type(table, 3, 2, 2);
    lv_table_set_cell_type(table, 4, 0, 3);
    lv_table_set_cell_type(table, 4, 1, 2);
    lv_table_set_cell_type(table, 4, 2, 2);
    lv_table_set_cell_type(table, 5, 0, 3);
    lv_table_set_cell_type(table, 5, 1, 2);
    lv_table_set_cell_type(table, 5, 2, 2);    
    lv_table_set_cell_type(table, 6, 0, 3);
    lv_table_set_cell_type(table, 6, 1, 2);
    lv_table_set_cell_type(table, 6, 2, 2);       


    lv_table_set_col_width(table, 0,60);
    lv_table_set_col_width(table, 1,80);
    lv_table_set_col_width(table, 2,80);

    lv_table_set_cell_value(table, 0, 0, "TEMP");
    lv_table_set_cell_value(table, 1, 0, "HR");
    lv_table_set_cell_value(table, 2, 0, "P");
    lv_table_set_cell_value(table, 3, 0, "NO2");
    lv_table_set_cell_value(table, 4, 0, "SO2");
    lv_table_set_cell_value(table, 5, 0, "O3");
    lv_table_set_cell_value(table, 6, 0, "CO");
    

    lv_table_ext_t * ext = lv_obj_get_ext_attr(table);
    ext->row_h[0] = 10;
    
    
    lv_scr_load(screen1);
    lv_task_handler();
    display_blanking_off(display_dev);
    
    
    k_thread_create(&take_a_reading_thread_data, take_a_reading_stack_area,
                                 K_THREAD_STACK_SIZEOF(take_a_reading_stack_area),
                                 take_a_reading,
                                 NULL, NULL, NULL,
                                 5, 0, K_NO_WAIT);

    

    k_thread_create(&update_results_thread_data, update_results_stack_area,
                                 K_THREAD_STACK_SIZEOF(update_results_stack_area),
                                 update_results,
                                 NULL, NULL, NULL,
                                 5, 0, K_NO_WAIT);
    while (1) {
        lv_task_handler();
        k_msleep(5);
    }

}



