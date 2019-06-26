#ifndef sample_main_loop_linux_h
#define sample_main_loop_linux_h

typedef struct tobii_device_t tobii_device_t;

void main_loop( tobii_device_t* device, void ( *action )( void* context ), void* context );

#endif // sample_main_loop_linux_h
