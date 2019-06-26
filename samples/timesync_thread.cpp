#include "timesync_thread.h"
#include <tobii/tobii.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

struct thread_context_t
{
    tobii_device_t* device;
    std::condition_variable cv;
    std::mutex cv_m;
    std::thread thread_handle;
    bool exit_event; // Used to signal that the background thead should exit
};

static void timesync_thread( void* param )
{
    thread_context_t* context = static_cast<thread_context_t*>( param );

    for( ;; )
    {
        std::unique_lock<std::mutex> lk( context->cv_m );
        std::chrono::milliseconds timesync_timeout{30 * 1000};
        context->cv.wait_for( lk, timesync_timeout, [&] { return context->exit_event; } );

        // Block here, waiting for one of the three events.
        if( !context->exit_event ) // Handle timesync event
        {
            tobii_error_t error = tobii_update_timesync( context->device );
            std::chrono::milliseconds timesync_update_interval{30 * 1000}; // Time sync every 30 s
            std::chrono::milliseconds timesync_retry_interval{100}; // Retry time sync every 100 ms
            if (error == TOBII_ERROR_NO_ERROR)
                timesync_timeout = timesync_update_interval;
            else
                timesync_timeout = timesync_retry_interval;
        }
        else // Handle exit event
        {
            // Exit requested, exiting the thread
            return;
        }
    }
}

thread_context_t* timesync_thread_create( tobii_device_t* device )
{
    auto context = new thread_context_t;
    context->device = device;
    context->exit_event = false;
    context->thread_handle = std::thread( timesync_thread, context );
    return context;
}

void timesync_thread_destroy( thread_context_t* context )
{
    // Signal timesync thread to exit and clean up event objects.
    {
        std::lock_guard<std::mutex> lk( context->cv_m );
        context->exit_event = true;
    }
    context->cv.notify_all();
    context->thread_handle.join();

    delete context;
}
