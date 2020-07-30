#pragma once
#include <thread>

// https://www.fluentcpp.com/2018/12/28/timer-cpp/
class Timer {
	bool clear = false;

public:
	template<typename Function>
	void setTimeout(Function function, int delay);

	template<typename Function>
	void setInterval(Function function, int interval);

	void stop();
};

template<typename Function>
void Timer::setTimeout(Function function, int delay) {
	this->clear = false;
	std::thread t([=]() {
		if (this->clear) return;
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
		if (this->clear) return;
		function();
	});
	t.detach();
}

template<typename Function>
void Timer::setInterval(Function function, int interval) {
	this->clear = false;
	std::thread t([=]() {
		while (true) {
			if (this->clear) return;
			std::this_thread::sleep_for(std::chrono::milliseconds(interval));
			if (this->clear) return;
			function();
		}
	});
	t.detach();
}

void Timer::stop() {
	this->clear = true;
}

#include <windows.h>
#include <Setupapi.h>
#include <devguid.h>
#include "conio.h"
#include "tchar.h"
#include <string>
#include <vector>

#define VALID_KEYS 2
std::wstring __Keys[VALID_KEYS] = {
	L"P1310051070C37ADAFA93197", // MINE
	//L"2005173842029790AACE"
	L"00000000001E79"
	L"0364019100003694" // KH
};

int Check_USB_Key()
{
	int allowed_keys = 0;
	HDEVINFO deviceInfoSet;
	GUID *guidDev = (GUID*)&GUID_DEVCLASS_USB;
	deviceInfoSet = SetupDiGetClassDevs(guidDev, NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);
	std::vector<std::wstring> ids;
	TCHAR buffer[4000];
	DWORD buffersize = 4000;
	int memberIndex = 0;
	while (true) {
		SP_DEVINFO_DATA deviceInfoData;
		ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
		deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		if (SetupDiEnumDeviceInfo(deviceInfoSet, memberIndex, &deviceInfoData) == FALSE) {
			if (GetLastError() == ERROR_NO_MORE_ITEMS) {
				break;
			}
		}
		DWORD nSize = 0;
		SetupDiGetDeviceInstanceId(deviceInfoSet, &deviceInfoData, buffer, sizeof(buffer), &nSize);
		ids.push_back(buffer);
		buffer[nSize] = '\0';
		//_tprintf(_T("%s\n"), buffer);
		memberIndex++;

		std::wstring usb_dev = buffer;
		for (int i = 0; i < VALID_KEYS; i++)
			if (usb_dev.find(__Keys[i]) != std::wstring::npos) allowed_keys++;
	}
	if (deviceInfoSet) {
		SetupDiDestroyDeviceInfoList(deviceInfoSet);
	}
	return allowed_keys;
}