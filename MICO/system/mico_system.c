/**
******************************************************************************
* @file    EasyLink.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide the easylink function and FTC server for quick 
*          provisioning and first time configuration.
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

#include <time.h>

#include "mico.h"

OSStatus m_system_current_time_get( struct tm* time )
{
  mico_rtc_time_t mico_time;
  /*Read current time from RTC.*/
  if( MicoRtcGetTime(&mico_time) == kNoErr ){
    time->tm_sec = mico_time.sec;
    time->tm_min = mico_time.min;
    time->tm_hour = mico_time.hr;
    time->tm_mday = mico_time.date;
    time->tm_wday = mico_time.weekday;
    time->tm_mon = mico_time.month - 1;
    time->tm_year = mico_time.year + 100;
    return kNoErr;
  }else
    return kGeneralErr;
}

static  mico_Context_t* context = NULL;

mico_Context_t* m_system_context_init( uint32_t user_config_data_size )
{
  void *user_config_data = NULL;

  if( context !=  NULL) {
    if( context->user_config_data != NULL )
      free( context->user_config_data );
    free( context );
    context = NULL;
  }

  user_config_data = calloc( 1, user_config_data_size );
  require( user_config_data, exit );

  context = calloc( 1, sizeof(mico_Context_t) );
  require( context, exit );

  context->user_config_data = user_config_data;
  context->user_config_data_size = user_config_data_size;

  mico_rtos_init_mutex( &context->flashContentInRam_mutex );
  MICOReadConfiguration( context );

exit:
  return context;  
}

mico_Context_t* m_system_context_get( void )
{
  return context;
}

void* m_system_context_get_user_data( mico_Context_t* const in_context )
{
  return in_context->user_config_data;
}


OSStatus m_system_init( mico_Context_t* in_context )
{
  OSStatus err = kNoErr;

  require_action( in_context, exit, err = kNotPreparedErr );

  /* Initialize power management daemen */
  err = m_system_power_daemon_start( in_context );
  require_noerr( err, exit ); 

  /* Initialize mico notify system */
  err = system_notification_init( in_context );
  require_noerr( err, exit ); 

#ifdef m_system_MONITOR_ENABLE
  /* MiCO system monitor */
  err = m_system_monitor_daemen_start( );
  require_noerr( err, exit ); 
#endif

#ifdef MICO_CLI_ENABLE
  /* MiCO command line interface */
  mico_cli_init();
#endif

  /* Network PHY driver and tcp/ip static init */
  err = system_network_daemen_start( in_context );
  require_noerr( err, exit ); 

  if( in_context->flashContentInRam.micoSystemConfig.configured == wLanUnConfigured ||
      in_context->flashContentInRam.micoSystemConfig.configured == unConfigured){
    system_log("Empty configuration. Starting configuration mode...");

#if (MICO_CONFIG_MODE == CONFIG_MODE_EASYLINK) || \
    (MICO_CONFIG_MODE == CONFIG_MODE_SOFT_AP) ||  \
    (MICO_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP) || \
    (MICO_CONFIG_MODE == CONFIG_MODE_AIRKISS)
    err = system_easylink_start( in_context );
    require_noerr( err, exit );
#elif ( MICO_CONFIG_MODE == CONFIG_MODE_WAC)
    err = mico_easylink_start( in_context );
    require_noerr( err, exit );
#else
    #error "Wi-Fi configuration mode is not defined"
#endif
  }
#ifdef MFG_MODE_AUTO
  else if( in_context->flashContentInRam.micoSystemConfig.configured == mfgConfigured ){
    system_log( "Enter MFG mode automatically" );
    mico_mfg_test( in_context );
    mico_thread_sleep( MICO_NEVER_TIMEOUT );
  }
#endif
  else{
    system_log("Available configuration. Starting Wi-Fi connection...");
    system_connect_wifi_fast( in_context );
  }

  /*Local configuration server*/
#ifdef MICO_CONFIG_SERVER_ENABLE
  MICOStartConfigServer( );
#endif

#ifdef MICO_NTP_CLIENT_ENABLE
  struct tm currentTime;
  m_system_current_time_get( &currentTime );
  system_log("Current Time: %s",asctime(&currentTime));
  err =  MICOStartNTPClient( );
  require_noerr_string( err, exit, "ERROR: Unable to start the NTP client thread." );
#endif

  require_noerr_action( err, exit, system_log("Closing main thread with err num: %d.", err) );
exit:
  
  return err;
}


