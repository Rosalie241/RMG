/* VI owns a separate persistent pool. Keep the platform implementation shared
 * so its publication, failure fallback and teardown rules stay identical. */
#define sr_threads_init sr_vi_threads_init
#define sr_threads_count sr_vi_threads_count
#define sr_threads_physical_cores sr_vi_threads_physical_cores
#define sr_threads_run sr_vi_threads_run
#define sr_threads_shutdown sr_vi_threads_shutdown
#ifdef _WIN32
#include "sr_threads_win32.c"
#else
#include "sr_threads_posix.c"
#endif
