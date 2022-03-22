/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.c to edit this template
 */



#include "opc.h"
#include "global_devices.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(opc);

// How many samples to skip in result
#define SKIP 2


const struct device *spi;



struct spi_cs_control *ctrl;
struct spi_config spi_cfg;
    
void opc_release() {
    uint8_t r;
    r = spi_release(spi, NULL);
    if (r == 0) {
        LOG_INF("SPI Released!");
    }
    r = gpio_pin_configure(gpioa, 7, GPIO_OUTPUT_LOW);
    r = gpio_pin_configure(gpioa, 5, GPIO_OUTPUT_LOW);
    r = gpio_pin_configure(gpioa, 6, GPIO_INPUT);
    r = gpio_pin_configure(gpioa, 4, GPIO_OUTPUT_LOW);
    
    
    
    gpio_pin_set(gpiob,4,0);
    gpio_pin_set(gpiob,7,0);
    //gpio_pin_set(gpiob,6,0);     // MISO (input line)
    gpio_pin_set(gpiob,5,0);
}    
void opc_init() {
    
   // Set up configuration structures for SPI
    
    /*   
    struct spi_cs_control *ctrl =
                &(struct spi_cs_control) {
                        .gpio_dev = device_get_binding("GPIOA"),
                        //.gpio_dev = device_get_binding("GPIO_0"),
                        .delay = 2,
                        .gpio_pin = 4,
                        //.gpio_pin = 19,
                        .gpio_dt_flags = GPIO_ACTIVE_LOW
    };
    */ 

    
    spi_cfg.frequency = 500000U;
    spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | 
            SPI_OP_MODE_MASTER | SPI_MODE_CPHA;

    
    spi = device_get_binding("OPC");

    if (spi == NULL) {
        /* No such node, or the node does not have status "okay". */
        LOG_ERR("Error: no device SPI found.");

    }
    
}
uint16_t to_16bit_int(uint8_t LSB, uint8_t MSB)
{
  // Combine two bytes into a 16-bit unsigned int
  return ((MSB << 8) | LSB);
}

float calculate_float(uint8_t val0, uint8_t val1, uint8_t val2, uint8_t val3)
{
  // Return an IEEE754 float from an array of 4 bytes
  union u_tag {
    uint8_t b[4];
    float val;
  } u;

  u.b[0] = val0;
  u.b[1] = val1;
  u.b[2] = val2;
  u.b[3] = val3;

  return u.val;
}

uint32_t to_32bit_int(uint8_t val0, uint8_t val1, uint8_t val2, uint8_t val3)
{
  // Return a 32-bit unsigned int from 4 bytes
  return ((val3 << 24) | (val2 << 16) | (val1 << 8) | val0);
}

uint8_t opc_send_command(uint8_t cmd) {
    
 
    
    uint8_t tx_buf[1];
    uint8_t rx_buf[1];
    struct spi_buf spi_buf_tx[] = {{.buf = tx_buf,.len = sizeof(tx_buf)},};
    struct spi_buf spi_buf_rx[] = {{.buf = rx_buf,.len = sizeof(rx_buf)},};
    struct spi_buf_set tx = {.buffers = spi_buf_tx,.count = 1};
    struct spi_buf_set rx = {.buffers = spi_buf_rx,.count = 1};
    
    tx_buf[0] = cmd;
    
    gpio_pin_set(gpioa,4,0);
    k_usleep(4);
    spi_transceive(spi, &spi_cfg, &tx, &rx);
    k_usleep(4);
    gpio_pin_set(gpioa,4,1);                
    LOG_INF("Sent %d, received: %d",cmd,rx_buf[0]);
    return rx_buf[0];
}


/* * * * * * * * * Start OPC * * * * * * * * * */

void opc_start() {
    LOG_INF("*** STARTING OPC ***");
    // Switch on the relay
    for (int t=3; t>0; t--) {
        LOG_INF("Switching the relay ON in %d",t);
        k_msleep(1000);
    }    
    gpio_pin_set(gpiob,3,1);
    
    uint8_t r1 = 0;
    uint8_t r2 = 0;
    
    uint8_t c = 0; 

    for (int t=3; t>0; t--) {
        LOG_INF("Sending ON Sequence in %d:",t);
        k_msleep(1000);
    }
    // Iterate until 243 (ready)
    while(1) {

        r1 = opc_send_command(0x03);
        k_msleep(10);
        r2 = opc_send_command(0x00);
        

        if (r1 == 243 && r2 == 3) {
                LOG_INF("**** Received 0xF3 and 0x03 :: OPC Unit is ready for start sequence...");
                break;
        }
        if (r1 == 49) {
                LOG_INF("Received busy status from OP");

        }		
        k_msleep(1000);

        if (c == 10) {
                LOG_INF("Waiting 5s for OPC's buffer to reset");
                k_msleep(5000);	
        }
        if (c > 20) {
                LOG_INF("power cycle...");
                gpio_pin_set(gpiob,3,0);
                gpio_pin_set(gpiob,4,0);
                gpio_pin_set(gpiob,7,0);
                //gpio_pin_set(gpiob,6,0);     // MISO (input line)
                gpio_pin_set(gpiob,5,0);
                k_msleep(8000);
                gpio_pin_set(gpiob,3,1);
                c=0;
        }				
        c++;
    }

    k_msleep(5000);    
}

