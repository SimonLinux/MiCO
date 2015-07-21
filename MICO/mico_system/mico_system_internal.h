
#include "MICO.h"
#include "MicoDefine.h"

#define system_log(M, ...) custom_log("Sys", M, ##__VA_ARGS__)
#define system_log_trace() custom_log_trace("Sys")

OSStatus mico_system_context_init( mico_Context_t** context_out );

OSStatus mico_system_context_read( mico_Context_t** context_out );

OSStatus mico_system_context_read( mico_Context_t** context_out );

OSStatus mico_system_power_daemon_start( mico_Context_t * const inContext );

void mico_system_power_perform( SYS_State_t new_state );


OSStatus mico_system_notify_init( mico_Context_t * const inContext );

void mico_system_connect_wifi_normal( mico_Context_t * const inContext);

void mico_system_connect_wifi_fast( mico_Context_t * const inContext);

OSStatus mico_easylink_wac_start( mico_Context_t * const inContext );

OSStatus mico_easylink_wps_tart( mico_Context_t * const inContext );


OSStatus mico_easylink_start( mico_Context_t * const inContext );

OSStatus mico_easylink_stop( mico_Context_t * const inContext);