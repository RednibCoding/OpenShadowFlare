/**
 * OpenShadowFlare Debug/Tracing System
 * 
 * Provides function call tracing and crash diagnostics.
 * Enable by defining OSF_DEBUG before including this header.
 * 
 * MANDATORY: Every implemented function MUST use OSF_FUNC_TRACE at the start!
 * 
 * Usage in DLL code:
 *   #define OSF_DEBUG 1
 *   #define DLL_NAME "RK_FUNCTION"
 *   #include "../../debug.h"
 *   
 *   int __cdecl SomeFunction(int arg) {
 *       OSF_FUNC_TRACE("arg=%d", arg);  // Logs ENTER and EXIT automatically
 *       // ... function code ...
 *       return result;
 *   }
 *   
 *   // For functions with no args:
 *   void __cdecl SimpleFunction() {
 *       OSF_FUNC_TRACE_NOARGS;
 *       // ...
 *   }
 */

#ifndef OSF_DEBUG_H
#define OSF_DEBUG_H

#include <windows.h>
#include <cstdio>
#include <cstdarg>

// Ring buffer size for tracking recent function calls
#define OSF_TRACE_BUFFER_SIZE 256
#define OSF_TRACE_ENTRY_LEN 256

#ifdef OSF_DEBUG

// Global trace state
namespace OsfDebug {
    // Ring buffer of recent function calls
    static char g_traceBuffer[OSF_TRACE_BUFFER_SIZE][OSF_TRACE_ENTRY_LEN];
    static int g_traceIndex = 0;
    static CRITICAL_SECTION g_traceLock;
    static bool g_initialized = false;
    static FILE* g_logFile = nullptr;
    static bool g_logToFile = true;
    static bool g_logToConsole = false;
    
    inline void Initialize() {
        if (g_initialized) return;
        InitializeCriticalSection(&g_traceLock);
        
        // Open log file (append mode)
        if (g_logToFile) {
            g_logFile = fopen("osf_trace.log", "a");
            if (g_logFile) {
                fprintf(g_logFile, "\n=== OpenShadowFlare Session Started ===\n");
                fflush(g_logFile);
            }
        }
        
        g_initialized = true;
    }
    
    inline void Shutdown() {
        if (!g_initialized) return;
        if (g_logFile) {
            fprintf(g_logFile, "=== Session Ended ===\n");
            fclose(g_logFile);
            g_logFile = nullptr;
        }
        DeleteCriticalSection(&g_traceLock);
        g_initialized = false;
    }
    
    inline void Trace(const char* dllName, const char* funcName, const char* fmt, ...) {
        if (!g_initialized) Initialize();
        
        EnterCriticalSection(&g_traceLock);
        
        // Format the message
        char argBuf[128] = "";
        if (fmt && fmt[0]) {
            va_list args;
            va_start(args, fmt);
            vsnprintf(argBuf, sizeof(argBuf), fmt, args);
            va_end(args);
        }
        
        // Get timestamp
        SYSTEMTIME st;
        GetLocalTime(&st);
        
        // Format trace entry
        char* entry = g_traceBuffer[g_traceIndex];
        snprintf(entry, OSF_TRACE_ENTRY_LEN, 
                 "[%02d:%02d:%02d.%03d] %s::%s(%s)",
                 st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                 dllName, funcName, argBuf);
        
        // Write to log file
        if (g_logFile) {
            fprintf(g_logFile, "%s\n", entry);
            fflush(g_logFile);
        }
        
        // Write to console if enabled
        if (g_logToConsole) {
            printf("%s\n", entry);
        }
        
        // Advance ring buffer
        g_traceIndex = (g_traceIndex + 1) % OSF_TRACE_BUFFER_SIZE;
        
        LeaveCriticalSection(&g_traceLock);
    }
    
