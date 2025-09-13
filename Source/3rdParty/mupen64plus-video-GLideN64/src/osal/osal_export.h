#if !defined(OSAL_EXPORT_H)
#define OSAL_EXPORT_H

#if defined(OS_WINDOWS)
#ifndef EXPORT
#define EXPORT	__declspec(dllexport)
#endif
#define CALL		__cdecl
#else  /* Not WINDOWS */
#ifndef EXPORT
#define EXPORT 	__attribute__((visibility("default")))
#endif
#define CALL
#endif

#endif /* #define OSAL_EXPORT_H */
