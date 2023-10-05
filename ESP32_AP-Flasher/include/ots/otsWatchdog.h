#include <atomic.h>
#include <ctime> 


// evt. Besser in Util unterbringen??
volatile time_t process1 = 0;
volatile time_t process2 = 0;
volatile time_t process3 = 0;
volatile time_t process4 = 0;

// ATOMIC_ENTER_CRITICAL(ATOMIC_EXIT_CRITICAL());