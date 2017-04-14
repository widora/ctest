/* It's a test for control the GPIO output & input, then use IIC control 
* IMIO Inc
* Author : zyc
* Date    : 2017-04-11
*/
#include "mem_gpio.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


uint8_t* gpio_mmap_reg = NULL;
int gpio_mmap_fd = 0;


/**
 *  mem remap
 * @return
 */
int gpio_mmap(void)
{
    if ((gpio_mmap_fd = open(MMAP_PATH, O_RDWR)) < 0)
    {
        fprintf(stderr, "unable to open mmap file");
        return -1;
    }

    gpio_mmap_reg = (uint8_t *) mmap(NULL, 1024, PROT_READ | PROT_WRITE,
                                     MAP_FILE | MAP_SHARED, gpio_mmap_fd, 0x10000000);
    if (gpio_mmap_reg == MAP_FAILED)
    {
        perror("foo");
        fprintf(stderr, "failed to mmap");
        gpio_mmap_reg = NULL;
        close(gpio_mmap_fd);
        return -1;
    }

    return 0;
}

/**
 * read pin input value
 * @param pin
 * @return
 */
int mt76x8_gpio_get_pin(int pin)
{
    uint32_t tmp = 0;

    /* MT7621, MT7628 */
    if (pin <= 31) 
    {
        tmp = *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIODATA);
        tmp = (tmp >> pin) & 1u;
    } 
    else if (pin <= 63) 
    {
        tmp = *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO6332DATA);
        tmp = (tmp >> (pin-32)) & 1u;
    } 
    else if (pin <= 95) 
    {
        tmp = *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO9564DATA);
        tmp = (tmp >> (pin-64)) & 1u;
        tmp = (tmp >> (pin-24)) & 1u;
    }
    return tmp;
}

/**
 * set gpio pin direction
 * @param pin number
 * @param is_output 1 == output, 0 == input
 */
void mt76x8_gpio_set_pin_direction(int pin, int is_output)
{
    uint32_t tmp;

    /* MT7621, MT7628 */
    if (pin <= 31) 
    {
        tmp = *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIODIR);
        if (is_output)
            tmp |=  (1u << pin);
        else
            tmp &= ~(1u << pin);
        *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIODIR) = tmp;
    } 
    else if (pin <= 63) 
    {
        tmp = *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO6332DIR);
        if (is_output)
            tmp |=  (1u << (pin-32));
        else
            tmp &= ~(1u << (pin-32));
        *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO6332DIR) = tmp;
    } 
    else if (pin <= 95) 
    {
        tmp = *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO9564DIR);
        if (is_output)
            tmp |=  (1u << (pin-64));
        else
            tmp &= ~(1u << (pin-64));
        *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO9564DIR) = tmp;
    }
}


/**
 * set gpio value
 * @param pin gpio number
 * @param value value, 1 == high, 0 == low
 */
void mt76x8_gpio_set_pin_value(int pin, int value)
{
    uint32_t tmp;

    /* MT7621, MT7628 */
    if (pin <= 31) 
    {
        tmp = (1u << pin);
        if (value)
            *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIOSET) = tmp;
        else
            *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIORESET) = tmp;
    } 
    else if (pin <= 63) 
    {
        tmp = (1u << (pin-32));
        if (value)
            *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO6332SET) = tmp;
        else
            *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO6332RESET) = tmp;
    } 
    else if (pin <= 95) 
    {
        tmp = (1u << (pin-64));
        if (value)
            *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO9564SET) = tmp;
        else
            *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO9564RESET) = tmp;
    }
}

void close_gpio()
{
    close(gpio_mmap_fd);
}

