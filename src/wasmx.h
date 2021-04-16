#include "wasm3.h"
#include <m3_api_defs.h>

#if WASI && d_m3HasWASI
    #include "m3_api_wasi.h"
    //#include "m3_api_libc.h"
    #include "m3_api_tracer.h"
#else
    #pragma message  "No WASI !"
#endif

#include <m3_env.h>
