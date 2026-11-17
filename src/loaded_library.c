/* Path of the Z3 library loaded into the process, for z3_build_info().
   Plain C, so that windows.h does not meet R's headers. */
#ifdef _WIN32
#include <windows.h>
#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#endif
#include <z3.h>

const char* z3r_loaded_library(void) {
#ifdef _WIN32
  static char path[MAX_PATH * 4];
  HMODULE h = NULL;
  if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          (LPCSTR)(void*)&Z3_get_full_version, &h)) {
    return "";
  }
  DWORD n = GetModuleFileNameA(h, path, (DWORD)sizeof(path));
  if (n == 0 || n >= sizeof(path)) return "";
  return path;
#else
  Dl_info info;
  if (!dladdr((void*)&Z3_get_full_version, &info) || !info.dli_fname) return "";
  return info.dli_fname;
#endif
}
