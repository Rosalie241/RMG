#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*funcptr)(void);

funcptr IntGetProcAddress(const char* name);

#ifdef __cplusplus
}
#endif