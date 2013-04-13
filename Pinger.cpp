/**
 * John Bradley (jrb@turrettech.com)
 */
#include "Pinger.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <websocket.h>

#pragma comment(lib, "ws2_32.lib")

#define FAILED_PING_TIME 9999
typedef unsigned long long XUINT64;

/* Returns the amount of milliseconds elapsed since the UNIX epoch. */
XUINT64 GetTimeInMilliseconds()
{
	FILETIME ft;
	LARGE_INTEGER li;

	GetSystemTimeAsFileTime(&ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	XUINT64 ret = li.QuadPart;
	ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
	ret /= 10000;

	return ret;
}

bool AddHostInformation(struct sockaddr_in *service, String &host, int port)
{
	bool isValid = true;
	
	char *hostname = host.CreateUTF8String();

    service->sin_addr.s_addr = inet_addr(hostname);
    if (service->sin_addr.s_addr == INADDR_NONE)
    {
        struct hostent *host = gethostbyname(hostname);
        if (host == NULL || host->h_addr == NULL)
        {
            Log(TEXT("Problem accessing the DNS. (addr: %s, error: %d)"), hostname, WSAGetLastError());
            return false;
        }
        service->sin_addr = *(struct in_addr *)host->h_addr;
    }

    service->sin_port = htons(port);
    return true;
}

__inline UINT uriTrimLocation(String &str) {
	UINT i = 0;
	for(; i < str.Length(); i++) {
		if (str[i] == '/') {
			break;
		}
	}
	return i;
}

double Pinger::StandardDeviation()
{
	double M = 0.0;
	double S = 0.0;
	int k = 1;

	for (UINT i = 0; i < pings->Num(); i++)
	{
		double value = pings->GetElement(i);
		double tmpM = M;
		M += (value - tmpM) / k;
		S += (value - tmpM) * (value - M);
		k++;
	}

	return ceil((sqrt(S / (k - 1)) * pow((double)10, 2)) - 0.49) / pow((double)10, 2);
}

double Pinger::Average() 
{
	return ((double)pingSum / (double)pings->Num());
}

void Pinger::Clear()
{
	pings->Clear();
	minimumPing = -1;
	maximumPing = -1;
	pingSum = 0;
}

Pinger::Pinger(XElement *serviceElement, XDataItem *serverDataItem)
{
	isValid = true;
	pings = new List<UINT>();
	service = NULL;

	minimumPing = -1;
	maximumPing = -1;
	pingSum = 0;

	serviceName = serviceElement->GetName();
	serverName = serverDataItem->GetName();
	serverUrl = serverDataItem->GetData();
}

Pinger::~Pinger()
{
	if (service != 0) {
		free(service);
	}
	delete pings;
}

void Pinger::Initialize() {
	isInitialized = true;
	
	// very naive parsing
	serverUrl.FindReplace(TEXT("rtmp://"),TEXT(""));
	StringList uriTokens;
	serverUrl.GetTokenList(uriTokens, ':', FALSE);

	String host;
	UINT port = 1935;
	if (uriTokens.Num() > 0) {
		host = uriTokens.GetElement(0);
		host = host.Mid(0, uriTrimLocation(host));
		if (uriTokens.Num() > 1) {
			String portString = uriTokens.GetElement(1);
			portString = portString.Mid(0, uriTrimLocation(portString));
			port = portString.ToInt();
		}
	} else {
		isValid = false;
		return;
	}

	String ip;
	struct sockaddr_in *clientService = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	memset(clientService, 0, sizeof(struct sockaddr_in));


    clientService->sin_family = AF_INET;

	if (AddHostInformation(clientService, host, port)) {
		service = clientService;
	} else {
		isValid = false;
		free(clientService);
	}
}

void Pinger::Ping()
{
	if (!isInitialized) {
		Initialize();
	}
	if (isValid) {
		latestPing = InternalPing();
		if (minimumPing == -1) {
			minimumPing = latestPing;
		} else {
			minimumPing = min(latestPing, minimumPing);
		}

		maximumPing = max(latestPing, maximumPing);
		
		pingSum += latestPing;
		pings->Add(latestPing);
	}
}

UINT Pinger::InternalPing()
{
	u_long                  on = 1L;
	int                     socketsFound;
	struct timeval          timeout;
	fd_set                  wfds;
	SOCKET sock;
	XUINT64 time;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
	{
		Log(TEXT("couldn't create socket: %ld\n"), WSAGetLastError());
		return FAILED_PING_TIME;
	}

	if (ioctlsocket(sock, FIONBIO, &on))
	{
		Log(TEXT("couldn't set non-blocking mode on socket: %ld\n"), WSAGetLastError());
		return FAILED_PING_TIME;
	}

	time = GetTimeInMilliseconds();

	char ip[512];
	strcpy(ip, inet_ntoa(service->sin_addr));

	if (connect(sock, (SOCKADDR *)service, sizeof(SOCKADDR)) < 0)
	{       
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			Log(TEXT("unable to connect: %ld\n"), WSAGetLastError());
			closesocket(sock);
			return FAILED_PING_TIME;
		}
	}

	timeout.tv_sec  = (long)2;
	timeout.tv_usec = 0;

	FD_ZERO(&wfds);
	FD_SET(sock, &wfds);

	socketsFound = select((int)sock+1, NULL, &wfds, NULL, &timeout);
	if (socketsFound > 0 && FD_ISSET(sock, &wfds))
	{
		closesocket(sock);
		return (UINT)(GetTimeInMilliseconds() - time);
	} else {
		closesocket(sock);
		return FAILED_PING_TIME;
	}
}