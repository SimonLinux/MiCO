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

#include "MICO.h"
#include "mico_system_context.h"
#include "system.h"

static bool               needs_update          = false;
static system_state_t     current_sys_state     = eState_Normal;
static mico_semaphore_t   sys_state_change_sem  = NULL;

WEAK void sendNotifySYSWillPowerOff(void){

}


USED void PlatformEasyLinkButtonClickedCallback(void)
{
  system_log_trace();
  mico_Context_t* context = NULL;
  
  system_context_read( &context );
  require( context, exit );
  
  if(context->flashContentInRam.micoSystemConfig.easyLinkByPass != EASYLINK_BYPASS_NO){
    context->flashContentInRam.micoSystemConfig.easyLinkByPass = EASYLINK_BYPASS_NO;
    needs_update = true;
  }
  
  if(context->flashContentInRam.micoSystemConfig.configured == allConfigured){
    context->flashContentInRam.micoSystemConfig.configured = wLanUnConfigured;
    needs_update = true;
  }

  system_power_perform( eState_Software_Reset );

exit: 
  return;
}

USED void PlatformEasyLinkButtonLongPressedCallback(void)
{
  system_log_trace();
  mico_Context_t* context = NULL;
  
  system_context_read( &context );
  require( context, exit );
  
  context->flashContentInRam.micoSystemConfig.configured = wLanUnConfigured;
  MICORestoreDefault(context);
  
  system_power_perform( eState_Software_Reset );

exit: 
  return;
}

USED void PlatformStandbyButtonClickedCallback(void)
{
  system_log_trace();
  mico_Context_t* context = NULL;
  
  system_context_read( &context );
  require( context, exit );
  
  system_power_perform( eState_Standby );

exit: 
  return;
}


static void _sys_state_thread(void *arg)
{  
  mico_Context_t* context = arg;
  
  /*System status changed*/
  while(mico_rtos_get_semaphore( &sys_state_change_sem, MICO_WAIT_FOREVER) == kNoErr ){
    
    if(needs_update == true)
      MICOUpdateConfiguration( context );
    
    switch( current_sys_state ){
    case eState_Normal:
      break;
    case eState_Software_Reset:
      sendNotifySYSWillPowerOff( );
      mico_thread_msleep( 500 );
      MicoSystemReboot( );
      break;
    case eState_Wlan_Powerdown:
      sendNotifySYSWillPowerOff( );
      mico_thread_msleep( 500 );
      micoWlanPowerOff( );
      break;
    case eState_Standby:
      sendNotifySYSWillPowerOff( );
      mico_thread_msleep( 500 );
      micoWlanPowerOff( );
      MicoSystemStandBy( MICO_WAIT_FOREVER );
      break;
    default:
      break;
    }
  }
  mico_rtos_delete_thread( NULL );
}

OSStatus system_power_daemon_start( mico_Context_t * const inContext )
{
  OSStatus err = kNoErr;

  require(inContext, exit);

  err = mico_rtos_init_semaphore( &sys_state_change_sem, 1 ); 
  require_noerr(err, exit);

  mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "Power Daemon", _sys_state_thread, 800, inContext ); 
  require_noerr(err, exit);
  
exit:
  return kNoErr;
}


void system_power_perform( system_state_t new_state )
{
  mico_Context_t* context = NULL;
  
  system_context_read( &context );
  require( context, exit );

  current_sys_state = new_state;
  require( sys_state_change_sem, exit);
  mico_rtos_set_semaphore( &sys_state_change_sem );   

exit:
  return; 
}









