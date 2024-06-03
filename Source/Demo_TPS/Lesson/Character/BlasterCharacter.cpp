// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetWork.h"
#include "../Weapon/Weapon.h"
#include "../Components/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BlasterAnimInstance.h"
#include "Demo_TPS/Demo_TPS.h"
#include "../Controller/BlasterPlayerController.h"
#include "../GameMode/BlasterGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "BlasterPlayerState.h"
#include "../Weapon/WeaponTypes.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Construct Camera
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // Because CameraBoom has set it true, and camera is attached to it.

	// BP will overwrite this var, remember to set this in BP
	// not allow character rotating along with controller rotation (when unequipped and unaimed)
	bUseControllerRotationYaw = false; 
	// character rotates depend on its movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	
	// Overhead Widget
	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	// Combat Component
	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	// Turn Crouch ability on
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// Close Camera Collision
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	// 设置角色Mesh对可见通道阻挡
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	// 将网格设置为指定的碰撞类型（方便检测子弹的碰撞）
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);

	// 初始化转身枚举
	TurningInPlace = ETuringInPlace::ETIP_NotTurning;

	// 网络更新频率
	NetUpdateFrequency = 100.f;
	MinNetUpdateFrequency = 33.f;

	// 角色重生设置
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 创建时间线组件
	DissolveTimelineComponent = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));


}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHUDHealth();

	// 在server上绑定伤害回调
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);
	HideCameraIfCharacterClose();
	PollInit();
}


void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETuringInPlace::ETIP_NotTurning;

		return;
	}

	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{// 不是模拟角色才会进行微调角色转向动作
		AimOffset(DeltaTime);
	}
	else
	{// 模拟代理角色和服务器端的非本地控制角色有专门转向逻辑
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > .25f) // 以一定的固定周期调用
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch(); // 这个要每帧调用
	}
}

void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToDefeats(0);
		}
	}
}



/************************************************************************/
/*                     注册Replicated变量                                */
/************************************************************************/
void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ABlasterCharacter, OverlappingWeapon); // Result in every player screen show that widget
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health); 
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

float ABlasterCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Jump() is exist in ACharacter, just need to bind to action
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABlasterCharacter::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABlasterCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ABlasterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ABlasterCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ABlasterCharacter::ReloadButtonPressed);

	// Binding Movement Function To Axis Mapping
	PlayerInputComponent->BindAxis("MoveForward", this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ABlasterCharacter::LookUp);
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

// 距离过近隐藏角色网格和武器
void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;
	if((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}



void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
}


void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	// 到时间销毁ElimBot
	if (ElimBotEffectComponent)
	{
		ElimBotEffectComponent->DestroyComponent();
	}

	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;
	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

/* 移动控制 */
void ABlasterCharacter::MoveForward(float Value)
{
	if (bDisableGameplay) return;
	if (Controller != nullptr && Value != 0.f) // make sure is being controlled and does get a none-zero value
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f); // get rotation on the ground
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value); // cant speed up here, should setup in MovementComponent
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if (bDisableGameplay) return;
	if (Controller != nullptr && Value != 0.f) // make sure is being controlled and does get a none-zero value
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f); // get rotation on the ground
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value); // cant speed up here, should setup in MovementComponent
	}
}

void ABlasterCharacter::Turn(float Value)
{
	AddControllerYawInput(Value); // value for how fast mouse move
}

void ABlasterCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value); // value for how fast mouse move
}

void ABlasterCharacter::Jump()
{
	if (bDisableGameplay) return;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

/* 拾取武器 */
void ABlasterCharacter::EquipButtonPressed()
{ // need server to validate this action, then tell client equip or not
	if (bDisableGameplay) return;
	if (Combat)
	{
		if (HasAuthority()) // must check by the server
		{// because many clients may press button to equip, it might be ambiguity. So let server check who can equip
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed(); // tell server, this client has this input, deal with it so client and server are same
		}
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{// if not server, send confirm from client by RPC
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

/* 开火控制 */
void ABlasterCharacter::FireButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (bDisableGameplay) return;
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->FireButtonPressed(false);
	}
}

/* 装填弹药 */
void ABlasterCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->Reload();
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay) return;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}



