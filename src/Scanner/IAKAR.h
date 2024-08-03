#pragma once
#include <array>
#include <vector>
#include <Windows.h>
#include <psapi.h>
#include <set>
#include <string>


class IAKAR
{
public:
	IAKAR()
	{
	}

    std::vector<std::wstring> GetDetectedACW() const;
	std::vector<std::wstring> GetAllDriversW();
	const std::set<std::wstring>& GetAllDrivers() const;
    std::wstring GetDriverPath(std::wstring_view name);

	void Update();

    void DumpAll() const;
    void DumpDetectd();

    std::size_t GetDriverCount() const;
    std::set<std::wstring> DetectedAC() const;

private:
    std::size_t m_CurrentDriversRunning = 0;

	std::set<std::wstring> m_DriverList = {};
    std::set<std::wstring> m_Blacklisted =
    {
        L"EasyAntiCheat.sys",
        L"BECore.sys",
        L"vgk.sys",
        L"Ricochet.sys",
        L"Xigncode3.sys",
        L"PnkBstrK.sys",
        L"FACEIT.sys",
        L"Guardians.sys",
        L"AntiCheat.sys",
        L"Aegis.sys",
        L"Hyperion.sys",
        L"GamerGuard.sys",
        L"DETECTOR.sys",
        L"Sentry.sys",
        L"Cybershield.sys",
        L"Shielded.sys",
        L"Guardian.sys",
        L"AntiCheatPro.sys"
    };
};

