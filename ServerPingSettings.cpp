/**
 * John Bradley (jrb@turrettech.com)
 */
#include <process.h>

#include "ServerPingSettings.h"
#include "ServerPingPlugin.h"
#include "Pinger.h"
#include "Localization.h"
#include "resource.h"

#define COLOR_GREEN RGB(0, 128, 0)
#define COLOR_OLIVE RGB(128, 128, 0)
#define COLOR_RED RGB(255, 0, 0)
#define COLOR_BLACK RGB(0, 0, 0)

void pingThread(void *param)
{
	ServerPingSettings *settings = ServerPingPlugin::instance->GetSettings();
	List<Pinger *> *pingers = settings->GetPingers();
	HWND hwndServerList = GetDlgItem(settings->GetHwnd(), IDC_SERVERLIST);

	while(!settings->isUpdateThreadFinished) {
		for(UINT i = 0; i < pingers->Num(); i++) {
			pingers->GetElement(i)->Ping();
			SendMessage(hwndServerList, LVM_UPDATE, i, 0);
			if (settings->isUpdateThreadFinished) {
				break;
			}
		}
	}

	_endthread();
}

ServerPingSettings::ServerPingSettings() : SettingsPane()
{
	// class privates
	pingers = new List<Pinger *>();

	// service
	
	UINT serviceCount;
	XConfig serverData;
	
	if(serverData.Open(TEXT("services.xconfig")))
    {
        XElement *services = serverData.GetElement(TEXT("services"));
        if(services) {
            serviceCount = services->NumElements();

            for(UINT i = 0; i < serviceCount; i++) {
                XElement *service = services->GetElementByID(i);
				if (service) {
					XElement *servers = service->GetElement(TEXT("servers"));
					if(servers) {
						UINT numServers = servers->NumDataItems();
						for(UINT i=0; i<numServers; i++) {
							XDataItem *server = servers->GetDataItemByID(i);
							if (server) {
								pingers->Add(new Pinger(service, server));
							}
						}
					}
				}
            }
        }
    }
	serverData.Close();
}

ServerPingSettings::~ServerPingSettings()
{
	isUpdateThreadFinished = true;
	for(UINT i = 0; i < pingers->Num(); i++) {
		delete pingers->GetElement(i);
	}

	delete pingers;
}

CTSTR ServerPingSettings::GetCategory() const
{
	return STR("PluginName");
}

HWND ServerPingSettings::CreatePane(HWND parentHwnd)
{
	hwnd = CreateDialogParam(
		ServerPingPlugin::hinstDLL, 
		MAKEINTRESOURCE(IDD_SERVERPING), 
		parentHwnd, 
		(DLGPROC)DialogProc, 
		(LPARAM)this);

	return hwnd;
}

VOID ServerPingSettings::DestroyPane()
{
	DestroyWindow(hwnd);
	hwnd = NULL;
}

void ServerPingSettings::ApplySettings()
{}

void ServerPingSettings::CancelSettings()
{}

INT_PTR ServerPingSettings::ProcMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
		case WM_INITDIALOG: 
		{
			InitDialog();
			return TRUE;
		}
		case WM_DESTROY:
		{
			DestroyDialog();
			return TRUE;
		}
		case WM_NOTIFY:
		{
			switch (((LPNMHDR) lParam)->code) {
				case LVN_GETDISPINFO: 
				{
					HandleLVNGetDispInfo((NMLVDISPINFO*)lParam);
					return TRUE;
				}
				case NM_CUSTOMDRAW:
				{
					SetWindowLong(hwnd, DWL_MSGRESULT, (LONG)HandleCustomDraw(lParam));
					return TRUE;
				}
			}
		}

	}

	return FALSE;
}

void ServerPingSettings::HandleLVNGetDispInfo(NMLVDISPINFO *lvdi)
{
	Pinger *pinger = pingers->GetElement(lvdi->item.iItem);

	String pszText;

	switch (lvdi->item.iSubItem) {
        case 0: {
			lvdi->item.pszText = AddPool(pinger->GetServiceName());
            break;
		}     
        case 1: {
            lvdi->item.pszText = AddPool(pinger->GetServerName());
            break;
		}
		case 2: {
			pszText = pinger->GetLatestPing();
			lvdi->item.pszText = AddPool(pszText);
			break;
		}
		case 3: {
			pszText = FloatString(pinger->Average());
			lvdi->item.pszText = AddPool(pszText);
			break;
		}
		case 4: {
			pszText = FloatString(pinger->StandardDeviation());
			lvdi->item.pszText = AddPool(pszText);
			break;
		}
        default:
            break;
	}
}

