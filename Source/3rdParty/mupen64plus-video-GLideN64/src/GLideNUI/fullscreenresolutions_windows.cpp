#include <windows.h>
#include <QObject>
#include <stdio.h>
#include <math.h>
#include "FullscreenResolutions.h"
#include "../Config.h"

#if defined(_MSC_VER) && _MSC_VER < 1900

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

__inline int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
	int count = -1;

	if (size != 0)
		count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
	if (count == -1)
		count = _vscprintf(format, ap);

	return count;
}

__inline int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
{
	int count;
	va_list ap;

	va_start(ap, format);
	count = c99_vsnprintf(outBuf, size, format, ap);
	va_end(ap);

	return count;
}

#endif

static
struct
{
	struct
	{
		DWORD width, height, refreshRate;
	} selected;

	struct
	{
		DWORD	width, height;
	} resolution[32];

	DWORD refreshRate[32];

	DWORD	numResolutions;
	DWORD	numRefreshRates;
} fullscreen;

static
void _fillFullscreenRefreshRateList(QStringList & _listRefreshRates, int & _rateIdx)
{
	memset(&fullscreen.refreshRate, 0, sizeof(fullscreen.refreshRate));
	fullscreen.numRefreshRates = 0;
	_rateIdx = 0;

	int i = 0;
	DEVMODE deviceMode;
	while (EnumDisplaySettings(NULL, i++, &deviceMode) != 0)
	{
		if (deviceMode.dmBitsPerPel != 32)
			continue;

		DWORD j = 0;
		for (; j < fullscreen.numRefreshRates; ++j)	{
			if ((deviceMode.dmDisplayFrequency == fullscreen.refreshRate[j]))
				break;
		}
		if ((deviceMode.dmDisplayFrequency != fullscreen.refreshRate[j]) &&
			(deviceMode.dmPelsWidth == fullscreen.selected.width) &&
			(deviceMode.dmPelsHeight == fullscreen.selected.height)) {

			fullscreen.refreshRate[j] = deviceMode.dmDisplayFrequency;
			//: Abbreviation for Hertz; include a leading space if appropriate
			_listRefreshRates.append(QString::number(deviceMode.dmDisplayFrequency) + QObject::tr(" Hz"));

			if (fullscreen.selected.refreshRate == deviceMode.dmDisplayFrequency)
				_rateIdx = fullscreen.numRefreshRates;

			++fullscreen.numRefreshRates;
		}
	}
}

void fillFullscreenResolutionsList(QStringList & _listResolutions, int & _resolutionIdx, QStringList & _listRefreshRates, int & _rateIdx)
{
	fullscreen.selected.width = config.video.fullscreenWidth;
	fullscreen.selected.height = config.video.fullscreenHeight;
	fullscreen.selected.refreshRate = config.video.fullscreenRefresh;

	memset(&fullscreen.resolution, 0, sizeof(fullscreen.resolution));
	memset(&fullscreen.refreshRate, 0, sizeof(fullscreen.refreshRate));
	fullscreen.numResolutions = 0;
	fullscreen.numRefreshRates = 0;
	_resolutionIdx = 0;

	constexpr int numRatios = 6;
	static
	struct
	{
		unsigned short x, y;
		const char *description;
	} ratios[numRatios] = {
		{ 3,  2, "3:2" },
		{ 4,  3, "4:3" },
		{ 5,  4, "5:4" },
		{ 16, 9, "16:9" },
		{ 8,  5, "16:10" },
		{ 21, 9, "21:9" }
	};

	int i = 0;
	char text[128];
	DEVMODE deviceMode;
	while (EnumDisplaySettings(NULL, i++, &deviceMode) != 0)
	{
		if (deviceMode.dmBitsPerPel != 32)
			continue;

		DWORD j = 0;
		for (; j < fullscreen.numResolutions; ++j) {
			if ((deviceMode.dmPelsWidth == fullscreen.resolution[j].width) &&
				(deviceMode.dmPelsHeight == fullscreen.resolution[j].height)) {
				break;
			}
		}
		if ((deviceMode.dmPelsWidth != fullscreen.resolution[j].width) ||
			(deviceMode.dmPelsHeight != fullscreen.resolution[j].height)) {

			fullscreen.resolution[fullscreen.numResolutions].width = deviceMode.dmPelsWidth;
			fullscreen.resolution[fullscreen.numResolutions].height = deviceMode.dmPelsHeight;
			snprintf(text, 128, "%i x %i", deviceMode.dmPelsWidth, deviceMode.dmPelsHeight);

			for (int j = 0; j < numRatios; ++j)
				if (fabs((float)deviceMode.dmPelsWidth / (float)deviceMode.dmPelsHeight
					- (float)ratios[j].x / (float)ratios[j].y) < 0.005f) {
					snprintf(text, 128, "%s (%s)", text, ratios[j].description);
					break;
				}

			_listResolutions.append(text);

			if ((fullscreen.selected.width == deviceMode.dmPelsWidth) &&
				(fullscreen.selected.height == deviceMode.dmPelsHeight))
				_resolutionIdx = fullscreen.numResolutions;

			++fullscreen.numResolutions;
		}
	}

	_fillFullscreenRefreshRateList(_listRefreshRates, _rateIdx);
}

