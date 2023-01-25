#ifndef MUMBLE_H
#define MUMBLE_H

struct Vector2
{
	float X;
	float Y;
};

struct Vector3
{
	float X;
	float Y;
	float Z;
};

enum class EMapType : unsigned
{
	AutoRedirect,
	CharacterCreation,
	PvP,
	GvG,
	Instance,
	Public,
	Tournament,
	Tutorial,
	UserTournament,
	WvW_EBG,
	WvW_BBL,
	WvW_GBL,
	WvW_RBL,
	WVW_FV,
	WvW_OS,
	WvW_EOTM,
	Public_Mini,
	BIG_BATTLE,
	WvW_Lounge,
	WvW
};

struct Compass
{
	unsigned short	Width;
	unsigned short	Height;
	float			Rotation; // radians
	Vector2			PlayerPosition; // continent
	Vector2			Center; // continent
	float			Scale;
};

enum class EMountIndex : unsigned char
{
	None,
	Jackal,
	Griffon,
	Springer,
	Skimmer,
	Raptor,
	RollerBeetle,
	Warclaw,
	Skyscale,
	Skiff,
	SiegeTurtle
};

struct Context
{
	unsigned char ServerAddress[28]; // contains sockaddr_in or sockaddr_in6
	unsigned MapID;
	EMapType MapType;
	unsigned ShardID;
	unsigned InstanceID;
	unsigned BuildID;

	/* data beyond this point is not necessary for identification */
	unsigned IsMapOpen : 1;
	unsigned IsCompassTopRight : 1;
	unsigned IsCompassRotating : 1;
	unsigned IsGameFocused : 1;
	unsigned IsCompetitive : 1;
	unsigned IsTextboxFocused : 1;
	unsigned IsInCombat : 1;
	// unsigned UNUSED1			: 1;
	Compass Compass;
	unsigned ProcessID;
	EMountIndex MountIndex;
};

struct LinkedMem
{
	unsigned UIVersion;
	unsigned UITick;
	Vector3 AvatarPosition;
	Vector3 AvatarFront;
	Vector3 AvatarTop;
	wchar_t Name[256];
	Vector3 CameraPosition;
	Vector3 CameraFront;
	Vector3 CameraTop;
	wchar_t Identity[256];
	unsigned ContextLength;
	Context Context;
	wchar_t Description[2048];
};

#endif