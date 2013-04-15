/**
 * John Bradley (jrb@turrettech.com)
 */
#pragma once

#include "OBSApi.h"

struct sockaddr_in;

class Pinger {

public:
	Pinger(XElement *serviceElement, XDataItem *serverDataItem);
	~Pinger();

public:
	void Ping();
	double StandardDeviation();
	double Average();

	void Clear();

private:
	void Initialize();
	UINT InternalPing();

private:
	bool isValid;
	bool isInitialized;

	String serviceName;
	String serverName;
	String serverUrl;
	
	List<UINT> *pings;
	
	struct sockaddr_in *service;

	int pingSum;

	int latestPing;
	int minimumPing;
	int maximumPing;

public:
	const String& GetServiceName() { return serviceName; }
	const String& GetServerName() { return serverName; }
	const String& GetServerUrl() { return serverUrl; }

	int GetLatestPing() { return latestPing; }
	int GetMinimumPing() { return minimumPing; }
	int GetMaximumPing() { return maximumPing; }

};
