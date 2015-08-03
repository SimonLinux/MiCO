/**
******************************************************************************
* @file    platform_config.h
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provides common configuration for current platform.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2014 MXCHIP Inc.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy 
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights 
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is furnished
*  to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
*  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************
*/ 

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
*                      Macros
******************************************************/

/******************************************************
*                    Constants
******************************************************/

#define HARDWARE_REVISION   "LPC5410x_1"
#define DEFAULT_NAME        "MiCOKit LPC5410x"
#define MODEL               "MiCOKit-LPC5410x"
#define Bootloader_REVISION  "V 0.1"

/* MICO RTOS tick rate in Hz */
#define MICO_DEFAULT_TICK_RATE_HZ                   (1000) 

/************************************************************************
 * Uncomment to disable watchdog. For debugging only */
#define MICO_DISABLE_WATCHDOG

/************************************************************************
 * Uncomment to disable standard IO, i.e. printf(), etc. */
//#define MICO_DISABLE_STDIO

/************************************************************************
 * Uncomment to disable MCU powersave API functions */
//#define MICO_DISABLE_MCU_POWERSAVE

/************************************************************************
 * Uncomment to enable MCU real time clock */
#define MICO_ENABLE_MCU_RTC

/************************************************************************
 * Restore default and start easylink after press down EasyLink button for 3 seconds. */
#define RestoreDefault_TimeOut                      (3000)


#define MCU_CLOCK_HZ            48000000

/******************************************************
 *  EMW1062 Options
 ******************************************************/
/*  Wi-Fi chip module */
#define EMW1062

/*  Wi-Fi GPIO0 pin is used for out-of-band interrupt */
//#define MICO_WIFI_OOB_IRQ_GPIO_PIN  ( 0 )


/*  Wi-Fi reset pin is present */
#define MICO_USE_WIFI_RESET_PIN

/*  Wi-Fi 32K pin is present */
#define MICO_USE_WIFI_32K_PIN

/*  WLAN Powersave Clock Source
 *  The WLAN sleep clock can be driven from one of two sources:
 *  1. MCO (MCU Clock Output) - default
 *     NOTE: Versions of BCM943362WCD4 up to and including P200 require a hardware patch to enable this mode
 *     - Connect STM32F205RGT6 pin 41 (PA8) to pin 44 (PA11)
 *  2. WLAN 32K internal oscillator (30% inaccuracy)
 *     - Comment the following directive : WICED_USE_WIFI_32K_CLOCK_MCO
 */
#define MICO_USE_WIFI_32K_CLOCK_MCO


/* Memory map */

#define MICO_FLASH_FOR_APPLICATION  MICO_INTERNAL_FLASH
#define APPLICATION_START_ADDRESS   (uint32_t)0x0000C000
#define APPLICATION_END_ADDRESS     (uint32_t)0x0007FFFF
#define APPLICATION_FLASH_SIZE      (APPLICATION_END_ADDRESS - APPLICATION_START_ADDRESS + 1) /* 480 bytes*/

#define MICO_FLASH_FOR_UPDATE       MICO_SPI_FLASH  /* Optional */
#define UPDATE_START_ADDRESS        (uint32_t)0x00050000 /* Optional */
#define UPDATE_END_ADDRESS          (uint32_t)0x000C3FFF /* Optional */
#define UPDATE_FLASH_SIZE           (UPDATE_END_ADDRESS - UPDATE_START_ADDRESS + 1) /* 320k bytes, optional*/

#define MICO_FLASH_FOR_BOOT         MICO_INTERNAL_FLASH
#define BOOT_START_ADDRESS          (uint32_t)0x00000000
#define BOOT_END_ADDRESS            (uint32_t)0x00007FFF
#define BOOT_VER_ADDRESS            (uint32_t)0x00007FE0
#define BOOT_FLASH_SIZE             (BOOT_END_ADDRESS - BOOT_START_ADDRESS + 1) /* 16k bytes*/

#define MICO_FLASH_FOR_DRIVER       MICO_SPI_FLASH
#define DRIVER_START_ADDRESS        (uint32_t)0x00002000
#define DRIVER_END_ADDRESS          (uint32_t)0x0004FFFF
#define DRIVER_FLASH_SIZE           (DRIVER_END_ADDRESS - DRIVER_START_ADDRESS + 1) /* 312k bytes*/

#define MICO_FLASH_FOR_PARA         MICO_SPI_FLASH
#define PARA_START_ADDRESS          (uint32_t)0x00000000
#define PARA_END_ADDRESS            (uint32_t)0x00000FFF
#define PARA_FLASH_SIZE             (PARA_END_ADDRESS - PARA_START_ADDRESS + 1)   /* 4k bytes*/

#define MICO_FLASH_FOR_EX_PARA      MICO_SPI_FLASH
#define EX_PARA_START_ADDRESS       (uint32_t)0x00001000
#define EX_PARA_END_ADDRESS         (uint32_t)0x00001FFF
#define EX_PARA_FLASH_SIZE          (EX_PARA_END_ADDRESS - EX_PARA_START_ADDRESS + 1)   /* 4k bytes*/

/******************************************************
*                   Enumerations
******************************************************/

/******************************************************
*                 Type Definitions
******************************************************/

/******************************************************
*                    Structures
******************************************************/

/******************************************************
*                 Global Variables
******************************************************/

/******************************************************
*               Function Declarations
******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif


