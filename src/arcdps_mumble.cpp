#pragma comment(lib, "winmm.lib")
#include <stdint.h>
#include <stdio.h>
#include <Windows.h>
#include <D3D9.h>
#include "arcdps_structs.h"
#include "imgui\imgui.h"
#include <string>
#include "mumble.h"

/* proto */
extern "C" __declspec(dllexport) void* get_init_addr(char* arcversion, ImGuiContext* imguictx, IDirect3DDevice9* id3dd9, HANDLE arcdll, void* mallocfn, void* freefn);
extern "C" __declspec(dllexport) void* get_release_addr();

arcdps_exports* mod_init();
uintptr_t mod_release();
uintptr_t mod_imgui(uint32_t not_charsel_or_loading); /* id3dd9::present callback, before imgui::render, fn(uint32_t not_charsel_or_loading) */
uintptr_t mod_options(); /* id3dd9::present callback, appending to the end of options window in arcdps, fn() */
uintptr_t mod_options_windows(const char* windowname); /* id3dd9::present callback, appending to the end of options window in arcdps, fn() */

void log(char* str);
void log_file(char* str);
void log_arc(char* str);

/* globals */
arcdps_exports arc_exports;
IDirect3DDevice9* d3d9device;
void* filelog;
void* arclog;

bool enable_compass = false;
bool enable_compass_strip = false;
bool enable_compass_character = false;
LinkedMem* p_Mumble = nullptr;

/* log to arcdps.log and log window*/
void log(char* str)
{
	log_file(str);
	log_arc(str);
	return;
}
/* log to arcdps.log, thread/async safe */
void log_file(char* str)
{
	size_t(*log)(char*) = (size_t(*)(char*))filelog;
	if (log) (*log)(str);
	return;
}
/* log to extensions tab in arcdps log window, thread/async safe */
void log_arc(char* str)
{
	size_t(*log)(char*) = (size_t(*)(char*))arclog;
	if (log) (*log)(str);
	return;
}

/* export -- arcdps looks for this exported function and calls the address it returns on client load */
extern "C" __declspec(dllexport) void* get_init_addr(char* arcversion, ImGuiContext* imguictx, IDirect3DDevice9* id3dd9, HANDLE arcdll, void* mallocfn, void* freefn)
{
	ImGui::SetCurrentContext((ImGuiContext*)imguictx);
	ImGui::SetAllocatorFunctions((void *(*)(size_t, void*))mallocfn, (void(*)(void*, void*))freefn); // on imgui 1.80+

	d3d9device = id3dd9;
	filelog = (void*)GetProcAddress((HMODULE)arcdll, "e3");
	arclog = (void*)GetProcAddress((HMODULE)arcdll, "e8");

	return mod_init;
}
/* export -- arcdps looks for this exported function and calls the address it returns on client exit */
extern "C" __declspec(dllexport) void* get_release_addr()
{
	return mod_release;
}

/* initialize mod -- return table that arcdps will use for callbacks. exports struct and strings are copied to arcdps memory only once at init */
arcdps_exports* mod_init()
{
	/* for arcdps */
	memset(&arc_exports, 0, sizeof(arc_exports));
	arc_exports.sig = 0xC8028A2E;
	arc_exports.imguivers = IMGUI_VERSION_NUM;
	arc_exports.size = sizeof(arc_exports);
	arc_exports.out_name = "Compass";
	arc_exports.out_build = "2021-06-24";
	arc_exports.imgui = mod_imgui;
	//arc_exports.options_end = mod_options;
	arc_exports.options_windows = mod_options_windows;

	log((char*)"Compass: MOD_INIT"); // if using vs2015+, project properties > c++ > conformance mode > permissive to avoid const to not const conversion error

	p_Mumble = mumble_link_create();

	return &arc_exports;
}
/* release mod -- return ignored */
uintptr_t mod_release()
{
	mumble_link_destroy();
	return 0;
}

