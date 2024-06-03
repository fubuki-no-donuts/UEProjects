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

		// 初始化默认FOV和当前FOV
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

		// 更新HUD上的准星
		SetHUDCrosshairs(DeltaTime);
		// 更新FOV
		InterpFOV(DeltaTime);
	}
}

// 注册Replicated变量
void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly); // 只需要所有者的机器同步
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

// 设置准星
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

			// 准星的动态变化
			// 角色速度的影响
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());
			// 角色跳跃的影响
			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}
			// 瞄准的影响
			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else 
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}
			// 射击的影响
			CrosshairShootFactor = FMath::FInterpTo(CrosshairShootFactor, 0.f, DeltaTime, 40.f);

			// 0.5f for Crosshair Base Spread
			HUDPackage.CrosshairSpread = 0.5f 
				+ CrosshairVelocityFactor 
				+ CrosshairInAirFactor 
				- CrosshairAimFactor 
				+ CrosshairShootFactor;

			// 设置准星Texture
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	// 获取当前状态下的FOV
	if (bAiming)
	{// 瞄准时将FOV插值到目标FOV
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{// 不瞄准要插值回默认的FOV
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	// 更新相机视角的FOV
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return; // check valid

	if (EquippedWeapon) EquippedWeapon->Dropped(); // 丢掉手里的武器

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped); // 手动设置 WeaponState 为 Replicated

	// Attach weapon to a hand socket on the skeleton mesh
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh()); // 自动传播到 client 上执行（但无法保证client上先变更Weapon状态还是先执行的这里）
	}

	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDAmmo();

	// 更新HUD上最大携带弹药量
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	// 播放音效
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

	// 携带武器后的另一套视角控制
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading) // 防止client向server发送大量无效的reload指令
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
		// 保证reload结束后才完成弹药装填
		UpdateAmmoValues(); // 保证在server上修改ammo，让ammo自动同步
	}

	// 装弹完毕后如果FireButton一直pressed，则直接开火
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
	// 计算弹匣剩余位置
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
	// 确保角色有相关类型的弹药
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()]; // 角色携带数量
		int32 Least = FMath::Min(RoomInMag, AmountCarried); // 最低能满足的弹药数量
		return FMath::Clamp(RoomInMag, 0, Least); // 返回装填的弹药数量
	}

	return 0;
}

void UCombatComponent::UpdateAmmoValues()
{
	if (EquippedWeapon == nullptr) return;
	// 计算要装填的Ammo数量
	int32 ReloadAmount = AmountToReload();
	// 更新角色身上的数量
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	// 更新HUD
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
{// 更新client上的HUD
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

// server 上装备武器后要告诉 client 执行的操作
void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		// 为了保证先设置武器状态（关闭了相关碰撞），再执行AttachActor，这里单独执行一次
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;

		// 播放音效
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

// 本地鼠标右键调用，所以需要 client 往 server 发送 RPC
void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming); // not need to check whether is on the server, check the document, in our situation, this will only run on the server
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}
// 给server发的RPC
void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}


// 开火操作：客户端输入事件
void UCombatComponent::FireButtonPressed(bool bPressed)
{// 只会在按下Button的时候执行，长按期间不会执行
	bFireButtonPressed = bPressed;
	if (bFireButtonPressed)
	{
		Fire();
	}
}

// 开火逻辑
void UCombatComponent::Fire()
{
	if(CanFire())
	{
		EquippedWeapon->bCanFire = false;
		ServerFire(HitTarget); // 客户端通知服务器开火
		StartFireTimer(); // 开火后开始计时
	}
}

// 开火计时器
void UCombatComponent::StartFireTimer()
{
	Character->GetWorldTimerManager().SetTimer(AutoFireTimer, this, &UCombatComponent::ResetFireTimer, EquippedWeapon->FireDelay);
}
// 重置开火计时
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

	// 判断子弹是否为空，非装填状态下可以开火，防止快速点击左键触发垃圾指令
	return !EquippedWeapon->IsEmpty() && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon->bCanFire;
}


// client 通知 server 开火
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{// 服务器执行
	MulticastFire(TraceHitTarget); // 服务器下发指令（包括自己）
}
// server 转发到其他 clients 一同执行（包括自己）
void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{// 服务器+全部客户端
	if (EquippedWeapon == nullptr) return;
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming); // 播放角色开火动画
		EquippedWeapon->Fire(TraceHitTarget); // 播放武器开火动画
		if (Character->IsLocallyControlled())
		{
			CrosshairShootFactor = EquippedWeapon->CrosshairShootFactor;
		}
	}
}


void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{// 思路：从屏幕中心投射射线
	// 先拿到视口大小
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	// 准星设置在视口中心
	FVector2D CrosshairLocation(ViewportSize.X/2.f, ViewportSize.Y/2.f);
	// 获取屏幕位置在世界中的位置和方向
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);
	// 转换成功后，开始追踪
	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition; // 追踪开始位置

		// 修正准星能瞄准角色位置之后的物体的问题
		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH; // 追踪结束位置
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);
		// 如果没有碰到物体
		if (!TraceHitResult.bBlockingHit)
		{// ImpactPoint是 line trace 碰撞点的位置
			TraceHitResult.ImpactPoint = End; // 没有碰到物体就设置为End位置
		}

		// 碰撞物体，准星变红
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{// 碰撞物体 并且 物体有实现对应的接口
			HUDPackage.CrosshairColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairColor = FLinearColor::White;
		}
	}
}

