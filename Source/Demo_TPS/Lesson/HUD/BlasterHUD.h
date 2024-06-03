// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

public:
	UTexture2D* CrosshairsCenter = nullptr;
	UTexture2D* CrosshairsLeft = nullptr;
	UTexture2D* CrosshairsRight = nullptr;
	UTexture2D* CrosshairsTop = nullptr;
	UTexture2D* CrosshairsBottom = nullptr;

	float CrosshairSpread; // ׼�ǵ���ɢ
	FLinearColor CrosshairColor; // ׼�ǵ���ɫ
};

/**
 * 
 */
UCLASS()
class DEMO_TPS_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	// ��CombatComponent�е������HUD����׼��
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }

	void AddCharacterOverlay(); // ��ӽ�ɫͷ��

	/**
	 * ��ĻѪ��
	 */
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;
	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY(EditAnywhere, Category = "Announcement")
	TSubclassOf<UUserWidget> AnnouncementClass;
	UPROPERTY()
	class UAnnouncement* Announcement;
	void AddAnnouncement();

protected:
	virtual void BeginPlay() override;


private:

	/* ��Ļ׼�� */

	FHUDPackage HUDPackage; // �������׼������
	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FLinearColor CrosshairColor, FVector2D Spread = FVector2D(0.f));

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f; // ׼����ɢ

};
