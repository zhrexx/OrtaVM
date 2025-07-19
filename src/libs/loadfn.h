#ifndef LOADFN_H
#define LOADFN_H


#ifdef _WIN32
#include <windows.h>
typedef HMODULE LibHandle;
#define LOAD_LIBRARY(path) LoadLibraryA(path)
#define GET_PROC_ADDRESS(handle, name) GetProcAddress(handle, name)
#define CLOSE_LIBRARY(handle) FreeLibrary(handle)
#else
#include <dlfcn.h>
typedef void* LibHandle;
#define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
#define GET_PROC_ADDRESS(handle, name) dlsym(handle, name)
#define CLOSE_LIBRARY(handle) dlclose(handle)
#endif

#include <stdio.h>


#endif