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


public:
	void UpdateSelectedItemValues(int pingerIndex);

private:
	bool InitServerListColumns(HWND hwndServerList);
	bool InitServerListData(HWND hwndServerList);

	void HandleClear();
	void HandleLVNGetDispInfo(NMLVDISPINFO *plvdi);
	void HandleLVNItemChanged(LPNMLISTVIEW pnmv);
	LRESULT HandleCustomDraw(LPARAM lParam);


	COLORREF GetColor(int pingerIndex, int listViewColumn);

    void InitDialog();
    void DestroyDialog();

	LPTSTR ServerPingSettings::AddPool(const String &pstr);

private:
	List<Pinger *> *pingers;

	String  stringPool[3];
	LPTSTR  lptstrPool[3];

	int nextFreePoolItem;

    HANDLE hThread;

public:
	CRITICAL_SECTION pingerMutex;
	bool isUpdateThreadFinished;

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
	virtual bool HasDefaults() const;
    virtual void SetDefaults();
};