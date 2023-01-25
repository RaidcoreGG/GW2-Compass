#include <Windows.h>
#include <string>
#include <cmath>

#include "AddonHost.h"
#include "imgui\imgui.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
		case DLL_PROCESS_ATTACH: break;
		case DLL_PROCESS_DETACH: break;
		case DLL_THREAD_ATTACH: break;
		case DLL_THREAD_DETACH: break;
    }
    return TRUE;
}

AddonAPI APIDefs;
AddonDefinition* AddonDef;

bool IsCompassStripVisible = false;
bool IsWorldCompassVisible = false;
LinkedMem* MumbleLink = nullptr;

void ProcessKeybind(const wchar_t* aIdentifier)
{
	/* if COMPASS_TOGGLEVIS is passed, we toggle the compass visibility */
	if (wcscmp(aIdentifier, L"COMPASS_TOGGLEVIS") == 0)
	{
		IsCompassStripVisible = !IsCompassStripVisible;
		return;
	}
}

void SetupKeybinds()
{
	Keybind ctrlC{};
	ctrlC.Ctrl = true;
	ctrlC.Key = 67;
	APIDefs.RegisterKeybind(L"COMPASS_TOGGLEVIS", ProcessKeybind, ctrlC);
}

void AddonLoad(AddonAPI aHostApi)
{
	APIDefs = aHostApi;
	ImGui::SetCurrentContext(aHostApi.ImguiContext);
	//ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))mallocfn, (void(*)(void*, void*))freefn); // on imgui 1.80+

	MumbleLink = APIDefs.MumbleLink;

	SetupKeybinds();
}

void AddonUnload()
{
	APIDefs.UnregisterKeybind(L"COMPASS_TOGGLEVIS");
	/* free some resources */
}

std::string GetMarkerText(int aRotation, bool notch = true)
{
	// adjust for 0-180 -180-0 -> 0-359
	if (aRotation < 0) { aRotation += 360; }
	if (aRotation == 360) { aRotation = 0; }

    std::string marker;

	switch (aRotation)
	{
		case   0: marker.append("N"); break;
		case  45: marker.append("NE"); break;
		case  90: marker.append("E"); break;
		case 135: marker.append("SE"); break;
		case 180: marker.append("S"); break;
		case 225: marker.append("SW"); break;
		case 270: marker.append("W"); break;
		case 315: marker.append("NW"); break;
		default:  marker.append(notch ? "|" : std::to_string(aRotation)); break;
	}

    return marker;
}

float widgetWidth = 600.0f;
float range = 180.0f;
float step = 15.0f;
float centerX = widgetWidth / 2;
float offsetPerDegree = widgetWidth / range;

void AddonRender()
{
	if (!IsCompassStripVisible) { return; }

	ImGui::PushItemWidth(widgetWidth);
    if (ImGui::Begin("COMPASS_STRIP", (bool *)0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar))
    {
        float fRot = atan2f(MumbleLink->CameraFront.X, MumbleLink->CameraFront.Z) * 180.0f / 3.14159f;
        int iRot = round(fRot);

		ImGui::Separator();

        ImVec2 initial = ImGui::GetCursorPos();
		initial.x = 0;

		/* this is to display text between 0-360 */
        std::string cText = GetMarkerText(iRot, false);

		/* center scope */
        ImVec2 scope = initial;
        scope.x += centerX;
		float scopeWidth = ImGui::CalcTextSize(cText.c_str()).x / 2;
        scope.x -= scopeWidth; // center the marker
        ImGui::SetCursorPos(scope);
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(221, 153, 0, 255));
        ImGui::Text(cText.c_str());
		ImGui::PopStyleColor();
		
		for (size_t i = 0; i < 360 / step; i++)
		{
			float fMarker = step * i;
			if (fMarker > 180) { fMarker -= 360; } // correct so we have negative wrapping again instead of 0-360;
			int iMarker = round(fMarker);

			/* this is to display text between 0-360 */
			std::string notchText = GetMarkerText(iMarker, false);

			ImVec2 pos = initial;
			pos.x += centerX;
			float offset = offsetPerDegree * (fMarker + (fRot * (-1)));
			
			/* fix behind */
			if (offset > (widgetWidth * 1.5)) { offset -= (widgetWidth * 2); }
			else if (offset < (widgetWidth * 0.5 * -1)) { offset += (widgetWidth * 2); }

			float centerPreText = offset;
			float markerWidth = ImGui::CalcTextSize(notchText.c_str()).x / 2;
			offset -= markerWidth; // center the marker
			pos.x += offset;

			// + 5.0f "padding"
			float centerLow = initial.x - (scopeWidth + markerWidth + 5.0f);
			float centerHigh = initial.x + (scopeWidth + markerWidth + 5.0f);
			if (centerPreText > centerLow && centerPreText < centerHigh) { continue; }

			ImGui::SetCursorPos(pos);
			ImGui::Text(notchText.c_str());
		}
    }

    ImGui::End();
}

void AddonOptions()
{
	/* This addon has no options *shrug* */
	return;
}

extern "C" __declspec(dllexport) AddonDefinition * GetAddonDef()
{
	AddonDef = new AddonDefinition();
	AddonDef->Signature = 17;
	AddonDef->Name = L"World Compass";
	AddonDef->Version = __DATE__ L" " __TIME__;
	AddonDef->Author = L"Raidcore";
	AddonDef->Description = L"Adds a simple compass widget to the UI, as well as to your character in the world.";
	AddonDef->Load = AddonLoad;
	AddonDef->Unload = AddonUnload;

	AddonDef->Render = AddonRender;
	AddonDef->Options = AddonOptions;

	/* not necessary if hosted on Raidcore, but shown anyway for the  example also useful as a backup resource */
	AddonDef->Provider = EUpdateProvider::GitHub;
	AddonDef->UpdateLink = L"https://github.com/RaidcoreGG/GW2-Compass";

	return AddonDef;
}