#include <tobii/tobii.h>
#include <tobii/tobii_wearable.h>

#include "main_loop_linux.h"
#include "timesync_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <chrono>
#include <thread>

static void wearable_callback( tobii_wearable_data_t const* wearable_data, void* user_data )
{
    // Store the latest wearable data in the supplied storage
    tobii_wearable_data_t* wearable_data_storage = (tobii_wearable_data_t*) user_data;
    *wearable_data_storage = *wearable_data;
}

struct url_receiver_context_t
{
    char** urls;
    int capacity;
    int count;
};

static void url_receiver( char const* url, void* user_data )
{
    // The memory context is passed through the user_data void pointer
    struct url_receiver_context_t* context = (struct url_receiver_context_t*) user_data;
    // Allocate more memory if maximum capacity has been reached
    if( context->count >= context->capacity )
    {
        context->capacity *= 2;
        char** urls = (char**) realloc( context->urls, sizeof( char* ) * context->capacity );
        if( !urls )
        {
            fprintf( stderr, "Allocation failed\n" );
            return;
        }
        context->urls = urls;
    }
    // Copy the url string input parameter to allocated memory
    size_t url_length = strlen( url ) + 1;
    context->urls[ context->count ] = (char*)malloc( url_length );
    if( !context->urls[ context->count ] )
    {
        fprintf( stderr, "Allocation failed\n" );
        return;
    }
    memcpy( context->urls[ context->count++ ], url, url_length );
}


struct device_list_t
{
    char** urls;
    int count;
};

static struct device_list_t list_devices( tobii_api_t* api )
{
    struct device_list_t list = { NULL, 0 };
    // Create a memory context that can be used by the url receiver callback
    struct url_receiver_context_t url_receiver_context;
    url_receiver_context.count = 0;
    url_receiver_context.capacity = 16;
    url_receiver_context.urls = (char**) malloc( sizeof( char* ) * url_receiver_context.capacity );
    if( !url_receiver_context.urls )
    {
        fprintf( stderr, "Allocation failed\n" );
        return list;
    }

    // Enumerate the connected devices, connected devices will be stored in the supplied memory context
    tobii_error_t error = tobii_enumerate_local_device_urls( api, url_receiver, &url_receiver_context );
    if( error != TOBII_ERROR_NO_ERROR ) fprintf( stderr, "Failed to enumerate devices.\n" );

    list.urls = url_receiver_context.urls;
    list.count = url_receiver_context.count;
    return list;
}


static void free_device_list( struct device_list_t* list )
{
    for( int i = 0; i < list->count; ++i )
        free( list->urls[ i ] );

    free( list->urls );
}


static char const* select_device( struct device_list_t* devices )
{
    int tmp = 0, selection = 0;

    // Present the available devices and loop until user has selected a valid device
    while( selection < 1 || selection > devices->count)
    {
        printf( "\nSelect a device\n\n" );
        for( int i = 0; i < devices->count; ++i ) printf( "%d. %s\n",  i + 1, devices->urls[ i ] ) ;
        if( scanf( "%d", &selection ) <= 0 ) while( ( tmp = getchar() ) != '\n' && tmp != EOF );
    }

    return devices->urls[ selection - 1 ];
}

static void log_func( void* log_context, tobii_log_level_t level, char const* text )
{
    (void)log_context;
    if( level == TOBII_LOG_LEVEL_ERROR )
        fprintf( stderr, "Logged error: %s\n", text );
}

