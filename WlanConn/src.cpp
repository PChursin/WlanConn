#ifndef UNICODE
#define UNICODE
#endif

#include "stddef.h"

#include "example.h"

BOOL scanWait;

VOID WlanNotification(WLAN_NOTIFICATION_DATA *wlan_notif, VOID *p) {
	if (wlan_notif->NotificationCode == wlan_notification_acm_scan_complete) {
		scanWait = false;
	}
	else if (wlan_notif->NotificationCode == wlan_notification_acm_scan_fail) {
		scanWait = false;
		wprintf(L"Scan failed: %s", wlan_notif->pData);
	}
}

std::string string_to_hex(const std::string& input)
{
	static const char* const lut = "0123456789ABCDEF";
	size_t len = input.length();

	std::string output;
	output.reserve(2 * len);
	for (size_t i = 0; i < len; ++i)
	{
		const unsigned char c = input[i];
		output.push_back(lut[c >> 4]);
		output.push_back(lut[c & 15]);
	}
	return output;
}

void replaceSubstring(std::string &s, const std::string &search, const std::string &replace) {
	for (size_t pos = 0; ; pos += replace.length()) {
		// Locate the substring to replace
		pos = s.find(search, pos);
		if (pos == std::string::npos) break;
		// Replace by erasing and inserting
		s.erase(pos, search.length());
		s.insert(pos, replace);
	}
}

std::string prepareProfile(std::string name) {
	std::string out, line;
	std::ifstream file("template.xml");
	while (std::getline(file, line)) {
		out.append(line);
		out.push_back('\n');
	}
	replaceSubstring(out, "%name%", name);
	replaceSubstring(out, "%hex%", string_to_hex(name));
	return out;
}

std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

