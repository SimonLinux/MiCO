/**
  ******************************************************************************
  * @file    user_config.h 
  * @author  Eshen Wang
  * @version V1.0.0
  * @date    14-May2015
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


/*******************************************************************************
 *                             APP INFO
 ******************************************************************************/
#define APP_INFO                           "FogCloud Demo based on MICO OS"

#define FIRMWARE_REVISION                  "MICO_FOG_1_0"
#define SERIAL_NUMBER                      "20150619"
#define PROTOCOL                           "com.mxchip.fog"

/* product type */
// NOTE: use your own product id/key create in FogCloud developper center(www.easylink.io).
#ifdef EMW3162
  #define PRODUCT_ID                        "062bfffc"
  #define PRODUCT_KEY                       "3cd53e02-5739-4b5a-a5e8-62fefb787598"
#elif EMW3165
  #define PRODUCT_ID                        "35036c4f"
  #define PRODUCT_KEY                       "2e9d0e7d-7f8a-478d-a2ed-9894c71e0f95"
#elif MICOKIT_3288
  #define PRODUCT_ID                        "28e6d490"
  #define PRODUCT_KEY                       "5f4ad7eb-4c71-41e5-91ba-8e3a8dfb2ec4"
#elif MICOKIT_3088
  #define PRODUCT_ID                        "52517d96"
  #define PRODUCT_KEY                       "b1ac6654-17ff-4b57-a722-248ac259f129"
#elif EMW5088
  #define PRODUCT_ID                        "52517d96"
  #define PRODUCT_KEY                       "b1ac6654-17ff-4b57-a722-248ac259f129"
#else

#endif

#define DEFAULT_ROM_VERSION                "v1.0.0"
#define DEFAULT_DEVICE_NAME                "MiCO_FOG"      // device name upload to cloud
#define DEFAULT_MANUFACTURER               "MXCHIP"       // device manufacturer

   
/*******************************************************************************
 *                             CONNECTING
 ******************************************************************************/
/* Wi-Fi configuration mode */
#define MICO_CONFIG_MODE                   CONFIG_MODE_EASYLINK_WITH_SOFTAP

/* MICO cloud service */
#define MICO_CLOUD_TYPE                    CLOUD_FOGCLOUD
   
// if need to auto activate afger first time configure, add this macro
//#define ENABLE_FOGCLOUD_AUTO_ACTIVATE

// if not need to check new firmware on server when system start, add this macro
//#define DISABLE_FOGCLOUD_OTA_CHECK

   
/*******************************************************************************
 *                             RESOURCES
 ******************************************************************************/
#define STACK_SIZE_USER_MAIN_THREAD         0x800
#define STACK_SIZE_NOTIFY_THREAD            0x800
#define MICO_PROPERTIES_NOTIFY_INTERVAL_MS  1000
   
/*User provided configurations*/
#define CONFIGURATION_VERSION               0x00000001 // if default configuration is changed, update this number

   
#endif  // __USER_CONFIG_H_
