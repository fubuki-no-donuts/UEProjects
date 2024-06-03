// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class DEMO_TPS_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();

protected:
	virtual void BeginPlay() override;
	
	// ����Ͷ�������ײ
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/*Ͷ�����˺�*/
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	/*������ײλ�õ���Ч*/
	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;
	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;

		/* ������� */
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;



private:

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	/*Ͷ������Ч*/
	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer; // ��������Ч
	class UParticleSystemComponent* TracerComponent; // ����ϵͳ����������ɺ͹��������Ч

};
