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



#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
        "No default LoRa radio specified in DT");
#define DEFAULT_RADIO DT_LABEL(DEFAULT_RADIO_NODE)

/* Customize based on network configuration */
#define LORAWAN_DEV_EUI   { 0x00, 0x80, 0xE1, 0x15, 0x05, 0x00, 0xDD, 0x1F }
#define LORAWAN_JOIN_EUI  { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 }
#define LORAWAN_APP_KEY   { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C }


/* size of stack area used by each thread */
#define STACKSIZE 1024


#define DELAY K_MSEC(5000)

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(lora_airlab);

char data[] = {'m', 'u', 'f', 'u', 'f', 'u'};

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

// Clock Functions and Data Types

static const char *format_time(time_t time,
        long nsec) {
    static char buf[64];
    char *bp = buf;
    char *const bpe = bp + sizeof (buf);
    struct tm tv;
    struct tm *tp = gmtime_r(&time, &tv);

    bp += strftime(bp, bpe - bp, "%Y-%m-%d %H:%M:%S", tp);
    if (nsec >= 0) {
        bp += snprintf(bp, bpe - bp, ".%09lu", nsec);
    }
    bp += strftime(bp, bpe - bp, " %a %j", tp);
    return buf;
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
char results_data[7 * sizeof (float) + 4];
char voltages_data[8 * sizeof (float) + 4];





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

void convertToBytes(char* array, float f) {
    //printk("Converting: %x\n",f);
    unsigned char *b = (unsigned char *) &f;
    for (int x = 0; x < 4; x++) {
        //printk("%x:", b[x]);
        array[x] = b[x];
    }
}



void query_ambient_sensors() {


    k_msleep(40);
    int rc = sensor_sample_fetch(dev_sht3xd);
    k_msleep(40);

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

    k_msleep(40);
    rc = sensor_sample_fetch(dev_bme280);
    k_msleep(40);

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
        k_msleep(20);

        cmd[0] = 0x00;
        ret = i2c_write(i2c3_dev, cmd, 1, addr);
        k_msleep(20);

        ret = i2c_read(i2c3_dev, readbuf, 2, addr);
        k_msleep(50);

        reading = (readbuf[0] << 8 | readbuf[1]);
        //printk("0,1: %x,%x   :   ",readbuf[0], readbuf[1]);

        alphasensor[i] = reading * voltageConv;

        //printk("Reading Alphasensor[%d]: %.2f\n", i, alphasensor[i]);
    }
    k_msleep(50);

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
        k_msleep(20);

        cmd[0] = 0x00;
        ret = i2c_write(i2c3_dev, cmd, 1, addr);
        k_msleep(20);

        ret = i2c_read(i2c3_dev, readbuf, 2, addr);
        k_msleep(50);

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
        printk("\nError: no device DS3231 found.\n");
        return NULL;
    }
    
    if (!device_is_ready(dev)) {
        printk("\nError: Device \"%s\" is not ready; "
                "check the driver initialization logs for errors.\n",
                dev->name);
        return NULL;
    }

    printk("Found device \"%s\", getting sensor data\n", dev->name);
    return dev;
}        
static const struct device *get_bme280_device(void) {
    const struct device *dev = device_get_binding("BME280");

    if (dev == NULL) {
        /* No such node, or the node does not have status "okay". */
        printk("\nError: no device BME280 found.\n");
        return NULL;
    }

    if (!device_is_ready(dev)) {
        printk("\nError: Device \"%s\" is not ready; "
                "check the driver initialization logs for errors.\n",
                dev->name);
        return NULL;
    }

    printk("Found device \"%s\", getting sensor data\n", dev->name);
    return dev;
}

static const struct device *get_sht3xd_device(void) {
    const struct device *dev = device_get_binding("SHT3XD");

    if (dev == NULL) {
        /* No such node, or the node does not have status "okay". */
        printk("\nError: no device SHT3XD found.\n");
        return NULL;
    }

    if (!device_is_ready(dev)) {
        printk("\nError: Device \"%s\" is not ready; "
                "check the driver initialization logs for errors.\n",
                dev->name);
        return NULL;
    }

    printk("Found device \"%s\", getting sensor data\n", dev->name);
    return dev;
}

