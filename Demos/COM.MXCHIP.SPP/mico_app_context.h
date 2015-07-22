/**
  ******************************************************************************
  * @file    mico_app_context.h 
  * @author  William Xu
  * @version V1.0.0
  * @date    22-July-2015
  * @brief   This file provide head files for context in application. This is 
  *          included in mico_context_t in mico system.
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
  * <h2><center>&copy; COPYRIGHT 2015 MXCHIP Inc.</center></h2>
  ******************************************************************************
  */

#pragma once

#include "MICO.h"
#include "Common.h"
#include "MiCOAPPDefine.h"

#ifdef __cplusplus
extern "C" {
#endif

/*Application's configuration stores in flash*/
typedef struct
{
  uint32_t          configDataVer;
  uint32_t          localServerPort;

  /*local services*/
  bool              localServerEnable;
  bool              remoteServerEnable;
  char              remoteServerDomain[64];
  int               remoteServerPort;

  /*IO settings*/
  uint32_t          USART_BaudRate;
} application_config_t;



/*Running status*/
typedef struct  {
  /*Local clients port list*/
  mico_queue_t*  socket_out_queue[MAX_QUEUE_NUM];
  mico_mutex_t   queue_mtx;
} current_app_status_t;

#ifdef __cplusplus
} /*extern "C" */
#endif

