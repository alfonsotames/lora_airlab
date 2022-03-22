/*
 * Class A LoRaWAN AirLab Sensor Application
 *
 * 
 */

#include <device.h>
#include <drivers/sensor.h>

#include <zephyr.h>
#include <drivers/i2c.h>
#include <drivers/display.h>
#include <drivers/rtc/maxim_ds3231.h>
#include <storage/disk_access.h>
#include <fs/fs.h>
#include <ff.h>
#include <lorawan/lorawan.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <string.h>
#include "file_system.h"
#include "time_system.h"

#include "global_devices.h"
#include "opc.h"
#include "ambient_sensors.h"
#include "gas_sensors.h"



/* size of stack area used by each thread */
#define STACKSIZE 4096
#define DELAY K_MSEC(5000)


/* * * * * * * * * * Logging  * * * * * * * * * * */
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(main);


/* * * * * * * * * * Global Devices  * * * * * * * * * * */

const struct device *gpiob;
const struct device *gpioa;

/* * * * * * * * * * * * * * * * LEDS  * * * * * * * * * * * * * * */

const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

/* * * * * * * * * * LoRa * * * * * * * * * * */

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
        "No default LoRa radio specified in DT");
#define DEFAULT_RADIO DT_LABEL(DEFAULT_RADIO_NODE)

/* Customize based on network configuration */
#define LORAWAN_DEV_EUI   { 0x00, 0x80, 0xE1, 0x15, 0x00, 0x0A, 0x93, 0xB8 }
#define LORAWAN_JOIN_EUI  { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 }
#define LORAWAN_APP_KEY   { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C }



static void dl_callback(uint8_t port, bool data_pending,
        int16_t rssi, int8_t snr,
        uint8_t len, const uint8_t *data);

static void lorwan_datarate_changed(enum lorawan_datarate dr);


/* * * * * * * * * * * * *  LoRa Messages  * * * * * * * * * * * * */
char gas_msg[200];
char opc_msg[200];

/* * * * * * * * * * * * *  LoRa Functions  * * * * * * * * * * * * */

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



/*----------------------------------------------------------------------*/
/* - * - * - * - * - * - * - *  THREADS * - * - * - * - * - * - * - * - */
/*----------------------------------------------------------------------*/

/* * * * * * * * THREAD: Count Particles * * * * * * * */

K_THREAD_STACK_DEFINE(count_particles_stack_area, 16384);
struct k_thread count_particles_thread_data;
extern void count_particles() {
    LOG_INF("Count Particles Thread Spawn!");
    ///AMBIENT SENSORS
    /*      0 Humidiy   1 Temperature   2 pressure            */
    float amb_sensors[3];
    char text_data[200];
    struct histogram data;
    while(1) {
        LOG_INF("Counting Particles...");
        gpio_pin_set_dt(&led_red,1);

        opc_init();    
        opc_start();
        data = opc_read_histogram(7);
        opc_stop();
        opc_release();
        
        gpio_pin_set_dt(&led_red,0);
        ambient_sensors_read(1, amb_sensors);
        snprintfcb(text_data, 200, "%s|%0.2f|%0.2f|%0.2f|%0.2f|"
                        "%0.2f|%0.2f|%0.2f|%0.2f\n",
                get_formatted_time(),
                data.period,
                data.sfr,
                data.pm1,
                data.pm25,
                data.pm10,
                amb_sensors[0],
                amb_sensors[1],
                amb_sensors[2]);
        strcpy(opc_msg, text_data);
        //LOG_INF("R: %s",text_data);
        save_data(text_data, "opc");
        k_msleep(120000);
    }
}

/* * * * * * * * THREAD: Read Gas & Ambient Sensors * * * * * * * */

K_THREAD_STACK_DEFINE(read_gas_and_ambient_sensors_stack_area, STACKSIZE);
struct k_thread read_gas_and_ambient_sensors_thread_data;
extern void read_gas_and_ambient_sensors() {
    
    LOG_INF("Read Gas Sensors Thread Spawn!");
    LOG_INF("Warming up!");
    k_msleep(10000);
    // AlphaSensors

    
    uint16_t gs[8];
    
    // AMBIENT SENSORS
    /*      0 Humidiy   1 Temperature   2 pressure            */
    float amb_sensors[30];
    char text_data[200];    

    while(1) {
        //LOG_INF("Reading gas sensors...");
        gas_sensors_read(gs,5);
        ambient_sensors_read(1, amb_sensors);
        snprintfcb(text_data, 200, "%s|%d|%d|%d|"
                "%d|%d|%d|%d|%d|%0.2f|%0.2f|%0.2f\n",
                get_formatted_time(),
                gs[0],
                gs[1],
                gs[2],
                gs[3],
                gs[4],
                gs[5],
                gs[6],
                gs[7],
                amb_sensors[0],
                amb_sensors[1],
                amb_sensors[2]);                
        save_data(text_data, "gas");
        strcpy(gas_msg, text_data);
    }
}

/* * * * * * * * THREAD: Send Data * * * * * * * */

