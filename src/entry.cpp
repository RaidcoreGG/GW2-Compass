#include <Windows.h>
#include <string>
#include <cmath>
#include <vector>
#include <filesystem>
#include <mutex>
#include <fstream>
#include <DirectXMath.h>

#include "Shared.h"
#include "Settings.h"

#include "Version.h"

#include "imgui/imgui.h"
#include "imgui/imgui_extensions.h"
#include "resource.h"

namespace dx = DirectX;

void ProcessKeybind(const char* aIdentifier);
void OnWindowResized(void* aEventArgs);
void OnMumbleIdentityUpdated(void* aEventArgs);
void ReceiveTexture(const char* aIdentifier, Texture* aTexture);

void AddonLoad(AddonAPI* aApi);
void AddonUnload();
void AddonRender();
void AddonOptions();
void AddonShortcut();

// little helper function for compass render
std::string GetMarkerText(int aRotation, bool notch = true);
std::string GetClosestMarkerText(int aRotation, bool full = false);

HMODULE hSelf;

AddonDefinition AddonDef{};

std::filesystem::path AddonPath;
std::filesystem::path SettingsPath;

Texture* hrTex = nullptr;

float padding = 5.0f;

ImVec2 CompassStripPosition = ImVec2(0, 0);

const char* COMPASS_TOGGLEVIS = "KB_COMPASS_TOGGLEVIS";
const char* WINDOW_RESIZED = "EV_WINDOW_RESIZED";
const char* MUMBLE_IDENTITY_UPDATED = "EV_MUMBLE_IDENTITY_UPDATED";
const char* HR_TEX = "TEX_SEPARATOR_DETAIL";

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

extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef()
{
	AddonDef.Signature = 17;
	AddonDef.APIVersion = NEXUS_API_VERSION;
	AddonDef.Name = "Compass";
	AddonDef.Version.Major = V_MAJOR;
	AddonDef.Version.Minor = V_MINOR;
	AddonDef.Version.Build = V_BUILD;
	AddonDef.Version.Revision = V_REVISION;
	AddonDef.Author = "Raidcore";
	AddonDef.Description = "Adds a simple compass widget to the UI, as well as to your character in the world.";
	AddonDef.Load = AddonLoad;
	AddonDef.Unload = AddonUnload;
	AddonDef.Flags = EAddonFlags_None;

	/* not necessary if hosted on Raidcore, but shown anyway for the example also useful as a backup resource */
	AddonDef.Provider = EUpdateProvider_GitHub;
	AddonDef.UpdateLink = "https://github.com/RaidcoreGG/GW2-Compass";

	return &AddonDef;
}

void AddonLoad(AddonAPI* aApi)
{
	APIDefs = aApi;
	ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
	ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))APIDefs->ImguiMalloc, (void(*)(void*, void*))APIDefs->ImguiFree); // on imgui 1.80+

	MumbleLink = (Mumble::Data*)APIDefs->GetResource("DL_MUMBLE_LINK");
	NexusLink = (NexusLinkData*)APIDefs->GetResource("DL_NEXUS_LINK");

	APIDefs->RegisterKeybindWithString(COMPASS_TOGGLEVIS, ProcessKeybind, "(null)");

	APIDefs->SubscribeEvent(WINDOW_RESIZED, OnWindowResized);
	APIDefs->SubscribeEvent(MUMBLE_IDENTITY_UPDATED, OnMumbleIdentityUpdated);

	APIDefs->LoadTextureFromResource(HR_TEX, IDB_PNG1, hSelf, ReceiveTexture);

	APIDefs->AddSimpleShortcut("QAS_COMPASS", AddonShortcut);

	APIDefs->RegisterRender(ERenderType_Render, AddonRender);
	APIDefs->RegisterRender(ERenderType_OptionsRender, AddonOptions);

	AddonPath = APIDefs->GetAddonDirectory("Compass");
	SettingsPath = APIDefs->GetAddonDirectory("Compass/settings.json");
	std::filesystem::create_directory(AddonPath);
	Settings::Load(SettingsPath);

	OnWindowResized(nullptr); // initialise self
}
void AddonUnload()
{
	APIDefs->DeregisterRender(AddonOptions);
	APIDefs->DeregisterRender(AddonRender);

	APIDefs->RemoveSimpleShortcut("QAS_COMPASS");

	// textures do not have to be freed

	APIDefs->UnsubscribeEvent(WINDOW_RESIZED, OnWindowResized);

	APIDefs->DeregisterKeybind(COMPASS_TOGGLEVIS);

	MumbleLink = nullptr;
	NexusLink = nullptr;

	Settings::Save(SettingsPath);
}

