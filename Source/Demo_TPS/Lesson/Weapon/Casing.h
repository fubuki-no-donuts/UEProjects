// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

UCLASS()
class DEMO_TPS_API ACasing : public AActor
{
	GENERATED_BODY()
	
public:	
	ACasing();

protected:
	virtual void BeginPlay() override;

	// 弹壳碰到地面就销毁
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* CasingMesh;

	// 设置弹壳抛出速度
	UPROPERTY(EditAnywhere)
	float ShellEjectionImpulse;

	// 弹壳落地音效
	UPROPERTY(EditAnywhere)
	class USoundCue* ShellSound;
};
