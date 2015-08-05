/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.2.5 at Wed May 28 15:52:00 2014. */

#include "sitewhere-arduino.pb.h"



const pb_field_t SiteWhere_fields[1] = {
    PB_LAST_FIELD
};

const pb_field_t SiteWhere_Header_fields[3] = {
    PB_FIELD2(  1, ENUM    , REQUIRED, STATIC  , FIRST, SiteWhere_Header, command, command, 0),
    PB_FIELD2(  2, STRING  , OPTIONAL, STATIC  , OTHER, SiteWhere_Header, originator, command, 0),
    PB_LAST_FIELD
};

const pb_field_t SiteWhere_RegisterDevice_fields[3] = {
    PB_FIELD2(  1, STRING  , REQUIRED, STATIC  , FIRST, SiteWhere_RegisterDevice, hardwareId, hardwareId, 0),
    PB_FIELD2(  2, STRING  , REQUIRED, STATIC  , OTHER, SiteWhere_RegisterDevice, specificationToken, hardwareId, 0),
    PB_LAST_FIELD
};

const pb_field_t SiteWhere_Acknowledge_fields[3] = {
    PB_FIELD2(  1, STRING  , REQUIRED, STATIC  , FIRST, SiteWhere_Acknowledge, hardwareId, hardwareId, 0),
    PB_FIELD2(  2, STRING  , OPTIONAL, STATIC  , OTHER, SiteWhere_Acknowledge, message, hardwareId, 0),
    PB_LAST_FIELD
};

const pb_field_t SiteWhere_DeviceLocation_fields[6] = {
    PB_FIELD2(  1, STRING  , REQUIRED, STATIC  , FIRST, SiteWhere_DeviceLocation, hardwareId, hardwareId, 0),
    PB_FIELD2(  2, FIXED64 , REQUIRED, STATIC  , OTHER, SiteWhere_DeviceLocation, latitude, hardwareId, 0),
    PB_FIELD2(  3, FIXED64 , REQUIRED, STATIC  , OTHER, SiteWhere_DeviceLocation, longitude, latitude, 0),
    PB_FIELD2(  4, FIXED64 , OPTIONAL, STATIC  , OTHER, SiteWhere_DeviceLocation, elevation, longitude, 0),
    PB_FIELD2(  5, FIXED64 , OPTIONAL, STATIC  , OTHER, SiteWhere_DeviceLocation, eventDate, elevation, 0),
    PB_LAST_FIELD
};

const pb_field_t SiteWhere_DeviceAlert_fields[5] = {
    PB_FIELD2(  1, STRING  , REQUIRED, STATIC  , FIRST, SiteWhere_DeviceAlert, hardwareId, hardwareId, 0),
    PB_FIELD2(  2, STRING  , REQUIRED, STATIC  , OTHER, SiteWhere_DeviceAlert, alertType, hardwareId, 0),
    PB_FIELD2(  3, STRING  , REQUIRED, STATIC  , OTHER, SiteWhere_DeviceAlert, alertMessage, alertType, 0),
    PB_FIELD2(  4, FIXED64 , OPTIONAL, STATIC  , OTHER, SiteWhere_DeviceAlert, eventDate, alertMessage, 0),
    PB_LAST_FIELD
};

const pb_field_t SiteWhere_Measurement_fields[3] = {
    PB_FIELD2(  1, STRING  , REQUIRED, STATIC  , FIRST, SiteWhere_Measurement, measurementId, measurementId, 0),
    PB_FIELD2(  2, FIXED64 , REQUIRED, STATIC  , OTHER, SiteWhere_Measurement, measurementValue, measurementId, 0),
    PB_LAST_FIELD
};

const pb_field_t SiteWhere_DeviceMeasurements_fields[4] = {
    PB_FIELD2(  1, STRING  , REQUIRED, STATIC  , FIRST, SiteWhere_DeviceMeasurements, hardwareId, hardwareId, 0),
    PB_FIELD2(  2, MESSAGE , REPEATED, STATIC  , OTHER, SiteWhere_DeviceMeasurements, measurement, hardwareId, &SiteWhere_Measurement_fields),
    PB_FIELD2(  3, FIXED64 , OPTIONAL, STATIC  , OTHER, SiteWhere_DeviceMeasurements, eventDate, measurement, 0),
    PB_LAST_FIELD
};

const pb_field_t Device_fields[1] = {
    PB_LAST_FIELD
};

const pb_field_t Device_Header_fields[5] = {
    PB_FIELD2(  1, ENUM    , REQUIRED, STATIC  , FIRST, Device_Header, command, command, 0),
    PB_FIELD2(  2, STRING  , OPTIONAL, STATIC  , OTHER, Device_Header, originator, command, 0),
    PB_FIELD2(  3, STRING  , OPTIONAL, STATIC  , OTHER, Device_Header, nestedPath, originator, 0),
    PB_FIELD2(  4, STRING  , OPTIONAL, STATIC  , OTHER, Device_Header, nestedSpec, nestedPath, 0),
    PB_LAST_FIELD
};

const pb_field_t Device_RegistrationAck_fields[4] = {
    PB_FIELD2(  1, ENUM    , REQUIRED, STATIC  , FIRST, Device_RegistrationAck, state, state, 0),
    PB_FIELD2(  2, ENUM    , OPTIONAL, STATIC  , OTHER, Device_RegistrationAck, errorType, state, 0),
    PB_FIELD2(  3, STRING  , OPTIONAL, STATIC  , OTHER, Device_RegistrationAck, errorMessage, errorType, 0),
    PB_LAST_FIELD
};


/* Check that field information fits in pb_field_t */
#if !defined(PB_FIELD_16BIT) && !defined(PB_FIELD_32BIT)
STATIC_ASSERT((pb_membersize(SiteWhere_DeviceMeasurements, measurement[0]) < 256), YOU_MUST_DEFINE_PB_FIELD_16BIT_FOR_MESSAGES_SiteWhere_SiteWhere_Header_SiteWhere_RegisterDevice_SiteWhere_Acknowledge_SiteWhere_DeviceLocation_SiteWhere_DeviceAlert_SiteWhere_Measurement_SiteWhere_DeviceMeasurements_Device_Device_Header_Device_RegistrationAck)
#endif

#if !defined(PB_FIELD_32BIT)
STATIC_ASSERT((pb_membersize(SiteWhere_DeviceMeasurements, measurement[0]) < 65536), YOU_MUST_DEFINE_PB_FIELD_32BIT_FOR_MESSAGES_SiteWhere_SiteWhere_Header_SiteWhere_RegisterDevice_SiteWhere_Acknowledge_SiteWhere_DeviceLocation_SiteWhere_DeviceAlert_SiteWhere_Measurement_SiteWhere_DeviceMeasurements_Device_Device_Header_Device_RegistrationAck)
#endif

