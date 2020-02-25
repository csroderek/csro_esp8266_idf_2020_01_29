#include "aw9523b.h"
#include "driver/i2c.h"

#ifdef AW9523B_SZ

#define I2C_MASTER_SCL_IO 4
#define I2C_MASTER_SDA_IO 2
#define I2C_MASTER_NUM I2C_NUM_0

static SemaphoreHandle_t i2c_mutex;

static void i2c_master_aw9523b_write(uint8_t reg_addr, uint8_t value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0xB6, 1);
    i2c_master_write_byte(cmd, reg_addr, 1);
    i2c_master_write_byte(cmd, value, 1);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
}

static uint8_t i2c_master_aw9523b_read(uint8_t reg_addr)
{
    uint8_t data = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0xB6, 1);
    i2c_master_write_byte(cmd, reg_addr, 1);
    i2c_master_stop(cmd);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0xB7, 1);
    i2c_master_read_byte(cmd, &data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return data;
}

void csro_aw9523b_init_sz(void)
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = 0;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = 0;
    i2c_driver_install(I2C_MASTER_NUM, conf.mode);
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_master_aw9523b_write(0x12, 0x00); // Set P0 as Led mode
    i2c_master_aw9523b_write(0x05, 0xFF); // Set P1 as GPIO_input mode
    i2c_mutex = xSemaphoreCreateMutex();
}

void csro_set_led_sz(uint8_t led_num, uint8_t red, uint8_t blue)
{
    if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE)
    {
        i2c_master_aw9523b_write(0x24 + led_num * 2, red);
        i2c_master_aw9523b_write(0x25 + led_num * 2, blue);
        xSemaphoreGive(i2c_mutex);
    }
}

void csro_get_key_sz(uint8_t *keys)
{
    if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE)
    {
        uint8_t value = i2c_master_aw9523b_read(0x01);
        keys[0] = (value & 0x01) == 0 ? 0 : 1;
        keys[1] = (value & 0x02) == 0 ? 0 : 1;
        keys[2] = (value & 0x04) == 0 ? 0 : 1;
        keys[3] = (value & 0x08) == 0 ? 0 : 1;
        xSemaphoreGive(i2c_mutex);
    }
}

#endif