extern "C" int wearable_game_loop_sample_main( void );
extern "C" int wearable_game_loop_sample_main( void )
{
    // Initialize critical section, used for thread synchronization in the log function
    tobii_custom_log_t custom_log = { nullptr, log_func };

    tobii_api_t* api;
    tobii_error_t error = tobii_api_create( &api, NULL, &custom_log );
    if( error != TOBII_ERROR_NO_ERROR )
    {
        fprintf( stderr, "Failed to initialize the Tobii Stream Engine API.\n" );
        return 1;
    }

    struct device_list_t devices = list_devices( api );
    if( devices.count == 0 )
    {
        fprintf( stderr, "No stream engine compatible device(s) found.\n" );
        free_device_list( &devices );
        tobii_api_destroy( api );
        return 1;
    }
    char const* selected_device = devices.count == 1 ? devices.urls[ 0 ] : select_device( &devices );
    printf( "Connecting to %s.\n", selected_device );

    tobii_device_t* device;
    error = tobii_device_create( api, selected_device, &device );
    free_device_list( &devices );
    if( error != TOBII_ERROR_NO_ERROR )
    {
        fprintf( stderr, "Failed to initialize the device with url %s.\n", selected_device );
        tobii_api_destroy( api );
        return 1;
    }

    tobii_wearable_data_t latest_wearable_data;
    latest_wearable_data.timestamp_tracker_us = 0LL;
    latest_wearable_data.timestamp_system_us = 0LL;
    latest_wearable_data.frame_counter = 0;
    latest_wearable_data.left.gaze_direction_validity = TOBII_VALIDITY_INVALID;
    latest_wearable_data.right.gaze_direction_validity = TOBII_VALIDITY_INVALID;
    // Start subscribing to wearable data, in this sample we supply a tobii_wearable_data_t variable to store
    // latest value.
    error = tobii_wearable_data_subscribe( device, wearable_callback, &latest_wearable_data );
    if( error != TOBII_ERROR_NO_ERROR )
    {
        fprintf( stderr, "Failed to subscribe to gaze stream.\n" );

        tobii_device_destroy( device );
        tobii_api_destroy( api );
        return 1;
    }

    // Create and run the reconnect and timesync thread
    thread_context_t* thread_context = timesync_thread_create( device );

    auto run_game_loop = []( void* context ) {
        auto wearable_data = static_cast<tobii_wearable_data_t*>( context );
        // Perform work i.e game loop code here - let's emulate it with a sleep
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        // Use the gaze point data
        printf( "Gaze Direction: T %" PRIu64, wearable_data->timestamp_tracker_us );
        if( wearable_data->left.gaze_direction_validity == TOBII_VALIDITY_VALID )
        {
            printf( "\tLeft {x:% 2.2f, y:% 2.2f, z:% 2.2f} ",
                wearable_data->left.gaze_direction_normalized_xyz[ 0 ],
                wearable_data->left.gaze_direction_normalized_xyz[ 1 ],
                wearable_data->left.gaze_direction_normalized_xyz[ 2 ] );
        }
        else
        {
            printf( "\tLeft INVALID\t\t\t" );
        }

        if( wearable_data->right.gaze_direction_validity == TOBII_VALIDITY_VALID )
        {
            printf( "\tRight {x:% 2.2f, y:% 2.2f, z:% 2.2f}",
                wearable_data->right.gaze_direction_normalized_xyz[ 0 ],
                wearable_data->right.gaze_direction_normalized_xyz[ 1 ],
                wearable_data->right.gaze_direction_normalized_xyz[ 2 ] );
        }
        else
        {
            printf( "\tRight INVALID\t\t\t" );
        }

        printf( "\n" );
        // Perform work which needs eye tracking data and the rest of the game loop
        std::this_thread::sleep_for( std::chrono::milliseconds( 6 ) );
    };

    main_loop( device, run_game_loop, &latest_wearable_data );
    timesync_thread_destroy( thread_context );

    error = tobii_wearable_data_unsubscribe( device );
    if( error != TOBII_ERROR_NO_ERROR )
        fprintf( stderr, "Failed to unsubscribe from wearable data stream.\n" );

    error = tobii_device_destroy( device );
    if( error != TOBII_ERROR_NO_ERROR )
        fprintf( stderr, "Failed to destroy device.\n" );

    error = tobii_api_destroy( api );
    if( error != TOBII_ERROR_NO_ERROR )
        fprintf( stderr, "Failed to destroy API.\n" );

    return 0;
}
