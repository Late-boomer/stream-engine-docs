#include <tobii/tobii_config.h>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#pragma warning( push )
#pragma warning( disable: 4255 )
#pragma warning( disable: 4668 )
#include <windows.h>
#pragma warning( pop )


static tobii_error_t calibrate_stimuli_point( tobii_device_context_t* device_context, float x, float y )
{
    tobii_error_t error;

    // If user is not gazing at the stimuli point TOBII_ERROR_OPERATION_FAILED will be returned after a few seconds.
    // You can retry this operation any number of times but you probably want to feedback to the user between every 
    // retry and maybe give the option to skip this point.
    for( size_t i = 0; i < 3; i++ )
    {
        printf( "Look at %f, %f\n", x, y );
        error = tobii_calibration_collect_data_2d( device_context, x, y );
        if( error != TOBII_ERROR_OPERATION_FAILED ) return error;
    }

    return TOBII_ERROR_OPERATION_FAILED;
}

static tobii_error_t calibrate( tobii_device_context_t* device_context )
{
    tobii_error_t error;

    // Start calibration sets a flag in the eye tracker that makes sure only this connection can calibrate. Make sure to call stop calibration when you are done (this flag is automatically cleared if you disconnect).
    error = tobii_calibration_start( device_context );
    if( error != TOBII_ERROR_NO_ERROR ) return error;


    // Calibrate stimuli point 1.
    error = calibrate_stimuli_point( device_context, 0.1f, 0.5f );
    if( error != TOBII_ERROR_NO_ERROR )
    {
        if( error == TOBII_ERROR_OPERATION_FAILED ) tobii_calibration_stop( device_context );
        return error;
    }

    // Calibrate stimuli point 2
    error = calibrate_stimuli_point( device_context, 0.5f, 0.5f );
    if( error != TOBII_ERROR_NO_ERROR )
    {
        if( error == TOBII_ERROR_OPERATION_FAILED ) tobii_calibration_stop( device_context );
        return error;
    }

    // Calibrate stimuli point 3
    error = calibrate_stimuli_point( device_context, 0.9f, 0.5f );
    if( error != TOBII_ERROR_NO_ERROR )
    {
        if( error == TOBII_ERROR_OPERATION_FAILED ) tobii_calibration_stop( device_context );
        return error;
    }


    // Use all collected samples and compute a calibration. This calibration will immediately be applied to the eye tracker.
    error = tobii_calibration_compute_and_apply( device_context );
    if( error != TOBII_ERROR_NO_ERROR )
    {
        if( error == TOBII_ERROR_OPERATION_FAILED ) tobii_calibration_stop( device_context );
        return error;
    }

    // Stop calibration clears the flag in the eye tracker and let other processes start a calibration
    error = tobii_calibration_stop( device_context );
    assert( error != TOBII_ERROR_CALIBRATION_NOT_STARTED ); // If this happened you probably have something wrong in your flow logic
    if( error != TOBII_ERROR_NO_ERROR ) return error;

    return TOBII_ERROR_NO_ERROR;
}

static size_t read_license_file(uint16_t* license)
{
    FILE *license_file = fopen("se_license_key_sample", "rb");

    if (!license_file)
    {
        printf("License key could not found!\n");
        return 0;
    }

    fseek(license_file, 0, SEEK_END);
    long file_size = ftell(license_file);
    rewind(license_file);

    if (file_size <= 0)
    {
        printf("License file is empty!\n");
        return 0;
    }

    if (license) { fread(license, sizeof(uint16_t), file_size / sizeof(uint16_t), license_file); }

    fclose(license_file);
    return (size_t) file_size;
}

static tobii_error_t context_create(tobii_api_context_t* api_context, tobii_device_context_t** device_context )
{
    //License
    size_t license_size = read_license_file( 0 ); assert( license_size > 0 );
    uint16_t* license_key = (uint16_t*)malloc( license_size );
    memset( license_key, 0, license_size );
    read_license_file( license_key );

    tobii_license_key_t license = { license_key, license_size };

    // Sending an empty url connects to the first eye tracker found. We specify a license key to get access to wearable data.
    tobii_error_t error = tobii_context_create_ex( api_context, 0, &license, 1, 0, device_context );
    free( license_key );
    return error;
}

int calibration_sample_main()
{
    // To be able to use the API you need to create the API device_context. This is your chance to provide custom handling for memory allocation and logging.
    tobii_api_context_t* api_context;
    // Almost all methods in the Tobii API returns a tobii_error_t. The Tobii API never throws an exception.
    tobii_error_t error = tobii_api_context_create( &api_context, 0, 0 ); assert( error == TOBII_ERROR_NO_ERROR );

    // A device context holds all the state related to a connection to an eye tracker. You will pass this as an argument to most API methods.
    tobii_device_context_t* device_context;
    error = context_create( api_context, &device_context ); assert( error == TOBII_ERROR_NO_ERROR );

    bool completed = false;

    while( !completed )
    {
        error = calibrate( device_context );
        if( error == TOBII_ERROR_NO_ERROR )
        {
            completed = true;
        }
        else if( error == TOBII_ERROR_CONNECTION_FAILED )
        {
            // Try to reconnect the eye tracker for all eternity. You probably want some smarter handling.
            while( error != TOBII_ERROR_NO_ERROR )
            {
                Sleep( 1000 ); // To avoid busy wait
                error = tobii_reconnect( device_context );
            }
        }
        else if( error == TOBII_ERROR_CALIBRATION_ALREADY_STARTED )
        {
            Sleep( 5000 ); // Sleep for a while and try again. Some other process was making a calibration.    
        }
    }


    // Destroying the device_context will free up resources held by the device_context
    error = tobii_context_destroy( device_context ); assert( error == TOBII_ERROR_NO_ERROR );

    // Assigning the device_context pointer to null after destroying it ensures that an error will be thrown if it were to be used
    // in any subsequent function call, instead of running the risk of experiencing undefined behavior
    device_context = 0;

    // Using the API after calling tobii_api_context_destroy will result in undefined behavior
    error = tobii_api_context_destroy( api_context ); assert( error == TOBII_ERROR_NO_ERROR );

    return 0;
}