// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../HUD/BlasterHUD.h"
#include "../Weapon/WeaponTypes.h"
#include "../Character/CharacterTypes/CombatState.h"
#include "CombatComponent.generated.h"



class ABlasterCharacter;
class AWeapon;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEMO_TPS_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	friend class ABlasterCharacter; // make a friend class

public:
	void EquipWeapon(AWeapon* WeaponToEquip);
	void Reload();
	UFUNCTION(BlueprintCallable)
	void FinishReload();

	// ���տ��𰴼��Ƿ���
	void FireButtonPressed(bool bPressed);

protected:
	UFUNCTION()
	void OnRep_EquippedWeapon();

	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);


	void Fire();
	void StartFireTimer(); // �����ʱ��
	void ResetFireTimer(); // ���ü�ʱ�������µ���Fire��ʵ����������

	// ͬ�� server reload ��Ϊ
	UFUNCTION(Server, Reliable)
	void ServerReload();
	void HandleReload();
	int32 AmountToReload();


	// �ж��Ƿ���Կ���
	bool CanFire();

	// ��������Ϳ�Я����ҩ
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo; // ��ǰ�����ĵ�ҩ��
	TMap<EWeaponType, int32> CarriedAmmoMap; // ����ӵ�еĵ�ҩ����͸��Ե�����
	UPROPERTY()
	int32 StartingARAmmo = 30; // ����ARĬ�ϵ�ҩ��
	UPROPERTY()
	int32 StartingRocketAmmo = 5; // ���ֻ��ͲĬ�ϵ�ҩ��
	UPROPERTY()
	int32 StartingPistolAmmo = 15;
	UPROPERTY()
	int32 StartingShotgunAmmo = 25;
	UPROPERTY()
	int32 StartingSniperAmmo = 25;
	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;
	UFUNCTION()
	void OnRep_CombatState();

	UFUNCTION()
	void OnRep_CarriedAmmo();

	// ֪ͨ����������
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);
	// ���������ͻ���ͬ������
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	// ���߼�⣨�������У�
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	// ����HUD׼�ǣ�׼�ǻ�����ɫ��̬�仯����������������ʱ��׼�Ƿ������漰����ֵ���㣬��������Ҫ����DeltaTime��
	void SetHUDCrosshairs(float DeltaTime);

private:
	ABlasterCharacter* Character;
	class ABlasterPlayerController* Controller;
	class ABlasterHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;
	bool bFireButtonPressed;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed = 600.f;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed = 450.f;

/**
	* �Զ������ʱ��
	*/
	// Fire Timer while holding fire button
	FTimerHandle AutoFireTimer;


/**
 * HUD and Crosshairs
 */
	FHUDPackage HUDPackage;
	float CrosshairVelocityFactor; // ��ɫ�ƶ���ɢ
	float CrosshairInAirFactor; // ��ɫ��Ծ��ɢ
	float CrosshairAimFactor; // ��ɫ��׼����
	float CrosshairShootFactor; // ��ɫ�����ɢ

/**
 * Use for Correct Weapon Rotation
 */
	FVector HitTarget;

/**
* ��׼��FOV
*/

	// Ĭ�ϵģ�����׼ʱ��FOV
	float DefaultFOV;
	float CurrentFOV;
	// 
	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.f;
	// �л�FOVʱ���ٶ�
	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);

	void UpdateAmmoValues();

};
