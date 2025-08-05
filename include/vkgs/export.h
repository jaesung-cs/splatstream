#ifndef VKGS_EXPORT_H
#define VKGS_EXPORT_H

// clang-format off
#if defined(_WIN32) || defined(__CYGWIN__)
  #ifdef VKGS_EXPORTS
    #define VKGS_API __declspec(dllexport)
  #else
    #define VKGS_API __declspec(dllimport)
  #endif
#else
  #define MYLIB_API
#endif
// clang-format on

#endif  // VKGS_EXPORT_H
