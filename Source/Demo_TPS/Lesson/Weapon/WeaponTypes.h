#pragma once

#define TRACE_LENGTH 80000

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DissplayName = "Assualt Rifle"),
	EWT_RocketLauncher UMETA(DissplayName = "Rocket Launcher"),
	EWT_Pistol UMETA(DissplayName = "Pistol"),
	EWT_Shotgun UMETA(DissplayName = "Shotgun"),
	EWT_SniperRifle UMETA(DissplayName = "Sniper Rifle"),

	EWT_Max UMETA(DissplayName = "DefaultMax")
};

