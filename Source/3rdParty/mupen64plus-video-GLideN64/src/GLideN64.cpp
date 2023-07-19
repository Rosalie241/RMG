#include <Revision.h>
char pluginName[] = "GLideN64";
#ifdef PLUGIN_REVISION
char pluginNameWithRevision[] = "GLideN64 rev." PLUGIN_REVISION;
#else // PLUGIN_REVISION
char pluginNameWithRevision[] = "GLideN64";
#endif // PLUGIN_REVISION
wchar_t pluginNameW[] = L"GLideN64";
void (*CheckInterrupts)( void );
