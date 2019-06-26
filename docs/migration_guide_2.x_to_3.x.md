# Stream Engine Migration Guide 2.x to 3.x

## Introduction

This document is intended to help developers transition from older stream engine library 2.x to latest stream engine version 3.0. It is written in the format of a change log with complementary information that will assist developer making a smooth transition.

## Wait for callbacks

The tobii_engine parameter has been removed from the `tobii_wait_for_callbacks` prototype, since the tobii_engine interface is no longer part of the API:

    tobii_error_t tobii_wait_for_callbacks( int device_count, tobii_device_t* const* devices );

The implicit time sync that occurred every 30 seconds has been removed from `tobii_wait_for_callbacks`. An explicit call to tobii_update_timesync is now required in order to synchronize host time with the tracker timestamps.

## Firmware upgrade state

New function `tobii_get_firmware_upgrade_state` has been added to the API, it is used for querying the current firmware upgrade status of a device:

    typedef enum tobii_firmware_upgrade_state_t
    {
        TOBII_FIRMWARE_UPGRADE_STATE_NOT_IN_PROGRESS,
        TOBII_FIRMWARE_UPGRADE_STATE_IN_PROGRESS,
    } tobii_firmware_upgrade_state_t;

    tobii_error_t tobii_get_firmware_upgrade_state( tobii_device_t* device, tobii_firmware_upgrade_state_t* firmware_upgrade_state );

When a device is undergoing firmware upgrade it can be enumerated by stream engine but not used, the implementer needs to take this into account and call this function in order to check if the device is ready for use at each startup.

## Tobii engine interface

The `tobii_engine` interface has been removed from the stream engine client API. A replacement API may be released at a later date. Old versions of the API will still be able to use this API, if the stream engine service or middleware service is installed on the host machine, only difference is that the device readiness will always be shown as `TOBII_DEVICE_READINESS_READY`.

## User position guide stream

A new `tobii_user_position_guide` stream has been added to the API, which is used to help a user position their eyes correctly in the track box:

    typedef struct tobii_user_position_guide_t
    {
        int64_t timestamp_us;
        tobii_validity_t left_position_validity;
        float left_position_normalized_xyz[3];
        tobii_validity_t right_position_validity;
        float right_position_normalized_xyz[3];
    } tobii_user_position_guide_t;

    typedef void( *tobii_user_position_guide_callback_t )( tobii_user_position_guide_t const * user_position_guide, void* user_data );

    tobii_error_t tobii_user_position_guide_subscribe( tobii_device_t* device, tobii_user_position_guide_callback_t callback, void* user_data );

    tobii_error_t tobii_user_position_guide_unsubscribe( tobii_device_t* device );

Device supports for this new stream, can be queried through the capabilities API. This has been split into two separate capabilities, one for xy-axis and one for z-axis, this is due to the fact that not all trackers support z-axis position guide:

- `TOBII_CAPABILITY_COMPOUND_STREAM_USER_POSITION_GUIDE_XY`
- `TOBII_CAPABILITY_COMPOUND_STREAM_USER_POSITION_GUIDE_Z`

The user position guide stream will replace the `tobii_eye_position_normalized` stream which is now deprecated and only supported on older trackers.

## Device info

New fields have been added to the `tobii_device_info_t` struct and array sizes has been updated:

    typedef struct tobii_device_info_t
    {
        char serial_number[256];
        char model[256];
        char generation[256];
        char firmware_version[256];
        char integration_id[128];
        char hw_calibration_version[128];
        char hw_calibration_date[128];
        char lot_id[128];
        char integration_type[256];
        char runtime_build_version[256];
    } tobii_device_info_t;

New fields explained:

- `integration_id`, a field that contains information that allows a software component to couple an eye-tracker with its integration hardware. Current examples are screens and HMD's.
- `hw_calibration_version`, production information, only used for internal traceability.
- `hw_calibration_date`, production information, only used for internal traceability.
- `lot_id`, production information, only used for internal traceability.
- `integration_type`, indicates the type of device the tracker is integrated into.
- `runtime_build_version`, contains the version of the platform runtime to which the client is currently connected.

