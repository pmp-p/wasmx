/*
 * Wasm3 - high performance WebAssembly interpreter written in C.
 * Copyright © 2020 Volodymyr Shymanskyy, Steven Massey.
 * All rights reserved.
 */

#ifndef LED_BUILTIN
    //ESP32
    //#define LED_BUILTIN 2

    //ESP32-S cam / wrover-b has none
    #define LED_BUILTIN 4

    //ESP12  gpio 2
    //ESP8266 16
#endif


#define WASI 1

#if defined(ARDUINO)
    #define d_m3HasWASI (1)
#endif

#ifndef d_m3HasWASI
    #error "d_m3HasWASI d_m3HasTracer"
#endif

#define WASM_STACK_SLOTS    512

// wasm3 --stack-size 4096 can run wapy header only test
#define NATIVE_STACK_SIZE   (4*1024)

// For (most) devices that cannot allocate a 64KiB wasm page

//#define WASM_MEMORY_LIMIT   4096 NO BLINK
//#define WASM_MEMORY_LIMIT   8192+1024  // BLINK
#define WASM_MEMORY_LIMIT   32768

#include "wasmx.h"

#if 1
    // arduino
    //#include "app.irom.h"

    // WASI tests
    //#include "/data/cross/wapy/examples_wasi/wapy/data/app.wasm.h"
    #include "/data/git/wasmx/examples_wasi/wapy/hello.h"

#else
    // arduino
    #include "app.iram.h"
#endif

void cout() {
    Serial.print("    ");
    Serial.print(RB);
    Serial.println("]");
    RB[0]=0;
}

extern void_v COUT_FUNCTIONPTR;


/*
 * API bindings
 *
 * Note: each RawFunction should complete with one of these calls:
 *   m3ApiReturn(val)   - Returns a value
 *   m3ApiSuccess()     - Returns void (and no traps)
 *   m3ApiTrap(trap)    - Returns a trap
 */

m3ApiRawFunction(m3_arduino_millis)
{
    m3ApiReturnType (uint32_t)
    m3ApiReturn(millis());
}

m3ApiRawFunction(m3_arduino_delay)
{
    m3ApiGetArg     (uint32_t, ms)
    delay(ms);
    m3ApiSuccess();
}

uint8_t mapPinMode(uint8_t mode)
{
    switch(mode) {
        case 0: return INPUT;
        case 1: return OUTPUT;
        case 2: return INPUT_PULLUP;
    }
    return INPUT;
}


m3ApiRawFunction(m3_arduino_pinMode)
{
    m3ApiGetArg     (uint32_t, pin)
    m3ApiGetArg     (uint32_t, mode)

#if !defined(PARTICLE)
    typedef uint8_t PinMode;
#endif
    pinMode(pin, (PinMode)mapPinMode(mode));

    m3ApiSuccess();
}

m3ApiRawFunction(m3_arduino_digitalWrite)
{
    m3ApiGetArg     (uint32_t, pin)
    m3ApiGetArg     (uint32_t, value)

    digitalWrite(pin, value);
/*
    Serial.print(F("m3_arduino_digitalWrite:"));
    Serial.println(value);
    yield();
*/
    m3ApiSuccess();
}

m3ApiRawFunction(m3_arduino_getPinLED)
{
    m3ApiReturnType (uint32_t)

    Serial.print(F("m3_arduino_getPinLED:"));
    Serial.println(LED_BUILTIN);
    yield();

    m3ApiReturn(LED_BUILTIN);
}

m3ApiRawFunction(m3_arduino_print)
{
    m3ApiGetArgMem  (const uint8_t *, buf)
    m3ApiGetArg     (uint32_t,        len)
    //char RB[256];

    //printf("api: print %p %d\n", buf, len);

    Serial.print("print[");
    for (int i=0; i<len; i++)
        Serial.write((unsigned char)buf[i]);
    Serial.println("]");
    yield();
    m3ApiSuccess();
}

const char PROGMEM greet[] = "Hello WASM world!"; // 😊";

m3ApiRawFunction(m3_arduino_getGreeting)
{
    m3ApiGetArgMem  (uint8_t *,    out)
    m3ApiGetArg     (int32_t,      out_len)


    memcpy_P(out, greet, min(sizeof(greet), (unsigned int)out_len));

    m3ApiSuccess();
}

