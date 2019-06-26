#include <tobii/tobii.h>
#include <tobii/tobii_wearable.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>

#ifdef _WIN32
    #include <conio.h>
#else
    #include <unistd.h>
    #include <sys/select.h>
    static int _kbhit()
    {
        struct timeval tv = { 0L, 0L };
        fd_set fds;
        FD_ZERO( &fds );
        FD_SET( 0, &fds );
        return select( 1, &fds, NULL, NULL, &tv );
    }
#endif
static void wearable_callback( tobii_wearable_data_t const* data, void* /* user_data */ )
{
    printf( "Gaze Direction: T %" PRIu64, data->timestamp_tracker_us );
    if( data->left.gaze_direction_validity == TOBII_VALIDITY_VALID )
    {
        printf( "\tLeft {x:% 2.2f, y:% 2.2f, z:% 2.2f} ",
            data->left.gaze_direction_normalized_xyz[ 0 ],
            data->left.gaze_direction_normalized_xyz[ 1 ],
            data->left.gaze_direction_normalized_xyz[ 2 ] );
    }
    else
    {
        printf( "\tLeft INVALID\t\t\t" );
    }

    if( data->right.gaze_direction_validity == TOBII_VALIDITY_VALID )
    {
        printf( "\tRight {x:% 2.2f, y:% 2.2f, z:% 2.2f}",
            data->right.gaze_direction_normalized_xyz[ 0 ],
            data->right.gaze_direction_normalized_xyz[ 1 ],
            data->right.gaze_direction_normalized_xyz[ 2 ] );
    }
    else
    {
        printf( "\tRight INVALID\t\t\t" );
    }

    printf( "\n" );
}

int wearable_sample_main()
{
    // Almost all methods in the Tobii API returns a tobii_error_t. The Tobii API never throws an exception.
    tobii_error_t error;

    // To be able to use the API you need to create the API. This is your chance to provide custom handling for memory allocation and logging.
    tobii_api_t* api;
    error = tobii_api_create( &api, 0, 0 ); assert( error == TOBII_ERROR_NO_ERROR );

    // A device holds all the state related to a connection to an eye tracker. You will pass this as an argument to most API methods.
    tobii_device_t* device;

    // Sending an empty url connects to the first eye tracker found.
    error = tobii_device_create( api, 0, &device );
    assert( error == TOBII_ERROR_NO_ERROR );

    // Subscribe to a stream of wearable eye tracking data. The callback you supply will be called from tobii_process_callbacks.
    error = tobii_wearable_data_subscribe( device, wearable_callback, 0 ); assert( error == TOBII_ERROR_NO_ERROR );

    // If you have a natural loop (game loop or similar) occurring frequently (> 10 times a second) in your system you
    // could process call back data in the loop
    while( !_kbhit() )
    {
        // This method will wait for eye tracker data and may return TOBII_ERROR_TIMED_OUT if data is not available
        // after arbitrary period of time. So this method could be used in a loop to wait for data and
        // process data streams
        error = tobii_wait_for_callbacks( device ); assert( error == TOBII_ERROR_NO_ERROR || error == TOBII_ERROR_TIMED_OUT );

        // Processing callbacks will read the input buffers and call the registered subscription callbacks.
        // If you have any subscriptions active this method needs to be called > 10 times a second or the eye tracker might disconnect.
        // You don't need to call this method if you have no active subscriptions.
        if( error == TOBII_ERROR_NO_ERROR )
        {
            error = tobii_process_callbacks( device );
        }
    }

    // Always unsubscribe as soon as you no longer need the data
    error = tobii_wearable_data_unsubscribe( device ); assert( error == TOBII_ERROR_NO_ERROR );

    // Destroying the device will free up resources held by the device
	error = tobii_device_destroy( device ); assert( error == TOBII_ERROR_NO_ERROR );

    // Assigning the device to null after destroying it ensures that an error will be thrown if it were to be used
    // in any subsequent function call, instead of running the risk of experiencing undefined behavior
    device = 0;

    // Using the API after calling tobii_api_destroy will result in undefined behavior
    error = tobii_api_destroy( api ); assert( error == TOBII_ERROR_NO_ERROR );

	return 0;
}

