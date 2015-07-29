/**
  ******************************************************************************
  * @file    user_config.h 
  * @author  Eshen Wang
  * @version V1.0.0
  * @date    14-May2015
  * @brief   User configuration file.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, MXCHIP Inc. SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2014 MXCHIP Inc.</center></h2>
  ******************************************************************************
  */ 

#ifndef __USER_CONFIG_H_
#define __USER_CONFIG_H_


/*******************************************************************************
 *                             APP INFO
 ******************************************************************************/

/*------------------------------ product -------------------------------------*/
#ifdef MICOKIT_3288
// wes' product, replace it with your own product
  #define PRODUCT_ID                   "e0ef908d"
  #define PRODUCT_KEY                  "86cc4e1b-b581-442f-bc73-e6d91df77dd8"
#elif  MICOKIT_3165
  #define PRODUCT_ID                   "e70ae531"
  #define PRODUCT_KEY                  "292ce4ad-a3e4-41bb-9598-27aafd3d0fc4"
#else

#endif

/*------------------------------ version -------------------------------------*/
#define SERIAL_NUMBER                  "1507231145"
#define FIRMWARE_REVISION              HARDWARE_REVISION"@"SERIAL_NUMBER

#define DEFAULT_ROM_VERSION            FIRMWARE_REVISION
#define DEFAULT_DEVICE_NAME            MODEL   // device name upload to cloud defined in platform_config.h

#define APP_INFO                       MODEL" Wechat humiture && RGB LED Demo based on MICO OS, fw version: "FIRMWARE_REVISION","
#define PROTOCOL                       "com.fogcloud.wechat"


/*******************************************************************************
 *                             CONNECTING
 ******************************************************************************/

/* Wi-Fi configuration mode */
#define MICO_CONFIG_MODE              CONFIG_MODE_AIRKISS

/* MICO cloud service */
#define MICO_CLOUD_TYPE               CLOUD_FOGCLOUD

/* Firmware update check
 * If need to check new firmware on server after wifi on, comment out this macro
 */
#define DISABLE_FOGCLOUD_OTA_CHECK


/*******************************************************************************
 *                             RESOURCES
 ******************************************************************************/

/* stack size of the user_main thread */
#define STACK_SIZE_USER_MAIN_THREAD    0x800

#define STACK_SIZE_USER_DOWNSTREAM_THREAD 0x400
#define STACK_SIZE_USER_UPSTREAM_THREAD   0x400


/* User provided configurations seed
 * If user configuration(params in flash) is changed, update this number to
 * indicate the bootloader to clean params in flash next time restart.
 */
#define CONFIGURATION_VERSION          0x00000003


#endif  // __USER_CONFIG_H_

