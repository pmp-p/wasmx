//
//  m3_core.c
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#define M3_IMPLEMENT_ERROR_STRINGS

#include "m3_core.h"
#include "m3_env.h"
#include "extra/m3_core.extra"

void m3_Abort(const char* message) {
#ifdef DEBUG
    fprintf(stderr, "Error: %s\n", message);
#endif
    abort();
}

M3_WEAK
M3Result m3_Yield ()
{
    return m3Err_none;
}

#if d_m3FixedHeap

#if 0
static u8 fixedHeap[d_m3FixedHeap];
static u8* fixedHeapPtr = fixedHeap;
static u8* const fixedHeapEnd = fixedHeap + d_m3FixedHeap;
static u8* fixedHeapLast = NULL;
#else

    extern uint8_t *dynHeapPtr;
    extern uint8_t* dynHeapEnd;
    extern uint8_t* dynHeapLast;

    #define fixedHeapPtr dynHeapPtr
    #define fixedHeapEnd dynHeapEnd
    #define fixedHeapLast dynHeapLast
#endif


#if d_m3FixedHeapAlign > 1
#   define HEAP_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3FixedHeapAlign-1)) & ~ (d_m3FixedHeapAlign-1));
#else
#   define HEAP_ALIGN_PTR(P)
#endif

void *  m3_Malloc  (size_t i_size)
{
    u8 * ptr = fixedHeapPtr;

    fixedHeapPtr += i_size;
    HEAP_ALIGN_PTR(fixedHeapPtr);

    if (fixedHeapPtr >= fixedHeapEnd)
    {
        return NULL;
    }

    memset (ptr, 0x0, i_size);
    fixedHeapLast = ptr;

    //printf("== alloc %d => %p\n", i_size, ptr);

    return ptr;
}