K_THREAD_STACK_DEFINE(my_stack_area, STACKSIZE);
struct k_thread my_thread_data;



extern void take_a_reading() {

    char humidity_str[40] = {0};
    char temperat_str[40] = {0};
    char press_str[40] = {0};
    char no2_str[40] = {0};
    char so2_str[40] = {0};
    char o3_str[40] = {0};

    int c = 0;

    while (1) {
        printk("Reading sensors...%d\n", c);
        k_msleep(200);

        query_alphasensors();
        query_ambient_sensors();
        
        
        
        snprintfcb(temperat_str, 40, "%0.2f", ambientsensor[1]);
        lv_table_set_cell_value(table, 1, 1, temperat_str);
        
        snprintfcb(humidity_str, 40, "%0.2f", ambientsensor[0]);
        lv_table_set_cell_value(table, 2, 1, humidity_str);        
        
        

        for (int n = 0; n < 8; n++) {
            voltages_table[n][c] = voltages[n];
        }
        for (int n = 0; n < 3; n++) {
            ambientsensor_table[n][c] = ambientsensor[n];
        }
        c++;
        if (c == readings) {
            printk("Doing calculations...\n");
            c = 0;
            
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
            printk("%s at %u ms past: %d\n", format_time(sp.rtc.tv_sec, sp.rtc.tv_nsec), syncclock, rc);


            
            
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

            // guarda los resultados
            float no2 = ((alphasensor[0] - 230)-(alphasensor[1] - 230)) / 165;
            float so2 = ((alphasensor[3] - 335)-(alphasensor[2] - 340)) / 240;
            float o3no2 = ((alphasensor[5] - 240)-(alphasensor[4] - 230)) / 216;

            float o3 = o3no2 - no2;

            float f;

            f = no2;
            snprintfcb(no2_str, 40, "NO2: %0.2f", f);
            //lv_label_set_text(no2_label, no2_str);
            

            f = so2;
            snprintfcb(so2_str, 40, "SO2: %0.2f", f);
            //lv_label_set_text(so2_label, so2_str);

            f = o3;
            snprintfcb(o3_str, 40, "O3: %0.2f", (f));
            //lv_label_set_text(o3_label, o3_str);

            snprintfcb(humidity_str, 40, "H(%%): %0.2f", sensor_value_to_double(&hum));
            //lv_label_set_text(humidity_label, humidity_str);
            //lv_table_set_cell_value(table, 1, 2, humidity_str);

            snprintfcb(temperat_str, 40, "%0.2f", ambientsensor[1]);
            //lv_label_set_text(temperat_label, temperat_str);
            
            
            lv_table_set_cell_value(table, 1, 2, temperat_str);

            snprintfcb(press_str, 40, "P(Kpa): %0.2f", sensor_value_to_double(&press));
            //lv_label_set_text(press_label, press_str);
            //lv_table_set_cell_value(table, 1, 3, press_str);


        }
    }
}

void update_results() {
    
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
        k_sleep(K_MSEC(10000));
        printk("\n*-*-*- Sensor Data: *-*-*-*-\n");
        for (int i = 0; i < sizeof (results_data); i++) {
            printk("%x:", results_data[i]);
        }
        printk("\n*-*-*-*-*-*-*-*-*-*-*\n");

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

        printk("\n*-*-*- Voltage Data: *-*-*-*-\n");
        for (int i = 0; i < sizeof (voltages_data); i++) {
            printk("%x:", voltages_data[i]);
        }
        printk("\n*-*-*-*-*-*-*-*-*-*-*\n");

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

}

