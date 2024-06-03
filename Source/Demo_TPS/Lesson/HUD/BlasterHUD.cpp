// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"
#include "Announcement.h"

void ABlasterHUD::BeginPlay()
{
	Super::BeginPlay();

}

// ��ӽ�ɫͷ����ʾ
void ABlasterHUD::AddCharacterOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}

}

// ����ʱ��
void ABlasterHUD::AddAnnouncement()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && AnnouncementClass)
	{
		Announcement = CreateWidget<UAnnouncement>(PlayerController, AnnouncementClass);
		Announcement->AddToViewport();
	}
}


/************************************************************************/
/*                    ���� HUD                                           */
/************************************************************************/

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (GEngine)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D VeiwportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
		
		// ��Ϸ�ڶ�̬�ı� SpreadScaled
		float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

		DrawCrosshair(HUDPackage.CrosshairsCenter, VeiwportCenter, HUDPackage.CrosshairColor);
		DrawCrosshair(HUDPackage.CrosshairsLeft, VeiwportCenter, HUDPackage.CrosshairColor, FVector2D(-SpreadScaled, 0.f));
		DrawCrosshair(HUDPackage.CrosshairsRight, VeiwportCenter, HUDPackage.CrosshairColor, FVector2D(SpreadScaled, 0.f));
		DrawCrosshair(HUDPackage.CrosshairsTop, VeiwportCenter, HUDPackage.CrosshairColor, FVector2D(0.f, -SpreadScaled));
		DrawCrosshair(HUDPackage.CrosshairsBottom, VeiwportCenter, HUDPackage.CrosshairColor, FVector2D(0.f, SpreadScaled));
	}
}


/************************************************************************/
/*                    ������Ļ׼��                                       */
/************************************************************************/

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FLinearColor CrosshairColor, FVector2D Spread)
{
	if (Texture)
	{
		const float TextureWidth = Texture->GetSizeX();
		const float TextureHeight = Texture->GetSizeY();
		const FVector2D TextureDrawPoint(
			ViewportCenter.X - (TextureWidth / 2.f) + Spread.X,
			ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y
		);
		DrawTexture(
			Texture,
			TextureDrawPoint.X, TextureDrawPoint.Y,
			TextureWidth, TextureHeight,
			0.f, 0.f, 1.f, 1.f,
			CrosshairColor
		);
	}
}


