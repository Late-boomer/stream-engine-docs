#include <tobii/tobii.h>
#include <tobii/tobii_config.h>
#include <stdio.h>
#include <assert.h>
#pragma warning( push )
#pragma warning( disable: 4255 )
#pragma warning( disable: 4668 )
#include <windows.h>
#include <conio.h>
#pragma warning( pop )

/*
    Disclaimer: There is no error handling in this sample but it will work in a happy case scenario
*/

static tobii_error_t calibrate_stimuli_point( tobii_device_context_t* device_context, float x, float y, float z )
{
    tobii_error_t error;

    // If user is not gazing at the stimuli point TOBII_ERROR_OPERATION_FAILED will be returned after a few seconds.
    // You can retry this operation any number of times but you probably want to feedback to the user between every 
    // retry and maybe give the option to skip this point.
    for( size_t i = 0; i < 3; i++ )
    {
        printf( "Look at %f, %f, %f\n", x, y, z );
        error = tobii_calibration_collect_data_3d( device_context, x, y, z );
        if( error != TOBII_ERROR_OPERATION_FAILED ) return error;
    }

    return TOBII_ERROR_OPERATION_FAILED;
}

#define ERR_REPORT(_e) do { if (_e != TOBII_ERROR_NO_ERROR) { printf(#_e " failed with error %d\n", _e); } } while ((void)0,0)

static tobii_error_t calibrate( tobii_device_context_t* device_context )
{
    // Start calibration sets a flag in the eye tracker that makes sure only this connection can calibrate. Make sure to call stop calibration when you are done (this flag is automatically cleared if you disconnect).
    ERR_REPORT( tobii_calibration_start( device_context ) );

    // Clear calibration
    ERR_REPORT( tobii_calibration_clear( device_context ) );

    // Run points
    ERR_REPORT( calibrate_stimuli_point( device_context, 200.0, 200.0, 0.0 ) );
    ERR_REPORT( calibrate_stimuli_point( device_context, -200.0, 200.0, 0.0 ) );
    ERR_REPORT( calibrate_stimuli_point( device_context, 200.0, -200.0, 0.0 ) );
    ERR_REPORT( calibrate_stimuli_point( device_context, -200.0, -200.0, 0.0 ) );
    ERR_REPORT( calibrate_stimuli_point( device_context, 0.0, 0.0, 0.0 ) );

    // Use all collected samples and compute a calibration. This calibration will immediately be applied to the eye tracker.
    ERR_REPORT( tobii_calibration_compute_and_apply( device_context ) );

    // Stop calibration clears the flag in the eye tracker and let other processes start a calibration
    ERR_REPORT( tobii_calibration_stop( device_context ) );
    return TOBII_ERROR_NO_ERROR;
}

int calibration_sample_vr_main()
{
    // Almost all methods in the Tobii API returns a tobii_error_t. The Tobii API never throws an exception.
    tobii_error_t error;

    // To be able to use the API you need to create the API device_context. This is your chance to provide custom handling for memory allocation and logging.
    tobii_api_context_t* api_context;
    error = tobii_api_context_create( &api_context, 0, 0 ); assert( error == TOBII_ERROR_NO_ERROR );
    if( error != TOBII_ERROR_NO_ERROR )
    {
        printf( "tobii_api_context_create failed with %d\n", error );
        _getch();
        exit( 1 );
    }

    // A device_context holds all the state related to a connection to an eye tracker. You will pass this as an argument to most API methods.
    tobii_device_context_t* device_context;

    // Sending an empty url connects to the first eye tracker found.
    error = tobii_context_create_ex( api_context, 0, 0, 0, 0, &device_context );

    if( error != TOBII_ERROR_NO_ERROR )
    {
        printf( "tobii_context_create failed with %d\n", error );
        _getch();
        exit( 1 );
    }

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

    // Destroying the device context will free up resources held by the device context
    ERR_REPORT( tobii_context_destroy( device_context ) );

    // Assigning the device_context to null after destroying it ensures that an error will be thrown if it were to be used
    // in any subsequent function call, instead of running the risk of experiencing undefined behavior
    device_context = 0;

    // Using the API after calling tobii_api_context_destroy will result in undefined behavior
    ERR_REPORT( tobii_api_context_destroy( api_context ) );

    _getch();

    return 0;
}
