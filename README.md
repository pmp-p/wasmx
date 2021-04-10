# WasmX
A WebAssembly interpreter with a twist (sched_yield+aio).

Based on Main repository: [**Wasm3 project**](https://github.com/wasm3/wasm3)  

This library can be used with 
[`Arduino`](https://www.arduinolibraries.info/libraries/wasm3), 
[`Android`](https://github.com/cnlohr/rawdrawandroid)

for running

[`WaPy`](https://pmp-p.github.io/wasi/wapy.html) 

[source](https://github.com/pmp-p/wapy/tree/wapy-wasi)

[discuss](https://gitter.im/Wasm-Python/Wapy)


## Supported devices

Wasm3 requires at least **~64Kb** Flash and **~10Kb** RAM even for minimal functionality.

The library was verified to work with:  
`ESP32`, `ESP8266`

## Disclaimer

Arduino and AIO API are here for an example. We do not encourage this way of programming, and we have to come up with a better API (i.e. **Event-Driven**, **Resource-Oriented** or similar).

### License
This project is released under The MIT License (MIT)