K_THREAD_STACK_DEFINE(send_data_stack_area, STACKSIZE);
struct k_thread send_data_thread_data;
extern void send_data() {
    

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
            gpio_pin_set_dt(&led_red,1);
            LOG_ERR("lorawan_join_network failed: %d", ret);
            k_sleep(K_MSEC(1000));
            gpio_pin_set_dt(&led_red,0);
            k_sleep(K_MSEC(1000));
        } else {
            ret = lorawan_set_datarate(LORAWAN_DR_2);
            if (ret < 0) {
                LOG_ERR("Could not set datarate!");
            }
            LOG_INF("Connected to network over OTAA!");
            
            gpio_pin_set_dt(&led_green,1);
        }
    }

    LOG_INF("Entering Loop...");
    
    while(1) {
        //LOG_INF("Sending data...");
        
        /*
        char test[] = "2022-03";
        LOG_INF("opc_msg=%s", test);
        LOG_INF("mide: %d",strlen(test)); 
        
        char test_opc[] = "2022-03-21 13:37:43|5.01|5.38|0.82|1.14|1.57|29.00|24.55|77.67";
        LOG_INF("opc_msg=%s", test_opc);
        LOG_INF("mide: %d",strlen(test_opc));        
        char test_gas[] = "2022-03-21 12:46:56|255.54|259.55|471.31|451.58|257.24|288.53|237.41|284.27|31.52|24.38|77.77";
        LOG_INF("gas_msg=%s", test_gas);
        LOG_INF("mide: %d",strlen(test_gas));
        */
        
        ret = lorawan_send(1, gas_msg, strlen(gas_msg), LORAWAN_MSG_CONFIRMED);


        /*
         * Note: The stack may return -EAGAIN if the provided data
         * length exceeds the maximum possible one for the region and
         * datarate. But since we are just sending the same data here,
         * we'll just continue.
         */
        if (ret == -EAGAIN) {
            //LOG_ERR("lorawan_send failed: %d. Continuing...", ret);
            k_msleep(2000);
        }

        if (ret < 0) {
            //LOG_ERR("lorawan_send failed: %d", ret);
            //return;
            k_msleep(2000);
        }
        if (ret == 0) {
            LOG_INF("* - * - * - * - * Sensor Data sent! * - * - * - * - * ");
            for (int i=0; i<20; i++) {
                gpio_pin_set_dt(&led_green,1);
                k_msleep(50);
                gpio_pin_set_dt(&led_green,0);
                k_msleep(50);
            }
        }
        
        k_msleep(10000);
        
        ret = lorawan_send(2, opc_msg, strlen(opc_msg), LORAWAN_MSG_CONFIRMED);
        
        if (ret == -EAGAIN) {
            //LOG_ERR("lorawan_send failed: %d. Continuing...", ret);
            k_msleep(2000);
        }

        if (ret < 0) {
            //LOG_ERR("lorawan_send failed: %d", ret);
            //return;
            k_msleep(2000);
        }
        if (ret >= 0) {
            LOG_INF("* - * - * - * - * Sensor Data sent! * - * - * - * - * ");
            for (int i=0; i<20; i++) {
                gpio_pin_set_dt(&led_green,1);
                k_msleep(50);
                gpio_pin_set_dt(&led_green,0);
                k_msleep(50);
            }            
        }
     

        k_msleep(10000);
    }
}



/* * * * * * * * * * * * * * * * * main * * * * * * * * * *  * * * * * * * */

void main(void) {
    
    LOG_INF("* * * * * * LoRaWAN Air Lab * * * * * * ");
    
    int ret;
    
    // System Initialitations
    
    
    init_time_system();
    init_and_mount_sdcard();
    
    
    dev_sht3xd = get_sht3xd_device();
    if (dev_sht3xd == NULL) {
        return;
    }

    dev_bme280 = get_bme280_device();
    if (dev_bme280 == NULL) {
        return;
    }
      
    
    gas_sensors_init();
 
            
    //LOG_INF("Date: %s",get_formatted_time());
    
    
    gpioa = device_get_binding("GPIOA");
    gpiob = device_get_binding("GPIOB");
    
    ret = gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    ret = gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
    
    ret = gpio_pin_configure(gpiob, 3, GPIO_OUTPUT_INACTIVE);
    ret = gpio_pin_configure(gpioa, 4, GPIO_OUTPUT_INACTIVE);
    ret = gpio_pin_configure(gpioa, 5, GPIO_OUTPUT_LOW);
    ret = gpio_pin_configure(gpioa, 7, GPIO_OUTPUT_LOW);
    ret = gpio_pin_configure(gpioa, 6, GPIO_INPUT);
    
    gpio_pin_set_dt(&led_green,0);
    gpio_pin_set_dt(&led_red,0);
    
    /* Thread Create: Read Ambient Sensors*/
    k_thread_create(&send_data_thread_data, send_data_stack_area,
                                 K_THREAD_STACK_SIZEOF(send_data_stack_area),
                                 send_data,
                                 NULL, NULL, NULL,
                                 5, 0, K_NO_WAIT);
    
    
    /* Thread Create: Read Gas Sensors*/
    k_thread_create(&read_gas_and_ambient_sensors_thread_data, read_gas_and_ambient_sensors_stack_area,
                                 K_THREAD_STACK_SIZEOF(read_gas_and_ambient_sensors_stack_area),
                                 read_gas_and_ambient_sensors,
                                 NULL, NULL, NULL,
                                 5, 0, K_NO_WAIT);

    
    /* Thread Create: Count Particles*/
    k_thread_create(&count_particles_thread_data, count_particles_stack_area,
                                 K_THREAD_STACK_SIZEOF(count_particles_stack_area),
                                 count_particles,
                                 NULL, NULL, NULL,
                                 5, 0, K_NO_WAIT);    
    
     
    while (1) {
        k_msleep(5);
    }
    
    
    
    
}