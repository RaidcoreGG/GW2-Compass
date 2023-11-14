#include <Windows.h>
#include <string>
#include <cmath>
#include <vector>

#include "nexus/Nexus.h"
#include "imgui\imgui.h"
#include "imgui\imgui_extensions.h"
#include "resource.h"
#include "mumble/Mumble.h"

void ProcessKeybind(const char* aIdentifier);
void OnWindowResized(void* aEventArgs);
void ReceiveTexture(const char* aIdentifier, Texture* aTexture);
void AddonCompassShortcut();

void AddonLoad(AddonAPI* aApi);
void AddonUnload();
void AddonRender();
void AddonOptions();

// little helper function for compass render
std::string GetMarkerText(int aRotation, bool notch = true);

HMODULE hSelf;

AddonAPI* APIDefs;
AddonDefinition* AddonDef;

bool IsCompassStripVisible = true;
bool IsWorldCompassVisible = true;
Mumble::Data* MumbleLink = nullptr;
Mumble::Identity* MumbleIdentity = nullptr;
NexusLinkData* NexusLink = nullptr;

Texture* hrTex = nullptr;

float widgetWidth = 600.0f;
float range = 180.0f;
float step = 15.0f;
float centerX = widgetWidth / 2;
float offsetPerDegree = widgetWidth / range;
float padding = 5.0f;

ImVec2 CompassStripPosition = ImVec2(0, 0);

const char* COMPASS_TOGGLEVIS = "KB_COMPASS_TOGGLEVIS";
const char* WINDOW_RESIZED = "EV_WINDOW_RESIZED";
const char* MUMBLE_IDENITY_UPDATED = "EV_MUMBLE_IDENTITY_UPDATED";
const char* HR_TEX = "TEX_SEPARATOR_DETAIL";
std::string NLINK_NAME = "DL_NEXUS_LINK_";

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH: hSelf = hModule; break;
		case DLL_PROCESS_DETACH: break;
		case DLL_THREAD_ATTACH: break;
		case DLL_THREAD_DETACH: break;
	}
	return TRUE;
}

extern "C" __declspec(dllexport) AddonDefinition * GetAddonDef()
{
	AddonDef = new AddonDefinition();
	AddonDef->Signature = 17;
	AddonDef->APIVersion = NEXUS_API_VERSION;
	AddonDef->Name = "World Compass";
	AddonDef->Version.Major = 1;
	AddonDef->Version.Minor = 0;
	AddonDef->Version.Build = 0;
	AddonDef->Version.Revision = 1;
	AddonDef->Author = "Raidcore";
	AddonDef->Description = "Adds a simple compass widget to the UI, as well as to your character in the world.";
	AddonDef->Load = AddonLoad;
	AddonDef->Unload = AddonUnload;
	AddonDef->Flags = EAddonFlags::None;

	/* not necessary if hosted on Raidcore, but shown anyway for the example also useful as a backup resource */
	AddonDef->Provider = EUpdateProvider::GitHub;
	AddonDef->UpdateLink = "https://github.com/RaidcoreGG/GW2-Compass";

	return AddonDef;
}

void AddonLoad(AddonAPI* aApi)
{
	APIDefs = aApi;
	ImGui::SetCurrentContext(APIDefs->ImguiContext);
	ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))APIDefs->ImguiMalloc, (void(*)(void*, void*))APIDefs->ImguiFree); // on imgui 1.80+

	MumbleLink = (Mumble::Data*)APIDefs->GetResource("DL_MUMBLE_LINK");

	NLINK_NAME.append(std::to_string(GetCurrentProcessId()));
	NexusLink = (NexusLinkData*)APIDefs->GetResource(NLINK_NAME.c_str());

	APIDefs->RegisterKeybindWithString(COMPASS_TOGGLEVIS, ProcessKeybind, "CTRL+C");

	APIDefs->SubscribeEvent(WINDOW_RESIZED, OnWindowResized);

	APIDefs->LoadTextureFromResource(HR_TEX, IDB_PNG1, hSelf, ReceiveTexture);

	APIDefs->AddSimpleShortcut("QAS_COMPASS", AddonCompassShortcut);
	//APIDefs.AddShortcut("QA_COMPASS", "ICON_GENERIC", "ICON_GENERIC_HOVER", "KB_COMPASS_TOGGLEVIS", "pee pee poo poo");

	APIDefs->RegisterRender(ERenderType::Render, AddonRender);
	APIDefs->RegisterRender(ERenderType::OptionsRender, AddonOptions);

	APIDefs->Log(ELogLevel::DEBUG, "Compass addon loaded.");

	OnWindowResized(nullptr);
}

