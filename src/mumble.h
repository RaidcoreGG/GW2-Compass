#pragma once
#include <D3D9.h>
#include <d3d9types.h>
#include <map>
#include <string>

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
	unsigned mapType;
	unsigned shardId;
	unsigned instance;
	unsigned buildId;
	unsigned IsMapOpen : 1;
	unsigned IsCompassTopRight : 1;
	unsigned IsCompassRotating : 1;
	unsigned IsGameFocused : 1;
	unsigned IsCompetitive : 1;
	unsigned IsTextboxFocused : 1;
	unsigned IsInCombat : 1;
	unsigned UNUSED1 : 1;
	unsigned short compassWidth; // pixels
	unsigned short compassHeight; // pixels
	float compassRotation; // radians
	float playerX; // continentCoords
	float playerY; // continentCoords
	float mapCenterX; // continentCoords
	float mapCenterY; // continentCoords
	float mapScale;
	unsigned processId;
	unsigned char mountIndex;
	wchar_t description[2048];
} LinkedMem;

LinkedMem* mumble_link_create();
void mumble_link_destroy();