#pragma once
#include <D3D9.h>
#include <d3d9types.h>

enum EMapType : unsigned
{
	AutoRedirect = 0,
	CharacterCreation = 1,
	PvP = 2,
	GvG = 3,
	Instance = 4,
	PvE = 5,
	Tournament = 6,
	Tutorial = 7,
	UserTournament = 8,
	WvW_EBG = 9,
	WvW_BBL = 10,
	WvW_GBL = 11,
	WvW_RBL = 12,
	WVW_REWARD = 13,            // SCRAPPED
	WvW_ObsidianSanctum = 14,
	WvW_EdgeOfTheMists = 15,
	PvE_Mini = 16,
	BIG_BATTLE = 17,            // SCRAPPED
	WvW_Lounge = 18,
	WvW = 19
};

enum EMounts : unsigned char
{
	Unmounted = 0x00,
	Jackal = 0x01,
	Griffon = 0x02,
	Springer = 0x03,
	Skimmer = 0x04,
	Raptor = 0x05,
	RollerBeetle = 0x06,
	Warclaw = 0x07,
	Skyscale = 0x08
};

typedef struct LinkedMem
{
	unsigned ui_version;
	unsigned ui_tick;
	_D3DVECTOR avatar_pos;
	_D3DVECTOR avatar_front;
	_D3DVECTOR avatar_top;
	wchar_t name[256];
	_D3DVECTOR cam_pos;
	_D3DVECTOR cam_front;
	_D3DVECTOR cam_top;
	wchar_t identity[256];
	unsigned context_len;
	unsigned char serverAddress[28]; // contains sockaddr_in or sockaddr_in6
	unsigned mapId;
	EMapType mapType;
	unsigned shardId;
	unsigned instance;
	unsigned buildId;
	byte IsMapOpen : 1;
	byte IsCompassTopRight : 1;
	byte IsCompassRotating : 1;
	byte IsGameFocused : 1;
	byte IsCompetitive : 1;
	byte IsTextboxFocused : 1;
	byte IsInCombat : 1;
	byte UNUSED1 : 1;
	byte UNUSED2;
	byte UNUSED3;
	byte UNUSED4;
	unsigned short compassWidth; // pixels
	unsigned short compassHeight; // pixels
	float compassRotation; // radians
	float playerX; // continentCoords
	float playerY; // continentCoords
	float mapCenterX; // continentCoords
	float mapCenterY; // continentCoords
	float mapScale;
	unsigned processId;
	EMounts mountIndex;
	wchar_t description[2048];
} LinkedMem;

LinkedMem* mumble_link_create();
void mumble_link_destroy();