uintptr_t mod_imgui(uint32_t not_charsel_or_loading)
{
	if (enable_compass)
	{
		char buffer[4096];
		char* p = &buffer[0];

		p += _snprintf_s(p, 400, _TRUNCATE, "== INTERFACE ==\n");
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %u\n", "version", p_Mumble->ui_version);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %u\n", "tick", p_Mumble->ui_tick);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %s\n", "mapopen", p_Mumble->IsMapOpen ? "true" : "false");
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %s\n", "focus", p_Mumble->IsGameFocused ? "true" : "false");
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %s\n", "pvp", p_Mumble->IsCompetitive ? "true" : "false");
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %s\n", "txtfocus", p_Mumble->IsTextboxFocused ? "true" : "false");
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %s\n", "combat", p_Mumble->IsInCombat ? "true" : "false");
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %s\n", "cmploc", p_Mumble->IsCompassTopRight ? "top" : "bottom");
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %u\n", "cmpwidth", p_Mumble->compassWidth);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %u\n", "cmpheight", p_Mumble->compassHeight);
		if (p_Mumble->IsCompassRotating)
		{
			p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %09.4f\n", "cmprot", p_Mumble->compassRotation);
		}
		else
		{
			p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %s\n", "cmprot", "disabled");
		}
		float camrotdeg = std::atan2(p_Mumble->cam_front.x, p_Mumble->cam_front.z) * 180 / 3.14159265f; // gets the rotation in degrees; north = 0, clock-wise to 360
		if (camrotdeg < 0) { camrotdeg += 360; }
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %09.4f\n", "mapx", p_Mumble->mapCenterX);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %09.4f\n", "mapy", p_Mumble->mapCenterY);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %09.4f\n", "mapscale", p_Mumble->mapScale);
		p += _snprintf_s(p, 400, _TRUNCATE, "\n");

		p += _snprintf_s(p, 400, _TRUNCATE, "== PLAYER ==\n");
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %+09.4f\n", "posx", p_Mumble->avatar_pos.x);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %+09.4f\n", "posy", p_Mumble->avatar_pos.y);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %+09.4f\n", "posz", p_Mumble->avatar_pos.z);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %+09.4f\n", "frontx", p_Mumble->avatar_front.x);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %+09.4f\n", "fronty", p_Mumble->avatar_front.y);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %+09.4f\n", "frontz", p_Mumble->avatar_front.z);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %+09.4f\n", "gx", p_Mumble->playerX);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %+09.4f\n", "gy", p_Mumble->playerY);
		std::string mount;
		switch (p_Mumble->mountIndex)
		{
			case 0x00: mount = "Unmounted"; break;
			case 0x01: mount = "Jackal"; break;
			case 0x02: mount = "Griffon"; break;
			case 0x03: mount = "Springer"; break;
			case 0x04: mount = "Skimmer"; break;
			case 0x05: mount = "Raptor"; break;
			case 0x06: mount = "RollerBeetle"; break;
			case 0x07: mount = "Warclaw"; break;
			case 0x08: mount = "Skyscale"; break;
		}
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %s\n", "mount", mount.c_str());
		p += _snprintf_s(p, 400, _TRUNCATE, "\n");

		p += _snprintf_s(p, 400, _TRUNCATE, "== CAMERA ==\n");
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %+09.4f\n", "posx", p_Mumble->cam_pos.x);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %+09.4f\n", "posy", p_Mumble->cam_pos.y);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %+09.4f\n", "posz", p_Mumble->cam_pos.z);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %+09.4f\n", "frontx", p_Mumble->cam_front.x);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %+09.4f\n", "fronty", p_Mumble->cam_front.y);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %+09.4f\n", "frontz", p_Mumble->cam_front.z);
		p += _snprintf_s(p, 400, _TRUNCATE, "\n");

		p += _snprintf_s(p, 400, _TRUNCATE, "== GAME ==\n");
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %u\n", "mapid", p_Mumble->mapId);
		std::string mapType;
		switch (p_Mumble->mapType)
		{
			case 0x00: mapType = "AutoRedirect"; break;
			case 0x01: mapType = "CharacterCreation"; break;
			case 0x02: mapType = "PvP"; break;
			case 0x03: mapType = "GvG"; break;
			case 0x04: mapType = "Instance"; break;
			case 0x05: mapType = "PvE"; break;
			case 0x06: mapType = "Tournament"; break;
			case 0x07: mapType = "Tutorial"; break;
			case 0x08: mapType = "UserTournament"; break;
			case 0x09: mapType = "WvW_EBG"; break;
			case 0x0A: mapType = "WvW_BBL"; break;
			case 0x0B: mapType = "WvW_GBL"; break;
			case 0x0C: mapType = "WvW_RBL"; break;
			case 0x0D: mapType = "WvW_FV"; break;
			case 0x0E: mapType = "WvW_OS"; break;
			case 0x0F: mapType = "WvW_EOTM"; break;
			case 0x10: mapType = "PvE_Mini"; break;
			case 0x11: mapType = "BIG_BATTLE"; break;
			case 0x12: mapType = "WvW_Lounge"; break;
			case 0x13: mapType = "WvW"; break;
		}
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %s\n", "maptype", mapType.c_str());
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %u.%u.%u.%u\n", "ip",
			p_Mumble->serverAddress[4], p_Mumble->serverAddress[5],
			p_Mumble->serverAddress[6], p_Mumble->serverAddress[7]);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %u\n", "shard", p_Mumble->shardId);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %u\n", "instance", p_Mumble->instance);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %u\n", "build", p_Mumble->buildId);
		p += _snprintf_s(p, 400, _TRUNCATE, "%-010s %u\n", "pid", p_Mumble->processId);

		//ImGui::ShowDemoWindow();
		ImGui::Begin("== DEBUG ==", 0, 99);
		ImGui::Text(buffer);
		ImGui::End();
	}

	return 0;
}

uintptr_t mod_options()
{
	ImGui::Checkbox("Compass", &enable_compass);
	return 0;
}

uintptr_t mod_options_windows(const char* windowname) {
	if (!windowname) {
		mod_options();
	}
	return 0;
}