#ifndef sample_timesync_thread_h
#define sample_timesync_thread_h

typedef struct thread_context_t thread_context_t;
typedef struct tobii_device_t tobii_device_t;

thread_context_t* timesync_thread_create( tobii_device_t* device );

void timesync_thread_destroy( thread_context_t* context );

#endif // sample_timesync_thread_h