void opc_stop() {
    LOG_INF("*** SHUTTING DOWN OPC ***");
    // Switch on the relay
    for (int t=3; t>0; t--) {
        LOG_INF("Switching the relay OFF in %d",t);
        k_msleep(1000);
    }    
    gpio_pin_set(gpiob,3,1);
    
    uint8_t r1 = 0;
    uint8_t r2 = 0;
    
    uint8_t c = 0; 

    for (int t=3; t>0; t--) {
        LOG_INF("Sending OFF Sequence in %d:",t);
        k_msleep(1000);
    }
    // Iterate until 243 (ready)
    while(1) {

        r1 = opc_send_command(0x03);
        k_msleep(10);
        r2 = opc_send_command(0x01);
        

        if (r1 == 243 && r2 == 3) {
                LOG_INF("**** Received 0xF3 and 0x03 :: "
                        "OPC Unit is ready for shutdown sequence...");
                break;
        }
        if (r1 == 49) {
                LOG_INF("Received busy status from OP");

        }		
        k_msleep(600);

        if (c == 10) {
                LOG_INF("Waiting 5s for OPC's buffer to reset");
                k_msleep(5000);	
        }
        if (c > 20) {
            LOG_INF("Couldn't confirm off command status");
            break;
        }				
        c++;
    }
    k_msleep(2000);
    gpio_pin_set(gpiob,3,0);
    gpio_pin_set(gpiob,4,0);
    gpio_pin_set(gpiob,7,0);
    gpio_pin_set(gpiob,6,0);    
    gpio_pin_set(gpiob,5,0);
}

/* * * * * * * * * Read Information String * * * * * * * * * */

void opc_read_information_string() {
    
    uint8_t r = 0;
    uint8_t tx_buf[1];
    uint8_t rx_buf[1];
    struct spi_buf spi_buf_tx[] = {{.buf = tx_buf,.len = sizeof(tx_buf)},};
    struct spi_buf spi_buf_rx[] = {{.buf = rx_buf,.len = sizeof(rx_buf)},};
    struct spi_buf_set tx = {.buffers = spi_buf_tx,.count = 1};
    struct spi_buf_set rx = {.buffers = spi_buf_rx,.count = 1};    
    uint8_t vals[61];
    

    r = opc_send_command(0x3F);
    k_msleep(3000);
    
    if (r == 243) {
        LOG_INF("Information String is READY!!!!");       
        gpio_pin_set(gpioa,4,0);
        for (int i=0; i<60; i++) {
            tx_buf[0] = 0x00;
            spi_transceive(spi, &spi_cfg, &tx, &rx);
            //printk("val: %d",rx_buf[0]);
            vals[i] = rx_buf[0];

        }
        gpio_pin_set(gpioa,4,1);        
    }
    LOG_HEXDUMP_INF(vals, 61, "Information String: ");    
    
}



/* * * * * * * * * Read Histogram * * * * * * * * * */

struct histogram opc_read_histogram(uint8_t samples) {
    
    uint8_t r = 0;
  
    struct histogram data, result;

    data.pm1  = 0;
    data.pm25 = 0;
    data.pm10 = 0;
    data.sfr = 0;
    data.period = 0;
    data.temp_pressure = 0;
    
    result.pm1  = 0;
    result.pm25 = 0;
    result.pm10 = 0;
    result.sfr = 0;
    result.period = 0;
    result.temp_pressure = 0;

