/**
******************************************************************************
* @file    MICODefine.h 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide constant definition and type declaration for MICO
*          running.
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
extern "C" {
#endif

#include "Common.h"
#include "mico_app_context.h"

#define maxSsidLen          32
#define maxKeyLen           64
#define maxNameLen          32
#define maxIpLen            16


typedef enum  {
  /*All settings are in default state, module will enter easylink mode when powered on. 
  Press down Easyink button for 3 seconds (defined by RestoreDefault_TimeOut) to enter this mode */
  unConfigured,
  /*Module will enter easylink mode temperaly when powered on, and go back to allConfigured
    mode after time out (Defined by EasyLink_TimeOut), This mode is used for changing wlan
    settings if module is moved to a new wlan enviroment. Press down Easyink button to
    enter this mode */                
  wLanUnConfigured,
  /*Normal working mode, module use the configured settings to connecte to wlan, and run 
    user's threads*/
  allConfigured,
  /*If MFG_MODE_AUTO is enabled and MICO settings are erased (maybe a fresh device just has 
    been programed or MICO settings is damaged), module will enter MFG mode when powered on. */
  mfgConfigured
}Config_State_t;

/* Upgrade iamge should save this table to flash */
typedef struct  _boot_table_t {
  uint32_t start_address; // the address of the bin saved on flash.
  uint32_t length; // file real length
  uint8_t version[8];
  uint8_t type; // B:bootloader, P:boot_table, A:application, D: 8782 driver
  uint8_t upgrade_type; //u:upgrade, 
  uint16_t crc;
  uint8_t reserved[4];
}boot_table_t;

typedef struct _mico_sys_config_t
{
  /*Device identification*/
  char            name[maxNameLen];

  /*Wi-Fi configuration*/
  char            ssid[maxSsidLen];
  char            user_key[maxKeyLen]; 
  int             user_keyLength;
  char            key[maxKeyLen]; 
  int             keyLength;
  char            bssid[6];
  int             channel;
  SECURITY_TYPE_E security;

  /*Power save configuration*/
  bool            rfPowerSaveEnable;
  bool            mcuPowerSaveEnable;

  /*Local IP configuration*/
  bool            dhcpEnable;
  char            localIp[maxIpLen];
  char            netMask[maxIpLen];
  char            gateWay[maxIpLen];
  char            dnsServer[maxIpLen];

  /*EasyLink configuration*/
  Config_State_t  configured;
  uint8_t         easyLinkByPass;
  uint32_t        reserved;

  /*Services in MICO system*/
  bool            reserved2;
  bool            reserved3;

  /*Update seed number when configuration is changed*/
  int32_t         seed;
} mico_sys_config_t;

typedef struct _flash_configuration_t {

  /*OTA options*/
  boot_table_t             bootTable;
  /*MICO system core configuration*/
  mico_sys_config_t        micoSystemConfig;
  /*Application configuration*/
  application_config_t     appConfig; 
} flash_content_t;

typedef struct _current_mico_status_t 
{
  /*MICO system Running status*/
  char                  localIp[maxIpLen];
  char                  netMask[maxIpLen];
  char                  gateWay[maxIpLen];
  char                  dnsServer[maxIpLen];
  char                  mac[18];
  char                  rf_version[50];
} current_mico_status_t;

typedef struct _mico_Context_t
{
  /*Flash content*/
  flash_content_t           flashContentInRam;
  mico_mutex_t              flashContentInRam_mutex;

  /*Running status*/
  current_mico_status_t     micoStatus;
  current_app_status_t      appStatus;
} mico_Context_t;

#ifdef __cplusplus
} /*extern "C" */
#endif
