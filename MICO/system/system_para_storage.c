/**
******************************************************************************
* @file    MICOParaStorage.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide functions to read and write MICO settings on 
*          nonvolatile memory.
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

#include "mico_system_context.h"
#include "MICO.h"
#include "mico_system.h"
#include "MicoPlatform.h"
#include "CheckSumUtils.h"

/* Update seed number every time*/
static int32_t seedNum = 0;

#define CONFIG_OFFSET ( sizeof( boot_table_t ) )
#define CONFIG_SIZE   ( sizeof( mico_sys_config_t ) + sizeof( application_config_t ) )
#define CRC_OFFSET    ( 0xE00 )
#define CRC_SIZE      ( 2 )

//#define para_log(M, ...) custom_log("MiCO Settting", M, ##__VA_ARGS__)

#define para_log(M, ...)

__weak void appRestoreDefault_callback(mico_Context_t *inContext)
{

}

static bool is_crc_match( uint16_t crc_1, uint16_t crc_2)
{
  if( crc_1 == 0 || crc_2 == 0)
    return false;
  
  if( crc_1 != crc_2 )
    return false;
      
  return true;
}

static OSStatus internal_update_config(mico_Context_t *inContext)
{
  OSStatus err = kNoErr;
  uint32_t para_offset;
  CRC16_Context crc_context;
  uint16_t crc_result;
  
  uint16_t crc_readback;;

  para_log("Flash write!");

  CRC16_Init( &crc_context );
  CRC16_Update( &crc_context, (uint8_t *)&inContext->flashContentInRam.micoSystemConfig, CONFIG_SIZE );
  CRC16_Final( &crc_context, &crc_result );

  err = MicoFlashErase( MICO_PARTITION_PARAMETER_1, 0x0, sizeof(flash_content_t) );
  require_noerr(err, exit);

  para_offset = 0x0;
  err = MicoFlashWrite( MICO_PARTITION_PARAMETER_1, &para_offset, (uint8_t *)&inContext->flashContentInRam, sizeof(flash_content_t));
  require_noerr(err, exit);

  para_offset = CRC_OFFSET;
  err = MicoFlashWrite( MICO_PARTITION_PARAMETER_1, &para_offset, (uint8_t *)&crc_result, CRC_SIZE );
  require_noerr(err, exit);
  
  /* Read back*/
  para_offset = CRC_OFFSET;
  err = MicoFlashRead( MICO_PARTITION_PARAMETER_1, &para_offset, (uint8_t *)&crc_readback, CRC_SIZE );
  para_log( "crc_readback = %d", crc_readback);
  if( crc_readback == 0x0)
    return kWriteErr;
  
  err = MicoFlashErase( MICO_PARTITION_PARAMETER_2, 0x0, sizeof(flash_content_t) );
  require_noerr(err, exit);

  para_offset = 0x0;
  err = MicoFlashWrite( MICO_PARTITION_PARAMETER_2, &para_offset, (uint8_t *)&inContext->flashContentInRam, sizeof(flash_content_t));
  require_noerr(err, exit);

  para_offset = CRC_OFFSET;
  err = MicoFlashWrite( MICO_PARTITION_PARAMETER_2, &para_offset, (uint8_t *)&crc_result, CRC_SIZE );
  require_noerr(err, exit);

exit:
  return err;
}

OSStatus MICORestoreDefault(mico_Context_t *inContext)
{ 
  OSStatus err = kNoErr;

  /*wlan configration is not need to change to a default state, use easylink to do that*/
  memset(&inContext->flashContentInRam, 0x0, sizeof(inContext->flashContentInRam));
  sprintf(inContext->flashContentInRam.micoSystemConfig.name, DEFAULT_NAME);
  inContext->flashContentInRam.micoSystemConfig.configured = unConfigured;
  inContext->flashContentInRam.micoSystemConfig.easyLinkByPass = EASYLINK_BYPASS_NO;
  inContext->flashContentInRam.micoSystemConfig.rfPowerSaveEnable = false;
  inContext->flashContentInRam.micoSystemConfig.mcuPowerSaveEnable = false;
  inContext->flashContentInRam.micoSystemConfig.seed = seedNum;

  /*Application's default configuration*/
  appRestoreDefault_callback(inContext);

  para_log("Restore to default");

  err = internal_update_config( inContext );
  require_noerr(err, exit);

exit:
  return err;
}

#ifdef MFG_MODE_AUTO
OSStatus MICORestoreMFG(mico_Context_t *inContext)
{ 
  OSStatus err = kNoErr;
  uint32_t para_offset = 0x0;

  /*wlan configration is not need to change to a default state, use easylink to do that*/
  sprintf(inContext->flashContentInRam.micoSystemConfig.name, DEFAULT_NAME);
  inContext->flashContentInRam.micoSystemConfig.configured = mfgConfigured;

  /*Application's default configuration*/
  appRestoreDefault_callback(inContext);

  err = internal_update_config( inContext );
  require_noerr(err, exit);

exit:
  return err;
}
#endif