    inline void DumpRecentCalls(const char* reason) {
        if (!g_initialized) return;
        
        FILE* crashLog = fopen("osf_crash.log", "a");
        if (!crashLog) return;
        
        SYSTEMTIME st;
        GetLocalTime(&st);
        
        fprintf(crashLog, "\n========================================\n");
        fprintf(crashLog, "CRASH/ERROR at %04d-%02d-%02d %02d:%02d:%02d\n",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        fprintf(crashLog, "Reason: %s\n", reason);
        fprintf(crashLog, "Recent function calls (oldest first):\n");
        fprintf(crashLog, "----------------------------------------\n");
        
        EnterCriticalSection(&g_traceLock);
        
        // Dump ring buffer from oldest to newest
        for (int i = 0; i < OSF_TRACE_BUFFER_SIZE; i++) {
            int idx = (g_traceIndex + i) % OSF_TRACE_BUFFER_SIZE;
            if (g_traceBuffer[idx][0]) {
                fprintf(crashLog, "  %s\n", g_traceBuffer[idx]);
            }
        }
        
        LeaveCriticalSection(&g_traceLock);
        
        fprintf(crashLog, "========================================\n");
        fclose(crashLog);
    }
    
    // Crash handler
    static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* pExceptionInfo) {
        const char* exceptionName = "UNKNOWN";
        switch (pExceptionInfo->ExceptionRecord->ExceptionCode) {
            case EXCEPTION_ACCESS_VIOLATION: exceptionName = "ACCESS_VIOLATION"; break;
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: exceptionName = "ARRAY_BOUNDS_EXCEEDED"; break;
            case EXCEPTION_BREAKPOINT: exceptionName = "BREAKPOINT"; break;
            case EXCEPTION_DATATYPE_MISALIGNMENT: exceptionName = "DATATYPE_MISALIGNMENT"; break;
            case EXCEPTION_FLT_DENORMAL_OPERAND: exceptionName = "FLT_DENORMAL_OPERAND"; break;
            case EXCEPTION_FLT_DIVIDE_BY_ZERO: exceptionName = "FLT_DIVIDE_BY_ZERO"; break;
            case EXCEPTION_FLT_INEXACT_RESULT: exceptionName = "FLT_INEXACT_RESULT"; break;
            case EXCEPTION_FLT_INVALID_OPERATION: exceptionName = "FLT_INVALID_OPERATION"; break;
            case EXCEPTION_FLT_OVERFLOW: exceptionName = "FLT_OVERFLOW"; break;
            case EXCEPTION_FLT_STACK_CHECK: exceptionName = "FLT_STACK_CHECK"; break;
            case EXCEPTION_FLT_UNDERFLOW: exceptionName = "FLT_UNDERFLOW"; break;
            case EXCEPTION_ILLEGAL_INSTRUCTION: exceptionName = "ILLEGAL_INSTRUCTION"; break;
            case EXCEPTION_IN_PAGE_ERROR: exceptionName = "IN_PAGE_ERROR"; break;
            case EXCEPTION_INT_DIVIDE_BY_ZERO: exceptionName = "INT_DIVIDE_BY_ZERO"; break;
            case EXCEPTION_INT_OVERFLOW: exceptionName = "INT_OVERFLOW"; break;
            case EXCEPTION_INVALID_DISPOSITION: exceptionName = "INVALID_DISPOSITION"; break;
            case EXCEPTION_NONCONTINUABLE_EXCEPTION: exceptionName = "NONCONTINUABLE_EXCEPTION"; break;
            case EXCEPTION_PRIV_INSTRUCTION: exceptionName = "PRIV_INSTRUCTION"; break;
            case EXCEPTION_SINGLE_STEP: exceptionName = "SINGLE_STEP"; break;
            case EXCEPTION_STACK_OVERFLOW: exceptionName = "STACK_OVERFLOW"; break;
        }
        
        char reason[256];
        snprintf(reason, sizeof(reason), "Exception %s (0x%08X) at address 0x%p",
                 exceptionName,
                 (unsigned int)pExceptionInfo->ExceptionRecord->ExceptionCode,
                 pExceptionInfo->ExceptionRecord->ExceptionAddress);
        
        DumpRecentCalls(reason);
        
        // Continue with default exception handling
        return EXCEPTION_CONTINUE_SEARCH;
    }
    
    inline void InstallCrashHandler() {
        SetUnhandledExceptionFilter(CrashHandler);
    }
    
    // RAII class for automatic ENTER/EXIT tracing
    class ScopedTrace {
    public:
        const char* m_dll;
        const char* m_func;
        
        ScopedTrace(const char* dll, const char* func, const char* fmt, ...) 
            : m_dll(dll), m_func(func) {
            char argBuf[128] = "";
            if (fmt && fmt[0]) {
                va_list args;
                va_start(args, fmt);
                vsnprintf(argBuf, sizeof(argBuf), fmt, args);
                va_end(args);
            }
            Trace(dll, func, "ENTER %s", argBuf);
        }
        
        ~ScopedTrace() {
            Trace(m_dll, m_func, "EXIT");
        }
    };
}

// Macro for easy tracing - define DLL_NAME before using
#ifndef DLL_NAME
#define DLL_NAME "UNKNOWN"
#endif

// Legacy macros (still usable)
#define OSF_TRACE(func, ...) OsfDebug::Trace(DLL_NAME, func, __VA_ARGS__)
#define OSF_TRACE_ENTER(func) OsfDebug::Trace(DLL_NAME, func, "ENTER")
#define OSF_TRACE_EXIT(func) OsfDebug::Trace(DLL_NAME, func, "EXIT")

// MANDATORY: Use these in every function - auto-logs ENTER on call, EXIT on return
#define OSF_FUNC_TRACE(...) \
    OsfDebug::ScopedTrace _osfTrace_(DLL_NAME, __func__, __VA_ARGS__)

#define OSF_FUNC_TRACE_NOARGS \
    OsfDebug::ScopedTrace _osfTrace_(DLL_NAME, __func__, "")

// Initialize in DllMain
#define OSF_DEBUG_INIT() do { \
    OsfDebug::Initialize(); \
    OsfDebug::InstallCrashHandler(); \
} while(0)

#define OSF_DEBUG_SHUTDOWN() OsfDebug::Shutdown()

#else // !OSF_DEBUG

// No-op versions when debugging is disabled
#define OSF_TRACE(func, ...) ((void)0)
#define OSF_TRACE_ENTER(func) ((void)0)
#define OSF_TRACE_EXIT(func) ((void)0)
#define OSF_FUNC_TRACE(...) ((void)0)
#define OSF_FUNC_TRACE_NOARGS ((void)0)
#define OSF_DEBUG_INIT() ((void)0)
#define OSF_DEBUG_SHUTDOWN() ((void)0)

#endif // OSF_DEBUG

#endif // OSF_DEBUG_H
