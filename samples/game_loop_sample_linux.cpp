#include <tobii/tobii.h>
#include <tobii/tobii_streams.h>

#include "main_loop_linux.h"
#include "timesync_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <chrono>
#include <thread>


static void gaze_callback( tobii_gaze_point_t const* gaze_point, void* user_data )
{
    // Store the latest gaze point data in the supplied storage
    tobii_gaze_point_t* gaze_point_storage = (tobii_gaze_point_t*) user_data;
    *gaze_point_storage = *gaze_point;
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

static void log_func( void*, tobii_log_level_t level, char const* text )
{
    if( level == TOBII_LOG_LEVEL_ERROR )
        fprintf( stderr, "Logged error: %s\n", text );
}

extern "C" int game_loop_sample_main( void );
extern "C" int game_loop_sample_main( void )
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
    error = tobii_device_create( api, selected_device, TOBII_FIELD_OF_USE_INTERACTIVE, &device );
    free_device_list( &devices );
    if( error != TOBII_ERROR_NO_ERROR )
    {
        fprintf( stderr, "Failed to initialize the device with url %s.\n", selected_device );
        tobii_api_destroy( api );
        return 1;
    }

    tobii_gaze_point_t latest_gaze_point;
    latest_gaze_point.timestamp_us = 0LL;
    latest_gaze_point.validity = TOBII_VALIDITY_INVALID;
    // Start subscribing to gaze point data, in this sample we supply a tobii_gaze_point_t variable to store latest value.
    error = tobii_gaze_point_subscribe( device, gaze_callback, &latest_gaze_point );
    if( error != TOBII_ERROR_NO_ERROR )
    {
        fprintf( stderr, "Failed to subscribe to gaze stream.\n" );

        tobii_device_destroy( device );
        tobii_api_destroy( api );
        return 1;
    }

    // Create event objects used for inter thread signaling
    thread_context_t* thread_context = timesync_thread_create( device );

    // Create and run the reconnect and timesync thread

    auto run_game_loop = []( void* context ) {
        auto gaze_point = static_cast<tobii_gaze_point_t*>( context );
        // Perform work i.e game loop code here - let's emulate it with a sleep
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

        // Use the gaze point data
        if( gaze_point->validity == TOBII_VALIDITY_VALID )
            printf( "Gaze point: %" PRIu64 " %f, %f\n", gaze_point->timestamp_us,
                gaze_point->position_xy[ 0 ], gaze_point->position_xy[ 1 ] );
        else
            printf( "Gaze point: %" PRIu64 " INVALID\n", gaze_point->timestamp_us );
        
        // Perform work which needs eye tracking data and the rest of the game loop
        std::this_thread::sleep_for( std::chrono::milliseconds( 6 ) );
    };

    main_loop( device, run_game_loop, &latest_gaze_point );
    timesync_thread_destroy( thread_context );

    error = tobii_gaze_point_unsubscribe( device );
    if( error != TOBII_ERROR_NO_ERROR )
        fprintf( stderr, "Failed to unsubscribe from gaze stream.\n" );

    error = tobii_device_destroy( device );
    if( error != TOBII_ERROR_NO_ERROR )
        fprintf( stderr, "Failed to destroy device.\n" );

    error = tobii_api_destroy( api );
    if( error != TOBII_ERROR_NO_ERROR )
        fprintf( stderr, "Failed to destroy API.\n" );

    return 0;
}