void  m3_FreeImpl  (void * i_ptr)
{
    // Handle the last chunk
    if (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr = fixedHeapLast;
        fixedHeapLast = NULL;
        //printf("== free %p\n", io_ptr);
    } else {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

void *  m3_Realloc  (void * i_ptr, size_t i_newSize, size_t i_oldSize)
{
    //printf("== realloc %p => %d\n", io_ptr, i_newSize);

    if (UNLIKELY(i_newSize == i_oldSize)) return i_ptr;

    void * newPtr;

    // Handle the last chunk
    if (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr = fixedHeapLast + i_newSize;
        HEAP_ALIGN_PTR(fixedHeapPtr);
        if (fixedHeapPtr >= fixedHeapEnd)
        {
            return NULL;
        }
        newPtr = i_ptr;
    } else {
        newPtr = m3_Malloc(i_newSize);
        if (!newPtr) {
            return NULL;
        }
        if (i_ptr) {
            BC_COPY(newPtr, i_ptr, i_oldSize);
        }
    }

    if (i_newSize > i_oldSize) {
        memset ((u8 *) newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
    }

    return newPtr;
}

#else

void *  m3_Malloc  (size_t i_size)
{
    void * ptr = calloc (i_size, 1);

//    printf("== alloc %d => %p\n", (u32) i_size, ptr);

    return ptr;
}

void  m3_FreeImpl  (void * io_ptr)
{
//    if (io_ptr) printf("== free %p\n", io_ptr);
    free ((void*)io_ptr);
}

void *  m3_Realloc  (void * i_ptr, size_t i_newSize, size_t i_oldSize)
{
    if (UNLIKELY(i_newSize == i_oldSize)) return i_ptr;

    void * newPtr = realloc (i_ptr, i_newSize);

    if (LIKELY(newPtr))
    {
        if (i_newSize > i_oldSize) {
            memset ((u8 *) newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
        }
        return newPtr;
    }
    return NULL;
}

#endif

void *  m3_CopyMem  (const void * i_from, size_t i_size)
{
    void * ptr = m3_Malloc(i_size);
    if (ptr) {
        BC_COPY(ptr, i_from, i_size);
    }
    return ptr;
}

//--------------------------------------------------------------------------------------------

#if d_m3LogNativeStack

static size_t stack_start;
static size_t stack_end;

void        m3StackCheckInit ()
{
    char stack;
    stack_end = stack_start = (size_t)&stack;
}

void        m3StackCheck ()
{
    char stack;
    size_t addr = (size_t)&stack;

    size_t stackEnd = stack_end;
    stack_end = M3_MIN (stack_end, addr);

//    if (stackEnd != stack_end)
//        printf ("maxStack: %ld\n", m3StackGetMax ());
}

int      m3StackGetMax  ()
{
    return stack_start - stack_end;
}

#endif

//--------------------------------------------------------------------------------------------

M3Result NormalizeType (u8 * o_type, i8 i_convolutedWasmType)
{
    M3Result result = m3Err_none;

    u8 type = -i_convolutedWasmType;
    if (type == 0x40)
        type = c_m3Type_none;
    else if (type < c_m3Type_i32 or type > c_m3Type_f64)
        result = m3Err_invalidTypeId;

    * o_type = type;

    return result;
}


bool  IsFpType  (u8 i_m3Type)
{
    return (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool  IsIntType  (u8 i_m3Type)
{
    return (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool  Is64BitType  (u8 i_m3Type)
{
    if (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64)
        return true;
    else if (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none)
        return false;
    else
        return (sizeof (voidptr_t) == 8); // all other cases are pointers
}

u32  SizeOfType  (u8 i_m3Type)
{
    if (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32)
        return sizeof (i32);

    return sizeof (i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3Result  Read_u64  (u64 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    const u8 * ptr = * io_bytes;
    ptr += sizeof (u64);

    if (ptr <= i_end)
    {
        BC_COPY(o_value, * io_bytes, sizeof(u64));
        M3_BSWAP_u64(*o_value);
        * io_bytes = ptr;
        return m3Err_none;
    }
    else return m3Err_wasmUnderrun;
}


M3Result  Read_u32  (u32 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    const u8 * ptr = * io_bytes;
    ptr += sizeof (u32);

    if (ptr <= i_end)
    {
        BC_COPY(o_value, * io_bytes, sizeof(u32));
        M3_BSWAP_u32(*o_value);
        * io_bytes = ptr;
        return m3Err_none;
    }
    else return m3Err_wasmUnderrun;
}

#if d_m3HasFloat || d_m3NoFloatDynamic

M3Result  Read_f64  (f64 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    const u8 * ptr = * io_bytes;
    ptr += sizeof (f64);

    if (ptr <= i_end)
    {
        BC_COPY(o_value, * io_bytes, sizeof(f64));
        M3_BSWAP_f64(*o_value);
        * io_bytes = ptr;
        return m3Err_none;
    }
    else return m3Err_wasmUnderrun;
}


M3Result  Read_f32  (f32 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    const u8 * ptr = * io_bytes;
    ptr += sizeof (f32);

    if (ptr <= i_end)
    {
        BC_COPY(o_value, * io_bytes, sizeof(f32));
        M3_BSWAP_f32(*o_value);
        * io_bytes = ptr;
        return m3Err_none;
    }
    else return m3Err_wasmUnderrun;
}

#endif


M3Result  Read_u8  (u8 * o_value, bytes_t  * io_bytes, cbytes_t i_end)
{
//factorize ?
    if ( (cbytes_t)*io_bytes >= i_end) {
CLOG("Read_u8 u/r io=%p end=%p", io_bytes, i_end);
        return m3Err_wasmUnderrun;
    }

    if (BC_IN_ROM){
        BC_COPY(o_value, *io_bytes, sizeof(u8));
        *io_bytes += sizeof(u8);
    } else {
        * o_value = **io_bytes;
        * io_bytes += sizeof(u8);
    }
    return m3Err_none;
}


M3Result  Read_opcode  (m3opcode_t * o_value, bytes_t  * io_bytes, cbytes_t i_end)
{
//factorize ?
    if ( (cbytes_t)*io_bytes >= i_end) {
underrun:
CLOG("Read_opcode u/r io=%p end=%p", io_bytes, i_end);
        return m3Err_wasmUnderrun;
    }

    m3opcode_t opcode = 0;
    u8 b = 0;
    //ROM
    if (BC_IN_ROM) {
        Read_u8(&b, io_bytes, i_end);
        opcode = (m3opcode_t)b;
    } else {
        opcode = **io_bytes;
        * io_bytes += sizeof(u8);
    }

#ifndef d_m3CompileExtendedOpcode
    if (UNLIKELY(opcode == 0xFC))
    {
        if ( (cbytes_t)*io_bytes >= i_end)
            goto underrun;
        Read_u8(&b, io_bytes, i_end);
        opcode = (opcode << 8) | b;
    }
#endif
    * o_value = opcode;
    return m3Err_none;
}


M3Result  ReadLebUnsigned  (u64 * o_value, u32 i_maxNumBits, bytes_t * io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    u64 value = 0;

    u32 shift = 0;

    const u8 * ptr = * io_bytes;

    while ( ptr  < i_end)
    {
        u64 byte;
        u8 b ;
        Read_u8(&b, io_bytes, i_end);
        byte = (u64)b;

        ptr++;

        value |= ((byte & 0x7f) << shift);
        shift += 7;

        if ((byte & 0x80) == 0)
        {
            result = m3Err_none;
            break;
        }

        if (shift >= i_maxNumBits)
        {
            result = m3Err_lebOverflow;
            break;
        }
    }
//factorize ?
    if ( ptr == (cbytes_t)io_bytes)
        return m3Err_wasmUnderrun;

    * o_value = value;
    * io_bytes = ptr;
    return result;
}


M3Result  ReadLebSigned  (i64 * o_value, u32 i_maxNumBits, bytes_t * io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    i64 value = 0;

    u32 shift = 0;

    const u8 * ptr = * io_bytes;


    while ( ptr < i_end)
    {
        u64 byte;
        u8 b ;
        Read_u8(&b, io_bytes, i_end);
        byte = (u64)b;
        ptr++;

        value |= ((byte & 0x7f) << shift);
        shift += 7;

        if ((byte & 0x80) == 0)
        {
            result = m3Err_none;

            if ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                value |= (~extend << shift);
            }

            break;
        }

        if (shift >= i_maxNumBits)
        {
            result = m3Err_lebOverflow;
            break;
        }
    }

    if ( ptr == (cbytes_t)io_bytes)
        return m3Err_wasmUnderrun;

    * o_value = value;
    * io_bytes = ptr;
    return result;
}


M3Result  ReadLEB_u32  (u32 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    u64 value;
    M3Result result = ReadLebUnsigned (& value, 32, io_bytes, i_end);
    * o_value = (u32) value;

    return result;
}


M3Result  ReadLEB_u7  (u8 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    u64 value;
    M3Result result = ReadLebUnsigned (& value, 7, io_bytes, i_end);
    * o_value = (u8) value;

    return result;
}


M3Result  ReadLEB_i7  (i8 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    i64 value;
    M3Result result = ReadLebSigned (& value, 7, io_bytes, i_end);
    * o_value = (i8) value;

    return result;
}


M3Result  ReadLEB_i32  (i32 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    i64 value;
    M3Result result = ReadLebSigned (& value, 32, io_bytes, i_end);
    * o_value = (i32) value;

    return result;
}


M3Result  ReadLEB_i64  (i64 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    i64 value;
    M3Result result = ReadLebSigned (& value, 64, io_bytes, i_end);
    * o_value = value;

    return result;
}

M3Result  Read_utf8  (cstr_t * o_utf8, bytes_t * io_bytes, cbytes_t i_end)
{
    //ROM
    if (BC_IN_ROM){

        *o_utf8 = NULL;
        u32 utf8Length;
        M3Result result = ReadLEB_u32 (& utf8Length, io_bytes, i_end);

        if (result)
            return result;
        if (utf8Length <= d_m3MaxSaneUtf8Length) {
            const u8 * end = *io_bytes + utf8Length;

            if (end <= i_end) {
//PMPP
#if STRCOMPRESS && BC_IN_ROM

#if ROM_BASE_BC
        //  1=@ 2=LEN 3456=ADDR 7=ZT
                #define STRHEADER 7
#else
                #define STRHEADER 11
#endif
                #define STRMAX STRHEADER-1
                char * utf8 = (char *)m3_Malloc ( STRHEADER );
                if (utf8) {
                    ++SECTION_TEXT_IDX;
                    SECTION_TEXT_SIZE += utf8Length;
#if ROM_BASE
                    snprintf(utf8, STRHEADER, "@%c%4x", utf8Length, *io_bytes - ROM_BASE_BC );
#else
                    snprintf(utf8, STRHEADER, "@%c%08x", utf8Length, *io_bytes );
#endif
                    utf8[STRMAX] = 0;
                    * o_utf8 = utf8;
                    if (utf8Length>SB_MAX) {
                        CLOG("525:utf8 B/O %d > %d", utf8Length, SB_MAX);
                        utf8Length=SB_MAX;
                    }

                    BC_COPY(&SB, *io_bytes, utf8Length);
                    SB[utf8Length]=0;
CLOG("Read_utf8[#%d %s \"%s\"", SECTION_TEXT_IDX, &utf8[2], SB);
                } else {
                    OOM = utf8Length;
                    CLOG("585:utf8 alloc %d", utf8Length);
                    return m3Err_wasmMemoryOverflow;
                }
#else
                char * utf8 = (char *)m3_Malloc (utf8Length + 1);
                if (utf8)
                {
                    BC_COPY(utf8, *io_bytes, utf8Length);
                    utf8 [utf8Length] = 0;
                    * o_utf8 = utf8;
                } else {
                    OOM = utf8Length;
CLOG("585:utf8 alloc %d", utf8Length);
                    return m3Err_wasmMemoryOverflow;
                }
#endif
                    *io_bytes = end;
            } else {
                result = m3Err_wasmUnderrun;
            }
        } else {
            result = m3Err_missingUTF8;
        }
        return result;

    // RAM
    } else {
        *o_utf8 = NULL;

        u32 utf8Length;
        M3Result result = ReadLEB_u32 (& utf8Length, io_bytes, i_end);

        if (not result)
        {
            if (utf8Length <= d_m3MaxSaneUtf8Length)
            {
                const u8 * ptr = * io_bytes;
                const u8 * end = ptr + utf8Length;

                if (end <= i_end)
                {
                    char * utf8 = (char *)m3_Malloc (utf8Length + 1);

                    if (utf8)
                    {
                        memcpy (utf8, ptr, utf8Length);
                        utf8 [utf8Length] = 0;
                        * o_utf8 = utf8;
                    }

                    * io_bytes = end;
                }
                else result = m3Err_wasmUnderrun;
            }
            else result = m3Err_missingUTF8;
        }

        return result;

    }


}






#if d_m3RecordBacktraces
u32  FindModuleOffset  (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3CodePage curr = i_runtime->pagesOpen;
    bool pageFound = false;

    while (curr)
    {
        if (ContainsPC (curr, i_pc))
        {
            pageFound = true;
            break;
        }
        curr = curr->info.next;
    }

    if (!pageFound)
    {
        curr = i_runtime->pagesFull;
        while (curr)
        {
            if (ContainsPC (curr, i_pc))
            {
                pageFound = true;
                break;
            }
            curr = curr->info.next;
        }
    }

    if (pageFound)
    {
        u32 result = 0;

        bool pcFound = MapPCToOffset (curr, i_pc, & result);
                                                                                d_m3Assert (pcFound);

        return result;
    }
    else return 0;
}


void  PushBacktraceFrame  (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    if (UNLIKELY (io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED))
        return;

    M3BacktraceFrame * newFrame = m3_AllocStruct(M3BacktraceFrame);

    if (!newFrame)
    {
        io_runtime->backtrace.lastFrame = M3_BACKTRACE_TRUNCATED;
        return;
    }

    newFrame->moduleOffset = FindModuleOffset (io_runtime, i_pc);

    if (!io_runtime->backtrace.frames || !io_runtime->backtrace.lastFrame)
        io_runtime->backtrace.frames = newFrame;
    else
        io_runtime->backtrace.lastFrame->next = newFrame;
    io_runtime->backtrace.lastFrame = newFrame;
}


void  FillBacktraceFunctionInfo  (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    if (UNLIKELY (io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED))
        return;

    if (!io_runtime->backtrace.lastFrame)
        return;

    io_runtime->backtrace.lastFrame->function = i_function;
}


void  ClearBacktrace  (IM3Runtime io_runtime)
{
    M3BacktraceFrame * currentFrame = io_runtime->backtrace.frames;
    while (currentFrame)
    {
        M3BacktraceFrame * nextFrame = currentFrame->next;
        m3_Free (currentFrame);
        currentFrame = nextFrame;
    }

    io_runtime->backtrace.frames = NULL;
    io_runtime->backtrace.lastFrame = NULL;
}
#endif // d_m3RecordBacktraces
