// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CharacterTypes/TurningInPlace.h"
#include "BlasterAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class DEMO_TPS_API UBlasterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	// TODO:
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

	// TODO:
private:
	// private cant use BlueprintReadOnly unless use meta down here
	UPROPERTY(BlueprintReadOnly, Category=Character, meta=(AllowPrivateAccess="true"))
	class ABlasterCharacter* BlasterCharacter; // who use this animation

	// 用于动画蓝图状态机
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	float Speed;
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bIsInAir;
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bIsAccelerating; // Telling if movement is adding, not physical meaning
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bWeaponEquipped;

	class AWeapon* EquippedWeapon;
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bIsCrouched;
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bIsAiming;

	// 动作左右移动以及视角左右偏转时，角色的偏移
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	float YawOffset;
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	float Lean;
	FRotator CharacterRotationLastFrame;
	FRotator CharacterRotation;
	FRotator DeltaRotation;

	// 用于设置瞄准偏移的上半身动画混合
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	float AO_Yaw;
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	float AO_Pitch;

	// 设置左手IK
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	FTransform LeftHandTransform;

	// 换弹时先取消左手IK
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bUseFABRIK;
	// 装填时不使用AimOffset
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bUseAimOffset;
	// 装填时关闭右手修正
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bTransformRightHand;

	// 角色的原地转动
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	ETuringInPlace TurningInPlace;
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bRotateRootBone;

	// 右手修正武器朝向
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	FRotator RightHandRotation;
	// 动画蓝图中调用 IsLocallyControll() 不是线程安全的，所以在这里设置 bool
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bIsLocallyControlled;

	// 角色被淘汰
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bElimmed;

};