void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (!Combat || !Combat->EquippedWeapon) return; // 未装备武器不需要计算瞄准偏移

	// Get Character's speed
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) // 角色静止站立
	{// 转动视角希望角色上半身在一定视角范围内发生左右转动
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(StartingAimRotation, CurrentAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETuringInPlace::ETIP_NotTurning)
		{// 没达到转身的角度前，要更新旋转身体的起始角度
			InterpAO_Yaw = AO_Yaw;
		}
		// 计算转身所需角度，此时要打开这个控制器控制旋转Yaw
		bUseControllerRotationYaw = true; // 

		// 设置原地转身动画
		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f || bIsInAir)
	{// 如果处于运动或空中，则不进行这个动作混合
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); // 所以这些是发生混合的起始位置
		bUseControllerRotationYaw = true; // 这时候要开启控制器控制
		TurningInPlace = ETuringInPlace::ETIP_NotTurning; // 这里关闭转身动画
		AO_Yaw = 0.f;
	}

	CalculateAO_Pitch();

}


void ABlasterCharacter::SimProxiesTurn()
{
	if (!Combat || !Combat->EquippedWeapon) return;

	bRotateRootBone = false;
	
	// 同样，非静止状态就不需要考虑这种转身微调
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	if (Speed > 0.f || bIsInAir)
	{
		TurningInPlace = ETuringInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;
	if (FMath::Abs(ProxyYaw) > TurnThreshold) // 超出阈值则转身
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETuringInPlace::ETIP_Right;
		}
		else if (ProxyYaw < TurnThreshold)
		{
			TurningInPlace = ETuringInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETuringInPlace::ETIP_NotTurning;
		}
		return;
	}
	// 没有超出阈值就不转身
	TurningInPlace = ETuringInPlace::ETIP_NotTurning;

}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled()) // 手动修正负数Pitch发送到server上显示不正确的问题
	{// 将AO_Pitch从[270, 360)映射到[-90, 0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETuringInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETuringInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETuringInPlace::ETIP_NotTurning)
	{// 开始旋转了，为了平滑转动，就从起始位置插值到 0
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw; // 更新旋转角度
		if(FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETuringInPlace::ETIP_NotTurning; // 身体转动角度小于某个值，停止转动并更新状态
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); // 注意也要更新AimRotation的起始旋转
		}
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled()) // not show widget when character from client overlaps weapon
	{// because its not server controlled character
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}


bool ABlasterCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_Max;
	return Combat->CombatState;
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance || FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage); // 播放开火蒙太奇
		// 选择正确的动画片段
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance || ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage); // 播放开火蒙太奇
		// 选择正确的动画片段
		FName SectionName;
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("Rifle");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}

}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance || HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage); // 播放蒙太奇
		// 选择正确的动画片段
		FName SectionName("FromFront"); // 暂时先播放一个，后边再挑选播放哪个
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance || ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage); // 播放蒙太奇
	}
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (Combat)
	{
		return Combat->HitTarget;
	}
	return FVector();
}

void ABlasterCharacter::UpdateHUDHealth()
{
	// 设置HUD血条
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}


void ABlasterCharacter::ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCasuer)
{// 只在Server上触发该函数

	Health = FMath::Clamp(Health-Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage(); // 触发受击动画（记得在Rep中同步client）

	if (Health == 0.f)
	{
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if (BlasterGameMode)
		{
			BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
	}
}
void ABlasterCharacter::OnRep_Health()
{// 往client同步
	UpdateHUDHealth();
	// 触发受击动画
	PlayHitReactMontage();
}



void ABlasterCharacter::Elim()
{

	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}

	MulticastElim();

	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this, 
		&ABlasterCharacter::ResetElimTimer,
		ElimDelay
	);
}



void ABlasterCharacter::MulticastElim_Implementation()
{
	// 被处决后重置HUD显示
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}

	bElimmed = true;
	PlayElimMontage();

	// 在多播RPC上实现材质溶解
	if (DissolveMaterialInstance)
	{// 创建动态材质的步骤
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		// 设置参数的初始值
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve(); // 启动材质溶解曲线

	// 关闭淘汰角色的一些功能
	GetCharacterMovement()->DisableMovement(); // 禁止移动
	GetCharacterMovement()->StopMovementImmediately(); // 禁止相机转向

	// 禁止输入
	bDisableGameplay = true; 
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}

	//if (BlasterPlayerController)
	//{
	//	DisableInput(BlasterPlayerController); // 禁止输入
	//} 

	// 关闭碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 淘汰机器人
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotEffectComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		);
	}
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, ElimBotSound, GetActorLocation());
	}
}

void ABlasterCharacter::ResetElimTimer()
{
	// 到时间重生
	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterGameMode)
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}
}

void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial); // 绑定委托
	if (DissolveCurve && DissolveTimelineComponent)
	{
		DissolveTimelineComponent->AddInterpFloat(DissolveCurve, DissolveTrack); // Track在曲线上变化并触发回调
		DissolveTimelineComponent->Play();
	}
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

