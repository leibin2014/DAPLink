/**
 * @file    gpio.c
 * @brief   
 *
 * DAPLink Interface Firmware
 * Copyright (c) 2009-2016, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "LPC11Uxx.h"
#include "RTL.h"
#include "gpio.h"
#include "compiler.h"
#include "target_reset.h"
#include "IO_Config.h"

// taken code from the Nxp App Note AN11305
/* This data must be global so it is not read from the stack */
typedef void (*IAP)(uint32_t [], uint32_t []);
static IAP iap_entry = (IAP)0x1fff1ff1;
static uint32_t command[5], result[4];
#define init_msdstate() *((uint32_t *)(0x10000054)) = 0x0

/* This function resets some microcontroller peripherals to reset
 * hardware configuration to ensure that the USB In-System Programming module
 * will work properly. It is normally called from reset and assumes some reset
 * configuration settings for the MCU.
 * Some of the peripheral configurations may be redundant in your specific
 * project.
 */
void ReinvokeISP(void)
{
    /* make sure USB clock is turned on before calling ISP */
    LPC_SYSCON->SYSAHBCLKCTRL |= 0x04000;
    /* make sure 32-bit Timer 1 is turned on before calling ISP */
    LPC_SYSCON->SYSAHBCLKCTRL |= 0x00400;
    /* make sure GPIO clock is turned on before calling ISP */
    LPC_SYSCON->SYSAHBCLKCTRL |= 0x00040;
    /* make sure IO configuration clock is turned on before calling ISP */
    LPC_SYSCON->SYSAHBCLKCTRL |= 0x10000;
    /* make sure AHB clock divider is 1:1 */
    LPC_SYSCON->SYSAHBCLKDIV = 1;
    /* Send Reinvoke ISP command to ISP entry point*/
    command[0] = 57;
    init_msdstate();					 /* Initialize Storage state machine */
    /* Set stack pointer to ROM value (reset default) This must be the last
     * piece of code executed before calling ISP, because most C expressions
     * and function returns will fail after the stack pointer is changed.
     */
    __set_MSP(*((volatile uint32_t *)0x00000000));
    /* Enter ISP. We call "iap_entry" to enter ISP because the ISP entry is done
     * through the same command interface as IAP.
     */
    iap_entry(command, result);
    // Not supposed to come back!
}

static void busy_wait(uint32_t cycles)
{
    volatile uint32_t i;
    i = cycles;

    while (i > 0) {
        i--;
    }
}

void gpio_init(void)
{
    // enable clock for GPIO port 0
    LPC_SYSCON->SYSAHBCLKCTRL |= (1UL << 6);
    
	//config the CFG 4 pins as input
    PIN_A0_IOCON |= PIN_A0_IOCON_INIT;
    LPC_GPIO->DIR[PIN_A0_PORT] &= ~PIN_A0;
    PIN_A1_IOCON |= PIN_A1_IOCON_INIT;
    LPC_GPIO->DIR[PIN_A1_PORT] &= ~PIN_A1;
    PIN_A2_IOCON |= PIN_A2_IOCON_INIT;
    LPC_GPIO->DIR[PIN_A2_PORT] &= ~PIN_A2;
    PIN_A3_IOCON |= PIN_A3_IOCON_INIT;
    LPC_GPIO->DIR[PIN_A3_PORT] &= ~PIN_A3;
    PIN_A4_IOCON |= PIN_A4_IOCON_INIT;
    LPC_GPIO->DIR[PIN_A4_PORT] &= ~PIN_A4;
    PIN_A5_IOCON |= PIN_A5_IOCON_INIT;
    LPC_GPIO->DIR[PIN_A5_PORT] &= ~PIN_A5;
    PIN_A6_IOCON |= PIN_A6_IOCON_INIT;
    LPC_GPIO->DIR[PIN_A6_PORT] &= ~PIN_A6;
    PIN_A7_IOCON |= PIN_A7_IOCON_INIT;
    LPC_GPIO->DIR[PIN_A7_PORT] &= ~PIN_A7;
    PIN_A8_IOCON |= PIN_A8_IOCON_INIT;
    LPC_GPIO->DIR[PIN_A8_PORT] &= ~PIN_A8;
    PIN_A9_IOCON |= PIN_A9_IOCON_INIT;
    LPC_GPIO->DIR[PIN_A9_PORT] &= ~PIN_A9;
    PIN_A10_IOCON |= PIN_A10_IOCON_INIT;
    LPC_GPIO->DIR[PIN_A10_PORT] &= ~PIN_A10;
    PIN_A11_IOCON |= PIN_A11_IOCON_INIT;
    LPC_GPIO->DIR[PIN_A11_PORT] &= ~PIN_A11;
    
    // configure GPIO-LED as output
    // DAP led (green)
    PIN_DAP_LED_IOCON |= PIN_DAP_LED_IOCON_INIT;
    LPC_GPIO->SET[PIN_DAP_LED_PORT] = PIN_DAP_LED;
    LPC_GPIO->DIR[PIN_DAP_LED_PORT] |= PIN_DAP_LED;
    // Serial LED (blue)
    PIN_CDC_LED_IOCON |= PIN_CDC_LED_IOCON_INIT;
    LPC_GPIO->SET[PIN_CDC_LED_PORT] = PIN_CDC_LED;
    LPC_GPIO->DIR[PIN_CDC_LED_PORT] |= PIN_CDC_LED;
    
    // configure Button(s) as input   
    PIN_RESET_IN_FWRD_IOCON |= PIN_RESET_IN_FWRD_IOCON_INIT;
    LPC_GPIO->DIR[PIN_RESET_IN_FWRD_PORT] &= ~PIN_RESET_IN_FWRD;
    
    /* Enable AHB clock to the FlexInt, GroupedInt domain. */
    LPC_SYSCON->SYSAHBCLKCTRL |= ((1 << 19) | (1 << 23) | (1 << 24));
    // Give the cap on the reset button time to charge
    busy_wait(10000);

    if (gpio_get_sw_reset() == 0) {
        IRQn_Type irq;
        // Disable SYSTICK timer and interrupt before calling into ISP
        SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk);

        // Disable all nvic interrupts
        for (irq = (IRQn_Type)0; irq < (IRQn_Type)32; irq++) {
            NVIC_DisableIRQ(irq);
            NVIC_ClearPendingIRQ(irq);
        }

        ReinvokeISP();
    }
}

