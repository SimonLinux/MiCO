/**
******************************************************************************
* @file    system.h
* @author  William Xu
* @version V1.0.0
* @date    22-July-2015
* @brief   This file provide function prototypes for for mico system.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2015 MXCHIP Inc.
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

#include "MICO.h"
#include "mico_system_context.h"

#ifdef __cplusplus
extern "C" {
#endif

#define system_log(M, ...) custom_log("Sys", M, ##__VA_ARGS__)
#define system_log_trace() custom_log_trace("Sys")

/* Define MICO service thread stack size */
#define STACK_SIZE_LOCAL_CONFIG_SERVER_THREAD   0x300
#define STACK_SIZE_LOCAL_CONFIG_CLIENT_THREAD   0x450
#define STACK_SIZE_NTP_CLIENT_THREAD            0x450
#define STACK_SIZE_MICO_SYSTEM_MONITOR_THREAD   0x300

#define EASYLINK_BYPASS_NO                      (0)
#define EASYLINK_BYPASS                         (1)
#define EASYLINK_SOFT_AP_BYPASS                 (2)


typedef enum
{
  eState_Normal,
  eState_Software_Reset,
  eState_Wlan_Powerdown,
  eState_Restore_default,
  eState_Standby,
} system_state_t;


OSStatus system_init( mico_Context_t** out_context );

OSStatus system_context_init( mico_Context_t** context_out );

OSStatus system_context_read( mico_Context_t** context_out );

OSStatus system_power_daemon_start( mico_Context_t * const inContext );

OSStatus system_notification_init( mico_Context_t * const inContext);

OSStatus system_network_daemen_start( mico_Context_t * const inContext );

OSStatus system_monitor_daemen_start( mico_Context_t * const inContext );

void system_power_perform( system_state_t new_state );

void system_connect_wifi_normal( mico_Context_t * const inContext);

void system_connect_wifi_fast( mico_Context_t * const inContext);

OSStatus system_easylink_wac_start( mico_Context_t * const inContext );

OSStatus system_easylink_start( mico_Context_t * const inContext );

OSStatus system_current_time_get( struct tm* time );


OSStatus MICOStartBonjourService        ( WiFi_Interface interface, mico_Context_t * const inContext );
OSStatus MICOStartConfigServer          ( mico_Context_t * const inContext );
OSStatus MICOStopConfigServer           ( void );
OSStatus MICOStartNTPClient             ( mico_Context_t * const inContext );

OSStatus MICORestoreDefault             ( mico_Context_t * const inContext );
OSStatus MICOReadConfiguration          ( mico_Context_t * const inContext );
OSStatus MICOUpdateConfiguration        ( mico_Context_t * const inContext );
#ifdef MFG_MODE_AUTO
OSStatus MICORestoreMFG                 ( mico_Context_t * const inContext );
#endif

void mico_mfg_test( mico_Context_t * const inContext );

#ifdef __cplusplus
} /*extern "C" */
#endif

