#include <tobii/tobii.h>

#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>

#include <stdio.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>

static int _kbhit()
{
    static const int STDIN = 0;
    static bool initialized = false;

    if( !initialized )
    {
        // Use termios to turn off line buffering
        termios term;
        tcgetattr( STDIN, &term );
        term.c_lflag &= ~ICANON;
        tcsetattr( STDIN, TCSANOW, &term );
        setbuf( stdin, NULL );
        initialized = true;
    }

    int bytesWaiting;
    ioctl( STDIN, FIONREAD, &bytesWaiting );
    return bytesWaiting;
}

void main_loop( tobii_device_t* device, void ( *action )( void* context ), void* context )
{
    bool running = true;
    bool try_reconnect = false;
    while( running )
    {
        tobii_error_t error = TOBII_ERROR_NO_ERROR;
        if( _kbhit() ) break;
        if( try_reconnect )
        {
            error = tobii_device_reconnect( device );
            if( error != TOBII_ERROR_NO_ERROR )
            {
                // We couldn't get the connection even if it doesn't return connection failed.
                // So try connection again after sleep time.
                std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
                continue;
            }
            try_reconnect = false;
        }
        error = tobii_wait_for_callbacks( 1, &device );
        if( error == TOBII_ERROR_CONNECTION_FAILED )
        {
            try_reconnect = true;
            continue;
        }

        if( error == TOBII_ERROR_NO_ERROR || error == TOBII_ERROR_TIMED_OUT )
            error = tobii_device_process_callbacks( device );

        if( error == TOBII_ERROR_CONNECTION_FAILED ) try_reconnect = true;

        action( context );
    }
}


