/**
******************************************************************************
* @file    MFi_WAC.h 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide header file for start a Apple WAC (wireless accessory
*          configuration) function thread.
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

#include "common.h"
#include "system.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef system_state_t   m_system_state_t;
typedef system_context_t mico_Context_t;

typedef enum{
  /* MICO system defined notifications */
  CONFIG_BY_NONE,
  CONFIG_BY_EASYLINK_V2,         
  CONFIG_BY_EASYLINK_PLUS,        
  CONFIG_BY_EASYLINK_MINUS,          
  CONFIG_BY_AIRKISS,             
  CONFIG_BY_SOFT_AP,  
  CONFIG_BY_WAC,          
} mico_config_source_t;


/** Structure to hold information about a system monitor item */
typedef struct
{
    uint32_t last_update;              /**< Time of the last system monitor update */
    uint32_t longest_permitted_delay;  /**< Longest permitted delay between checkins with the system monitor */
} m_system_monitor_t;


typedef notify_wlan_t WiFiEvent;

typedef enum{
  /* MICO system defined notifications */
  mico_notify_WIFI_SCAN_COMPLETED,          //void (*function)(ScanResult *pApList, void* arg);
  mico_notify_WIFI_STATUS_CHANGED,          //void (*function)(WiFiEvent status, void* arg);
  mico_notify_WiFI_PARA_CHANGED,            //void (*function)(apinfo_adv_t *ap_info, char *key, int key_len, void* arg);
  mico_notify_DHCP_COMPLETED,               //void (*function)(IPStatusTypedef *pnet, void* arg);
  mico_notify_EASYLINK_WPS_COMPLETED,       //void (*function)(network_InitTypeDef_st *nwkpara, void* arg);
  mico_notify_EASYLINK_GET_EXTRA_DATA,      //void (*function)(int datalen, char*data, void* arg);
  mico_notify_TCP_CLIENT_CONNECTED,         //void (*function)(char *str, int len, void* arg);
  mico_notify_DNS_RESOLVE_COMPLETED,        //void (*function)(char *str, int len, void* arg);
  mico_notify_SYS_WILL_POWER_OFF,           //void (*function)(void* arg);
  mico_notify_WIFI_CONNECT_FAILED,          //void join_fail(OSStatus err, void* arg);
  mico_notify_WIFI_SCAN_ADV_COMPLETED,      //void (*function)(ScanResult_adv *pApList, void* arg);
  mico_notify_WIFI_Fatal_ERROR,             //void (*function)(void* arg);
  mico_notify_Stack_Overflow_ERROR,         //void (*function)(char *taskname, void* arg);
 
  /* User defined notifications */

} mico_notify_types_t;

/*****************************************************************************/
/** @defgroup system       MiCO System functions
 *
 *  MiCO System provide a basic framework for application
 */
/*****************************************************************************/

/*****************************************************************************/
/** @addtogroup system_context       System Core Storage
 *  @ingroup system
 *
 *  MICO System Core Storage Functions
 *
 *  @{
 */
/*****************************************************************************/

/* System core data managment, should initialized before other system functions */

mico_Context_t* m_system_context_init( uint32_t size_of_user_data );

mico_Context_t* m_system_context_get( void );

void* m_system_context_get_user_data( mico_Context_t* const in_context );

OSStatus m_system_context_restore( mico_Context_t* const in_context );

OSStatus m_system_context_update( mico_Context_t* const in_context );

/** @} */
/*****************************************************************************/
/** @addtogroup system       System Framework
 *  @ingroup system
 *
 *  MICO System Framework Functions
 *
 *  @{
 */
/*****************************************************************************/

/* mico system framework initialize */
OSStatus m_system_init( mico_Context_t* const in_context );

/* System config delegates */
void m_system_delegate_config_will_start( void );

void m_system_delegate_config_will_stop( void );

void m_system_delegate_config_recv_ssid ( void );

void m_system_delegate_config_success( mico_config_source_t source );

OSStatus m_system_delegate_config_recv_auth_data( char * userInfo );

/** @} */
/*****************************************************************************/
/** @addtogroup system       System Minotor
 *  @ingroup system
 *
 *  MICO System Monitor Functions
 *
 *  @{
 */
/*****************************************************************************/

/* System monitor functions*/
OSStatus m_system_monitor_daemen_start( void );

OSStatus m_system_monitor_update ( m_system_monitor_t* system_monitor, uint32_t permitted_delay );

OSStatus m_system_monitor_register( m_system_monitor_t* system_monitor, uint32_t initial_permitted_delay );

/** @} */
/*****************************************************************************/
/** @addtogroup system       System Power
 *  @ingroup system
 *
 *  MICO System Power Management
 *
 *  @{
 */
/*****************************************************************************/
/* Start power management daemon */
OSStatus m_system_power_daemon_start( mico_Context_t* const in_context );
/* Perform a system power change */
OSStatus m_system_power_perform( mico_Context_t* const in_context, m_system_state_t new_state );

/** @} */
/*****************************************************************************/
/** @addtogroup system       System Notify
 *  @ingroup system
 *
 *  MICO System Notification Functions
 *
 *  @{
 */
/*****************************************************************************/
/* mico nitifictions */
OSStatus m_system_notify_register  ( mico_notify_types_t notify_type, void* functionAddress, void* arg );

OSStatus m_system_notify_remove       ( mico_notify_types_t notify_type, void *functionAddress );

OSStatus m_system_notify_remove_all    ( mico_notify_types_t notify_type);


/* Read current system clock */
OSStatus m_system_current_time_get( struct tm* time );



#ifdef __cplusplus
} /*extern "C" */
#endif
