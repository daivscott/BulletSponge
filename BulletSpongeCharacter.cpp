// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "BulletSpongeCharacter.h"
#include "BulletSpongeProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealNetwork.h"
#include "BulletSponge.h"
#include "MyPlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// ABulletSpongeCharacter

ABulletSpongeCharacter::ABulletSpongeCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->RelativeLocation = FVector(-39.56f, 1.75f, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(false);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->RelativeRotation = FRotator(1.9f, -19.19f, 5.2f);
	Mesh1P->RelativeLocation = FVector(-0.5f, -4.4f, -155.7f);

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(false);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	Health = 100.f;

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	
}

void ABulletSpongeCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	
}

void ABulletSpongeCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABulletSpongeCharacter, Task);
	DOREPLIFETIME(ABulletSpongeCharacter, Health);

}

//////////////////////////////////////////////////////////////////////////
// Input

void ABulletSpongeCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	InputComponent->BindAction("fire", IE_Pressed, this, &ABulletSpongeCharacter::StartFiring);
	InputComponent->BindAction("fire", IE_Released, this, &ABulletSpongeCharacter::StopFiring);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ABulletSpongeCharacter::OnFire);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABulletSpongeCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABulletSpongeCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ABulletSpongeCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ABulletSpongeCharacter::LookUpAtRate);

}

void ABulletSpongeCharacter::StartFiring()
{
	PerformTask(ETaskEnum::Fire);
}

void ABulletSpongeCharacter::StopFiring()
{
	PerformTask(ETaskEnum::None);
}

void ABulletSpongeCharacter::PerformTask(ETaskEnum::Type NewTask)
{
	if (GetNetMode() == NM_Client)
	{
		ServerPerformTask(NewTask);
		return;
	}

	Task = NewTask;
	OnRep_Task();
}

void ABulletSpongeCharacter::ServerPerformTask_Implementation(ETaskEnum::Type NewTask)
{
	PerformTask(NewTask);
}

bool ABulletSpongeCharacter::ServerPerformTask_Validate(ETaskEnum::Type NewTask)
{
	return true;
}

void ABulletSpongeCharacter::OnFire()
{
	// Check if firing projectile
	if (Task != ETaskEnum::Fire) return;

	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			
				const FRotator SpawnRotation = GetViewRotation();
				// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
				const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

				//Set Spawn Collision Handling Override
				FActorSpawnParameters ActorSpawnParams;
				ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

				// spawn the projectile at the muzzle
				World->SpawnActor<ABulletSpongeProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
			
		}
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
	// Set automatic fire
	GetWorldTimerManager().SetTimer(TimerHandle_Task, this, &ABulletSpongeCharacter::OnFire, 0.09f);
}


void ABulletSpongeCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ABulletSpongeCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void ABulletSpongeCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ABulletSpongeCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ABulletSpongeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Setting pawn control rotation manually
	FirstPersonCameraComponent->SetWorldRotation(GetViewRotation());
}

	// get view rotation inheruted from pawn
FRotator ABulletSpongeCharacter::GetViewRotation() const
{
	// if controller exits, , 
	if (Controller)
	{
		//get controller rotation
		return Controller->GetControlRotation();
	}

	//return RemoteViewPitch which is unsigned iteger 8
	return FRotator(RemoteViewPitch / 255.f * 360.f, GetActorRotation().Yaw, 0.f);
}

float ABulletSpongeCharacter::TakeDamage(float DamageAmount, const FDamageEvent & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	Health -= DamageAmount;
	if (Health < 0.f)
	{
		AMyPlayerController * PC = Cast<AMyPlayerController>(Controller);
		if (PC)
		{
			PC->OnKilled();
		}

		Destroy();
	}

	OnRep_Health();
	return DamageAmount;
}

void ABulletSpongeCharacter::OnRep_Task()
{
	switch (Task)
	{
	case (ETaskEnum::None):
		break;
	case (ETaskEnum::Fire):
		OnFire();
		break;
	}
}
void ABulletSpongeCharacter::OnRep_Health()
{
	FirstPersonCameraComponent->PostProcessSettings.SceneFringeIntensity = 5.f - Health * 0.05f;
}

