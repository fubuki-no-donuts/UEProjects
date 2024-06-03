// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "../Weapon/Weapon.h"
#include "../Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetWork.h"
#include "GameFramework//CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "../Controller/BlasterPlayerController.h"
#include "Camera/CameraComponent.h"
#include "Sound/SoundCue.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// ...

}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		// ��ʼ��Ĭ��FOV�͵�ǰFOV
		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}

		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}


	

}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// 
	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		// ����HUD�ϵ�׼��
		SetHUDCrosshairs(DeltaTime);
		// ����FOV
		InterpFOV(DeltaTime);
	}
}

// ע��Replicated����
void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly); // ֻ��Ҫ�����ߵĻ���ͬ��
	DOREPLIFETIME(UCombatComponent, CombatState);
}


void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingSniperAmmo);
}

// ����׼��
void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->GetController() == nullptr) return;

	Controller = (Controller == nullptr) ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
	if(Controller)
	{
		HUD = (HUD == nullptr) ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
			}

			// ׼�ǵĶ�̬�仯
			// ��ɫ�ٶȵ�Ӱ��
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());
			// ��ɫ��Ծ��Ӱ��
			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}
			// ��׼��Ӱ��
			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else 
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}
			// �����Ӱ��
			CrosshairShootFactor = FMath::FInterpTo(CrosshairShootFactor, 0.f, DeltaTime, 40.f);

			// 0.5f for Crosshair Base Spread
			HUDPackage.CrosshairSpread = 0.5f 
				+ CrosshairVelocityFactor 
				+ CrosshairInAirFactor 
				- CrosshairAimFactor 
				+ CrosshairShootFactor;

			// ����׼��Texture
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	// ��ȡ��ǰ״̬�µ�FOV
	if (bAiming)
	{// ��׼ʱ��FOV��ֵ��Ŀ��FOV
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{// ����׼Ҫ��ֵ��Ĭ�ϵ�FOV
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	// ��������ӽǵ�FOV
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return; // check valid

	if (EquippedWeapon) EquippedWeapon->Dropped(); // �������������

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped); // �ֶ����� WeaponState Ϊ Replicated

	// Attach weapon to a hand socket on the skeleton mesh
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh()); // �Զ������� client ��ִ�У����޷���֤client���ȱ��Weapon״̬������ִ�е����
	}

	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDAmmo();

	// ����HUD�����Я����ҩ��
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	// ������Ч
	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->EquipSound,
			Character->GetActorLocation()
		);
	}

	// Auto reload while picking up and it is empty
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}

	// Я�����������һ���ӽǿ���
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading) // ��ֹclient��server���ʹ�����Ч��reloadָ��
	{
		ServerReload(); // 
	}
}

void UCombatComponent::FinishReload()
{
	if (Character == nullptr) return;
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		// ��֤reload���������ɵ�ҩװ��
		UpdateAmmoValues(); // ��֤��server���޸�ammo����ammo�Զ�ͬ��
	}

	// װ����Ϻ����FireButtonһֱpressed����ֱ�ӿ���
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::ServerReload_Implementation()
{// server
	if (Character == nullptr || EquippedWeapon == nullptr) return;

	CombatState = ECombatState::ECS_Reloading;
	HandleReload();

}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	// ���㵯ϻʣ��λ��
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
	// ȷ����ɫ��������͵ĵ�ҩ
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()]; // ��ɫЯ������
		int32 Least = FMath::Min(RoomInMag, AmountCarried); // ���������ĵ�ҩ����
		return FMath::Clamp(RoomInMag, 0, Least); // ����װ��ĵ�ҩ����
	}

	return 0;
}

void UCombatComponent::UpdateAmmoValues()
{
	if (EquippedWeapon == nullptr) return;
	// ����Ҫװ���Ammo����
	int32 ReloadAmount = AmountToReload();
	// ���½�ɫ���ϵ�����
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	// ����HUD
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(-ReloadAmount);
}



void UCombatComponent::OnRep_CombatState()
{// client
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;
	}
}

void UCombatComponent::OnRep_CarriedAmmo()
{// ����client�ϵ�HUD
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

// server ��װ��������Ҫ���� client ִ�еĲ���
void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		// Ϊ�˱�֤����������״̬���ر��������ײ������ִ��AttachActor�����ﵥ��ִ��һ��
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;

		// ������Ч
		if (EquippedWeapon->EquipSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				EquippedWeapon->EquipSound,
				Character->GetActorLocation()
			);
		}
	}
}

// ��������Ҽ����ã�������Ҫ client �� server ���� RPC
void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming); // not need to check whether is on the server, check the document, in our situation, this will only run on the server
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}
// ��server����RPC
void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}


// ����������ͻ��������¼�
void UCombatComponent::FireButtonPressed(bool bPressed)
{// ֻ���ڰ���Button��ʱ��ִ�У������ڼ䲻��ִ��
	bFireButtonPressed = bPressed;
	if (bFireButtonPressed)
	{
		Fire();
	}
}

// �����߼�
void UCombatComponent::Fire()
{
	if(CanFire())
	{
		EquippedWeapon->bCanFire = false;
		ServerFire(HitTarget); // �ͻ���֪ͨ����������
		StartFireTimer(); // �����ʼ��ʱ
	}
}

// �����ʱ��
void UCombatComponent::StartFireTimer()
{
	Character->GetWorldTimerManager().SetTimer(AutoFireTimer, this, &UCombatComponent::ResetFireTimer, EquippedWeapon->FireDelay);
}
// ���ÿ����ʱ
void UCombatComponent::ResetFireTimer()
{
	EquippedWeapon->bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}

	// Auto Reload when weapon empty
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}



bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;

	// �ж��ӵ��Ƿ�Ϊ�գ���װ��״̬�¿��Կ��𣬷�ֹ���ٵ�������������ָ��
	return !EquippedWeapon->IsEmpty() && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon->bCanFire;
}


// client ֪ͨ server ����
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{// ������ִ��
	MulticastFire(TraceHitTarget); // �������·�ָ������Լ���
}
// server ת�������� clients һִͬ�У������Լ���
void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{// ������+ȫ���ͻ���
	if (EquippedWeapon == nullptr) return;
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming); // ���Ž�ɫ���𶯻�
		EquippedWeapon->Fire(TraceHitTarget); // �����������𶯻�
		if (Character->IsLocallyControlled())
		{
			CrosshairShootFactor = EquippedWeapon->CrosshairShootFactor;
		}
	}
}


void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{// ˼·������Ļ����Ͷ������
	// ���õ��ӿڴ�С
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	// ׼���������ӿ�����
	FVector2D CrosshairLocation(ViewportSize.X/2.f, ViewportSize.Y/2.f);
	// ��ȡ��Ļλ���������е�λ�úͷ���
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);
	// ת���ɹ��󣬿�ʼ׷��
	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition; // ׷�ٿ�ʼλ��

		// ����׼������׼��ɫλ��֮������������
		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH; // ׷�ٽ���λ��
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);
		// ���û����������
		if (!TraceHitResult.bBlockingHit)
		{// ImpactPoint�� line trace ��ײ���λ��
			TraceHitResult.ImpactPoint = End; // û���������������ΪEndλ��
		}

		// ��ײ���壬׼�Ǳ��
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{// ��ײ���� ���� ������ʵ�ֶ�Ӧ�Ľӿ�
			HUDPackage.CrosshairColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairColor = FLinearColor::White;
		}
	}
}

