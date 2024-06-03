// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class USoundCue;
class UAudioComponent;
class USoundAttenuation;

/**
 * 
 */
UCLASS()
class DEMO_TPS_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()
public:
	AProjectileRocket();
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	// ����Ͷ�������ײ
	//UFUNCTION() // override ����Ҫ��UFUNCTION��Ϊ��̳и����
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	void DestroyTimerFinished();
	
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* TrailSystem;
	UPROPERTY()
	UNiagaraComponent* TrailSystemComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop;
	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation;
	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

private:

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;

	FTimerHandle DestroyTimer;
	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;

};
