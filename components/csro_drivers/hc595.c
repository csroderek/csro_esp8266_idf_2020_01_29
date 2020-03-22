#include "hc595.h"

#define HC595_SRCLK GPIO_NUM_16
#define HC595_RCLK GPIO_NUM_12
#define HC595_SER GPIO_NUM_13
#define GPIO_HC595_PIN_SEL ((1ULL << HC595_SRCLK) | (1ULL << HC595_RCLK) | (1ULL << HC595_SER))

static uint8_t hc595_data = 0;

void csro_hc595_init(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_HC595_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
}

void csro_hc595_set_bit(uint8_t index, bool value)
{
    if (value == true)
    {
        hc595_data = hc595_data | (1 << index);
    }
    else
    {
        hc595_data = hc595_data & (~(1 << index));
    }
}

void csro_hc595_send_data(void)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        gpio_set_level(HC595_SER, ((hc595_data << i) & 0x80) == 0 ? 0 : 1);
        os_delay_us(1);
        gpio_set_level(HC595_SRCLK, 1);
        os_delay_us(1);
        gpio_set_level(HC595_SRCLK, 0);
    }
    gpio_set_level(HC595_RCLK, 1);
    os_delay_us(1);
    gpio_set_level(HC595_RCLK, 0);
}