void main(void) {

    results_data[0] = 0x00;
    voltages_data[0] = 0x01;

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



    /* * * * * * * * * * * * * i2c3 Device * * * * * * * * * * * * * */


    uint32_t i2c3_cfg = I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_MASTER;

    printk("Initializing I2C 3...\n");
    i2c3_dev = device_get_binding("I2C_3");
    if (!i2c3_dev) {
        printk("I2C3: Device driver not found.\n");
        return;
    }

    if (i2c_configure(i2c3_dev, i2c3_cfg)) {
        printk("I2C3 config failed\n");
    } else {
        printk("i2c3 configured...\n");
    }


    printk("Size of Float %d\n", sizeof (float));


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
    static lv_style_t data_style;
    

    lv_style_init(&screenStyle1);
    lv_style_init(&style1);
    lv_style_init(&data_style);

    lv_style_set_bg_color(&screenStyle1, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_text_font(&style1, LV_STATE_DEFAULT, &lv_font_montserrat_24);
    lv_style_set_text_color(&style1, LV_STATE_DEFAULT, LV_COLOR_WHITE);

    lv_style_set_text_font(&data_style, LV_STATE_DEFAULT, &lv_font_montserrat_16);
    lv_style_set_text_color(&data_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);

    lv_obj_t *screen1;
    screen1 = lv_obj_create(NULL, NULL);
    lv_obj_add_style(screen1, LV_LABEL_PART_MAIN, &screenStyle1);

    title_label = lv_label_create(screen1, NULL);
    lv_obj_add_style(title_label, LV_OBJ_PART_MAIN, &style1);
    
    
    /*Create a Table */
    
    table = lv_table_create(screen1, NULL);
    
    static lv_style_t table_style;
    lv_style_init(&table_style);
    lv_style_set_pad_top(&table_style, LV_STATE_DEFAULT, 0);
    lv_style_set_pad_bottom(&table_style, LV_STATE_DEFAULT, 0);
    lv_style_set_text_line_space(&table_style, LV_STATE_DEFAULT, 0);
    lv_style_set_border_width(&table_style, LV_STATE_DEFAULT, 0);
    lv_style_set_pad_left(&table_style, LV_STATE_DEFAULT, 0);
    lv_style_set_pad_right(&table_style, LV_STATE_DEFAULT, 0);
    lv_style_set_text_letter_space(&table_style, LV_STATE_DEFAULT, 0);
    
    lv_style_set_pad_top(&table_style, LV_TABLE_PART_CELL1, 0);
    lv_style_set_pad_bottom(&table_style, LV_TABLE_PART_CELL1, 0);
    
    lv_style_set_pad_top(&table_style, LV_TABLE_PART_CELL2, 0);
    lv_style_set_pad_bottom(&table_style, LV_TABLE_PART_CELL2, 0);
    
    lv_style_set_pad_top(&table_style, LV_TABLE_PART_CELL3, 0);
    lv_style_set_pad_bottom(&table_style, LV_TABLE_PART_CELL3, 0);
    
    lv_style_set_pad_top(&table_style, LV_TABLE_PART_CELL4, 0);
    lv_style_set_pad_bottom(&table_style, LV_TABLE_PART_CELL4, 0);
    
    lv_style_set_text_letter_space(&table_style, LV_TABLE_PART_CELL1, 0);
    lv_style_set_text_letter_space(&table_style, LV_TABLE_PART_CELL2, 0);
    lv_style_set_text_letter_space(&table_style, LV_TABLE_PART_CELL3, 0);
    lv_style_set_text_letter_space(&table_style, LV_TABLE_PART_CELL4, 0);

    lv_style_set_border_width(&table_style, LV_TABLE_PART_CELL1, 0);
    lv_style_set_border_width(&table_style, LV_TABLE_PART_CELL2, 0);
    lv_style_set_border_width(&table_style, LV_TABLE_PART_CELL3, 0);
    lv_style_set_border_width(&table_style, LV_TABLE_PART_CELL4, 0);
    
    lv_style_set_text_line_space(&table_style, LV_TABLE_PART_CELL1, 0);
    lv_style_set_text_line_space(&table_style, LV_TABLE_PART_CELL2, 0);
    lv_style_set_text_line_space(&table_style, LV_TABLE_PART_CELL3, 0);
    lv_style_set_text_line_space(&table_style, LV_TABLE_PART_CELL4, 0);
    
    
    lv_style_set_text_color(&table_style, LV_TABLE_PART_BG, LV_COLOR_RED);
   
    
    
    lv_style_set_bg_color(&table_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_obj_add_style(table, LV_TABLE_PART_BG, &table_style);
    lv_obj_add_style(table, LV_TABLE_PART_CELL1, &data_style);
    

    
    
    lv_table_set_col_cnt(table, 3);
    lv_table_set_row_cnt(table, 7);
    
    lv_table_set_cell_type(table, 1, 0, 1);
    lv_table_set_cell_type(table, 1, 1, 1);
    lv_table_set_cell_type(table, 1, 2, 1);
    
    //lv_obj_align(table, NULL, LV_ALIGN_CENTER, 0, 0);
    
    

    /*Make the cells of the first row center aligned */
    //lv_table_set_cell_align(table, 0, 0, LV_LABEL_ALIGN_CENTER);
    //lv_table_set_cell_align(table, 0, 1, LV_LABEL_ALIGN_CENTER);

    /*Align the price values to the right in the 2nd column*/
    //lv_table_set_cell_align(table, 1, 1, LV_LABEL_ALIGN_RIGHT);
    //lv_table_set_cell_align(table, 2, 1, LV_LABEL_ALIGN_RIGHT);
    //lv_table_set_cell_align(table, 3, 1, LV_LABEL_ALIGN_RIGHT);

    lv_table_set_col_width(table, 0,70);
    lv_table_set_col_width(table, 1,70);
    lv_table_set_col_width(table, 2,70);
    

    
    
    
    



    lv_table_set_cell_value(table, 0, 0, "N");
    lv_table_set_cell_value(table, 0, 1, "RT");
    lv_table_set_cell_value(table, 0, 2, "AVG");
    lv_table_set_cell_value(table, 1, 0, "T");
    lv_table_set_cell_value(table, 2, 0, "HR");
    lv_table_set_cell_value(table, 3, 0, "P");
    lv_table_set_cell_value(table, 4, 0, "NO2");
    lv_table_set_cell_value(table, 5, 0, "SO2");
    lv_table_set_cell_value(table, 6, 0, "O3");
    
    


    


    
    /*
    for (int row=2; row < 8; row++) {
        for (int col=2; col <3; col++) {
            lv_table_set_cell_value(table, col, row, "0");
        }
    }
*/
    lv_table_ext_t * ext = lv_obj_get_ext_attr(table);
    ext->row_h[0] = 20;
    
    
    /*
    
    humidity_label = lv_label_create(screen1, NULL);
    temperat_label = lv_label_create(screen1, NULL);
    press_label = lv_label_create(screen1, NULL);
    no2_label = lv_label_create(screen1, NULL);
    so2_label = lv_label_create(screen1, NULL);
    o3_label = lv_label_create(screen1, NULL);
 
     
    
    lv_obj_add_style(humidity_label, LV_OBJ_PART_MAIN, &data_style);
    lv_obj_add_style(temperat_label, LV_OBJ_PART_MAIN, &data_style);
    lv_obj_add_style(press_label, LV_OBJ_PART_MAIN, &data_style);
    lv_obj_add_style(no2_label, LV_OBJ_PART_MAIN, &data_style);
    lv_obj_add_style(so2_label, LV_OBJ_PART_MAIN, &data_style);
    lv_obj_add_style(o3_label, LV_OBJ_PART_MAIN, &data_style);

    lv_obj_align(humidity_label, NULL, LV_ALIGN_IN_LEFT_MID, 0, -50);
    lv_obj_align(temperat_label, NULL, LV_ALIGN_IN_LEFT_MID, 0, -20);
    lv_obj_align(press_label, NULL, LV_ALIGN_IN_LEFT_MID, 0, 10);
    lv_obj_align(no2_label, NULL, LV_ALIGN_IN_LEFT_MID, 0, 40);
    lv_obj_align(so2_label, NULL, LV_ALIGN_IN_LEFT_MID, 0, 70);
    lv_obj_align(o3_label, NULL, LV_ALIGN_IN_LEFT_MID, 0, 100);
    */

    lv_label_set_text(title_label, "AirLab");
    lv_obj_align(title_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);



    lv_scr_load(screen1);
    lv_task_handler();
    display_blanking_off(display_dev);
    
    
    k_thread_create(&my_thread_data, my_stack_area,
                                 K_THREAD_STACK_SIZEOF(my_stack_area),
                                 take_a_reading,
                                 NULL, NULL, NULL,
                                 5, 0, K_NO_WAIT);

    

/*
    printk("\n*-*-*- Data: *-*-*-*-\n");
    for (int i = 0; i < sizeof (results_data); i++) {
        printk("%x:", results_data[i]);
    }
    printk("\n*-*-*-*-*-*-*-*-*-*-*\n");
*/

    while (1) {
        lv_task_handler();
        k_msleep(5);
    }

}



