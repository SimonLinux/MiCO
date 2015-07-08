/**
  ******************************************************************************
  * @file    user_config.h 
  * @author  Eshen Wang
  * @version V1.0.0
  * @date    17-Mar-2015
  * @brief   This file contains user config for app.
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

#include "platform_config.h"


/*******************************************************************************
 *                              APP INFO
 ******************************************************************************/
//#define WECHAT_SUPPORT       // add this macro if the product supports wechat

/* product type */
#ifdef MICOKIT_3288
  #ifdef WECHAT_SUPPORT
    #define PRODUCT_ID                       "b574d4b8"  //  wechat support
    #define PRODUCT_KEY                      "3cb9d67f-bb69-45e1-b9b8-15c5b7eae304"
  #else
    #define PRODUCT_ID                       "d64f517c"
    #define PRODUCT_KEY                      "e935ef56-1d03-4432-9524-8d4a691a26ec"
  #endif
#elif MICOKIT_G55
  #ifdef WECHAT_SUPPORT
    #define PRODUCT_ID                       "8f362375"  //  wechat support
    #define PRODUCT_KEY                      "0a7a2b47-9766-4807-be14-d2c35919485a"
  #else
    #define PRODUCT_ID                       "b95b6242"
    #define PRODUCT_KEY                      "52731f33-edba-4483-9e4a-dc3859976c41"
  #endif
#endif

/*------------------------- for ota test -----------------*/
//#define OTA_TEST

#ifdef OTA_TEST
  #ifdef PRODUCT_ID
    #undef PRODUCT_ID
  #endif
  #ifdef PRODUCT_KEY
    #undef PRODUCT_KEY
  #endif
  #define PRODUCT_ID                       "6be15e13"  // OTA test product
  #define PRODUCT_KEY                      "87618cf2-29ea-4ad8-b51b-96e0f489643d"
#endif
/*---------------------------------------------------------*/

#define SERIAL_NUMBER                      "1507081602"
#define FIRMWARE_REVISION                  HARDWARE_REVISION"@"SERIAL_NUMBER

#define DEFAULT_ROM_VERSION                FIRMWARE_REVISION
#define DEFAULT_DEVICE_NAME                MODEL   // device name upload to cloud defined in platform_config.h

#define APP_INFO                           MODEL" Demo based on MICO OS, fw version: "FIRMWARE_REVISION","
#define PROTOCOL                           "com.mxchip.micokit"


/*******************************************************************************
 *                             CONNECTING
 ******************************************************************************/
/* Wi-Fi configuration mode */
#ifdef WECHAT_SUPPORT
  #define MICO_CONFIG_MODE                 CONFIG_MODE_AIRKISS  // may be hybrid mode later
#else
  #define MICO_CONFIG_MODE                 CONFIG_MODE_EASYLINK
#endif

/* MICO cloud service type */
#define MICO_CLOUD_TYPE                    CLOUD_FOGCLOUD

// disalbe FogCloud OTA check when system start
#define DISABLE_FOGCLOUD_OTA_CHECK


/*******************************************************************************
 *                             RESOURCES
 ******************************************************************************/
#define STACK_SIZE_USER_MAIN_THREAD         0x800
#define STACK_SIZE_USER_MSG_HANDLER_THREAD  0x800
#define STACK_SIZE_NOTIFY_THREAD            0x800
#define MICO_PROPERTIES_NOTIFY_INTERVAL_MS  1000


/*User provided configurations*/
#define CONFIGURATION_VERSION               0x00000001 // if default configuration is changed, update this number
   
#endif  // __USER_CONFIG_H_
