/**
******************************************************************************
* @file    MicoFogCloud.h 
* @author  Eshen Wang
* @version V1.0.0
* @date    17-Mar-2015
* @brief   This header contains the cloud service interfaces 
*          for MICO. 
  operation
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

#ifndef __MICO_FOGCLOUD_H_
#define __MICO_FOGCLOUD_H_

#include "MICO.h"
#include "MiCOAppDefine.h"
#include "MicoFogCloudDef.h"
#include "FogCloudUtils.h"


/*******************************************************************************
 *                                DEFINES
 ******************************************************************************/


/*******************************************************************************
 *                            USER INTERFACES
 ******************************************************************************/

/*----------------------------------- init -----------------------------------*/
// init FogCloud
OSStatus MicoStartFogCloudService(app_context_t* const app_context);
// restore default config for FogCloud
void MicoFogCloudRestoreDefault(app_context_t* app_context);


/*-------------------------- get MicoFogCloud state --------------------------*/
// device activate state
bool MicoFogCloudIsActivated(app_context_t* const app_context);
// cloud connect state
bool MicoFogCloudIsConnect(app_context_t* const app_context);


/*--------------------------- send && recv message ---------------------------*/
// Module <=> Cloud
OSStatus MicoFogCloudMsgSend(app_context_t* const app_context, const char* topic,
                             unsigned char *inBuf, unsigned int inBufLen);

OSStatus MicoFogCloudMsgRecv(app_context_t* const app_context, fogcloud_msg_t **msg, 
                             uint32_t timeout_ms);

/*------------------------------ device control ------------------------------*/
//activate
OSStatus MicoFogCloudActivate(app_context_t* const app_context, 
                              MVDActivateRequestData_t activateData);
//authorize
OSStatus MicoFogCloudAuthorize(app_context_t* const app_context,
                               MVDAuthorizeRequestData_t authorizeData);
//reset device info on cloud
OSStatus MicoFogCloudResetCloudDevInfo(app_context_t* const app_context,
                                       MVDResetRequestData_t devResetData);
// just set need cloud reset flag, device will reset itself from cloud.
void MicoFogCloudNeedResetDevice(void);

//OTA
OSStatus MicoFogCloudFirmwareUpdate(app_context_t* const app_context,
                                    MVDOTARequestData_t OTAData);
//get device state info(activate/connect)
OSStatus MicoFogCloudGetState(app_context_t* const app_context,
                              MVDGetStateRequestData_t getStateRequestData,
                              void* outDevState);

#endif  // __MICO_FOGCLOUD_H_
