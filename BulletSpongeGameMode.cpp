// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "BulletSpongeGameMode.h"
#include "BulletSpongeHUD.h"
#include "BulletSpongeCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "MyPlayerController.h"

ABulletSpongeGameMode::ABulletSpongeGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = ABulletSpongeHUD::StaticClass();

	PlayerControllerClass = AMyPlayerController::StaticClass();
}
