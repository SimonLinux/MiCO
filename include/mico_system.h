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
#include "json_c/json.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef system_state_t   mico_system_state_t;
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
} mico_system_monitor_t;


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

mico_Context_t* mico_system_context_init( uint32_t size_of_user_data );

mico_Context_t* mico_system_context_get( void );

void* mico_system_context_get_user_data( mico_Context_t* const in_context );

OSStatus mico_system_context_restore( mico_Context_t* const in_context );

OSStatus mico_system_context_update( mico_Context_t* const in_context );

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
OSStatus mico_system_init( mico_Context_t* const in_context );

/* System config delegates */
void mico_system_delegate_config_will_start( void );

void mico_system_delegate_config_will_stop( void );

void mico_system_delegate_soft_ap_will_start( void );

void mico_system_delegate_config_recv_ssid ( char *ssid, char *key );

void mico_system_delegate_config_success( mico_config_source_t source );

OSStatus mico_system_delegate_config_recv_auth_data( char * userInfo );

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
OSStatus mico_system_monitor_daemen_start( void );

OSStatus mico_system_monitor_update ( mico_system_monitor_t* system_monitor, uint32_t permitted_delay );

OSStatus mico_system_monitor_register( mico_system_monitor_t* system_monitor, uint32_t initial_permitted_delay );

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
OSStatus mico_system_power_daemon_start( mico_Context_t* const in_context );
/* Perform a system power change */
OSStatus mico_system_power_perform( mico_Context_t* const in_context, mico_system_state_t new_state );

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
OSStatus mico_system_notify_register  ( mico_notify_types_t notify_type, void* functionAddress, void* arg );

OSStatus mico_system_notify_remove       ( mico_notify_types_t notify_type, void *functionAddress );

OSStatus mico_system_notify_remove_all    ( mico_notify_types_t notify_type);


/* Read current system clock */
OSStatus mico_system_current_time_get( struct tm* time );



/** @} */
/*****************************************************************************/
/** @addtogroup config_server       System Config Server Daemon
 *  @ingroup system
 *
 *  MiCO Config Server Daemon
 *
 *  @{
 */
/*****************************************************************************/
/* Start local config server */
OSStatus config_server_start ( mico_Context_t *inContext );

OSStatus config_server_stop ( void );


/* config server delegate */
void config_server_delegate_recv( const char *key, json_object *value, bool *need_reboot, mico_Context_t *inContext );

void config_server_delegate_report( json_object *app_menu, mico_Context_t *inContext );



/* Create device info menu on EasyLink APP that connected to local config server */
OSStatus config_server_create_sector        (json_object* sector_array, char* const name, json_object *cell_array);

OSStatus config_server_create_string_cell   (json_object* cell_array, char* const name, char* const content, char* const privilege, json_object* secectionArray);

OSStatus config_server_create_number_cell   (json_object* cell_array, char* const name, int content, char* const privilege, json_object* secectionArray);

OSStatus config_server_create_float_cell    (json_object* cell_array, char* const name, float content, char* const privilege, json_object* secectionArray);

OSStatus config_server_create_bool_cell     (json_object* cell_array, char* const name, boolean content, char* const privilege);

OSStatus config_server_create_sub_menu_cell (json_object* cell_array, char* const name, json_object* sector_array);

/** @} */
/*****************************************************************************/
/** @addtogroup cli       System Command Line Interface
 *  @ingroup system
 *
 *  MiCO Command Line Interface
 *
 *  @{
 */
/*****************************************************************************/

/** Initialize the CLI module
 *
 * \return kNoErr on success
 * \return error code otherwise.
 *
 * \note This function is called by the wm_core_init function. Applications need
 * not explicity call this function if they have already called wm_core_init().
 *
 */
int cli_init(void);

/** @} */
/*****************************************************************************/
/** @addtogroup mdns       mdns prorocol
 *  @ingroup system
 *
 *  MiCO mdns service for service seraching
 *
 *  @{
 */
/*****************************************************************************/
 typedef struct
{
  //char *name;
  char *service_name;
  char *host_name;
  char *instance_name;
  char *txt_record;
  uint16_t service_port;
} mdns_init_t;

/* mdns service */
OSStatus mdns_add_record( mdns_init_t init, WiFi_Interface interface, uint32_t time_to_live );

void mdns_suspend_record( char *service_name, WiFi_Interface interface, bool will_remove );

void mdns_resume_record( char *service_name, WiFi_Interface interface );

void mdns_update_txt_record( char *service_name, WiFi_Interface interface, char *txt_record );

/** @} */
/*****************************************************************************/
/** @addtogroup tftp_ota       TFTP OTA
 *  @ingroup system
 *
 *  Download new firmware from tftp server
 *
 *  @{
 */
/*****************************************************************************/
void tftp_ota(void);

/** @} */

#ifdef __cplusplus
} /*extern "C" */
#endif