std::vector<Vector3> av_interp;
std::vector<Vector3> avf_interp;

Vector3 Average(std::vector<Vector3> aVectors)
{
	Vector3 avg{};
	for (size_t i = 0; i < aVectors.size(); i++)
	{
		avg.X += aVectors[i].X;
		avg.Y += aVectors[i].Y;
		avg.Z += aVectors[i].Z;
	}

	avg.X /= aVectors.size();
	avg.Y /= aVectors.size();
	avg.Z /= aVectors.size();

	return avg;
}

constexpr float r = 24 * 2.54f / 100;
constexpr float deg = 360.0f / 200;

void AddonRender()
{
	if (!NexusLink || !MumbleLink|| !MumbleIdentity || !NexusLink->IsGameplay) { return; }
	
	/* get rotation in float for accuracy and int for display */
	float fRot = atan2f(MumbleLink->CameraFront.X, MumbleLink->CameraFront.Z) * 180.0f / 3.14159f;
	int iRot = round(fRot);

	if (Settings::IsAgentEnabled)
	{
		Vector3 cameraFacing = MumbleLink->CameraFront;
		Vector3 camera = MumbleLink->CameraPosition;

		av_interp.push_back(MumbleLink->AvatarPosition);
		avf_interp.push_back(MumbleLink->AvatarFront);
		if (av_interp.size() < 15) { return; }
		av_interp.erase(av_interp.begin());
		avf_interp.erase(avf_interp.begin());

		Vector3 av = Average(av_interp);
		Vector3 av_facing = Average(avf_interp);

		Vector3 origin = av;

		float deltaAvCam = sqrt((camera.X - origin.X) * (camera.X - origin.X) +
			(camera.Y - origin.Y) * (camera.Y - origin.Y) +
			(camera.Z - origin.Z) * (camera.Z - origin.Z));

		bool cull = false;

		if (deltaAvCam < 2.5f && cameraFacing.Y > -0.5f) { cull = true; }

		float facingDeg = atan2f(av_facing.X, av_facing.Z) * 180.0f / 3.14159f;

		Vector3 north = { r * 2 * sin(0.0f * 3.14159f / 180.0f) + origin.X, origin.Y, r * 2 * cos(0.0f * 3.14159f / 180.0f) + origin.Z };
		Vector3 east = { r * 2 * sin(90.0f * 3.14159f / 180.0f) + origin.X, origin.Y, r * 2 * cos(90.0f * 3.14159f / 180.0f) + origin.Z };
		Vector3 south = { r * 2 * sin(180.0f * 3.14159f / 180.0f) + origin.X, origin.Y, r * 2 * cos(180.0f * 3.14159f / 180.0f) + origin.Z };
		Vector3 west = { r * 2 * sin(270.0f * 3.14159f / 180.0f) + origin.X, origin.Y, r * 2 * cos(270.0f * 3.14159f / 180.0f) + origin.Z };

		dx::XMVECTOR vCamPos = { MumbleLink->CameraPosition.X, MumbleLink->CameraPosition.Y, MumbleLink->CameraPosition.Z };
		dx::XMVECTOR vCamFront = { MumbleLink->CameraFront.X, MumbleLink->CameraFront.Y, MumbleLink->CameraFront.Z };

		dx::XMVECTOR lookAtPosition = dx::XMVectorAdd(vCamPos, vCamFront);
		dx::XMMATRIX matView = dx::XMMatrixLookAtLH(vCamPos, lookAtPosition, { 0, 1.0f, 0 });
		dx::XMMATRIX matProj = dx::XMMatrixPerspectiveFovLH(MumbleIdentity->FOV, (float)NexusLink->Width / (float)NexusLink->Height, 1.0f, 10000.0f);

		dx::XMMATRIX matWorld = dx::XMMatrixIdentity();

		ImDrawList* dl = ImGui::GetBackgroundDrawList();

		dx::XMVECTOR northProj = dx::XMVector3Project({ north.X, north.Y, north.Z }, 0, 0, NexusLink->Width, NexusLink->Height, 1.0f, 10000.0f, matProj, matView, matWorld);
		dx::XMVECTOR eastProj = dx::XMVector3Project({ east.X, east.Y, east.Z }, 0, 0, NexusLink->Width, NexusLink->Height, 1.0f, 10000.0f, matProj, matView, matWorld);
		dx::XMVECTOR southProj = dx::XMVector3Project({ south.X, south.Y, south.Z }, 0, 0, NexusLink->Width, NexusLink->Height, 1.0f, 10000.0f, matProj, matView, matWorld);
		dx::XMVECTOR westProj = dx::XMVector3Project({ west.X, west.Y, west.Z }, 0, 0, NexusLink->Width, NexusLink->Height, 1.0f, 10000.0f, matProj, matView, matWorld);

		/* two passes for black first "shadow" then white for actual lines */

		ImGui::PushFont((ImFont*)NexusLink->FontBig);
		float fontSize = ImGui::GetFontSize();
		ImGui::PopFont();

		ImVec2 nSize = ImGui::CalcTextSize("N");
		ImVec2 eSize = ImGui::CalcTextSize("E");
		ImVec2 sSize = ImGui::CalcTextSize("S");
		ImVec2 wSize = ImGui::CalcTextSize("W");

		float camRot = fRot;
		if (camRot < 0.0f) { camRot += 360.0f; }
		if (camRot == 0.0f) { camRot = 360.0f; }

		dl->AddText((ImFont*)NexusLink->FontBig, fontSize, ImVec2(northProj.m128_f32[0] - (nSize.x / 2) + 1.0f, northProj.m128_f32[1] - (nSize.y / 2) + 1.0f), ImColor(0, 0, 0, Settings::FadeOutCameraDirection ? (int)(abs(0.0f - fRot) / 45.0f * 255) : 255), "N");
		dl->AddText((ImFont*)NexusLink->FontBig, fontSize, ImVec2(eastProj.m128_f32[0] - (eSize.x / 2) + 1.0f, eastProj.m128_f32[1] - (eSize.y / 2) + 1.0f), ImColor(0, 0, 0, Settings::FadeOutCameraDirection ? (int)(abs(90.0f - camRot) / 45.0f * 255) : 255), "E");
		dl->AddText((ImFont*)NexusLink->FontBig, fontSize, ImVec2(southProj.m128_f32[0] - (sSize.x / 2) + 1.0f, southProj.m128_f32[1] - (sSize.y / 2) + 1.0f), ImColor(0, 0, 0, Settings::FadeOutCameraDirection ? (int)(abs(180.0f - camRot) / 45.0f * 255) : 255), "S");
		dl->AddText((ImFont*)NexusLink->FontBig, fontSize, ImVec2(westProj.m128_f32[0] - (wSize.x / 2) + 1.0f, westProj.m128_f32[1] - (wSize.y / 2) + 1.0f), ImColor(0, 0, 0, Settings::FadeOutCameraDirection ? (int)(abs(270.0f - camRot) / 45.0f * 255) : 255), "W");

		dl->AddText((ImFont*)NexusLink->FontBig, fontSize, ImVec2(northProj.m128_f32[0] - (nSize.x / 2), northProj.m128_f32[1] - (nSize.y / 2)), ImColor(255, 255, 255, Settings::FadeOutCameraDirection ? (int)(abs(0.0f - fRot) / 45.0f * 255) : 255), "N");
		dl->AddText((ImFont*)NexusLink->FontBig, fontSize, ImVec2(eastProj.m128_f32[0] - (eSize.x / 2), eastProj.m128_f32[1] - (eSize.y / 2)), ImColor(255, 255, 255, Settings::FadeOutCameraDirection ? (int)(abs(90.0f - camRot) / 45.0f * 255) : 255), "E");
		dl->AddText((ImFont*)NexusLink->FontBig, fontSize, ImVec2(southProj.m128_f32[0] - (sSize.x / 2), southProj.m128_f32[1] - (sSize.y / 2)), ImColor(255, 255, 255, Settings::FadeOutCameraDirection ? (int)(abs(180.0f - camRot) / 45.0f * 255) : 255), "S");
		dl->AddText((ImFont*)NexusLink->FontBig, fontSize, ImVec2(westProj.m128_f32[0] - (wSize.x / 2), westProj.m128_f32[1] - (wSize.y / 2)), ImColor(255, 255, 255, Settings::FadeOutCameraDirection ? (int)(abs(270.0f - camRot) / 45.0f * 255) : 255), "W");
	}

	if (Settings::IsWidgetEnabled)
	{
		ImGuiIO& io = ImGui::GetIO();

		/* use Menomonia */
		ImGui::PushFont((ImFont*)NexusLink->Font);

		/* set width and position */
		ImGui::PushItemWidth(Settings::WidgetWidth);
		ImGui::SetNextWindowPos(CompassStripPosition);
		if (ImGui::Begin("COMPASS_STRIP", (bool*)0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar))
		{
			float offsetTop = 0.0f;

			// draw separator texture
			if (hrTex != nullptr)
			{
				float imgScaleFactor = Settings::WidgetWidth / hrTex->Width;

				ImGui::SetCursorPos(ImVec2(0, 0));
				ImGui::Image(hrTex->Resource, ImVec2(Settings::WidgetWidth, hrTex->Height * imgScaleFactor));
				offsetTop = hrTex->Height * imgScaleFactor;
			}
			else
			{
				ImGui::Separator();
				offsetTop = ImGui::GetFontSize();
			}

			/* use Menomonia but bigger */
			ImGui::PushFont((ImFont*)NexusLink->FontBig);

			/* center scope */
			/* logic, the draw point for the scope is the center of the widget minus half of the center text size */
			std::string scopeText = GetMarkerText(iRot, false); /* this is to display text between 0-360 */
			float scopeOffset = ImGui::CalcTextSize(scopeText.c_str()).x / 2; /* width of the center text divided by 2 */
			ImVec2 scopeDrawPos = ImVec2(Settings::WidgetCenterX - scopeOffset, offsetTop); /* center minus half of center text */
			ImGui::SetCursorPos(scopeDrawPos);
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(221, 153, 0, 255));
			ImGui::TextOutlined(scopeText.c_str());
			ImGui::PopStyleColor();

			/* stop using big Menomonia */
			ImGui::PopFont();

			/* iterate through 360 degrees to get the notches/markers */
			for (size_t i = 0; i < 360 / Settings::WidgetStepDegrees; i++)
			{
				float fMarker = Settings::WidgetStepDegrees * i; /* this is a fake "rotation" based on the current notch */
				//if (fMarker > 180) { fMarker -= 360; } // correct so we have negative wrapping again instead of 0-360;
				int iMarker = round(fMarker);

				float markerRelativeOffset = Settings::WidgetOffsetPerDegree * (fMarker + (fRot * (-1))); /* where is the marker going to be drawn, relative to the center notch */

				/* fix behind */
				/* I don't even know why this works, I managed to get this functioning by changing random calculations */
				if (markerRelativeOffset > (Settings::WidgetWidth * 1.5)) { markerRelativeOffset -= (Settings::WidgetWidth * 2); }
				else if (markerRelativeOffset < (Settings::WidgetWidth * 0.5 * -1)) { markerRelativeOffset += (Settings::WidgetWidth * 2); }


				/* this is to display text between 0-360 */
				std::string notchText = GetMarkerText(iMarker, false);
				float markerCenter = Settings::WidgetCenterX + markerRelativeOffset;
				float markerWidth = ImGui::CalcTextSize(notchText.c_str()).x;
				float markerOffset = markerWidth / 2; /* marker text width divided by 2 */
				ImVec2 markerDrawPos = ImVec2(Settings::WidgetCenterX + markerRelativeOffset - markerOffset, offsetTop); /* based on the center + offset from center (already accounted for wrap around) - markerOffset (which is markerText divided by 2) */

				float helperWidth = ImGui::CalcTextSize("XXX").x;
				float helperOffset = helperWidth / 2;

				float t = 255; /* transparency variable*/

				float markerLeft = markerCenter - markerOffset - padding; // left side of the text
				float markerRight = markerCenter + markerOffset + padding; // right side of the text

				/* breakpoints where fade should start */
				float brL = markerWidth;
				float brLC = Settings::WidgetCenterX - markerWidth - helperWidth;
				float brRC = Settings::WidgetCenterX + markerWidth + helperWidth;
				float brR = Settings::WidgetWidth - markerWidth;

				/*ImGui::SetCursorPos(ImVec2(brL - (ImGui::CalcTextSize(".").x / 2), offsetTop));
				ImGui::Text(".");
				ImGui::SetCursorPos(ImVec2(brLC - (ImGui::CalcTextSize(".").x / 2), offsetTop));
				ImGui::Text(".");
				ImGui::SetCursorPos(ImVec2(brRC - (ImGui::CalcTextSize(".").x / 2), offsetTop));
				ImGui::Text(".");
				ImGui::SetCursorPos(ImVec2(brR - (ImGui::CalcTextSize(".").x / 2), offsetTop));
				ImGui::Text(".");*/

				if (markerLeft <= 0 || markerRight >= Settings::WidgetWidth) { continue; } // skip oob
				else if (markerLeft < brL)
				{
					t *= (markerLeft / markerWidth);
				}
				else if (markerRight > brR)
				{
					t *= ((Settings::WidgetWidth - markerRight) / markerWidth);
				}
				else if (markerRight > brLC && markerLeft < Settings::WidgetCenterX)
				{
					t *= (((Settings::WidgetCenterX - helperOffset) - markerRight) / (markerWidth + helperOffset));
					if (markerRight > (Settings::WidgetCenterX - helperOffset)) { t = 0; }
				}
				else if (markerLeft < brRC && markerRight > Settings::WidgetCenterX)
				{
					t *= ((markerLeft - (Settings::WidgetCenterX + helperOffset)) / (markerWidth + helperOffset));
					if (markerLeft < (Settings::WidgetCenterX + helperOffset)) { t = 0; }
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

	if (Settings::IsIndicatorEnabled)
	{
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
		if (ImGui::Begin("COMPASS_INDICATOR", (bool*)0, (Settings::IsIndicatorLocked ? (ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs) : 0) | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar))
		{
			std::string outStr;
			if (!Settings::IndicatorPrefix.empty())
			{
				outStr.append(Settings::IndicatorPrefix);
			}
			
			outStr.append(GetClosestMarkerText(iRot, Settings::IndicatorLong));

			if (Settings::IndicatorOutline)
			{
				ImGui::TextOutlined(outStr.c_str());
			}
			else
			{
				ImGui::Text(outStr.c_str());
			}
		}
		ImGui::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}
}

void AddonOptions()
{
	ImGui::Text("Compass");
	ImGui::TextDisabled("Widget");
	if (ImGui::Checkbox("Enabled##Widget", &Settings::IsWidgetEnabled))
	{
		Settings::Settings[IS_COMPASS_STRIP_VISIBLE] = Settings::IsWidgetEnabled;
		Settings::Save(SettingsPath);
	}
	if (ImGui::DragFloat("Vertical Offset##Widget", &Settings::WidgetOffsetV, 1.0f, NexusLink->Height * -1.0f, NexusLink->Height * 1.0f))
	{
		Settings::Settings[COMPASS_STRIP_OFFSET_V] = Settings::WidgetOffsetV;
		OnWindowResized(nullptr);
		Settings::Save(SettingsPath);
	}

	ImGui::TextDisabled("World");
	if (ImGui::Checkbox("Enabled##World", &Settings::IsAgentEnabled))
	{
		Settings::Settings[IS_COMPASS_WORLD_VISIBLE] = Settings::IsAgentEnabled;
		Settings::Save(SettingsPath);
	}
	if (ImGui::Checkbox("Fade out camera direction##World", &Settings::FadeOutCameraDirection))
	{
		Settings::Settings[WORLD_FADE_OUT_CAMERA_DIRECTION] = Settings::FadeOutCameraDirection;
		Settings::Save(SettingsPath);
	}

	ImGui::TextDisabled("Indicator");
	if (ImGui::Checkbox("Enabled##Indicator", &Settings::IsIndicatorEnabled))
	{
		Settings::Settings[IS_COMPASS_INDICATOR_VISIBLE] = Settings::IsIndicatorEnabled;
		Settings::Save(SettingsPath);
	}
	if (ImGui::Checkbox("Locked##Indicator", &Settings::IsIndicatorLocked))
	{
		Settings::Settings[IS_COMPASS_INDICATOR_LOCKED] = Settings::IsIndicatorLocked;
		Settings::Save(SettingsPath);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::Text("Locked means that the indicator can not be moved or clicked.");
		ImGui::EndTooltip();
	}
	if (ImGui::InputText("Prefix Text##Indicator", Settings::IndicatorPrefixC, sizeof(Settings::IndicatorPrefixC)))
	{
		Settings::IndicatorPrefix = Settings::IndicatorPrefixC;
		Settings::Settings[COMPASS_INDICATOR_TEXT_PREFIX] = Settings::IndicatorPrefix;
		Settings::Save(SettingsPath);
	}
	if (ImGui::Checkbox("Long Text##Indicator", &Settings::IndicatorLong))
	{
		Settings::Settings[COMPASS_INDICATOR_LONG] = Settings::IndicatorLong;
		Settings::Save(SettingsPath);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::Text("Long text means showing \"North West\" instead of \"NW\".");
		ImGui::EndTooltip();
	}
	if (ImGui::Checkbox("Outlined Text##Indicator", &Settings::IndicatorOutline))
	{
		Settings::Settings[COMPASS_INDICATOR_OUTLINE] = Settings::IndicatorOutline;
		Settings::Save(SettingsPath);
	}
}

void AddonShortcut()
{
	if (ImGui::Checkbox("Widget", &Settings::IsWidgetEnabled))
	{
		Settings::Settings[IS_COMPASS_STRIP_VISIBLE] = Settings::IsWidgetEnabled;
		Settings::Save(SettingsPath);
	}
	if (ImGui::Checkbox("Indicator", &Settings::IsIndicatorEnabled))
	{
		Settings::Settings[IS_COMPASS_INDICATOR_VISIBLE] = Settings::IsIndicatorEnabled;
		Settings::Save(SettingsPath);
	}
}

void ProcessKeybind(const char* aIdentifier)
{
	std::string str = aIdentifier;

	/* if COMPASS_TOGGLEVIS is passed, we toggle the compass visibility */
	if (str == COMPASS_TOGGLEVIS)
	{
		Settings::IsWidgetEnabled = !Settings::IsWidgetEnabled;
	}
}

void OnWindowResized(void* aEventArgs)
{
	CompassStripPosition = ImVec2((NexusLink->Width - Settings::WidgetWidth) / 2, (NexusLink->Height * .2f) + Settings::WidgetOffsetV);
}

void OnMumbleIdentityUpdated(void* aEventArgs)
{
	MumbleIdentity = (Mumble::Identity*)aEventArgs;
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

std::string GetClosestMarkerText(int aRotation, bool full)
{
	// adjust for 0-180 -180-0 -> 0-359
	if (aRotation < 0) { aRotation += 360; }
	if (aRotation == 360) { aRotation = 0; }

	std::string marker;

	if (aRotation >= 337.5 || aRotation < 22.5) { marker.append(full ? "North" : "N"); }
	else if (aRotation >= 22.5 && aRotation < 67.5) { marker.append(full ? "North East" : "NE"); }
	else if (aRotation >= 67.5 && aRotation < 112.5) { marker.append(full ? "East" : "E"); }
	else if (aRotation >= 112.5 && aRotation < 157.5) { marker.append(full ? "South East" : "SE"); }
	else if (aRotation >= 157.5 && aRotation < 202.5) { marker.append(full ? "South" : "S"); }
	else if (aRotation >= 202.5 && aRotation < 247.5) { marker.append(full ? "South West" : "SW"); }
	else if (aRotation >= 247.5 && aRotation < 292.5) { marker.append(full ? "West" : "W"); }
	else if (aRotation >= 292.5 && aRotation < 337.5) { marker.append(full ? "North West" : "NW"); }

	return marker;
}