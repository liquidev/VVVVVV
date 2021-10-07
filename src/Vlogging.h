#ifndef VLOGGING_H
#define VLOGGING_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <SDL2/SDL.h>
#include <pspdebug.h>

void vlog_init(void);

void vlog_toggle_output(int enable_output);

void vlog_toggle_color(int enable_color);

void vlog_toggle_debug(int enable_debug);

void vlog_toggle_info(int enable_info);

void vlog_toggle_warn(int enable_warn);

void vlog_toggle_error(int enable_error);

#if 0

SDL_PRINTF_VARARG_FUNC(1) int vlog_debug(const char* text, ...);

SDL_PRINTF_VARARG_FUNC(1) int vlog_info(const char* text, ...);

SDL_PRINTF_VARARG_FUNC(1) int vlog_warn(const char* text, ...);

SDL_PRINTF_VARARG_FUNC(1) int vlog_error(const char* text, ...);

#endif

#define vlog_debug(...) \
   do { \
      pspDebugScreenPrintf("[D] "); \
      pspDebugScreenPrintf(__VA_ARGS__); \
      pspDebugScreenPrintf("\n"); \
   } while (0)
#define vlog_info(...) \
   do { \
      pspDebugScreenPrintf("[I] "); \
      pspDebugScreenPrintf(__VA_ARGS__); \
      pspDebugScreenPrintf("\n"); \
   } while (0)
#define vlog_warn(...) \
   do { \
      pspDebugScreenPrintf("[W] "); \
      pspDebugScreenPrintf(__VA_ARGS__); \
      pspDebugScreenPrintf("\n"); \
   } while (0)
#define vlog_error(...) \
   do { \
      pspDebugScreenPrintf("[E] "); \
      pspDebugScreenPrintf(__VA_ARGS__); \
      pspDebugScreenPrintf("\n"); \
   } while (0)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VLOGGING_H */
