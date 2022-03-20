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
#include <drivers/gpio.h>
#include <drivers/spi.h>

#include "file_system.h"
#include "time_system.h"

#include "global_devices.h"
#include "opc.h"


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



/* * * * * * * * * * LEDS  * * * * * * * * * * */

const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

            
           
/*----------------------------------------------------------------------*/
/* - * - * - * - * - * - * - *  THREADS * - * - * - * - * - * - * - * - */
/*----------------------------------------------------------------------*/



/* * * * * * * * THREAD: Count Particles * * * * * * * */

K_THREAD_STACK_DEFINE(count_particles_stack_area, 16384);
struct k_thread count_particles_thread_data;
extern void count_particles() {
  
    LOG_INF("Count Particles Thread Spawn!");

    char text_data[100];
    struct histogram data;
    while(1) {
        LOG_INF("Counting Particles...");
        gpio_pin_set_dt(&led_red,1);
        opc_init();    
        opc_start();
        data = opc_read_histogram(5);
        
        snprintfcb(text_data, 100, "%s|%0.2f|%0.2f|%0.2f|%0.2f|%0.2f",
                get_formatted_time(),
                data.period,
                data.sfr,
                data.pm1,
                data.pm25,
                data.pm10);
        
        LOG_INF("R: %s",text_data);
        
        //save_data(text_data, "opc");

        gpio_pin_set_dt(&led_red,0);
        opc_stop();
        opc_release();
        k_msleep(120000);
        

    }
}

/* * * * * * * * THREAD: Read Gas & Ambient Sensors * * * * * * * */

K_THREAD_STACK_DEFINE(read_gas_and_ambient_sensors_stack_area, STACKSIZE);
struct k_thread read_gas_and_ambient_sensors_thread_data;
extern void read_gas_and_ambient_sensors() {
    
    LOG_INF("Read Gas Sensors Thread Spawn!");
    
    while(1) {
        //LOG_INF("Reading gas sensors...");
        //gpio_pin_set_dt(&led_green,1);

        k_msleep(500);
        //gpio_pin_set_dt(&led_green,0);
        k_msleep(500);
    }
}

/* * * * * * * * THREAD: Send Data * * * * * * * */

K_THREAD_STACK_DEFINE(send_data_stack_area, STACKSIZE);
struct k_thread send_data_thread_data;
extern void send_data() {
    
    LOG_INF("Send Data Thread Spawn!");
    
    while(1) {
        //LOG_INF("Sending data...");
        //gpio_pin_set_dt(&led_blue,1);

        k_msleep(500);
        //gpio_pin_set_dt(&led_blue,0);
        k_msleep(3000);
    }
}



/* * * * * * * * * * * * * * * * * main * * * * * * * * * *  * * * * * * * */

void main(void) {
    
    LOG_INF("* * * * * * LoRaWAN Air Lab * * * * * * ");
    
    int ret;
    
    // System Initialitations
    
    init_time_system();
    init_and_mount_sdcard();
            
    LOG_INF("Date: %s",get_formatted_time());
    
    gpioa = device_get_binding("GPIOA");
    gpiob = device_get_binding("GPIOB");
    
    ret = gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    ret = gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
    
    ret = gpio_pin_configure(gpiob, 3, GPIO_OUTPUT_INACTIVE);
    ret = gpio_pin_configure(gpioa, 4, GPIO_OUTPUT_INACTIVE);
    
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