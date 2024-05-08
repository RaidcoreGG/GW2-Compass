#ifndef SETTINGS_H
#define SETTINGS_H

#include <mutex>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

extern const char* IS_COMPASS_STRIP_VISIBLE;
extern const char* COMPASS_STRIP_OFFSET_V;

extern const char* IS_COMPASS_WORLD_VISIBLE;
extern const char* WORLD_FADE_OUT_CAMERA_DIRECTION;

extern const char* IS_COMPASS_INDICATOR_VISIBLE;
extern const char* IS_COMPASS_INDICATOR_LOCKED;
extern const char* COMPASS_INDICATOR_TEXT_PREFIX;
extern const char* COMPASS_INDICATOR_LONG;
extern const char* COMPASS_INDICATOR_OUTLINE;

namespace Settings
{
	extern std::mutex	Mutex;
	extern json			Settings;

	/* Loads the settings. */
	void Load(std::filesystem::path aPath);
	/* Saves the settings. */
	void Save(std::filesystem::path aPath);

	/* Global */

	/* Widget */
	extern bool IsWidgetEnabled;
	extern float WidgetOffsetV;
	extern float WidgetWidth;
	extern float WidgetRangeDegrees;
	extern float WidgetStepDegrees;

	/* Widget derived */
	extern float WidgetCenterX;
	extern float WidgetOffsetPerDegree;

	/* World/Agent */
	extern bool IsAgentEnabled;
	extern bool FadeOutCameraDirection;

	/* Indicator */
	extern bool IsIndicatorEnabled;
	extern bool IsIndicatorLocked;
	extern std::string IndicatorPrefix;
	extern char IndicatorPrefixC[64];
	extern bool IndicatorLong;
	extern bool IndicatorOutline;
}

#endif
