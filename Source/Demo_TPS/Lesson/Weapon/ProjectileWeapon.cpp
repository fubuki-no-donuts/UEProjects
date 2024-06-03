// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());

	// ��������Ͷ�������������λ�ã�������� MuzzleFlash������һ�� AmmoEject �����׵���
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector ToTarget = HitTarget - SocketTransform.GetLocation(); // ����ʱ��ǹ�ڵ�Ŀ��ķ���
		FRotator TargetRotation = ToTarget.Rotation();
		if (ProjectileClass && InstigatorPawn)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;
			UWorld* World = GetWorld();
			if(World)
			{
				World->SpawnActor<AProjectile>(
					ProjectileClass, // ����������
					SocketTransform.GetLocation(), // ����λ��
					TargetRotation, // ����ʱ�Ľ��ӵ���ת����Ŀ��
					SpawnParams
				);
			}
		}
	}

}