int adhocConnect()
{

	// Declare and initialize variables.

	HANDLE hClient = NULL;
	DWORD dwMaxClient = 2;      //    
	DWORD dwCurVersion = 0;
	DWORD dwResult = 0;
	DWORD dwRetVal = 0;
	int iRet = 0;

	WCHAR GuidString[39] = { 0 };

	unsigned int i, j, k;

	/* variables used for WlanEnumInterfaces  */

	PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
	PWLAN_INTERFACE_INFO pIfInfo = NULL;

	PWLAN_AVAILABLE_NETWORK_LIST pBssList = NULL;
	PWLAN_AVAILABLE_NETWORK pBssEntry = NULL;

	std::vector<PWLAN_AVAILABLE_NETWORK> adhocNetworks;

	dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
	if (dwResult != ERROR_SUCCESS) {
		wprintf(L"WlanOpenHandle failed with error: %u\n", dwResult);
		return 1;
		// You can use FormatMessage here to find out why the function failed
	}

	dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
	if (dwResult != ERROR_SUCCESS) {
		wprintf(L"WlanEnumInterfaces failed with error: %u\n", dwResult);
		return 1;
		// You can use FormatMessage here to find out why the function failed
	}
	else {
		wprintf(L"Num Entries: %lu\n", pIfList->dwNumberOfItems);
		wprintf(L"Current Index: %lu\n", pIfList->dwIndex);
		for (i = 0; i < (int)pIfList->dwNumberOfItems; i++) {
			pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[i];
			wprintf(L"  Interface Index[%u]:\t %lu\n", i, i);
			iRet = StringFromGUID2(pIfInfo->InterfaceGuid, (LPOLESTR)&GuidString,
				sizeof(GuidString) / sizeof(*GuidString));
			// For c rather than C++ source code, the above line needs to be
			// iRet = StringFromGUID2(&pIfInfo->InterfaceGuid, (LPOLESTR) &GuidString, 
			//     sizeof(GuidString)/sizeof(*GuidString)); 
			if (iRet == 0)
				wprintf(L"StringFromGUID2 failed\n");
			else {
				wprintf(L"  InterfaceGUID[%d]: %ws\n", i, GuidString);
			}
			wprintf(L"  Interface Description[%d]: %ws", i,
				pIfInfo->strInterfaceDescription);
			wprintf(L"\n");
			wprintf(L"  Interface State[%d]:\t ", i);
			switch (pIfInfo->isState) {
			case wlan_interface_state_not_ready:
				wprintf(L"Not ready\n");
				break;
			case wlan_interface_state_connected:
				wprintf(L"Connected\n");
				break;
			case wlan_interface_state_ad_hoc_network_formed:
				wprintf(L"First node in a ad hoc network\n");
				break;
			case wlan_interface_state_disconnecting:
				wprintf(L"Disconnecting\n");
				break;
			case wlan_interface_state_disconnected:
				wprintf(L"Not connected\n");
				break;
			case wlan_interface_state_associating:
				wprintf(L"Attempting to associate with a network\n");
				break;
			case wlan_interface_state_discovering:
				wprintf(L"Auto configuration is discovering settings for the network\n");
				break;
			case wlan_interface_state_authenticating:
				wprintf(L"In process of authenticating\n");
				break;
			default:
				wprintf(L"Unknown state %ld\n", pIfInfo->isState);
				break;
			}
			wprintf(L"\n");

			DWORD dwPrevNotif;

			if (dwResult = WlanRegisterNotification(hClient, WLAN_NOTIFICATION_SOURCE_ACM, TRUE,
				(WLAN_NOTIFICATION_CALLBACK)WlanNotification, NULL, NULL, &dwPrevNotif) != ERROR_SUCCESS) {
				wprintf(L"Error occured: %d", dwResult);
				exit(1);
			}

			if (dwResult = WlanScan(hClient, &pIfInfo->InterfaceGuid, NULL, NULL, NULL) != ERROR_SUCCESS) {
				wprintf(L"Error occured: %d", dwResult);
				exit(1);
			}

			wprintf(L"Scanning..");
			scanWait = true;
			while (scanWait) {
				wprintf(L".");
				Sleep(500);
			}
			wprintf(L"\n");

			WlanRegisterNotification(hClient, WLAN_NOTIFICATION_SOURCE_NONE, TRUE,
				NULL, NULL, NULL, &dwPrevNotif);

			dwResult = WlanGetAvailableNetworkList(hClient,
				&pIfInfo->InterfaceGuid,
				0,
				NULL,
				&pBssList);

			if (dwResult != ERROR_SUCCESS) {
				wprintf(L"WlanGetAvailableNetworkList failed with error: %u\n",
					dwResult);
				dwRetVal = 1;
			} else {
				for (j = 0; j < pBssList->dwNumberOfItems; j++) {
					pBssEntry =
						(WLAN_AVAILABLE_NETWORK *)& pBssList->Network[j];
					if (pBssEntry->dot11BssType == dot11_BSS_type_independent) {
						adhocNetworks.push_back(pBssEntry);
					}
				}
			}
		}
	}
	
	wprintf(L"available wlan interfaces:\n");
	for (int i = 0; i < (int)pIfList->dwNumberOfItems; i++) {
		wprintf(L"%d) %s\n", i+1, ((WLAN_INTERFACE_INFO *)&(pIfList->InterfaceInfo[i]))->strInterfaceDescription);
	}
	int intCh = -1;
	while (intCh < 1 || intCh > pIfList->dwNumberOfItems) {
		printf("Choose interface: ");
		scanf("%d", &intCh);
		printf("\n");
	}

	wprintf(L"available adHoc networks:\n");
	std::string tmp;
	int idx = 1;
	std::vector<std::string> netNames;
	for (PWLAN_AVAILABLE_NETWORK net : adhocNetworks) {
		tmp.clear();
		for (k = 0; k < net->dot11Ssid.uSSIDLength; k++) {
			tmp.append(1, net->dot11Ssid.ucSSID[k]);
		}
		netNames.push_back(tmp);
		printf("%d) %s\n", idx++, tmp.c_str());
	}

	int netCh = -1;
	while (netCh < 1 || netCh > idx) {
		printf("Choose network: ");
		scanf("%d", &netCh);
		printf("\n");
	}
	std::string profile = prepareProfile(netNames[netCh - 1]);

	std::wstring t = s2ws(profile.c_str()).c_str();
//	wprintf(L"%s\n", t.c_str());

	PWLAN_AVAILABLE_NETWORK net = adhocNetworks[netCh-1];
	PWLAN_CONNECTION_PARAMETERS conPars = new WLAN_CONNECTION_PARAMETERS();
	conPars->wlanConnectionMode = wlan_connection_mode_temporary_profile;
	conPars->strProfile = t.c_str();
	conPars->pDot11Ssid = &net->dot11Ssid;
	conPars->pDesiredBssidList = NULL;
	conPars->dot11BssType = net->dot11BssType;
	conPars->dwFlags = WLAN_CONNECTION_ADHOC_JOIN_ONLY;

	dwResult = WlanConnect(hClient, &pIfList->InterfaceInfo[intCh - 1].InterfaceGuid, conPars, NULL);
	if (dwResult != ERROR_SUCCESS) {
		wprintf(L"Error occured: %d", dwResult);
	} else {
		wprintf(L"Seems like connection SUCCESS!\n");
	}

	if (pBssList != NULL) {
		WlanFreeMemory(pBssList);
		pBssList = NULL;
	}

	if (pIfList != NULL) {
		WlanFreeMemory(pIfList);
		pIfList = NULL;
	}

	if (conPars != NULL) {
		delete conPars;
	}
	return dwRetVal;
}

#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")
int wmain() {
	//ex1();
	adhocConnect();
}