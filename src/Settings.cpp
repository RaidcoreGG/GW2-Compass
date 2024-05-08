#include "Settings.h"

#include "Shared.h"

#include <filesystem>
#include <fstream>

const char* IS_COMPASS_STRIP_VISIBLE = "IsCompassStripVisible";
const char* COMPASS_STRIP_OFFSET_V = "CompassStripOffsetV";
const char* IS_COMPASS_WORLD_VISIBLE = "IsCompassWorldVisible";
const char* WORLD_FADE_OUT_CAMERA_DIRECTION = "WorldFadeOutCameraDirection";
const char* IS_COMPASS_INDICATOR_VISIBLE = "IsCompassIndicatorVisible";
const char* IS_COMPASS_INDICATOR_LOCKED	= "IsCompassIndicatorLocked";
const char* COMPASS_INDICATOR_TEXT_PREFIX = "CompassIndicatorTextPrefix";
const char* COMPASS_INDICATOR_LONG = "CompassIndicatorLong";
const char* COMPASS_INDICATOR_OUTLINE = "CompassIndicatorOutline";

namespace Settings
{
	std::mutex	Mutex;
	json		Settings = json::object();

	void Load(std::filesystem::path aPath)
	{
		if (!std::filesystem::exists(aPath)) { return; }

		Settings::Mutex.lock();
		{
			try
			{
				std::ifstream file(aPath);
				Settings = json::parse(file);
				file.close();
			}
			catch (json::parse_error& ex)
			{
				APIDefs->Log(ELogLevel_WARNING, "Compass", "Settings.json could not be parsed.");
				APIDefs->Log(ELogLevel_WARNING, "Compass", ex.what());
			}
		}
		Settings::Mutex.unlock();

		/* Widget */
		if (!Settings[IS_COMPASS_STRIP_VISIBLE].is_null())
		{
			Settings[IS_COMPASS_STRIP_VISIBLE].get_to<bool>(IsWidgetEnabled);
		}
		if (!Settings[COMPASS_STRIP_OFFSET_V].is_null())
		{
			Settings[COMPASS_STRIP_OFFSET_V].get_to<float>(WidgetOffsetV);
		}

		/* Widget derived */
		WidgetCenterX = WidgetWidth / 2;
		WidgetOffsetPerDegree = WidgetWidth / WidgetRangeDegrees;

		/* World/Agent */
		if (!Settings[IS_COMPASS_WORLD_VISIBLE].is_null())
		{
			Settings[IS_COMPASS_WORLD_VISIBLE].get_to<bool>(IsAgentEnabled);
		}

		/* Indicator */
		if (!Settings[IS_COMPASS_INDICATOR_VISIBLE].is_null())
		{
			Settings[IS_COMPASS_INDICATOR_VISIBLE].get_to<bool>(IsIndicatorEnabled);
		}
		if (!Settings[IS_COMPASS_INDICATOR_LOCKED].is_null())
		{
			Settings[IS_COMPASS_INDICATOR_LOCKED].get_to<bool>(IsIndicatorLocked);
		}
		if (!Settings[COMPASS_INDICATOR_TEXT_PREFIX].is_null())
		{
			Settings[COMPASS_INDICATOR_TEXT_PREFIX].get_to<std::string>(IndicatorPrefix);
			strcpy_s(IndicatorPrefixC, sizeof(IndicatorPrefixC), IndicatorPrefix.c_str());
		}
		if (!Settings[COMPASS_INDICATOR_LONG].is_null())
		{
			Settings[COMPASS_INDICATOR_LONG].get_to<bool>(IndicatorLong);
		}
		if (!Settings[COMPASS_INDICATOR_OUTLINE].is_null())
		{
			Settings[COMPASS_INDICATOR_OUTLINE].get_to<bool>(IndicatorOutline);
		}
	}
	void Save(std::filesystem::path aPath)
	{
		Settings::Mutex.lock();
		{
			std::ofstream file(aPath);
			file << Settings.dump(1, '\t') << std::endl;
			file.close();
		}
		Settings::Mutex.unlock();
	}

	/* Global */

	/* Widget */
	bool IsWidgetEnabled = true;
	float WidgetOffsetV = 0.0f;
	float WidgetWidth = 600.0f;
	float WidgetRangeDegrees = 180.0f;
	float WidgetStepDegrees = 15.0f;

	/* Widget derived */
	float WidgetCenterX = WidgetWidth / 2;
	float WidgetOffsetPerDegree = WidgetWidth / WidgetRangeDegrees;

	/* World/Agent */
	bool IsAgentEnabled = true;
	bool FadeOutCameraDirection = true;

	/* Indicator */
	bool IsIndicatorEnabled = false;
	bool IsIndicatorLocked = false;
	std::string IndicatorPrefix;
	char IndicatorPrefixC[64] = "";
	bool IndicatorLong = true;
	bool IndicatorOutline = true;
}