m3ApiRawFunction(m3_dummy)
{
    m3ApiSuccess();
}



PROGMEM M3Result LinkArduino(IM3Module module) {
    const char* arduino = "arduino";
    if (module) {
        m3_LinkRawFunction (module, arduino, "millis",           "i()",    &m3_arduino_millis);
        m3_LinkRawFunction (module, arduino, "delay",            "v(i)",   &m3_arduino_delay);
        m3_LinkRawFunction (module, arduino, "pinMode",          "v(ii)",  &m3_arduino_pinMode);
        m3_LinkRawFunction (module, arduino, "digitalWrite",     "v(ii)",  &m3_arduino_digitalWrite);

        // Test functions
        m3_LinkRawFunction (module, arduino, "getPinLED",        "i()",    &m3_arduino_getPinLED);
        m3_LinkRawFunction (module, arduino, "getGreeting",      "v(*i)",  &m3_arduino_getGreeting);
        m3_LinkRawFunction (module, arduino, "print",            "v(*i)",  &m3_arduino_print);

        // Dummy (for TinyGo)
        m3_LinkRawFunction (module, "env",   "io_get_stdout",    "i()",    &m3_dummy);
    } else {
        cdbg(PSTR("193: runtime->modules is null"));
    }

    return m3Err_none;
}

/*
 * Engine start, liftoff!
 */


#define FATAL(func, msg) { Serial.print(PSTR("Fatal: " func " ")); Serial.println(msg); return; }
#define CHECK(msg, rvt) { Serial.println(PSTR(msg)); if(rvt) FATAL(msg, rvt); }

// 46912
PROGMEM void wasm_task(void*)
{

    if ( (uintptr_t)app_wasm > ROM_BASE) {
        cdbg(PSTR("From ROM"));
        bc_in_rom = True;
    } else {
        cdbg(PSTR("From RAM"));
        bc_in_rom = False;
    }

    COUT_FUNCTIONPTR = cout;

    M3Result result = m3Err_none;

    IM3Environment env = m3_NewEnvironment ();
    if (!env)
        FATAL("NewEnvironment", "failed");

    IM3Runtime runtime = m3_NewRuntime (env, WASM_STACK_SLOTS, NULL);
    if (!runtime)
        FATAL("NewRuntime", "failed");

#ifdef WASM_MEMORY_LIMIT
    runtime->memoryLimit = WASM_MEMORY_LIMIT;
#endif

    IM3Module module;


    if (bc_in_rom) {
        CHECK("ParseModule_ROM", m3_ParseModule (env, &module, app_wasm, app_wasm_len) );
    } else {
        CHECK("ParseModule_RAM", m3_ParseModule (env, &module, &app_wasm[0], app_wasm_len) );
    }

    cdbg(PSTR("PARSED"));


    CHECK("LoadModule", m3_LoadModule (runtime, module) );

    CHECK("LinkArduino", LinkArduino(runtime->modules) );

    #if WASI
        CHECK("LinkWASI",  m3_LinkWASI(runtime->modules) );
    #endif


    IM3Function f;

    // Check for TinyGo entry function first
    // See https://github.com/tinygo-org/tinygo/issues/866
    result = m3_FindFunction (&f, runtime, "cwa_main");

    if (result) {
        result = m3_FindFunction (&f, runtime, "_start");
    }

    CHECK("FindFunction", result);

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

void setup()
{
    Serial.begin(115200);
    // Wait for serial port to connect
    // Needed for native USB port only



    while(!Serial) {}
    delay(333);
    Serial.println(F("\r\nWasm3 v" M3_VERSION " (" M3_ARCH "), build " __DATE__ " " __TIME__));

    pinMode( LED_BUILTIN, OUTPUT);

#if 0 //ESP32
    // On ESP32, we can launch in a separate thread
    xTaskCreate(&wasm_task, "wasm3", NATIVE_STACK_SIZE, NULL, 5, NULL);
#else
    wasm_task(NULL);
#endif
    Serial.println(F("\r\ndone?"));
}

void loop()
{
    /*
    digitalWrite(LED_BUILTIN, HIGH);
    delay(10);

    digitalWrite(LED_BUILTIN, LOW);
    delay(990);
    */
    delay(1000);
}

