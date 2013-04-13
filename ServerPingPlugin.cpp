/**
 * John Bradley (jrb@turrettech.com)
 */
#include "ServerPingPlugin.h"
#include "ServerPingSettings.h"
#include "Localization.h"
#include <WinSock2.h>

HINSTANCE ServerPingPlugin::hinstDLL = NULL;
ServerPingPlugin *ServerPingPlugin::instance = NULL;

ServerPingPlugin::ServerPingPlugin() {
	isDynamicLocale = false;
	settings = NULL;

	if (!locale->HasLookup(KEY("PluginName"))) {
		isDynamicLocale = true;
		int localizationStringCount = sizeof(localizationStrings) / sizeof(CTSTR);
		Log(TEXT("Server Ping plugin strings not found, dynamically loading %d strings"), sizeof(localizationStrings) / sizeof(CTSTR));
		for(int i = 0; i < localizationStringCount; i += 2) {
			locale->AddLookupString(localizationStrings[i], localizationStrings[i+1]);
		}
		if (!locale->HasLookup(KEY("PluginName"))) {
			AppWarning(TEXT("Uh oh..., unable to dynamically add our localization keys"));
		}
	}
	
	WSADATA wsaData;
	int startupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) {
		AppWarning(TEXT("Unable to startup winsock, plugin may not work"));
    }

	settings = new ServerPingSettings();
	OBSAddSettingsPane(settings);
}

ServerPingPlugin::~ServerPingPlugin() {
	
	if (isDynamicLocale) {
		int localizationStringCount = sizeof(localizationStrings) / sizeof(CTSTR);
		Log(TEXT("Server Ping plugin instance deleted; removing dynamically loaded localization strings"));
		for(int i = 0; i < localizationStringCount; i += 2) {
			locale->RemoveLookupString(localizationStrings[i]);
		}
	}

	isDynamicLocale = false;

	OBSRemoveSettingsPane(settings);
	delete settings;

	WSACleanup();
}

bool LoadPlugin()
{
    if(ServerPingPlugin::instance != NULL)
        return false;
    ServerPingPlugin::instance = new ServerPingPlugin();
    return true;
}

void UnloadPlugin()
{
    if(ServerPingPlugin::instance == NULL)
        return;
    delete ServerPingPlugin::instance;
    ServerPingPlugin::instance = NULL;
}

void OnStartStream()
{
}

void OnStopStream()
{
}

CTSTR GetPluginName()
{
    return STR("PluginName");
}

CTSTR GetPluginDescription()
{
    return STR("PluginDescription");
}

BOOL CALLBACK DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if(fdwReason == DLL_PROCESS_ATTACH)
        ServerPingPlugin::hinstDLL = hinstDLL;
    return TRUE;
}
