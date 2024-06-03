// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"


namespace MatchState
{
	extern DEMO_TPS_API const FName Cooldown; // match duration has been reached, display winner and begin cooldown timer
}


/**
 * 
 */
UCLASS()
class DEMO_TPS_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABlasterGameMode();

	virtual void Tick(float DeltaTime) override;

	virtual void PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController, class ABlasterPlayerController* AttackerController);
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController);

	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

public:
	// Default Warmup Time
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	// Default Match Time
	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	// Default Cooldown Time
	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;

	// Record the time when this level is start
	float LevelStartingTime = 0.f; // Time from launching the game to actually entering the level

private:
	// Countdownt time to check if warmup state can be ended
	float CountdownTime = 0.f;
};