void gpio_set_hid_led(gpio_led_state_t state)
{
    if (!state) {
        LPC_GPIO->SET[PIN_DAP_LED_PORT] = PIN_DAP_LED;
    } else {
        LPC_GPIO->CLR[PIN_DAP_LED_PORT] = PIN_DAP_LED;
    }
}

void gpio_set_cdc_led(gpio_led_state_t state)
{
    if (!state) {
        LPC_GPIO->SET[PIN_CDC_LED_PORT] = PIN_CDC_LED;
    } else {
        LPC_GPIO->CLR[PIN_CDC_LED_PORT] = PIN_CDC_LED;
    }
}

static uint32_t gpio_get_config0(void)
{
	return ((LPC_GPIO->PIN[PIN_A0_PORT] & PIN_A0) != 0x00)? PIN_HIGH:PIN_LOW;
}

static uint32_t gpio_get_config1(void)
{
	return ((LPC_GPIO->PIN[PIN_A1_PORT] & PIN_A1) != 0x00)? PIN_HIGH:PIN_LOW;
}

static uint32_t gpio_get_config2(void)
{
	return ((LPC_GPIO->PIN[PIN_A2_PORT] & PIN_A2) != 0x00)? PIN_HIGH:PIN_LOW;
}

static uint32_t gpio_get_config3(void)
{
	return ((LPC_GPIO->PIN[PIN_A3_PORT] & PIN_A3) != 0x00)? PIN_HIGH:PIN_LOW;
}

uint8_t gpio_get_config(uint8_t cfgid)
{
    uint8_t rc = 0;
	switch (cfgid)
	{
        case PIN_CONFIG_DT01:
            rc = gpio_get_config0();
            break;

		case PIN_CONFIG_1:
			rc = gpio_get_config1();
			break;

		case PIN_CONFIG_2:
			rc = gpio_get_config2();
            break;

		case PIN_CONFIG_3:
			rc = gpio_get_config3();
			break;
    }

	return rc;
}

uint16_t gpio_all_pins(void)
{
    uint16_t rc = 0;

    uint16_t a0 = ((LPC_GPIO->PIN[PIN_A0_PORT] & PIN_A0) != 0x00);
    uint16_t a1 = ((LPC_GPIO->PIN[PIN_A1_PORT] & PIN_A1) != 0x00);
    uint16_t a2 = ((LPC_GPIO->PIN[PIN_A2_PORT] & PIN_A2) != 0x00);
    uint16_t a3 = ((LPC_GPIO->PIN[PIN_A3_PORT] & PIN_A3) != 0x00);
    uint16_t a4 = ((LPC_GPIO->PIN[PIN_A4_PORT] & PIN_A4) != 0x00);
    uint16_t a5 = ((LPC_GPIO->PIN[PIN_A5_PORT] & PIN_A5) != 0x00);
    uint16_t a6 = ((LPC_GPIO->PIN[PIN_A6_PORT] & PIN_A6) != 0x00);
    uint16_t a7 = ((LPC_GPIO->PIN[PIN_A7_PORT] & PIN_A7) != 0x00);
    uint16_t a8 = ((LPC_GPIO->PIN[PIN_A8_PORT] & PIN_A8) != 0x00);
    uint16_t a9 = ((LPC_GPIO->PIN[PIN_A9_PORT] & PIN_A9) != 0x00);
    uint16_t a10 = ((LPC_GPIO->PIN[PIN_A10_PORT] & PIN_A10) != 0x00);
    uint16_t a11 = ((LPC_GPIO->PIN[PIN_A11_PORT] & PIN_A11) != 0x00);

    rc = a0 + (a1<<1) + (a2<<2) + (a3<<3) + (a4<<4) + (a5<<5) + (a6<<6) + (a7<<7) + (a8<<8) + (a9<<9) + (a10<<10) + (a11<<11);

    return rc;
}

uint8_t gpio_get_sw_reset(void)
{
    static uint8_t last_reset_forward_pressed = 0;
    uint8_t reset_forward_pressed;
    uint8_t reset_pressed;
    reset_forward_pressed = LPC_GPIO->PIN[PIN_RESET_IN_FWRD_PORT] & PIN_RESET_IN_FWRD ? 0 : 1;

    // Forward reset if the state of the button has changed
    //    This must be done on button changes so it does not interfere
    //    with other reset sources such as programming or CDC Break
    if (last_reset_forward_pressed != reset_forward_pressed) {
        if (reset_forward_pressed) {
            target_set_state(RESET_HOLD);
        } else {
            target_set_state(RESET_RUN);
        }

        last_reset_forward_pressed = reset_forward_pressed;
    }

    reset_pressed = reset_forward_pressed ;
    return !reset_pressed;
}

void target_forward_reset(bool assert_reset)
{
    // Do nothing - reset is forwarded in gpio_get_sw_reset
}
