// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BulletSpongeGameMode.generated.h"

UCLASS(minimalapi)
class ABulletSpongeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABulletSpongeGameMode();

	void PostLogin(APlayerController * NewPlayer) override;

	AActor * ChoosePlayerStart_Implementation(AController * Player) override;

	bool ShouldSpawnAtStartSpot(AController * Player) override { return false; };
};