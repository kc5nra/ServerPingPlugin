/**
 * John Bradley (jrb@turrettech.com)
 */
#pragma once

#include "OBSApi.h"

class Pinger;

class ServerPingSettings : public SettingsPane
{
public:
    ServerPingSettings();
    virtual ~ServerPingSettings();

private:
	bool InitServerListColumns(HWND hwndServerList);
	bool InitServerListData(HWND hwndServerList);

	void HandleLVNGetDispInfo(NMLVDISPINFO *plvdi);
	LRESULT ServerPingSettings::HandleCustomDraw(LPARAM lParam);
	COLORREF GetColor(int pingerIndex, int listViewColumn);

    void InitDialog();
    void DestroyDialog();

	LPTSTR ServerPingSettings::AddPool(const String &pstr);

private:
	List<Pinger *> *pingers;

	String  stringPool[3];
	LPTSTR  lptstrPool[3];

	int     nextFreePoolItem;

public:
	volatile bool isUpdateThreadFinished;

public:
	List<Pinger *> *GetPingers() { return pingers; }
	HWND GetHwnd() { return hwnd; }

public:
    virtual CTSTR GetCategory() const;
    virtual HWND CreatePane(HWND parentHwnd);
    virtual void DestroyPane();
    virtual INT_PTR ProcMessage(UINT message, WPARAM wParam, LPARAM lParam);
    virtual void ApplySettings();
    virtual void CancelSettings();
};