#include <xwlib/x_types.h>

#if defined(_WINDLL)

__plugin_sect_start__ static const plugin_t InitSegStart = 0;
__plugin_sect_end__ static const plugin_t InitSegEnd = 0;

static void 
sessions_dll_init(void)
{
	int i=0;
	plugin_t *fu = (plugin_t *)&InitSegStart;
	plugin_t *fu__ = (plugin_t *)&InitSegEnd;
	
	ENTER;

	for (; fu < fu__; ++fu)
	{
		if (*fu)
		{
			(*fu)();
		}
	}
	
	EXIT;

}

BOOLEAN WINAPI 
DllMain(HINSTANCE hDllHandle,DWORD nReason,LPVOID Reserved) 
{
	TRACE("Registering plugin...\n");
	switch (nReason) 
	{
		case DLL_PROCESS_ATTACH: 
			DisableThreadLibraryCalls( hDllHandle ); 
			sessions_dll_init();
			break;
		case DLL_PROCESS_DETACH: 
			break; 
	} 
	return TRUE;
}
#endif
