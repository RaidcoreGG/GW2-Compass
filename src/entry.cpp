#include <Windows.h>
#include <string>
#include <cmath>

#include "AddonHost.h"
#include "imgui\imgui.h"
#include "imgui\imgui_extensions.h"

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

float widgetWidth = 600.0f;
float range = 180.0f;
float step = 15.0f;
float centerX = widgetWidth / 2;
float offsetPerDegree = widgetWidth / range;

ImVec2 CompassStripPosition = ImVec2(0,0);

void ProcessKeybind(std::string aIdentifier)
{
	/* if COMPASS_TOGGLEVIS is passed, we toggle the compass visibility */
	if (aIdentifier == "COMPASS_TOGGLEVIS")
	{
		IsCompassStripVisible = !IsCompassStripVisible;
		return;
	}
}

void OnWindowResized(void* aEventArgs)
{
	/* event args are nullptr, ignore */
	CompassStripPosition = ImVec2((*APIDefs.WindowWidth - widgetWidth) / 2, *APIDefs.WindowHeight * .2f);
}

const char* COMPASS_TOGGLEVIS =	"COMPASS_TOGGLEVIS";
const char* WINDOW_RESIZED =	"WINDOW_RESIZED";

void AddonLoad(AddonAPI aHostApi)
{
	APIDefs = aHostApi;
	ImGui::SetCurrentContext(aHostApi.ImguiContext);
	//ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))mallocfn, (void(*)(void*, void*))freefn); // on imgui 1.80+

	MumbleLink = APIDefs.MumbleLink;

	/* set keybinds */
	APIDefs.RegisterKeybind(COMPASS_TOGGLEVIS, ProcessKeybind, "CTRL+C");

	/* set events */
	APIDefs.SubscribeEvent(WINDOW_RESIZED, OnWindowResized);

	OnWindowResized(nullptr);
}

void AddonUnload()
{
	/* release keybinds */
	APIDefs.UnregisterKeybind(COMPASS_TOGGLEVIS);

	/* release events */
	APIDefs.UnsubscribeEvent(WINDOW_RESIZED, OnWindowResized);
}

std::string GetMarkerText(int aRotation, bool notch = true)
{
	// adjust for 0-180 -180-0 -> 0-359
	if (aRotation < 0) { aRotation += 360; }
	if (aRotation == 360) { aRotation = 0; }

    std::string marker;

	if (aRotation > 355 || aRotation < 5) { marker.append("N"); }
	else if (aRotation > 40 && aRotation < 50) { marker.append("NE"); }
	else if (aRotation > 85 && aRotation < 95) { marker.append("E"); }
	else if (aRotation > 130 && aRotation < 140) { marker.append("SE"); }
	else if (aRotation > 175 && aRotation < 185) { marker.append("S"); }
	else if (aRotation > 220 && aRotation < 230) { marker.append("SW"); }
	else if (aRotation > 265 && aRotation < 275) { marker.append("W"); }
	else if (aRotation > 310 && aRotation < 320) { marker.append("NW"); }
	else { marker.append(notch ? "|" : std::to_string(aRotation)); }

    return marker;
}

void AddonRender()
{
	if (!IsCompassStripVisible) { return; }

	ImGuiIO& io = ImGui::GetIO();
	ImGui::PushFont(io.Fonts->Fonts[1]);

	ImGui::PushItemWidth(widgetWidth);
	ImGui::SetNextWindowPos(CompassStripPosition);
    if (ImGui::Begin("COMPASS_STRIP", (bool *)0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing))
    {
        float fRot = atan2f(MumbleLink->CameraFront.X, MumbleLink->CameraFront.Z) * 180.0f / 3.14159f;
        int iRot = round(fRot);

		ImGui::Separator();

        ImVec2 initial = ImGui::GetCursorPos();
		initial.x = 0;

		/* this is to display text between 0-360 */
        std::string cText = GetMarkerText(iRot, false);

		ImGui::PushFont(io.Fonts->Fonts[2]);

		/* center scope */
        ImVec2 scope = initial;
        scope.x += centerX;
		float scopeWidth = ImGui::CalcTextSize(cText.c_str()).x / 2;
        scope.x -= scopeWidth; // center the marker
        ImGui::SetCursorPos(scope);
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(221, 153, 0, 255));
        ImGui::TextOutlined(cText.c_str());
		ImGui::PopStyleColor();

		ImGui::PopFont();
		
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
			ImGui::TextOutlined(notchText.c_str());
		}
    }

	ImGui::PopFont();

    ImGui::End();
}

extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef()
{
	AddonDef = new AddonDefinition();
	AddonDef->Signature = 17;
	AddonDef->Name = "World Compass";
	AddonDef->Version = __DATE__ " " __TIME__;
	AddonDef->Author = "Raidcore";
	AddonDef->Description = "Adds a simple compass widget to the UI, as well as to your character in the world.";
	AddonDef->Load = AddonLoad;
	AddonDef->Unload = AddonUnload;

	AddonDef->Render = AddonRender;

	/* not necessary if hosted on Raidcore, but shown anyway for the  example also useful as a backup resource */
	AddonDef->Provider = EUpdateProvider::GitHub;
	AddonDef->UpdateLink = "https://github.com/RaidcoreGG/GW2-Compass";

	return AddonDef;
}