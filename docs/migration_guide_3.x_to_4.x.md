# Stream Engine Migration Guide 3.x to 4.x

## Introduction

This document is intended to help developers transition from older stream engine library 3.x to latest stream engine version 4.0. It is written in the format of a change log with complementary information that will assist developer making a smooth transition.

## Device create

A new `field_of_use` parameter has been added to the `tobii_device_create` and `tobii_device_create_ex` functions. This forces the developer to make a choice regarding the usage of the eye tracking data:

-   `TOBII_FIELD_OF_USE_INTERACTIVE`

Device will be created for interactive use. No special license is required for this type of use. Eye tracking data is only used as a user input for interaction experiences and cannot be stored, transmitted, nor analyzed or processed for other purposes.

-   `TOBII_FIELD_OF_USE_ANALYTICAL`

Device will be created for analytical use. This requires a special license from Tobii. Eye tracking data is used to analyze user attention, behavior or decisions in applications that store, transfer, record or analyze the data.

## Firmware upgrade state

The firmware upgrade state signaling has been re-designed. The `tobii_device_create`, `tobii_device_create_ex` and `tobii_device_reconnect` functions will now return the error code, `TOBII_ERROR_FIRMWARE_UPGRADE_IN_PROGRESS`, when the device is in firmware upgrade state. It is considered best practice to check the return code from these functions and if the device is in firmware upgrade state, continue polling until a `TOBII_ERROR_NO_ERROR` error code is recieved.

The `tobii_get_fiirmware_upgrade_state` function that was previously used to query the firmware upgrade state has been removed.

## Wearable data stream

The wearable data stream has been removed and split into two separate streams `wearable_consumer` and `wearable_advanced`. The `wearable_consumer` stream includes data suitable for consumer use like combined gaze direction, gaze origin and eyeposition. Where as the `wearable_advanced` stream is a superset of the consumer stream and includes per eye gaze data and requires a professional license.

## Advanced gaze data stream

The field `eye_position_validity` has been added to the `tobii_gaza_data_t` struct. The data field `eye_position_in_track_box_normalized_xyz` previously was erroneously grouped with the `gaze_origin_validity` but now has it's own validity flag `eye_position_validity`.

## .NET Interop bindings

Added user data parameters to subscription functions and subscription callbacks in the .NET interop binding. This is to support .NET frameworks that do not support lambdas and are therefore dependent on having the ability to send custom data through the callback interface.