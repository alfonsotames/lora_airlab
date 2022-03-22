#include <device.h>
#include <toolchain.h>

/* 1 : /soc/rcc@58000000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/clocks/pll)
 * Supported:
 *   - (/soc/adc@40012400)
 *   - (/soc/aes@58001800)
 *   - (/soc/dac@40007400)
 *   - (/soc/dma@40020000)
 *   - (/soc/dma@40020400)
 *   - (/soc/dmamux@40020800)
 *   - (/soc/i2c@40005400)
 *   - /soc/i2c@40005800
 *   - /soc/i2c@40005c00
 *   - (/soc/rng@58001000)
 *   - /soc/rtc@40002800
 *   - (/soc/serial@40004400)
 *   - /soc/serial@40008000
 *   - (/soc/serial@40013800)
 *   - /soc/spi@40003800
 *   - /soc/spi@40013000
 *   - /soc/spi@58010000
 *   - (/soc/timers@40000000)
 *   - (/soc/timers@40007c00)
 *   - (/soc/timers@40012c00)
 *   - (/soc/timers@40014400)
 *   - (/soc/timers@40014800)
 *   - (/soc/watchdog@40002c00)
 *   - /soc/pin-controller@48000000/gpio@48000000
 *   - /soc/pin-controller@48000000/gpio@48000400
 *   - /soc/pin-controller@48000000/gpio@48000800
 *   - /soc/pin-controller@48000000/gpio@48001c00
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_rcc_58000000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, 12, 3, 9, 14, 4, 7, 13, 5, 8, 10, 6, DEVICE_HANDLE_ENDS };

/* 2 : /soc/interrupt-controller@58000800:
 * Direct Dependencies:
 *   - (/soc)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_interrupt_controller_58000800[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 3 : /soc/pin-controller@48000000/gpio@48001c00:
 * Direct Dependencies:
 *   - (/soc/pin-controller@48000000)
 *   - /soc/rcc@58000000
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pin_controller_48000000_S_gpio_48001c00[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 4 : /soc/pin-controller@48000000/gpio@48000800:
 * Direct Dependencies:
 *   - (/soc/pin-controller@48000000)
 *   - /soc/rcc@58000000
 * Supported:
 *   - (/gpio_keys/button_2)
 *   - /soc/spi@58010000/radio@0
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pin_controller_48000000_S_gpio_48000800[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, 15, DEVICE_HANDLE_ENDS };

/* 5 : /soc/pin-controller@48000000/gpio@48000400:
 * Direct Dependencies:
 *   - (/soc/pin-controller@48000000)
 *   - /soc/rcc@58000000
 * Supported:
 *   - (/leds/led_0)
 *   - (/leds/led_1)
 *   - (/leds/led_2)
 *   - /soc/spi@40003800
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pin_controller_48000000_S_gpio_48000400[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, 13, DEVICE_HANDLE_ENDS };

/* 6 : /soc/pin-controller@48000000/gpio@48000000:
 * Direct Dependencies:
 *   - (/soc/pin-controller@48000000)
 *   - /soc/rcc@58000000
 * Supported:
 *   - (/gpio_keys/button_0)
 *   - (/gpio_keys/button_1)
 *   - /soc/spi@40013000
 *   - /soc/i2c@40005800/sht3xd@44
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pin_controller_48000000_S_gpio_48000000[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, 14, 18, DEVICE_HANDLE_ENDS };

/* 7 : /soc/serial@40008000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/soc/interrupt-controller@e000e100)
 *   - /soc/rcc@58000000
 *   - (/soc/pin-controller@48000000/lpuart1_rx_pa3)
 *   - (/soc/pin-controller@48000000/lpuart1_tx_pa2)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_serial_40008000[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 8 : /soc/rtc@40002800:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/soc/interrupt-controller@e000e100)
 *   - /soc/rcc@58000000
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_rtc_40002800[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 9 : /soc/i2c@40005c00:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/soc/interrupt-controller@e000e100)
 *   - /soc/rcc@58000000
 *   - (/soc/pin-controller@48000000/i2c3_scl_pb10)
 *   - (/soc/pin-controller@48000000/i2c3_sda_pb4)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_i2c_40005c00[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 10 : /soc/i2c@40005800:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/soc/interrupt-controller@e000e100)
 *   - /soc/rcc@58000000
 *   - (/soc/pin-controller@48000000/i2c2_scl_pa12)
 *   - (/soc/pin-controller@48000000/i2c2_sda_pa11)
 * Supported:
 *   - /soc/i2c@40005800/bme280@77
 *   - /soc/i2c@40005800/ds3231@68
 *   - /soc/i2c@40005800/sht3xd@44
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_i2c_40005800[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, 18, 17, 11, DEVICE_HANDLE_ENDS };

/* 11 : /soc/i2c@40005800/ds3231@68:
 * Direct Dependencies:
 *   - /soc/i2c@40005800
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_i2c_40005800_S_ds3231_68[] = { 10, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 12 : /soc/spi@58010000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/soc/interrupt-controller@e000e100)
 *   - /soc/rcc@58000000
 * Supported:
 *   - /soc/spi@58010000/radio@0
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_spi_58010000[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, 15, DEVICE_HANDLE_ENDS };

/* 13 : /soc/spi@40003800:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/soc/interrupt-controller@e000e100)
 *   - /soc/rcc@58000000
 *   - /soc/pin-controller@48000000/gpio@48000400
 *   - (/soc/pin-controller@48000000/spi2_miso_pb14)
 *   - (/soc/pin-controller@48000000/spi2_mosi_pb15)
 *   - (/soc/pin-controller@48000000/spi2_nss_pb12)
 *   - (/soc/pin-controller@48000000/spi2_sck_pb13)
 * Supported:
 *   - /soc/spi@40003800/sdhc@0
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_spi_40003800[] = { 5, 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, 16, DEVICE_HANDLE_ENDS };

/* 14 : /soc/spi@40013000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/soc/interrupt-controller@e000e100)
 *   - /soc/rcc@58000000
 *   - /soc/pin-controller@48000000/gpio@48000000
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_spi_40013000[] = { 1, 6, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 15 : /soc/spi@58010000/radio@0:
 * Direct Dependencies:
 *   - (/soc/interrupt-controller@e000e100)
 *   - /soc/spi@58010000
 *   - /soc/pin-controller@48000000/gpio@48000800
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_spi_58010000_S_radio_0[] = { 4, 12, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 16 : /soc/spi@40003800/sdhc@0:
 * Direct Dependencies:
 *   - /soc/spi@40003800
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_spi_40003800_S_sdhc_0[] = { 13, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 17 : /soc/i2c@40005800/bme280@77:
 * Direct Dependencies:
 *   - /soc/i2c@40005800
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_i2c_40005800_S_bme280_77[] = { 10, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 18 : /soc/i2c@40005800/sht3xd@44:
 * Direct Dependencies:
 *   - /soc/i2c@40005800
 *   - /soc/pin-controller@48000000/gpio@48000000
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_i2c_40005800_S_sht3xd_44[] = { 10, 6, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };
