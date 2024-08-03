#include "IAKAR.h"

#include <codecvt>
#include <fstream>
#include <iostream>

#include "Nt.h"

std::wstring AnsiBytesToWString(char* bytes, size_t length) {
    std::wstring result(length, L'\0'); // Erstelle ein std::wstring mit der Länge
    for (size_t i = 0; i < length; ++i) {
        result[i] = static_cast<wchar_t>(bytes[i]); // Einfaches Casting für ANSI
    }
    return result;
}

std::vector<std::wstring> IAKAR::GetDetectedACW() const
{
    std::set<std::wstring> detectedAC = DetectedAC();
    return { detectedAC.begin(), detectedAC.end() };
}

std::vector<std::wstring> IAKAR::GetAllDriversW()
{
    return { m_DriverList.begin(), m_DriverList.end() };
}

const std::set<std::wstring>& IAKAR::GetAllDrivers() const
{
	return m_DriverList;
}

std::wstring IAKAR::GetDriverPath(std::wstring_view name)
{
    
    tNtQuerySystemInformation query = reinterpret_cast<tNtQuerySystemInformation>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQuerySystemInformation"));
    if (query == nullptr)
        throw std::runtime_error("Failed to get NtQuerySystemInformation");

    std::vector<std::wstring> driverPaths;
    
    // Query the list of loaded drivers
    ULONG bufferSize = 0;
    NTSTATUS status = query(SystemModuleInformation, nullptr, bufferSize, &bufferSize);
    if (NT_SUCCESS(status))
    {
        throw std::runtime_error("NtQuerySystemInformation went wrong");
        return {};
    }
     
    std::vector<BYTE> buffer(bufferSize);
    status = query(SystemModuleInformation, buffer.data(), bufferSize, &bufferSize);
    if (NT_ERROR(status))
    {
        return {};
    }
    
    // Parse the list of loaded drivers
    SYSTEM_MODULE_INFORMATION* modules = reinterpret_cast<SYSTEM_MODULE_INFORMATION*>(buffer.data());
    for (ULONG i = 0; i < modules->ModulesCount; ++i)
    {
        PSYSTEM_MODULE module = &modules->Modules[i];
        std::wstring driverPath = AnsiBytesToWString(module->Name, 256);
        driverPaths.push_back(driverPath);
    }

    m_CurrentDriversRunning = modules->ModulesCount;

    for (const auto& driver : driverPaths)
    {
        if (driver.contains(name))
            return driver;

    }

    return {L"Not found"};
}

void IAKAR::Update()
{
    m_DriverList.clear();
    std::vector<LPVOID> base(1024);
    DWORD needed = 0;

    if (!K32EnumDeviceDrivers(base.data(), static_cast<DWORD>(base.size() * sizeof(LPVOID)), &needed))
    {
        throw std::runtime_error("Failed to enumerate device drivers");
    }

    size_t driverCount = needed / sizeof(LPVOID);
    base.resize(driverCount);

    if (!K32EnumDeviceDrivers(base.data(), static_cast<DWORD>(base.size() * sizeof(LPVOID)), &needed))
    {
        throw std::runtime_error("Failed to enumerate device drivers");
    }

    constexpr size_t nameBufferSize = 256;

    for (size_t i = 0; i < driverCount; ++i)
    {
        wchar_t nameBuffer[nameBufferSize];
        if (K32GetDeviceDriverBaseNameW(base[i], nameBuffer, static_cast<DWORD>(nameBufferSize)))
        {
            m_DriverList.insert(nameBuffer); 
        }
    }
}

void IAKAR::DumpAll() const
{
    auto file = std::wofstream(L"DumpAll.txt");
    if (!file.is_open())
        file.open(L"DumpAll.txt");


    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing");
    }

    int count = 1;
	for (const auto& entry : m_DriverList)
	{
        file << count << ": " << entry << '\n';
        count++;
	}

    file.close();
}

void IAKAR::DumpDetectd()
{
    auto drivers = DetectedAC();

    if (drivers.empty())
        return;

    auto file = std::wofstream(L"DumpDetected.txt");
    if (!file.is_open())
        file.open(L"DumpDetected.txt");


    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing");
    }

    int count = 1;
    for (const auto& entry : drivers)
    {
        file << count << ": " << entry << '\n';
        count++;
    }

    file.close();
}

std::size_t IAKAR::GetDriverCount() const
{
    return m_CurrentDriversRunning;
}

std::set<std::wstring> IAKAR::DetectedAC() const
{
    std::set<std::wstring> detectedAC;

    for (const auto& driver : m_DriverList)
    {
        if (m_Blacklisted.contains(driver))
        {
            detectedAC.insert(driver);
        }
    }

    return detectedAC;
}
