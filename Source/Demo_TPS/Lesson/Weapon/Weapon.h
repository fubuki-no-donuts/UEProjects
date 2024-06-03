// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Weapon.generated.h"

class USphereComponent;
class UWidgetComponent;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"), // on the ground
	EWS_Equipped UMETA(DisplayName = "Equipped"), // equip by player
	EWS_Dropped UMETA(DisplayName = "Dropped"), // drop, when open collision

	EWS_MAX UMETA(DisplayName = "DefaultMAX"), // Default Max constant, for check how many constants here
};

UCLASS()
class DEMO_TPS_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();

	virtual void Tick(float DeltaTime) override;
	// Register variables to be replicated
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRep_Owner() override;

	void ShowPickupWidget(bool bShowWidget);
	void Dropped(); // ����������

	void AddAmmo(int32 AmmoToAdd);

public:
/**
 * Getter And Setter
 */

	void SetWeaponState(EWeaponState State);

	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	virtual void Fire(const FVector& HitTarget);
	void SetHUDAmmo();

	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }

	bool IsEmpty();

protected:
	virtual void BeginPlay() override;

	// ����ʰȡ���
	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
		);
	UFUNCTION()
	virtual void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

private:
	UFUNCTION()
	void OnRep_WeaponState();

public:
/*
	Textures for the weapon Crosshairs
*/
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsCenter;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsLeft;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsRight;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsTop;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsBottom;
	// ǹе������ɢ
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float CrosshairShootFactor = .75f;

	/**
	 * �Զ�����
	 */
	UPROPERTY(EditAnywhere, Category = "Weapon Ammo")
	bool bAutomatic = false; // �Ƿ����Զ�����
	// Auto fire rate while timer on
	UPROPERTY(EditAnywhere)
	float FireDelay = 0.1;
	bool bCanFire = true; // ���´�FireTimer����֮ǰ������������

	/**
	 * ��Ч
	 */
	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

private:	
	UPROPERTY(VisibleAnywhere, Category="Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;
	UPROPERTY(VisibleAnywhere, Category="Weapon Properties")
	USphereComponent* AreaSphere;
	UPROPERTY()
	class ABlasterCharacter* BlasterOwnerCharacter;
	UPROPERTY()
	class ABlasterPlayerController* BlasterOwnerController;

	// ������ҩ��
	UPROPERTY(EditAnywhere, Category = "Weapon Ammo", ReplicatedUsing = OnRep_Ammo)
	int32 Ammo;
	UPROPERTY(EditAnywhere, Category = "Weapon Ammo")
	int32 MagCapacity;

	// ��������
	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

	UFUNCTION()
	void OnRep_Ammo();
	void SpendRound();

	UPROPERTY(EditAnywhere, Category = "Weapon Ammo")
	TSubclassOf<class ACasing> CasingClass; // �����׳���������

	// ����״̬��װ����δװ��
	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon State")
	EWeaponState WeaponState;

	// ������ʰȡ��ʾ�ؼ�
	UPROPERTY(VisibleAnywhere, Category="Weapon Widget")
	UWidgetComponent* PickupWidget;

	// �������𶯻�
	UPROPERTY(EditAnywhere, Category = "Weapon Anim")
	class UAnimationAsset* FireAnimation;

	// ��׼ʱ�� FOV ����
	UPROPERTY(EditAnywhere, Category = "Weapon FOV")
	float ZoomedFOV = 45.f;
	// ��һ���ٶ����ŵ�Ŀ��FOV
	UPROPERTY(EditAnywhere, Category = "Weapon FOV")
	float ZoomInterpSpeed = 30.f;

};