## Notification name changes

The `TOBII_NOTIFICATION_TYPE_COMBINED_GAZE_FACTOR_CHANGED` notification name has been changed to `TOBII_NOTIFICATION_TYPE_COMBINED_GAZE_EYE_SELECTION_CHANGED`.

## New error return code

New `tobii_error_t` return code:
- `TOBII_ERROR_CONNECTION_FAILED_DRIVER` will be returned whenever the runtime reports that the tracker has disconnected from the runtime.

- `TOBII_ERROR_CONNECTION_FAILED` will now only be returned whenever a disconnect occurs between SEC and the platform runtime.

## New capabilities

More compound stream capabilities have been added to the `tobii_capability_t` enum.

    typedef enum tobii_capability_t
    {
        ...
        TOBII_CAPABILITY_COMPOUND_STREAM_USER_POSITION_GUIDE_XY,
        TOBII_CAPABILITY_COMPOUND_STREAM_USER_POSITION_GUIDE_Z,
        TOBII_CAPABILITY_COMPOUND_STREAM_WEARABLE_LIMITED_IMAGE,
        TOBII_CAPABILITY_COMPOUND_STREAM_WEARABLE_PUPIL_DIAMETER,
        TOBII_CAPABILITY_COMPOUND_STREAM_WEARABLE_PUPIL_POSITION,
        TOBII_CAPABILITY_COMPOUND_STREAM_WEARABLE_EYE_OPENNESS,
        TOBII_CAPABILITY_COMPOUND_STREAM_WEARABLE_3D_GAZE_PER_EYE,
        TOBII_CAPABILITY_COMPOUND_STREAM_WEARABLE_USER_POSITION_GUIDE_XY,
        TOBII_CAPABILITY_COMPOUND_STREAM_WEARABLE_TRACKING_IMPROVEMENTS,
        TOBII_CAPABILITY_COMPOUND_STREAM_WEARABLE_CONVERGENCE_DISTANCE,
    } tobii_capability_t;

## Updates to the Wearable API

New fields for combined gaze, convergence distance and tracking improvements has been added to the to `tobii_wearable_data_t` struct:

    typedef struct tobii_wearable_data_t
    {
        ...
        tobii_validity_t gaze_origin_combined_validity;
        float gaze_origin_combined_mm_xyz[3];
        tobii_validity_t gaze_direction_combined_validity;
        float gaze_direction_combined_normalized_xyz[3];
        tobii_validity_t convergence_distance_validity;
        float convergence_distance_mm;
        int tracking_improvements_count;
        tobii_wearable_tracking_improvement_t tracking_improvements[ 10 ];
        tobii_state_bool_t improve_user_position_hmd;
        tobii_state_bool_t increase_eye_relief;
    } tobii_wearable_data_t;

New fields for position guide signal has been added to the `tobii_wearable_eye_t` struct:

    typedef struct tobii_wearable_eye_t
    {
        ...
        tobii_validity_t position_guide_validity;
        float position_guide_xy[2];
    } tobii_wearable_data_t;

The `tracking_improvements` field can have the following values:

    typedef enum tobii_wearable_tracking_improvement_t
    {
        TOBII_WEARABLE_TRACKING_IMPROVEMENT_USER_POSITION_HMD,
        TOBII_WEARABLE_TRACKING_IMPROVEMENT_CALIBRATION_CONTAINS_POOR_DATA,
        TOBII_WEARABLE_TRACKING_IMPROVEMENT_CALIBRATION_DIFFERENT_BRIGHTNESS,
        TOBII_WEARABLE_TRACKING_IMPROVEMENT_IMAGE_QUALITY,
        TOBII_WEARABLE_TRACKING_IMPROVEMENT_INCREASE_EYE_RELIEF,
    } tobii_wearable_tracking_improvement_t;