// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "BulletSpongeGameMode.h"
#include "BulletSpongeHUD.h"
#include "BulletSpongeCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "MyPlayerController.h"
#include "MyPlayerState.h"
#include "MyPlayerStart.h"
#include "EngineUtils.h"

ABulletSpongeGameMode::ABulletSpongeGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = ABulletSpongeHUD::StaticClass();

	PlayerControllerClass = AMyPlayerController::StaticClass();

	PlayerStateClass = AMyPlayerState::StaticClass();
}

void ABulletSpongeGameMode::PostLogin(APlayerController * NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (NewPlayer)
	{
		AMyPlayerState * PS = Cast<AMyPlayerState>(NewPlayer->PlayerState);
		if (PS && GameState)
		{
			uint8 NumTeamA = 0;
			uint8 NumTeamB = 0;

			for (APlayerState * It : GameState->PlayerArray)
			{
				AMyPlayerState * OtherPS = Cast<AMyPlayerState>(It);
				if (OtherPS)
				{
					if (OtherPS->bTeamB)
					{
						NumTeamB++;
					}
					else
					{
						NumTeamA++;
					}
				}
			}

			if (NumTeamA > NumTeamB)
			{
				PS->bTeamB = true;
			}
		}
	}
}

AActor * ABulletSpongeGameMode::ChoosePlayerStart_Implementation(AController * Player)
{
	if (Player)
	{
		AMyPlayerState * PS = Cast<AMyPlayerState>(Player->PlayerState);
		if (PS)
		{
			TArray<AMyPlayerStart *> Starts;
			for (TActorIterator<AMyPlayerStart> StartItr(GetWorld()); StartItr; ++StartItr)
			{
				if (StartItr->bTeamB == PS->bTeamB)
				{
					Starts.Add(*StartItr);
				}
			}

			return Starts[FMath::RandRange(0, Starts.Num() - 1)];
		}
	}

	return NULL;
}