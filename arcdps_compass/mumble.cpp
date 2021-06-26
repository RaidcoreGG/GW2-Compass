#include "mumble.h"
#include <Windows.h>
#include <string>
#include <math.h>

static HANDLE hMumble = nullptr;
extern LinkedMem* p_Mem = nullptr;

std::wstring get_mumble_name();

LinkedMem* mumble_link_create()
{
	std::wstring mumble_name = get_mumble_name();

	hMumble = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, mumble_name.c_str());
	if (hMumble == 0)
	{
		hMumble = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, sizeof(LinkedMem), mumble_name.c_str());
	}

	if (hMumble)
	{
		p_Mem = (LinkedMem*)MapViewOfFile(hMumble, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(LinkedMem));
		return p_Mem;
	}

	return nullptr;
}

void mumble_link_destroy()
{
	if (p_Mem)
	{
		UnmapViewOfFile((LPVOID)p_Mem);
		p_Mem = NULL;
	}

	if (hMumble)
	{
		CloseHandle(hMumble);
		hMumble = NULL;
	}
}

std::wstring get_mumble_name()
{
	static std::wstring const command = L"-mumble";
	std::wstring commandLine = GetCommandLine();

	size_t index = commandLine.find(command, 0);

	if (index != std::wstring::npos) {
		if (index + command.length() < commandLine.length()) {
			auto const start = index + command.length() + 1;
			auto const end = commandLine.find(' ', start);
			std::wstring mumble = commandLine.substr(start, (end != std::wstring::npos ? end : commandLine.length()) - start);

			return mumble;
		}
	}

	return L"MumbleLink";
}