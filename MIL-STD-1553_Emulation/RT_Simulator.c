#include "MIL-STD-1553_Library.c"

#define SOURCE_PORT 2001
#define DESTINATION_PORT 2000
#define RT_ADDRESS THRUSTER //Assign this the value the RT represents
#define USER_CLASS RT_CLASS

/* =====================================================

   This program is used to initialize a bus controller
   on the machine simulating the BC. The port the BC is
   listening on (SOURCE_PORT) and the port the BC should
   send data to (DESTINATION_PORT) can be defined in the
   variables above. All initialization is done within
   the MIL-STD-1553_Library. To add more remote terminals,
   define their function and address at the top of the 
   1553 Library code.

   ===================================================== */

int main()
{
    initialize_library(SOURCE_PORT, DESTINATION_PORT, RT_ADDRESS, USER_CLASS);
    while (1);
    return 0;
    
}
