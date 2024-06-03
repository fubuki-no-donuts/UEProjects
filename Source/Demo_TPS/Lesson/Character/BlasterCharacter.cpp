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

	// ���ý�ɫMesh�Կɼ�ͨ���赲
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	// ����������Ϊָ������ײ���ͣ��������ӵ�����ײ��
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);

	// ��ʼ��ת��ö��
	TurningInPlace = ETuringInPlace::ETIP_NotTurning;

	// �������Ƶ��
	NetUpdateFrequency = 100.f;
	MinNetUpdateFrequency = 33.f;

	// ��ɫ��������
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// ����ʱ�������
	DissolveTimelineComponent = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));


}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHUDHealth();

	// ��server�ϰ��˺��ص�
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
	{// ����ģ���ɫ�Ż����΢����ɫת����
		AimOffset(DeltaTime);
	}
	else
	{// ģ������ɫ�ͷ������˵ķǱ��ؿ��ƽ�ɫ��ר��ת���߼�
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > .25f) // ��һ���Ĺ̶����ڵ���
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch(); // ���Ҫÿ֡����
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
/*                     ע��Replicated����                                */
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

// ����������ؽ�ɫ���������
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

	// ��ʱ������ElimBot
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

/* �ƶ����� */
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

/* ʰȡ���� */
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

/* ������� */
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

/* װ�ҩ */
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
	if (!Combat || !Combat->EquippedWeapon) return; // δװ����������Ҫ������׼ƫ��

	// Get Character's speed
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) // ��ɫ��ֹվ��
	{// ת���ӽ�ϣ����ɫ�ϰ�����һ���ӽǷ�Χ�ڷ�������ת��
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(StartingAimRotation, CurrentAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETuringInPlace::ETIP_NotTurning)
		{// û�ﵽת��ĽǶ�ǰ��Ҫ������ת�������ʼ�Ƕ�
			InterpAO_Yaw = AO_Yaw;
		}
		// ����ת������Ƕȣ���ʱҪ�����������������תYaw
		bUseControllerRotationYaw = true; // 

		// ����ԭ��ת����
		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f || bIsInAir)
	{// ��������˶�����У��򲻽�������������
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); // ������Щ�Ƿ�����ϵ���ʼλ��
		bUseControllerRotationYaw = true; // ��ʱ��Ҫ��������������
		TurningInPlace = ETuringInPlace::ETIP_NotTurning; // ����ر�ת����
		AO_Yaw = 0.f;
	}

	CalculateAO_Pitch();

}


void ABlasterCharacter::SimProxiesTurn()
{
	if (!Combat || !Combat->EquippedWeapon) return;

	bRotateRootBone = false;
	
	// ͬ�����Ǿ�ֹ״̬�Ͳ���Ҫ��������ת��΢��
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
	if (FMath::Abs(ProxyYaw) > TurnThreshold) // ������ֵ��ת��
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
	// û�г�����ֵ�Ͳ�ת��
	TurningInPlace = ETuringInPlace::ETIP_NotTurning;

}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled()) // �ֶ���������Pitch���͵�server����ʾ����ȷ������
	{// ��AO_Pitch��[270, 360)ӳ�䵽[-90, 0)
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
	{// ��ʼ��ת�ˣ�Ϊ��ƽ��ת�����ʹ���ʼλ�ò�ֵ�� 0
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw; // ������ת�Ƕ�
		if(FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETuringInPlace::ETIP_NotTurning; // ����ת���Ƕ�С��ĳ��ֵ��ֹͣת��������״̬
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); // ע��ҲҪ����AimRotation����ʼ��ת
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
		AnimInstance->Montage_Play(FireWeaponMontage); // ���ſ�����̫��
		// ѡ����ȷ�Ķ���Ƭ��
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
		AnimInstance->Montage_Play(ReloadMontage); // ���ſ�����̫��
		// ѡ����ȷ�Ķ���Ƭ��
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
		AnimInstance->Montage_Play(HitReactMontage); // ������̫��
		// ѡ����ȷ�Ķ���Ƭ��
		FName SectionName("FromFront"); // ��ʱ�Ȳ���һ�����������ѡ�����ĸ�
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance || ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage); // ������̫��
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
	// ����HUDѪ��
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}


void ABlasterCharacter::ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCasuer)
{// ֻ��Server�ϴ����ú���

	Health = FMath::Clamp(Health-Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage(); // �����ܻ��������ǵ���Rep��ͬ��client��

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
{// ��clientͬ��
	UpdateHUDHealth();
	// �����ܻ�����
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
	// ������������HUD��ʾ
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}

	bElimmed = true;
	PlayElimMontage();

	// �ڶಥRPC��ʵ�ֲ����ܽ�
	if (DissolveMaterialInstance)
	{// ������̬���ʵĲ���
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		// ���ò����ĳ�ʼֵ
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve(); // ���������ܽ�����

	// �ر���̭��ɫ��һЩ����
	GetCharacterMovement()->DisableMovement(); // ��ֹ�ƶ�
	GetCharacterMovement()->StopMovementImmediately(); // ��ֹ���ת��

	// ��ֹ����
	bDisableGameplay = true; 
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}

	//if (BlasterPlayerController)
	//{
	//	DisableInput(BlasterPlayerController); // ��ֹ����
	//} 

	// �ر���ײ
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// ��̭������
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
	// ��ʱ������
	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterGameMode)
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}
}

void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial); // ��ί��
	if (DissolveCurve && DissolveTimelineComponent)
	{
		DissolveTimelineComponent->AddInterpFloat(DissolveCurve, DissolveTrack); // Track�������ϱ仯�������ص�
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

