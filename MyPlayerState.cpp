// Fill out your copyright notice in the Description page of Project Settings.

#include "MyPlayerState.h"
#include "BulletSponge.h"
#include "UnrealNetwork.h"

void AMyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMyPlayerState, bTeamB);
}