/**
******************************************************************************
* @file    MICOEntrance.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   MICO system main entrance.
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

#include "time.h"
#include "MicoPlatform.h"
#include "platform.h"
#include "MICOAppDefine.h"
#include "mico_system.h"

#include "MICONotificationCenter.h"
#include "MICOSystemMonitor.h"
#include "MicoCli.h"
#include "Airkiss/Airkiss.h"
#include "StringUtils.h"
#include "MDNSUtils.h"
#include "mico_system/mico_system_internal.h"

static mico_Context_t *context;
static mico_timer_t _watchdog_reload_timer;
static mico_system_monitor_t mico_monitor;



/* ========================================
User provide callback functions 
======================================== */

static void _watchdog_reload_timer_handler( void* arg )
{
  (void)(arg);
  MICOUpdateSystemMonitor(&mico_monitor, APPLICATION_WATCHDOG_TIMEOUT_SECONDS*1000);
}



void mico_write_ota_tbl(int len, uint16_t crc)
{
  mico_logic_partition_t* ota_partition = MicoFlashGetInfo( MICO_PARTITION_OTA_TEMP );
  memset(&context->flashContentInRam.bootTable, 0, sizeof(boot_table_t));
  context->flashContentInRam.bootTable.length = len;
  context->flashContentInRam.bootTable.start_address = ota_partition->partition_start_addr;
  context->flashContentInRam.bootTable.type = 'A';
  context->flashContentInRam.bootTable.upgrade_type = 'U';
  context->flashContentInRam.bootTable.crc = crc;
  MICOUpdateConfiguration(context);
}

int application_start(void)
{
  OSStatus err = kNoErr;
  IPStatusTypedef para;
  struct tm currentTime;
  mico_rtc_time_t time;
  system_log_trace(); 

  /* Read mico context that holds all system configurations and runtime status */
  err = mico_system_context_init( &context );
  require_noerr( err, exit ); 

  /* Initialize power management daemen */
  err = mico_system_power_daemon_start( context );
  require_noerr( err, exit ); 

  /* Initialize mico system */
  err = mico_system_notify_init( context );
  require_noerr( err, exit ); 

  /*wlan driver and tcpip init*/
  wifimgr_debug_enable(true);
  system_log( "MiCO starting..." );
  MicoInit();
#ifdef MICO_CLI_ENABLE
  MicoCliInit();
#endif
  MicoSysLed(true);
  system_log("Free memory %d bytes", MicoGetMemoryInfo()->free_memory); 
  micoWlanGetIPStatus(&para, Station);
  formatMACAddr(context->micoStatus.mac, (char *)&para.mac);
  MicoGetRfVer(context->micoStatus.rf_version, sizeof(context->micoStatus.rf_version));
  system_log("%s mxchipWNet library version: %s", APP_INFO, MicoGetVer());
  system_log("Wi-Fi driver version %s, mac %s", context->micoStatus.rf_version, context->micoStatus.mac);

  if(context->flashContentInRam.micoSystemConfig.rfPowerSaveEnable == true){
    micoWlanEnablePowerSave();
  }

  if(context->flashContentInRam.micoSystemConfig.mcuPowerSaveEnable == true){
    MicoMcuPowerSaveConfig(true);
  }

  /*Start system monotor thread*/
  //err = MICOStartSystemMonitor(context);
  require_noerr_string( err, exit, "ERROR: Unable to start the system monitor." );

  err = MICORegisterSystemMonitor(&mico_monitor, APPLICATION_WATCHDOG_TIMEOUT_SECONDS*1000);
  require_noerr( err, exit );
  mico_init_timer(&_watchdog_reload_timer,APPLICATION_WATCHDOG_TIMEOUT_SECONDS*1000/2, _watchdog_reload_timer_handler, NULL);
  mico_start_timer(&_watchdog_reload_timer);

  /*Read current time from RTC.*/
  if( MicoRtcGetTime(&time) == kNoErr ){
    currentTime.tm_sec = time.sec;
    currentTime.tm_min = time.min;
    currentTime.tm_hour = time.hr;
    currentTime.tm_mday = time.date;
    currentTime.tm_wday = time.weekday;
    currentTime.tm_mon = time.month - 1;
    currentTime.tm_year = time.year + 100;
    system_log("Current Time: %s",asctime(&currentTime));
  }else
    system_log("RTC function unsupported");

  if( context->flashContentInRam.micoSystemConfig.configured == wLanUnConfigured ||
      context->flashContentInRam.micoSystemConfig.configured == unConfigured){
    system_log("Empty configuration. Starting configuration mode...");

#if (MICO_CONFIG_MODE == CONFIG_MODE_EASYLINK) || (MICO_CONFIG_MODE == CONFIG_MODE_SOFT_AP) || (MICO_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP)
    err = mico_easylink_start( context );
    require_noerr( err, exit );
#elif (MICO_CONFIG_MODE == CONFIG_MODE_AIRKISS)
    err = startAirkiss( context );
    require_noerr( err, exit );
#elif (MICO_CONFIG_MODE == CONFIG_MODE_WPS) || MICO_CONFIG_MODE == defined (CONFIG_MODE_WPS_WITH_SOFTAP)
    err = mico_easylink_wps_tart( context );
    require_noerr( err, exit );
#elif ( MICO_CONFIG_MODE == CONFIG_MODE_WAC)
    err = mico_easylink_start( context );
    require_noerr( err, exit );
#else
    #error "Wi-Fi configuration mode is not defined"
#endif
  }
  else{
    system_log("Available configuration. Starting Wi-Fi connection...");
    mico_system_connect_wifi_fast( context );
  }

#ifdef MFG_MODE_AUTO
  if( context->flashContentInRam.micoSystemConfig.configured == mfgConfigured ){
    mico_log( "Enter MFG mode automatically" );
    mico_mfg_test(context);
    mico_thread_sleep(MICO_NEVER_TIMEOUT);
  }
#endif


 
  /*Local configuration server*/
#ifdef MICO_CONFIG_SERVER_ENABLE
  MICOStartConfigServer(context);
#endif

  err =  MICOStartNTPClient(context);
  require_noerr_string( err, exit, "ERROR: Unable to start the NTP client thread." );

  /*Start mico application*/
  err = MICOStartApplication( context );
  require_noerr( err, exit );
  
  system_log("Free memory %d bytes", MicoGetMemoryInfo()->free_memory) ; 
  
  require_noerr_action( err, exit, system_log("Closing main thread with err num: %d.", err) );

exit:
  mico_rtos_delete_thread(NULL);
  return kNoErr;
}