OSStatus MICOReadConfiguration(mico_Context_t *inContext)
{
  uint32_t para_offset = 0x0;
  uint32_t config_offset = CONFIG_OFFSET;
  uint32_t crc_offset = CRC_OFFSET;
  CRC16_Context crc_context;
  uint16_t crc_result, crc_target;
  uint16_t crc_backup_result, crc_backup_target;
  mico_logic_partition_t *partition; 
  uint8_t *backup_data = NULL;

  OSStatus err = kNoErr;

  backup_data = malloc( CONFIG_SIZE );
  require_action( backup_data, exit, err = kNoMemoryErr );


  /* Load data and crc from main partition */
  para_offset = 0x0;
  err = MicoFlashRead( MICO_PARTITION_PARAMETER_1, &para_offset, (uint8_t *)&inContext->flashContentInRam, sizeof( flash_content_t ) );

  CRC16_Init( &crc_context );
  CRC16_Update( &crc_context, (uint8_t *)&inContext->flashContentInRam.micoSystemConfig, CONFIG_SIZE );
  CRC16_Final( &crc_context, &crc_result );
  para_log( "crc_result = %d", crc_result);

  crc_offset = CRC_OFFSET;
  err = MicoFlashRead( MICO_PARTITION_PARAMETER_1, &crc_offset, (uint8_t *)&crc_target, CRC_SIZE );
  para_log( "crc_target = %d", crc_target);

  /* Load data and crc from backup partition */
  config_offset = CONFIG_OFFSET;
  err = MicoFlashRead( MICO_PARTITION_PARAMETER_2, &config_offset, backup_data, CONFIG_SIZE );

  CRC16_Init( &crc_context );
  CRC16_Update( &crc_context, backup_data, CONFIG_SIZE );
  CRC16_Final( &crc_context, &crc_backup_result );
  para_log( "crc_backup_result = %d", crc_backup_result);

  crc_offset = CRC_OFFSET;
  err = MicoFlashRead( MICO_PARTITION_PARAMETER_2, &crc_offset, (uint8_t *)&crc_backup_target, CRC_SIZE );  
  para_log( "crc_backup_target = %d", crc_backup_target);
  
  /* Data collapsed at main partition */
  if( is_crc_match( crc_result, crc_target ) == false ){
    /* Data collapsed at main partition and backup partition both, restore to default */
    if( is_crc_match( crc_backup_result, crc_backup_target ) == false ){
      para_log("Config failed on both partition, restore to default settings!");
      err = MICORestoreDefault(inContext);
      require_noerr(err, exit);
    }
    /* main collapsed, backup correct, copy data from back up to main */
    else {
      para_log("Config failed on main, recover!");

      memset(&inContext->flashContentInRam, 0x0, sizeof(inContext->flashContentInRam));
      memcpy( (uint8_t *)&inContext->flashContentInRam.micoSystemConfig, backup_data, CONFIG_SIZE);

      partition = MicoFlashGetInfo( MICO_PARTITION_PARAMETER_1 );

      err = MicoFlashErase( MICO_PARTITION_PARAMETER_1 ,0x0, partition->partition_length );
      require_noerr(err, exit);

      para_offset = 0x0;
      err = MicoFlashWrite( MICO_PARTITION_PARAMETER_1, &para_offset, (uint8_t *)&inContext->flashContentInRam, sizeof(flash_content_t) );
      require_noerr(err, exit);

      crc_offset = CRC_OFFSET;
      err = MicoFlashWrite( MICO_PARTITION_PARAMETER_1, &crc_offset, (uint8_t *)&crc_backup_result, CRC_SIZE );
      require_noerr(err, exit);
    }
  }   
  /* main correct */
  else { 
      /* main correct , backup collapsed, or main!=backup, copy data from main to back up */
    if( is_crc_match( crc_result, crc_backup_result ) == false || is_crc_match( crc_backup_result, crc_backup_target ) == false ){
      para_log("Config failed on backup, recover!");

      partition = MicoFlashGetInfo( MICO_PARTITION_PARAMETER_2 );

      err = MicoFlashErase( MICO_PARTITION_PARAMETER_2 ,0x0, partition->partition_length );
      require_noerr(err, exit);
  
      para_offset = 0x0;
      err = MicoFlashWrite( MICO_PARTITION_PARAMETER_2, &para_offset, (uint8_t *)&inContext->flashContentInRam, sizeof(flash_content_t) );
      require_noerr(err, exit);

      crc_offset = CRC_OFFSET;
      err = MicoFlashWrite( MICO_PARTITION_PARAMETER_2, &crc_offset, (uint8_t *)&crc_target, CRC_SIZE );
      require_noerr(err, exit);
    }
  }

  para_log(" Config read, seed = %d!", inContext->flashContentInRam.micoSystemConfig.seed);

  seedNum = inContext->flashContentInRam.micoSystemConfig.seed;
  if(seedNum == -1) seedNum = 0;

  if(inContext->flashContentInRam.appConfig.configDataVer != CONFIGURATION_VERSION){
    para_log("configDataVer error, restore to default");
#ifdef MFG_MODE_AUTO
    err = MICORestoreMFG(inContext);
#else
    err = MICORestoreDefault(inContext);
#endif
    require_noerr(err, exit);
  }

  if(inContext->flashContentInRam.micoSystemConfig.dhcpEnable == DHCP_Disable){
    strcpy((char *)inContext->micoStatus.localIp, inContext->flashContentInRam.micoSystemConfig.localIp);
    strcpy((char *)inContext->micoStatus.netMask, inContext->flashContentInRam.micoSystemConfig.netMask);
    strcpy((char *)inContext->micoStatus.gateWay, inContext->flashContentInRam.micoSystemConfig.gateWay);
    strcpy((char *)inContext->micoStatus.dnsServer, inContext->flashContentInRam.micoSystemConfig.dnsServer);
  }

exit: 
  if( backup_data!= NULL) free( backup_data );
  return err;
}

OSStatus MICOUpdateConfiguration(mico_Context_t *inContext)
{
  OSStatus err = kNoErr;

  inContext->flashContentInRam.micoSystemConfig.seed = ++seedNum;

  err = internal_update_config( inContext );
  require_noerr(err, exit);

exit:
  return err;
}


