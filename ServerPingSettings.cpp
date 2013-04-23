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

volatile int currentRowBeingPinged = -1;

void pingThread(void *arg)
{

	ServerPingSettings *settings = ServerPingPlugin::instance->GetSettings();
	List<Pinger *> *pingers = settings->GetPingers();
	HWND hwndServerList = GetDlgItem(settings->GetHwnd(), IDC_SERVERLIST);

	int previousRow = -1;
	while(!settings->isUpdateThreadFinished) {
		
		for(UINT i = 0; i < pingers->Num(); i++) {
			currentRowBeingPinged = i;
			// update the previous row to turn off the background
			SendMessage(hwndServerList, LVM_UPDATE, previousRow, 0);
			// update the new row signifying that it is currently being pinged
			SendMessage(hwndServerList, LVM_UPDATE, i, 0);

			LeaveCriticalSection(&settings->pingerMutex);
			pingers->GetElement(i)->Ping();
			LeaveCriticalSection(&settings->pingerMutex);

			int selectedPingerIndex = ListView_GetNextItem(hwndServerList, -1, LVNI_SELECTED);
			if (selectedPingerIndex == i) {
				settings->UpdateSelectedItemValues(i);
			}

			// update the row with the new information
			SendMessage(hwndServerList, LVM_UPDATE, i, 0);
			previousRow = i;
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
	InitializeCriticalSection(&pingerMutex);

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

bool ServerPingSettings::HasDefaults() const
{
	return false;
}

void ServerPingSettings::SetDefaults() {
}

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
		case WM_COMMAND:
		{
			if(LOWORD(wParam) == IDC_CLEAR) {
				HandleClear();
				return TRUE;
			}
		}
		case WM_NOTIFY:
		{
			switch (((LPNMHDR) lParam)->code) {
				case LVN_GETDISPINFO: 
				{
					HandleLVNGetDispInfo((NMLVDISPINFO*)lParam);
					return TRUE;
				}
				case LVN_ITEMCHANGED: 
				{
					HandleLVNItemChanged((LPNMLISTVIEW)lParam);
					return TRUE;
				}
				case NM_CUSTOMDRAW:
				{
					SetWindowLong(hwnd, 0, (LONG)HandleCustomDraw(lParam));
					return TRUE;
				}	
			}
			break;
		}
	}

	return FALSE;
}

void ServerPingSettings::HandleClear() {
	HWND hwndServerList = GetDlgItem(hwnd, IDC_SERVERLIST);
	int selectedPingerIndex = ListView_GetNextItem(hwndServerList, -1, LVNI_SELECTED);
	
	EnterCriticalSection(&pingerMutex);

	for(UINT i = 0; i < pingers->Num(); i++) {
		pingers->GetElement(i)->Clear();
		SendMessage(hwndServerList, LVM_UPDATE, i, 0);
		if (selectedPingerIndex == i) {
			UpdateSelectedItemValues(i);
		}
	}

	LeaveCriticalSection(&pingerMutex);


}

void ServerPingSettings::HandleLVNGetDispInfo(NMLVDISPINFO *lvdi)
{
	Pinger *pinger = pingers->GetElement(lvdi->item.iItem);

	String pszText = " ";

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
			int latestPing = pinger->GetLatestPing();
			if (latestPing > -1) {
				pszText = latestPing;
			}
			lvdi->item.pszText = AddPool(pszText);
			break;
		}
		case 3: {
			double averagePing = pinger->Average();
			if (_finite(averagePing)) {
				pszText = FloatString(pinger->Average());
			}
			lvdi->item.pszText = AddPool(pszText);
			break;
		}
		case 4: {
			double standardDeviation = pinger->StandardDeviation();
			if (_finite(standardDeviation)) {
				pszText = FloatString(abs(standardDeviation));
			}
			lvdi->item.pszText = AddPool(pszText);
			break;
		}
        default:
            break;
	}
}

void ServerPingSettings::HandleLVNItemChanged(LPNMLISTVIEW pnmv)
{
	BOOL isSelectedNow = (pnmv->uNewState & LVIS_SELECTED);
	BOOL wasSelectedBefore = (pnmv->uOldState  & LVIS_SELECTED);
    if (isSelectedNow && !wasSelectedBefore) {
		UpdateSelectedItemValues(pnmv->iItem);
	}
}

void ServerPingSettings::UpdateSelectedItemValues(int pingerIndex)
{
	HWND hwndServiceAndServerGroup = GetDlgItem(hwnd, IDC_SERVER_AND_SERVICE_NAME);
	HWND hwndLowestPingLabel = GetDlgItem(hwnd, IDC_LOWEST_PING);
	HWND hwndHighestPingLabel = GetDlgItem(hwnd, IDC_HIGHEST_PING);
	HWND hwndServerUrl = GetDlgItem(hwnd, IDC_SERVER_URL);

	Pinger *pinger = pingers->GetElement(pingerIndex);
	String string;
	CTSTR ctstr;

	ctstr = string = pinger->GetServiceName() + L" >> " + pinger->GetServerName();
	SendMessage(hwndServiceAndServerGroup, WM_SETTEXT, NULL, (LPARAM)ctstr);
	ctstr = string = pinger->GetServerUrl();
	SendMessage(hwndServerUrl, WM_SETTEXT, NULL, (LPARAM)ctstr);
	ctstr = string = (pinger->GetMinimumPing() == -1) ? 0 : pinger->GetMinimumPing();
	SendMessage(hwndLowestPingLabel, WM_SETTEXT, NULL, (LPARAM)ctstr);
	ctstr = string = (pinger->GetMaximumPing() == -1) ? 0 : pinger->GetMaximumPing();
	SendMessage(hwndHighestPingLabel, WM_SETTEXT, NULL, (LPARAM)ctstr);
}

COLORREF ServerPingSettings::GetColor(int pingerIndex, int listViewColumn) {
	switch (listViewColumn) {
		case 0:
		case 1: {
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
			if (lplvcd->nmcd.dwItemSpec == currentRowBeingPinged) {
				lplvcd->clrTextBk = RGB(240, 248, 255);
			} else {
				lplvcd->clrTextBk = RGB(255, 255, 255);
			}
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

	ListView_SetExtendedListViewStyleEx(hwndServerList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	InitServerListColumns(hwndServerList);
	InitServerListData(hwndServerList);

	isUpdateThreadFinished = false;
	_beginthread(pingThread, 0, (void *)this);
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
