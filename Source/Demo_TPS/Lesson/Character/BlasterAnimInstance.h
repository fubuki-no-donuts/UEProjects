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

	// ���ڶ�����ͼ״̬��
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

	// ���������ƶ��Լ��ӽ�����ƫתʱ����ɫ��ƫ��
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	float YawOffset;
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	float Lean;
	FRotator CharacterRotationLastFrame;
	FRotator CharacterRotation;
	FRotator DeltaRotation;

	// ����������׼ƫ�Ƶ��ϰ��������
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	float AO_Yaw;
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	float AO_Pitch;

	// ��������IK
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	FTransform LeftHandTransform;

	// ����ʱ��ȡ������IK
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bUseFABRIK;
	// װ��ʱ��ʹ��AimOffset
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bUseAimOffset;
	// װ��ʱ�ر���������
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bTransformRightHand;

	// ��ɫ��ԭ��ת��
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	ETuringInPlace TurningInPlace;
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bRotateRootBone;

	// ����������������
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	FRotator RightHandRotation;
	// ������ͼ�е��� IsLocallyControll() �����̰߳�ȫ�ģ��������������� bool
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bIsLocallyControlled;

	// ��ɫ����̭
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta= (AllowPrivateAccess = "true"))
	bool bElimmed;

};