void fillFullscreenRefreshRateList(int _resolutionIdx, QStringList & _listRefreshRates, int & _rateIdx)
{
	fullscreen.selected.width = fullscreen.resolution[_resolutionIdx].width;
	fullscreen.selected.height = fullscreen.resolution[_resolutionIdx].height;
	_fillFullscreenRefreshRateList(_listRefreshRates, _rateIdx);
	_rateIdx = fullscreen.numRefreshRates - 1;
}

void getFullscreenResolutions(int _idx, unsigned int & _width, unsigned int & _height)
{
	_width = fullscreen.resolution[_idx].width;
	_height = fullscreen.resolution[_idx].height;
}

void getFullscreenRefreshRate(int _idx, unsigned int & _rate)
{
	_rate = fullscreen.refreshRate[_idx];
}

namespace {
struct Monitors
{
	std::vector<HMONITOR> hMonitors;
	std::vector<RECT> rcMonitors;
	std::vector<std::wstring> nameDevices;

	static BOOL CALLBACK MonitorEnum(HMONITOR hMon, HDC /*hdc*/, LPRECT lprcMonitor, LPARAM pData)
	{
		auto pMonitors = reinterpret_cast<Monitors*>(pData);
		pMonitors->hMonitors.push_back(hMon);
		pMonitors->rcMonitors.push_back(*lprcMonitor);
		return TRUE;
	}

	Monitors()
	{
		EnumDisplayMonitors(0, 0, MonitorEnum, (LPARAM)this);
		MONITORINFOEX minfo;
		minfo.cbSize = sizeof(MONITORINFOEX);
		for (auto hMon : hMonitors)
			if (GetMonitorInfo(hMon, &minfo))
				nameDevices.emplace_back(minfo.szDevice);
			else
				nameDevices.emplace_back();
		}
};
}

std::vector<DisplayInfo> getDisplayInfo()
{
	Monitors monitors;
	DISPLAY_DEVICE dd, ddm;
	dd.cb = sizeof(DISPLAY_DEVICE);
	ddm.cb = sizeof(DISPLAY_DEVICE);
	DWORD devIdx = 0;
	std::vector<DisplayInfo> res;
	while (EnumDisplayDevices(NULL, devIdx++, &dd, EDD_GET_DEVICE_INTERFACE_NAME)) {
		if ((dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0)
			continue;

		EnumDisplayDevices(dd.DeviceName, 0, &ddm, EDD_GET_DEVICE_INTERFACE_NAME);
		QString monName = QString::fromWCharArray(ddm.DeviceString);
		int leftBound = 0;
		for (size_t i = 0; i < monitors.nameDevices.size(); ++i) {
			if (monitors.nameDevices[i] == dd.DeviceName) {
				monName = QString(monName + " (%1x%2)").arg(
					QString::number(std::abs(monitors.rcMonitors[i].right - monitors.rcMonitors[i].left)),
					QString::number(std::abs(monitors.rcMonitors[i].top - monitors.rcMonitors[i].bottom)));
				leftBound = static_cast<int>(monitors.rcMonitors[i].left);
				break;
			}
		}
		res.push_back({ monName, QString::fromWCharArray(dd.DeviceName), leftBound });
	}

	std::sort(res.begin(), res.end(), [](DisplayInfo const& a, DisplayInfo const& b) {
		return a.m_leftBound < b.m_leftBound; });

	return res;
}
