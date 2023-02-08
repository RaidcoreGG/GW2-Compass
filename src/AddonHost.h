#ifndef ADDONHOST_H
#define ADDONHOST_H

#include <Windows.h>
#include <d3d11.h>
#include <mutex>

#include "imgui/imgui.h"
#include "Mumble.h"

// MinHook Error Codes.
enum MH_STATUS
{
	MH_UNKNOWN = -1,
	MH_OK = 0,
	MH_ERROR_ALREADY_INITIALIZED,
	MH_ERROR_NOT_INITIALIZED,
	MH_ERROR_ALREADY_CREATED,
	MH_ERROR_NOT_CREATED,
	MH_ERROR_ENABLED,
	MH_ERROR_DISABLED,
	MH_ERROR_NOT_EXECUTABLE,
	MH_ERROR_UNSUPPORTED_FUNCTION,
	MH_ERROR_MEMORY_ALLOC,
	MH_ERROR_MEMORY_PROTECT,
	MH_ERROR_MODULE_NOT_FOUND,
	MH_ERROR_FUNCTION_NOT_FOUND
};

typedef MH_STATUS(__stdcall*MINHOOK_CREATE)(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal);
typedef MH_STATUS(__stdcall*MINHOOK_REMOVE)(LPVOID pTarget);
typedef MH_STATUS(__stdcall*MINHOOK_ENABLE)(LPVOID pTarget);
typedef MH_STATUS(__stdcall*MINHOOK_DISABLE)(LPVOID pTarget);

struct VTableMinhook
{
	MINHOOK_CREATE		CreateHook;
	MINHOOK_REMOVE		RemoveHook;
	MINHOOK_ENABLE		EnableHook;
	MINHOOK_DISABLE		DisableHook;
};

enum class ELogLevel : unsigned char
{
	OFF = 0,
	CRITICAL = 1,
	WARNING = 2,
	INFO = 3,
	DEBUG = 4,
	TRACE = 5,
	ALL
};

typedef struct LogEntry
{
	ELogLevel LogLevel;
	unsigned long long Timestamp;
	std::wstring Message;

	std::wstring TimestampString(bool aIncludeDate = true);
	std::wstring ToString();
};

class ILogger
{
	public:
	ILogger() = default;
	virtual ~ILogger() = default;

	ELogLevel GetLogLevel();
	void SetLogLevel(ELogLevel aLogLevel);

	virtual void LogMessage(LogEntry aLogEntry) = 0;

	protected:
	ELogLevel LogLevel;
	std::mutex MessageMutex;
};

typedef void (*LOGGER_LOGA)(ELogLevel aLogLevel, const char* aFmt, ...);
typedef void (*LOGGER_ADDREM)(ILogger* aLogger);

struct VTableLogging
{
	LOGGER_LOGA			LogA;
	LOGGER_ADDREM		RegisterLogger;
	LOGGER_ADDREM		UnregisterLogger;
};

typedef void (*EVENTS_CONSUME)(void* aEventArgs);
typedef void (*EVENTS_RAISE)(std::string aEventName, void* aEventData);
typedef void (*EVENTS_SUBSCRIBE)(std::string aEventName, EVENTS_CONSUME aConsumeEventCallback);

struct Keybind
{
	WORD Key;
	bool Alt;
	bool Ctrl;
	bool Shift;
};

typedef void (*KEYBINDS_PROCESS)(std::string aIdentifier);
typedef void (*KEYBINDS_REGISTER)(std::string aIdentifier, KEYBINDS_PROCESS aKeybindHandler, std::string aKeybind);
typedef void (*KEYBINDS_UNREGISTER)(std::string aIdentifier);

typedef void* (*DATALINK_GETRESOURCE)(std::string aIdentifier);
typedef void* (*DATALINK_SHARERESOURCE)(std::string aIdentifier, size_t aResourceSize);

struct AddonAPI
{
	IDXGISwapChain*			SwapChain;
	ImGuiContext*			ImguiContext;
	unsigned*				WindowWidth;
	unsigned*				WindowHeight;

	VTableMinhook			MinhookFunctions;
	VTableLogging			LoggingFunctions;

	/* Events */
	EVENTS_RAISE			RaiseEvent;
	EVENTS_SUBSCRIBE		SubscribeEvent;
	EVENTS_SUBSCRIBE		UnsubscribeEvent;

	/* Keybinds */
	KEYBINDS_REGISTER		RegisterKeybind;
	KEYBINDS_UNREGISTER		UnregisterKeybind;

	/* DataLink */
	DATALINK_GETRESOURCE	GetResource;
	DATALINK_SHARERESOURCE	ShareResource;

	/* API */
		// GW2 API FUNCS
		// LOGITECH API FUNCS
};

enum class EUpdateProvider
{
	None = 0,		/* Does not support auto updating */
	Raidcore = 1,	/* Provider is Raidcore (via API) */
	GitHub = 2,		/* Provider is GitHub Releases */
	Direct = 3		/* Provider is direct file link */
};

typedef void (*ADDON_LOAD)(AddonAPI aHostApi);
typedef void (*ADDON_UNLOAD)();
typedef void (*ADDON_RENDER)();
typedef void (*ADDON_OPTIONS)();

struct AddonDefinition
{
    signed int      Signature;      /* Raidcore Addon ID, set to random negative integer if not on Raidcore */
    const char*		Name;           /* Name of the addon as shown in the library */
    const char*		Version;        /* Leave as `__DATE__ L" " __TIME__` to maintain consistency */
    const char*		Author;         /* Author of the addon */
    const char*		Description;    /* Short description */
    ADDON_LOAD      Load;           /* Pointer to Load Function of the addon */
    ADDON_UNLOAD    Unload;         /* Pointer to Unload Function of the addon */

    ADDON_RENDER    Render;         /* Present callback to render imgui */
    ADDON_OPTIONS   Options;        /* Options window callback, called when opening options for this addon */

    EUpdateProvider Provider;       /* What platform is the the addon hosted on */
    const char*		UpdateLink;     /* Link to the update resource */
};

#endif