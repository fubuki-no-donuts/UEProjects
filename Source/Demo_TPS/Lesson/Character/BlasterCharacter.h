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

	// ���Ŷ���
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayHitReactMontage();
	void PlayElimMontage();

	FVector GetHitTarget() const;

	//
	void Elim(); // ֻ��server��ִ�е�
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(); // ����RPC��

	//
	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	// ����
	void FireButtonPressed();

protected:
	virtual void BeginPlay() override;

	void UpdateHUDHealth();
	// Poll for any relevant classes and initialize our HUD
	void PollInit();


	// �ж�����
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	virtual void Jump() override;

	// ���ƿ���
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();

	// AimOffset
	void AimOffset(float DeltaTime);
	float CalculateSpeed();
	void CalculateAO_Pitch();

	//Ϊ�����ɫר��ʵ��ת���߼�
	void SimProxiesTurn();

	// Equip Control
	void EquipButtonPressed();
	// ����

	void FireButtonReleased();
	// ����
	void ReloadButtonPressed();



	// �˺��ص�
	UFUNCTION()
	void ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCasuer);

private:
	// Setup Character's Camera
	UPROPERTY(VisibleAnywhere, Category=Camera)
	class USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, Category=Camera)
	class UCameraComponent* FollowCamera;
	// ���ؽ�ɫ
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
	*	��ɫ����
	*/
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;
	UFUNCTION()
	void OnRep_Health();

	/**
	 * ��̭��ɫ
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
	 * �ܽ�Ч��
	 */
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance; // ��ͼ�������������
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance; // ������ʱ�����ϱߵĲ�������̬���������̬����ʵ��
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimelineComponent;
	UPROPERTY(EditAnywhere, Category = Elim)
	UCurveFloat* DissolveCurve; // ��Ҫָ������� Curve
	FOnTimelineFloat DissolveTrack; // Curve �ϵĵ㣬������һ����̬ί��
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue); // �󶨵� Track �϶�̬���²���
	void StartDissolve();


	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;

	/**
	* ��ɫ��������
	*/
	float AO_Yaw; // ���ڼ�����׼ƫ��ʱ�Ķ������
	float InterpAO_Yaw; // ���ڲ�ֵ��ת root bone
	float AO_Pitch;
	FRotator StartingAimRotation;
	bool bRotateRootBone; // ģ�������Ҫ rotate root bone
	float TurnThreshold = .5f; // �ڴ����ɫ�ϲ���ת����
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

	// ��ɫ����׼ƫ��ʱ�Զ����е�ת��
	ETuringInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	/**
	 * ��ɫ������̫��
	 */

	// ���𶯻�
	UPROPERTY(EditAnywhere, Category = Anim)
	class UAnimMontage* FireWeaponMontage;

	// ��������
	UPROPERTY(EditAnywhere, Category = Anim)
	class UAnimMontage* ReloadMontage;

	// �ܻ�����
	UPROPERTY(EditAnywhere, Category = Anim)
	UAnimMontage* HitReactMontage;

	// ��̭����
	UPROPERTY(EditAnywhere, Category = Elim)
	UAnimMontage* ElimMontage; 


private:
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed(); // RPC

public:
	FORCEINLINE UCombatComponent* GetCombatComponent() const { return Combat; }
};
