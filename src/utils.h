#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <windows.h>
#include <cstdint>
#include "utils.h"

# define THISCALL __thiscall


/**
 * CallFunctionInDLL - A template function that dynamically loads a DLL, retrieves
 * the address of a function from the DLL, calls the function, and then unloads the DLL.
 *
 * Parameters:
 * - dllName:   A const char pointer specifying the name of the DLL to load.
 * - funcName:  A const char pointer specifying the name of the function to retrieve from the DLL.
 * - args:      Variadic template arguments that are passed to the function when it is called.
 *
 * Return Value:
 * - If the specified function in the DLL has a non-void return type, this function
 *   returns the value returned by the DLL function.
 * - If the specified function in the DLL has a void return type, this function does not
 *   return a value.
 * - If there's an error (e.g., the DLL cannot be loaded or the function cannot be found),
 *   and the return type is non-void, it will return a default value (usually 0 or equivalent).
 *
 * Usage Example:
 *
 * To call a function `DrawNormalFont` from `o_RKC_FONTMAKER.dll` that has a return type of bool
 * and takes parameters of types RKC_FONTMAKER*, HDC*, and unsigned char*, you'd use:
 *
 * bool result = CallFunctionInDLL<bool>(
 *                 "o_RKC_FONTMAKER.dll",
 *                 "?DrawNormalFont@RKC_FONTMAKER@@QAEHPAUHDC__@@E@Z",
 *                 self, hdc, ucharParam);
 *
 * Notes:
 * - Ensure that the order and types of the arguments passed to CallFunctionInDLL match
 *   the signature of the function in the DLL.
 * - This function does not handle exceptions thrown by the DLL function. It's up to the caller
 *   to manage exceptions if necessary.
 */
template <typename RetType, typename... Args>
RetType CallFunctionInDLL(const char* dllName, const char* funcName, Args... args) {
    // Load the DLL into memory
    HINSTANCE loadedDll = LoadLibraryA(dllName);

    // Check if the DLL loaded successfully
    if (!loadedDll) {
        printf("Failed to load %s with error code: 0x%x\n", dllName, GetLastError());
        if constexpr (!std::is_void<RetType>()) {
            return static_cast<RetType>(0);
        } else {
            return;
        }
    }

    // Get the address of the function from the loaded DLL
    typedef RetType(THISCALL* FuncType)(Args...);
    FuncType funcPtr = (FuncType)GetProcAddress(loadedDll, funcName);

    // Check if we successfully retrieved the address of the function
    if (!funcPtr) {
        printf("Failed to find %s function in %s with error code: 0x%x\n", funcName, dllName, GetLastError());
        FreeLibrary(loadedDll);
        if constexpr (!std::is_void<RetType>()) {
            return static_cast<RetType>(0);
        } else {
            return;
        }
    }

    // Call the function using the function pointer
    if constexpr (!std::is_void<RetType>()) {
        RetType result = funcPtr(args...);
        FreeLibrary(loadedDll);
        return result;
    } else {
        funcPtr(args...);
        FreeLibrary(loadedDll);
    }
}

#endif // UTILS_H