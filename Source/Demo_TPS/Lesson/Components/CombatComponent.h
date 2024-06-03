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

	// 接收开火按键是否按下
	void FireButtonPressed(bool bPressed);

protected:
	UFUNCTION()
	void OnRep_EquippedWeapon();

	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);


	void Fire();
	void StartFireTimer(); // 开火计时器
	void ResetFireTimer(); // 重置计时器（重新调用Fire，实现连续开火）

	// 同步 server reload 行为
	UFUNCTION(Server, Reliable)
	void ServerReload();
	void HandleReload();
	int32 AmountToReload();


	// 判断是否可以开火
	bool CanFire();

	// 武器种类和可携带弹药
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo; // 当前武器的弹药量
	TMap<EWeaponType, int32> CarriedAmmoMap; // 身上拥有的弹药种类和各自的数量
	UPROPERTY()
	int32 StartingARAmmo = 30; // 开局AR默认弹药量
	UPROPERTY()
	int32 StartingRocketAmmo = 5; // 开局火箭筒默认弹药量
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

	// 通知服务器开火
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);
	// 服务器给客户端同步操作
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	// 射线检测（开火命中）
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	// 设置HUD准星（准星会跟随角色动态变化，比如描述后坐力时的准星方法，涉及到插值计算，所以这里要传入DeltaTime）
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
	* 自动开火计时器
	*/
	// Fire Timer while holding fire button
	FTimerHandle AutoFireTimer;


/**
 * HUD and Crosshairs
 */
	FHUDPackage HUDPackage;
	float CrosshairVelocityFactor; // 角色移动扩散
	float CrosshairInAirFactor; // 角色跳跃扩散
	float CrosshairAimFactor; // 角色瞄准缩放
	float CrosshairShootFactor; // 角色射击扩散

/**
 * Use for Correct Weapon Rotation
 */
	FVector HitTarget;

/**
* 瞄准和FOV
*/

	// 默认的，非瞄准时的FOV
	float DefaultFOV;
	float CurrentFOV;
	// 
	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.f;
	// 切换FOV时的速度
	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);

	void UpdateAmmoValues();

};