COLORREF ServerPingSettings::GetColor(int pingerIndex, int listViewColumn) {
	switch (listViewColumn) {
		case 1:
		case 0: {
			return COLOR_BLACK;
		}
		case 2:  {
			int value = pingers->GetElement(pingerIndex)->GetLatestPing();
			if (value < 75) {
				return COLOR_GREEN;
			}
			if (value < 150) {
				return COLOR_OLIVE;
			}
			return COLOR_RED;
		}

		case 3: {
			double value = pingers->GetElement(pingerIndex)->Average();
			if (value < 75) {
				return COLOR_GREEN;
			}
			if (value < 150) {
				return COLOR_OLIVE;
			}
			return COLOR_RED;
		}
		case 4: {
			double value = pingers->GetElement(pingerIndex)->StandardDeviation();
			if (value < 7.5) {
				return COLOR_GREEN;
			}
			if (value < 20.0) {
				return COLOR_OLIVE;
			}
			return COLOR_RED;
		}
	}
	return COLOR_BLACK;
}

LRESULT ServerPingSettings::HandleCustomDraw(LPARAM lParam)
{
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

	switch(lplvcd->nmcd.dwDrawStage) 
	{
		case CDDS_PREPAINT: {
			return	CDRF_NOTIFYITEMDRAW;
		}
		case CDDS_ITEMPREPAINT: {
			return CDRF_NOTIFYSUBITEMDRAW; 
		}

		//Before a subitem is drawn
		case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
			lplvcd->clrText = GetColor(lplvcd->nmcd.dwItemSpec, lplvcd->iSubItem);
			return CDRF_NEWFONT;
		}
	}
	return CDRF_DODEFAULT;
}

LPTSTR ServerPingSettings::AddPool(const String &pstr)
{
	LPTSTR pstrRetVal;
	int nOldest = nextFreePoolItem;
	stringPool[nextFreePoolItem] = pstr;
	pstrRetVal = stringPool[nextFreePoolItem];
	lptstrPool[nextFreePoolItem++] = pstrRetVal;
	
	if (nextFreePoolItem == 3)
		nextFreePoolItem = 0;

	return pstrRetVal;
}

void ServerPingSettings::InitDialog()
{
	LocalizeWindow(hwnd);



	HWND hwndServerList = GetDlgItem(hwnd, IDC_SERVERLIST);

	for(UINT i = 0; i < pingers->Num(); i++) {
		pingers->GetElement(i)->Clear();
	}

	InitServerListColumns(hwndServerList);
	InitServerListData(hwndServerList);

	isUpdateThreadFinished = false;
	_beginthread(pingThread, 0, NULL);
}

bool ServerPingSettings::InitServerListColumns(HWND hwndServerList)
{


	int columnIndex = 0;

	LVCOLUMN lvc; 
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.iSubItem = columnIndex;
	lvc.pszText = (LPWSTR)STR("Service");
	lvc.cx = 150;
	lvc.fmt = LVCFMT_LEFT;

	if (ListView_InsertColumn(hwndServerList, columnIndex, &lvc) == -1) {
		return false;
	}

	columnIndex++; 

	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.iSubItem = columnIndex;
	lvc.pszText = (LPWSTR)STR("ServerName");
	lvc.cx = 150;
	lvc.fmt = LVCFMT_LEFT;

	if (ListView_InsertColumn(hwndServerList, columnIndex, &lvc) == -1) {
		return false;
	}

	columnIndex++;

	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.iSubItem = columnIndex;
	lvc.pszText = (LPWSTR)STR("Last");
	lvc.cx = 100;
	lvc.fmt = LVCFMT_LEFT;

	if (ListView_InsertColumn(hwndServerList, columnIndex, &lvc) == -1) {
		return false;
	}

	columnIndex++;

	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.iSubItem = columnIndex;
	lvc.pszText = (LPWSTR)STR("Average");
	lvc.cx = 100;
	lvc.fmt = LVCFMT_LEFT;

	if (ListView_InsertColumn(hwndServerList, columnIndex, &lvc) == -1) {
		return false;
	}

	columnIndex++;

	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.iSubItem = columnIndex;
	lvc.pszText = (LPWSTR)STR("Jitter");
	lvc.cx = 100;
	lvc.fmt = LVCFMT_LEFT;

	if (ListView_InsertColumn(hwndServerList, columnIndex, &lvc) == -1) {
		return false;
	}

	return true;

}

bool ServerPingSettings::InitServerListData(HWND hwndServerList)
{
	LVITEM lvi;

    lvi.pszText   = LPSTR_TEXTCALLBACK;
    lvi.mask      = LVIF_TEXT | LVIF_STATE;
    lvi.stateMask = 0;
    lvi.iSubItem  = 0;
    lvi.state     = 0;
	
	for(UINT i = 0; i < pingers->Num(); i++) {
		lvi.iItem = i;
		if (ListView_InsertItem(hwndServerList, &lvi) == -1) {
			return false;
		}
	}
	return true;
}

void ServerPingSettings::DestroyDialog()
{
	isUpdateThreadFinished = true;
}
