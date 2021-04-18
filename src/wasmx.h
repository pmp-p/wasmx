#ifndef WASMX_CONFIG_DONE

#ifndef WASI
    #define WASI (1)
#endif

#define NATIVE_STACK_SIZE   3700
// wasm3 --stack-size 3700 can run wapy header only test

#define WASM_STACK_SLOTS    512


// For (most) devices that cannot allocate a 64KiB wasm page
//#define WASM_MEMORY_LIMIT   4096 NO BLINK
//#define WASM_MEMORY_LIMIT   8192+1024  // BLINK
// 41000 no modules

// ARDUINO !
#if defined(ESP8266)
    //#define WASM_MEMORY_LIMIT   40120
    #define WASM_MEMORY_LIMIT   40100

#elif defined(ESP32)
    // #define WASM_MEMORY_LIMIT   66576

    #if 1
        #define WASM_MEMORY_LIMIT   1*65535
    #else
        #define WASM_MEMORY_LIMIT   3*32768 + 16384 + 8192
    #endif

#else
// 66576 for wasi native test
    // #define WASM_MEMORY_LIMIT   2*32768+1040
    #define WASM_MEMORY_LIMIT   2*65535
#endif

#if 0
    #define d_m3FixedHeap                       (11*65535)
    #undef WASM_MEMORY_LIMIT
    #define WASM_MEMORY_LIMIT d_m3FixedHeap

    #define d_m3MaxFunctionStackHeight 2000
    #define d_m3CodePageAlignSize 4096
#else
    #define d_m3FixedHeap                       (11*65535)
    #undef WASM_MEMORY_LIMIT
    #define WASM_MEMORY_LIMIT d_m3FixedHeap
    #define d_m3MaxFunctionStackHeight          96
    #define d_m3CodePageAlignSize               1024
#endif

#define WASMX_CONFIG_DONE
#endif // WASMX_CONFIG_DONE

#ifndef WASMX_CONFIG_ONLY

#include "wasm3.h"

#include "m3_config.h"


#include <m3_api_defs.h>

#if WASI && d_m3HasWASI
    #include "m3_api_wasi.h"
    //#include "m3_api_libc.h"
    #include "m3_api_tracer.h"
#else
    #pragma message  "No WASI !"
#endif

#include <m3_env.h>

#endif // WASMX_CONFIG_ONLY
