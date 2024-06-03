// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CharacterTypes/TurningInPlace.h"
#include "../Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "CharacterTypes/CombatState.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class DEMO_TPS_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;

	void RotateInPlace(float DeltaTime);

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void OnRep_ReplicatedMovement() override;
	// Register variables to be replicated
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override; // Initialize component variable as soon as possible
	virtual void Destroyed() override;

public:

	void SetOverlappingWeapon(AWeapon* Weapon);

	bool IsWeaponEquipped();
	bool IsAiming();

	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon();
	FORCEINLINE ETuringInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	ECombatState GetCombatState() const;

	// 播放动画
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayHitReactMontage();
	void PlayElimMontage();

	FVector GetHitTarget() const;

	//
	void Elim(); // 只在server上执行的
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(); // 用来RPC的

	//
	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	// 开火
	void FireButtonPressed();

protected:
	virtual void BeginPlay() override;

	void UpdateHUDHealth();
	// Poll for any relevant classes and initialize our HUD
	void PollInit();


	// 行动控制
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	virtual void Jump() override;

	// 姿势控制
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();

	// AimOffset
	void AimOffset(float DeltaTime);
	float CalculateSpeed();
	void CalculateAO_Pitch();

	//为代理角色专门实现转身逻辑
	void SimProxiesTurn();

	// Equip Control
	void EquipButtonPressed();
	// 开火

	void FireButtonReleased();
	// 换弹
	void ReloadButtonPressed();



	// 伤害回调
	UFUNCTION()
	void ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCasuer);

private:
	// Setup Character's Camera
	UPROPERTY(VisibleAnywhere, Category=Camera)
	class USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, Category=Camera)
	class UCameraComponent* FollowCamera;
	// 隐藏角色
	UPROPERTY(EditAnywhere, Category = Camera)
	float CameraThreshold = 200.f;
	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	class UCombatComponent* Combat;

	UPROPERTY()
	class ABlasterPlayerState* BlasterPlayerState;

	/**
	*	角色属性
	*/
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;
	UFUNCTION()
	void OnRep_Health();

	/**
	 * 淘汰角色
	 */
	bool bElimmed = false;
	FTimerHandle ElimTimer;
	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;
	void ResetElimTimer();

	/* Elim Bot*/
	UPROPERTY(EditAnywhere, Category = Elim)
	UParticleSystem* ElimBotEffect;
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UParticleSystemComponent* ElimBotEffectComponent;
	UPROPERTY(EditAnywhere, Category = Elim)
	class USoundCue* ElimBotSound;

	/**
	 * 溶解效果
	 */
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance; // 蓝图上设置这个材质
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance; // 在运行时基于上边的材质来动态创建这个动态材质实例
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimelineComponent;
	UPROPERTY(EditAnywhere, Category = Elim)
	UCurveFloat* DissolveCurve; // 需要指定具体的 Curve
	FOnTimelineFloat DissolveTrack; // Curve 上的点，并且是一个动态委托
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue); // 绑定到 Track 上动态更新材质
	void StartDissolve();


	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;

	/**
	* 角色动画设置
	*/
	float AO_Yaw; // 用于计算瞄准偏移时的动画混合
	float InterpAO_Yaw; // 用于插值旋转 root bone
	float AO_Pitch;
	FRotator StartingAimRotation;
	bool bRotateRootBone; // 模拟代理不需要 rotate root bone
	float TurnThreshold = .5f; // 在代理角色上播放转身动画
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

	// 角色在瞄准偏移时自动进行的转向
	ETuringInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	/**
	 * 角色动画蒙太奇
	 */

	// 开火动画
	UPROPERTY(EditAnywhere, Category = Anim)
	class UAnimMontage* FireWeaponMontage;

	// 换弹动画
	UPROPERTY(EditAnywhere, Category = Anim)
	class UAnimMontage* ReloadMontage;

	// 受击动画
	UPROPERTY(EditAnywhere, Category = Anim)
	UAnimMontage* HitReactMontage;

	// 淘汰动画
	UPROPERTY(EditAnywhere, Category = Elim)
	UAnimMontage* ElimMontage; 


private:
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed(); // RPC

public:
	FORCEINLINE UCombatComponent* GetCombatComponent() const { return Combat; }
};
