/*
 * Wasm3 - high performance WebAssembly interpreter written in C.
 * Copyright © 2020 Volodymyr Shymanskyy, Steven Massey.
 * All rights reserved.
 */
#include <Arduino.h>


//#define MEMTEST (1)
#define WASI (1)
#define PSRAM (1)



#ifndef d_m3HasWASI
    #define d_m3HasWASI WASI
#endif


#ifndef LED_BUILTIN
    //ESP32
    //#define LED_BUILTIN 2

    //ESP32-S cam / wrover-b has none
    #define LED_BUILTIN 4

    //ESP12  gpio 2
    //ESP8266 16
#endif




#include "wasmx.h"

#if defined(__cplusplus) && !defined(ESP32)
extern "C" {
#endif
void cdbg(const char *fmt, ...) {
    va_list argptr;
    va_start (argptr, fmt );
    vsnprintf_P(RB,RB_SIZE,fmt,argptr);
    if (COUT_FUNCTIONPTR)
        COUT_FUNCTIONPTR();
    va_end (argptr);
}
#if defined(__cplusplus) && !defined(ESP32)
}
#endif


#if WASI

    // arduino
    //#include "app.irom.h"

    // WASI tests
    #include "/data/git/wasmx/wapy.wasi.h"
    //#include "/data/git/wasmx/examples_wasi/wapy/hello.h"

#else
    // arduino
    #include "app.irom.h"
#endif


#define FATAL(func, msg) { Serial.print(PSTR("Fatal: " func " ")); Serial.println(msg); return; }

#if DEBUG
    #define CHECK(msg, rvt) { Serial.println(PSTR(msg)); if(rvt) FATAL(msg, rvt); }
#else
    #define CHECK(msg, rvt) rvt
#endif


extern void_v COUT_FUNCTIONPTR;

PROGMEM void cout() {
    static int last_slash = 0;
    if (RB[0]=='/') {
        for (int i=0;i<RB_MAX;i++) {
                if (RB[i]==':') {
                    last_slash++;
                    goto display;
                }
                if (RB[i]=='/')
                    last_slash = i;
        }
        last_slash = 0;
    }
display:
    Serial.println(&RB[last_slash]);
    RB[0]=0;
}


// 104 B
PROGMEM void common_setup() {
    pinMode( LED_BUILTIN, OUTPUT);
    Serial.begin(115200);

    // Wait for serial port to connect
    // Needed for native USB port only
    while(!Serial) { yield();}

    delay(333);
    Serial.println(PSTR("\r\nWasm3 v" M3_VERSION " (" M3_ARCH "), build " __DATE__ " " __TIME__));
}

#if defined(MEMTEST)
#pragma message "memory size test mode"
// 26884 e7ba0bd1ddc97930a642b429e6960f2dc8d8e9d8  (d1 8266)
void setup()
{
    common_setup(); // 8 B
}
#else

#if WASI
    #pragma message TOSTRING(WASM_MEMORY_LIMIT)
#else
    #include "extra/arduino.extra"
#endif


PROGMEM void wasm_task(void*)
{
    #ifndef BC_IN_ROM
        BC_IN_ROM = (uintptr_t)app_wasm > ROM_BASE;
    #else
        #pragma message "BC_IN_ROM is defined"
    #endif

    // 384 - 27268
    COUT_FUNCTIONPTR = cout;

    if ( init_ram( d_m3FixedHeap ) ) {

        M3Result result = m3Err_none;

        // 1416 - 28684
        IM3Environment env = m3_NewEnvironment ();

#if DEBUG
        if (!env)
            FATAL("NewEnvironment", "failed");
#endif

        // 268 - 28952
        IM3Runtime runtime = m3_NewRuntime (env, WASM_STACK_SLOTS, NULL);

#if DEBUG
        if (!runtime)
            FATAL("NewRuntime", "failed");
#endif

        runtime->memoryLimit = WASM_MEMORY_LIMIT;

#pragma message "do r/w test of WASM memory !"


        // 28952
        IM3Module module;

#ifndef BC_IN_ROM
        if (BC_IN_ROM) {
            CHECK("ParseModule_ROM", m3_ParseModule (env, &module, app_wasm, app_wasm_len) );
        } else {
            CHECK("ParseModule_RAM", m3_ParseModule (env, &module, &app_wasm[0], app_wasm_len) );
        }
#else
        // 6076 - 35024
        CHECK("ParseModule_ROM", m3_ParseModule(env, &module, app_wasm, app_wasm_len) );
#endif

#if DEBUG
        CLOG("PARSED");
#endif

        // 4 - 35028
        CHECK("LoadModule", m3_LoadModule (runtime, module) );

        #if WASI
            // 620 - 35648
            CHECK("LinkWASI",  m3_LinkWASI(runtime->modules) );
        #else
            CHECK("LinkArduino", LinkArduino(runtime->modules) );
        #endif

        IM3Function f;

#if DEBUG
        // Check for TinyGo entry function first
        // See https://github.com/tinygo-org/tinygo/issues/866
        result = m3_FindFunction (&f, runtime, "cwa_main");

        if (result) {
            result = m3_FindFunction (&f, runtime, "_start");
        }

        CHECK("FindFunction", result);
#else
        // 8 - 35036
        CHECK("FindFunction", m3_FindFunction (&f, runtime, "_start"));

#ifndef __ARDUINO__
        delay(3000);
#endif

        // 12 - 35048
        result = (M3Result)( m3_CallV (f) );
        CLOG("exit value = %d", result) ;
    }
#if ESP32
    vTaskDelete(NULL);
#else
    delay(32000);
#endif

#endif

}

void setup()
{
    common_setup();

#if ESP32
    // On ESP32, we can launch in a separate thread
    xTaskCreate(&wasm_task, "wasm3", NATIVE_STACK_SIZE, NULL, 5, NULL);
#else
    wasm_task(NULL);
    Serial.println(F("\r\ndone?"));
#endif

}


#endif

#if 0


// 46912
PROGMEM void wasm_task(void*)
{



    //const char* i_argv[1] = { NULL };

    if (f) {
        Serial.println(F("Running WebAssembly..."));
        M3Result rv = (M3Result)( m3_CallV (f) ); //, 0, i_argv);
        Serial.print(F("\r\n-------\r\nResult = "));
        Serial.println(rv);

    } else {
        Serial.println(F("entry not found !"));
    }

    // Should not arrive here
    if (result) {
        M3ErrorInfo info;
        m3_GetErrorInfo (runtime, &info);
        Serial.print(F("Error: "));
        Serial.print(result);
        Serial.print(" (");
        Serial.print(info.message);
        Serial.println(")");
        if (info.file && strlen(info.file) && info.line) {
            Serial.print(F("At "));
            Serial.print(info.file);
            Serial.print(":");
            Serial.println(info.line);
        }
    }

}

#endif



void loop() {
    delay(1000);
}
