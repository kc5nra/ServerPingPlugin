/**
 * John Bradley (jrb@turrettech.com)
 */
#pragma once

#define STR_PREFIX L"Plugins.ServerPing."
#define KEY(k) (STR_PREFIX L ## k)
#define STR(text) locale->LookupString(KEY(text))

#ifndef SPP_VERSION
#define SPP_VERSION L" !INTERNAL VERSION!"
#endif

static CTSTR localizationStrings[] = {
	KEY("PluginName"),			L"Server Ping",
	KEY("PluginDescription"),	L"Pings all the available RTMP servers to assist in determining"
								L" which server may provide the best streaming experience"
								L" (latency and consistency)\n\n"
								L"Plugin Version: " SPP_VERSION,
	KEY("ServerListGroup"),		L"Servers",
	KEY("Service"),				L"Service",
	KEY("ServerName"),			L"Server Name",
	KEY("Last"),				L"Last",
	KEY("Average"),				L"Average",
	KEY("Jitter"),				L"Jitter",
	KEY("ServerUrl"),			L"Server URL",
	KEY("LowestPing"),			L"Lowest Ping",
	KEY("HighestPing"),			L"Highest Ping",
	KEY("Clear"),				L"Clear",
};