void AddonUnload()
{
	/* release keybinds */
	//APIDefs.UnregisterKeybind(COMPASS_TOGGLEVIS);

	/* release events */
	//APIDefs.UnsubscribeEvent(WINDOW_RESIZED, OnWindowResized);

	APIDefs->UnregisterRender(AddonOptions);
}

void AddonRender()
{
	if (!IsCompassStripVisible || !NexusLink->IsGameplay) { return; }

	ImGuiIO& io = ImGui::GetIO();

	/* use Menomonia */
	ImGui::PushFont(NexusLink->Font);

	/* set width and position */
	ImGui::PushItemWidth(widgetWidth);
	ImGui::SetNextWindowPos(CompassStripPosition);
	if (ImGui::Begin("COMPASS_STRIP", (bool*)0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
	{
		float offsetTop = 0.0f;

		// draw separator texture
		if (hrTex != nullptr)
		{
			ImGui::SetCursorPos(ImVec2(0, 0));
			ImGui::Image(hrTex->Resource, ImVec2(hrTex->Width, hrTex->Height));
			offsetTop = hrTex->Height;
		}
		else
		{
			ImGui::Separator();
			offsetTop = ImGui::GetFontSize();
		}

		/* get rotation in float for accuracy and int for display */
		float fRot = atan2f(MumbleLink->CameraFront.X, MumbleLink->CameraFront.Z) * 180.0f / 3.14159f;
		int iRot = round(fRot);

		/* use Menomonia but bigger */
		ImGui::PushFont(NexusLink->FontBig);

		/* center scope */
		/* logic, the draw point for the scope is the center of the widget minus half of the center text size */
		std::string scopeText = GetMarkerText(iRot, false); /* this is to display text between 0-360 */
		float scopeOffset = ImGui::CalcTextSize(scopeText.c_str()).x / 2; /* width of the center text divided by 2 */
		ImVec2 scopeDrawPos = ImVec2(centerX - scopeOffset, offsetTop); /* center minus half of center text */
		ImGui::SetCursorPos(scopeDrawPos);
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(221, 153, 0, 255));
		ImGui::TextOutlined(scopeText.c_str());
		ImGui::PopStyleColor();

		/* stop using big Menomonia */
		ImGui::PopFont();

		/* iterate through 360 degrees to get the notches/markers */
		for (size_t i = 0; i < 360 / step; i++)
		{
			float fMarker = step * i; /* this is a fake "rotation" based on the current notch */
			//if (fMarker > 180) { fMarker -= 360; } // correct so we have negative wrapping again instead of 0-360;
			int iMarker = round(fMarker);

			float markerRelativeOffset = offsetPerDegree * (fMarker + (fRot * (-1))); /* where is the marker going to be drawn, relative to the center notch */

			/* fix behind */
			/* I don't even know why this works, I managed to get this functioning by changing random calculations */
			if (markerRelativeOffset > (widgetWidth * 1.5)) { markerRelativeOffset -= (widgetWidth * 2); }
			else if (markerRelativeOffset < (widgetWidth * 0.5 * -1)) { markerRelativeOffset += (widgetWidth * 2); }


			/* this is to display text between 0-360 */
			std::string notchText = GetMarkerText(iMarker, false);
			float markerCenter = centerX + markerRelativeOffset;
			float markerWidth = ImGui::CalcTextSize(notchText.c_str()).x;
			float markerOffset = markerWidth / 2; /* marker text width divided by 2 */
			ImVec2 markerDrawPos = ImVec2(centerX + markerRelativeOffset - markerOffset, offsetTop); /* based on the center + offset from center (already accounted for wrap around) - markerOffset (which is markerText divided by 2) */

			float helperWidth = ImGui::CalcTextSize("XXX").x;
			float helperOffset = helperWidth / 2;

			float t = 255; /* transparency variable*/

			float markerLeft = markerCenter - markerOffset - padding; // left side of the text
			float markerRight = markerCenter + markerOffset + padding; // right side of the text

			/* breakpoints where fade should start */
			float brL = markerWidth;
			float brLC = centerX - markerWidth - helperWidth;
			float brRC = centerX + markerWidth + helperWidth;
			float brR = widgetWidth - markerWidth;

			/*ImGui::SetCursorPos(ImVec2(brL - (ImGui::CalcTextSize(".").x / 2), offsetTop));
			ImGui::Text(".");
			ImGui::SetCursorPos(ImVec2(brLC - (ImGui::CalcTextSize(".").x / 2), offsetTop));
			ImGui::Text(".");
			ImGui::SetCursorPos(ImVec2(brRC - (ImGui::CalcTextSize(".").x / 2), offsetTop));
			ImGui::Text(".");
			ImGui::SetCursorPos(ImVec2(brR - (ImGui::CalcTextSize(".").x / 2), offsetTop));
			ImGui::Text(".");*/

			if (markerLeft <= 0 || markerRight >= widgetWidth) { continue; } // skip oob
			else if (markerLeft < brL)
			{
				t *= (markerLeft / markerWidth);
			}
			else if (markerRight > brR)
			{
				t *= ((widgetWidth - markerRight) / markerWidth);
			}
			else if (markerRight > brLC && markerLeft < centerX)
			{
				t *= (((centerX - helperOffset) - markerRight) / (markerWidth + helperOffset));
				if (markerRight > (centerX - helperOffset)) { t = 0; }
			}
			else if (markerLeft < brRC && markerRight > centerX)
			{
				t *= ((markerLeft - (centerX + helperOffset)) / (markerWidth + helperOffset));
				if (markerLeft < (centerX + helperOffset)) { t = 0; }
			}

			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, t));
			ImGui::SetCursorPos(markerDrawPos);
			ImGui::TextOutlinedT(t, notchText.c_str());
			ImGui::PopStyleColor();
		}
	}

	ImGui::PopFont();

	ImGui::End();
}

void AddonOptions()
{
	ImGui::Separator();
	ImGui::TextDisabled("Compass addon");
	ImGui::Checkbox("Compass Strip", &IsCompassStripVisible);
	ImGui::Checkbox("Compass World", &IsWorldCompassVisible);
}

void AddonCompassShortcut()
{
	ImGui::Checkbox("Compass Strip", &IsCompassStripVisible);
	ImGui::Checkbox("Compass World", &IsWorldCompassVisible);
}

void ProcessKeybind(const char* aIdentifier)
{
	std::string str = aIdentifier;

	/* if COMPASS_TOGGLEVIS is passed, we toggle the compass visibility */
	if (str == COMPASS_TOGGLEVIS)
	{
		IsCompassStripVisible = !IsCompassStripVisible;
	}
}

void OnWindowResized(void* aEventArgs)
{
	/* event args are nullptr, ignore */
	CompassStripPosition = ImVec2((NexusLink->Width - widgetWidth) / 2, NexusLink->Height * .2f);
}

void ReceiveTexture(const char* aIdentifier, Texture* aTexture)
{
	std::string str = aIdentifier;

	if (str == HR_TEX)
	{
		hrTex = aTexture;
	}
}

std::string GetMarkerText(int aRotation, bool notch)
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