    for (uint8_t s=0; s<samples; s++) {
        LOG_INF("READING SAMPLE %d",s);
        r = opc_send_command(0x30);
        k_msleep(12);

        uint8_t tx_buf[1];
        uint8_t rx_buf[1];
        struct spi_buf spi_buf_tx[] = {{.buf = tx_buf,.len = sizeof(tx_buf)},};
        struct spi_buf spi_buf_rx[] = {{.buf = rx_buf,.len = sizeof(rx_buf)},};
        struct spi_buf_set tx = {.buffers = spi_buf_tx,.count = 1};
        struct spi_buf_set rx = {.buffers = spi_buf_rx,.count = 1};

        uint8_t vals[62];

        if (r == 243) {
            LOG_INF("Histogram %d is READY!!!!",s);

            gpio_pin_set(gpioa,4,0);
            
            k_usleep(6);
            for (int i=0; i<62; i++) {
                tx_buf[0] = 0xC0;
                spi_transceive(spi, &spi_cfg, &tx, &rx);
                //printk("val: %d",rx_buf[0]);
                vals[i] = rx_buf[0];

            }
            k_usleep(6);
            gpio_pin_set(gpioa,4,1);

        }
        LOG_HEXDUMP_INF(vals, 62, "Histogram: ");


        data.period = calculate_float(vals[44], vals[45], vals[46], vals[47]);
        data.sfr    = calculate_float(vals[36], vals[37], vals[38], vals[39]);

        // If convert_to_conc = True, convert from raw data to concentration

        bool convert_to_conc = true;
        double conv;

        if ( convert_to_conc != true ) {
            conv = 1.0;
        }
        else {
            conv = data.sfr * data.period;
        }

        // Calculate all of the values!
        data.bin0   = (double)to_16bit_int(vals[0], vals[1]) / conv;
        data.bin1   = (double)to_16bit_int(vals[2], vals[3]) / conv;
        data.bin2   = (double)to_16bit_int(vals[4], vals[5]) / conv;
        data.bin3   = (double)to_16bit_int(vals[6], vals[7]) / conv;
        data.bin4   = (double)to_16bit_int(vals[8], vals[9]) / conv;
        data.bin5   = (double)to_16bit_int(vals[10], vals[11]) / conv;
        data.bin6   = (double)to_16bit_int(vals[12], vals[13]) / conv;
        data.bin7   = (double)to_16bit_int(vals[14], vals[15]) / conv;
        data.bin8   = (double)to_16bit_int(vals[16], vals[17]) / conv;
        data.bin9   = (double)to_16bit_int(vals[18], vals[19]) / conv;
        data.bin10  = (double)to_16bit_int(vals[20], vals[21]) / conv;
        data.bin11  = (double)to_16bit_int(vals[22], vals[23]) / conv;
        data.bin12  = (double)to_16bit_int(vals[24], vals[25]) / conv;
        data.bin13  = (double)to_16bit_int(vals[26], vals[27]) / conv;
        data.bin14  = (double)to_16bit_int(vals[28], vals[29]) / conv;
        data.bin15  = (double)to_16bit_int(vals[30], vals[31]) / conv;

        data.bin1MToF = vals[32] / 3.0;
        data.bin3MToF = vals[33] / 3.0;
        data.bin5MToF = vals[34] / 3.0;
        data.bin7MToF = vals[35] / 3.0;    

        data.temp_pressure = to_32bit_int(vals[40], vals[41], vals[42], vals[43]);
        data.checksum = to_16bit_int(vals[48], vals[49]);

        data.pm1 =  calculate_float(vals[50], vals[51], vals[52], vals[53]);
        data.pm25 = calculate_float(vals[54], vals[55], vals[56], vals[57]);
        data.pm10 = calculate_float(vals[58], vals[59], vals[60], vals[61]);
        
        uint8_t testpm10[4];
        testpm10[0] = vals[58];
        testpm10[1] = vals[59];
        testpm10[2] = vals[60];
        testpm10[3] = vals[61];
        
        uint8_t testpm25[4];
        testpm25[0] = vals[54];
        testpm25[1] = vals[55];
        testpm25[2] = vals[56];
        testpm25[3] = vals[57];        
        
        LOG_HEXDUMP_INF(testpm10, 4, "PM10 Bytes: ");
        LOG_HEXDUMP_INF(testpm25, 4, "PM25 Bytes: ");
        
        char fperiod[40];
        snprintfcb(fperiod, 40, "%0.2f", data.period);
        char fpm1[40];
        snprintfcb(fpm1, 40, "%0.2f", data.pm1);
        char fpm25[40];
        snprintfcb(fpm25, 40, "%0.2f", data.pm25);
        char fpm10[40];
        snprintfcb(fpm10, 40, "%0.2f", data.pm10);
        char fflowrate[40];
        snprintfcb(fflowrate, 40, "%0.2f", data.sfr);

        LOG_INF("Sampling Period: %s", fperiod);
        LOG_INF("Sample Flow Rate: %s",fflowrate);
        LOG_INF("PM1: %s", fpm1);
        LOG_INF("PM2.5: %s", fpm25);
        LOG_INF("PM10: %s", fpm10);
        
        if (s > (SKIP-1)) {
            result.pm1  = result.pm1 + data.pm1;
            result.pm10 = result.pm10 + data.pm10;
            result.pm25 = result.pm25 + data.pm25;
            result.period = result.period + data.period;
            result.sfr = result.sfr + data.sfr;
        }
        k_msleep(5000);
    }
    
    //LOG_INF("Calculating OPC data average");
    // Calculate Average
    float div = samples-SKIP;
    //LOG_INF("Dividing %0.2f over %d",result.pm10, samples-SKIP);
    result.pm1 = result.pm1 / div;
    result.pm25 = result.pm25 / div;
    result.pm10 = result.pm10 / div;
    result.period = result.period / div;
    result.sfr = result.sfr / div;
    LOG_INF("Returning OPC read result...");
    
    return result;
}

