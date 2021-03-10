#include "MIL-STD-1553_Library.c"
#include <stdlib.h>
#include <time.h>

#define SOURCE_PORT 2000
#define DESTINATION_PORT 2001
#define RT_ADDRESS 0x00
#define USER_CLASS BC_CLASS


/* =====================================================

   This program is used to initialize a bus controller
   on the machine simulating the BC. The port the BC is
   listening on (SOURCE_PORT) and the port the BC should
   send data to (DESTINATION_PORT) can be defined in the
   variables above. All initialization is done within
   the MIL-STD-1553_Library.

   ===================================================== */




void init_bc()
{
    initialize_library(SOURCE_PORT, DESTINATION_PORT, RT_ADDRESS, USER_CLASS);    
}
