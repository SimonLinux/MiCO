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

#include "mico_system_context.h"
#include "mico_system.h"


OSStatus mico_system_current_time_get( struct tm* time )
{
  return system_current_time_get( time );
}

void mico_system_power_perform( mico_system_state_t new_state )
{
  system_power_perform( new_state );
}

OSStatus mico_system_context_read( mico_Context_t** out_context )
{
  return system_context_read( out_context );
}

OSStatus mico_system_init( mico_Context_t** out_context )
{
  return system_init( out_context );
}


