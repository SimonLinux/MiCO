/**
******************************************************************************
* @file    main.c 
* @author  William Xu
* @version V1.0.0
* @date    21-May-2015
* @brief   First MiCO application to say hello world!
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

#include "MiCO.h" 

#define power_log(M, ...) custom_log("PM", M, ##__VA_ARGS__)

#define POWER_MEASURE_PROGRAM RTOS_INITIALIZED
#define MCU_POWERSAVE_ENABLED 0

#define RTOS_INITIALIZED          1
#define RTOS_FULL_CPU_THREAD      2
#define RTOS_FLASH_READ           3
#define RTOS_FLASH_WRITE          4
#define RTOS_FLASH_WRITE          5
#define RTOS_WLAN_INITIALIZED     6
#define RTOS_WLAN_SOFT_AP         7
#define RTOS_WLAN_EASYLINK        8
#define RTOS_WLAN_CONNECT         9

#if POWER_MEASURE_PROGRAM == RTOS_INITIALIZED
int application_start( void )
{
#if MCU_POWERSAVE_ENABLED
  MicoMcuPowerSaveConfig(true);
#endif
  host_platform_init( );
  host_platform_power_wifi( true );
    
  power_log( "Power measure program: RTOS initialized and no application is running" );
  mico_rtos_delete_thread( NULL );
  return 0;
}
#endif

#if POWER_MEASURE_PROGRAM == RTOS_FULL_CPU_THREAD
int application_start( void )
{
#if MCU_POWERSAVE_ENABLED
  MicoMcuPowerSaveConfig(true);
#endif
  host_platform_init( );
  host_platform_power_wifi( true );
    
  while(1);
  return 0;
}
#endif

#if POWER_MEASURE_PROGRAM == RTOS_WLAN_INITIALIZED
int application_start( void )
{
#if MCU_POWERSAVE_ENABLED
  MicoMcuPowerSaveConfig(true);
#endif
  power_log( "Power measure program: RTOS and wlan initialized and no application is running" );
  MicoInit( );
    
  mico_rtos_delete_thread( NULL );
  return 0;
}
#endif

#if POWER_MEASURE_PROGRAM == RTOS_WLAN_SOFY_AP
int application_start( void )
{
  network_InitTypeDef_st wNetConfig;
#if MCU_POWERSAVE_ENABLED
  MicoMcuPowerSaveConfig(true);
#endif
  power_log( "Power measure program: RTOS and wlan initialized and setup soft ap" );
  MicoInit( );
  
  memset(&wNetConfig, 0, sizeof(network_InitTypeDef_st));
  wNetConfig.wifi_mode = Soft_AP;
  snprintf(wNetConfig.wifi_ssid, 32, "EasyLink_PM" );
  strcpy((char*)wNetConfig.wifi_key, "");
  strcpy((char*)wNetConfig.local_ip_addr, "10.10.10.1");
  strcpy((char*)wNetConfig.net_mask, "255.255.255.0");
  strcpy((char*)wNetConfig.gateway_ip_addr, "10.10.10.1");
  wNetConfig.dhcpMode = DHCP_Server;
  micoWlanStart(&wNetConfig);
    
  mico_rtos_delete_thread( NULL );
  return 0;
}
#endif

#if POWER_MEASURE_PROGRAM == RTOS_WLAN_EASYLINK
int application_start( void )
{
  network_InitTypeDef_st wNetConfig;
#if MCU_POWERSAVE_ENABLED
  MicoMcuPowerSaveConfig(true);
#endif
  power_log( "Power measure program: RTOS and wlan initialized and start easylink" );
  MicoInit( );
  
  micoWlanStartEasyLinkPlus( MICO_NEVER_TIMEOUT );
    
  mico_rtos_delete_thread( NULL );
  return 0;
}
#endif

#if POWER_MEASURE_PROGRAM == RTOS_WLAN_CONNECT
int application_start( void )
{
  network_InitTypeDef_adv_st wNetConfig;
#if MCU_POWERSAVE_ENABLED
  MicoMcuPowerSaveConfig(true);
#endif
  power_log( "Power measure program: RTOS and wlan initialized and connect wlan, wait 10sec to measure" );
  MicoInit( );
   micoWlanEnablePowerSave();
  memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_adv_st));
  
  strncpy((char*)wNetConfig.ap_info.ssid, "William Xu", 32);
  wNetConfig.ap_info.security = SECURITY_TYPE_AUTO;
  strncpy((char*)wNetConfig.key, "mx099555", 64);
  wNetConfig.key_len = 8;
  wNetConfig.dhcpMode = true;
  wNetConfig.wifi_retry_interval = 100;
  micoWlanStartAdv(&wNetConfig);
  power_log("connect to %s.....", wNetConfig.ap_info.ssid);
    
  mico_rtos_delete_thread( NULL );
  return 0;
}
#endif



