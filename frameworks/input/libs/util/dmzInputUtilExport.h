#ifndef DMZ_INPUT_UTIL_EXPORT_DOT_H
#define DMZ_INPUT_UTIL_EXPORT_DOT_H

#ifdef _WIN32
#   ifdef DMZ_INPUT_UTIL_EXPORT
#      define DMZ_INPUT_UTIL_LINK_SYMBOL __declspec (dllexport)
#   else
#      define DMZ_INPUT_UTIL_LINK_SYMBOL __declspec (dllimport)
#   endif
#else
#   define DMZ_INPUT_UTIL_LINK_SYMBOL
#endif

#endif // DMZ_INPUT_UTIL_EXPORT_DOT_H
