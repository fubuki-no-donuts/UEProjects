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
	
	// 处理投射物的碰撞
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/*投射物伤害*/
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	/*设置碰撞位置的特效*/
	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;
	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;

		/* 组件设置 */
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;



private:

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	/*投射物特效*/
	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer; // 这里是特效
	class UParticleSystemComponent* TracerComponent; // 粒子系统组件负责生成和管理这个特效

};
