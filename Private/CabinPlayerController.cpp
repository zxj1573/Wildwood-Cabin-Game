#include "CabinPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
 
void ACabinPlayerController::BeginPlay()
{
	Super::BeginPlay();
 
	if (!IsLocalController()) return;
 
	// 获取当前地图名称
	FString MapName = GetWorld()->GetMapName();
 
	// --- 排他性 UI 逻辑 ---
	
	// 如果在森林关卡：只显示 PlayerHUD (准星/局内材料)
	if (MapName.Contains(TEXT("Forest")))
	{
		// 如果之前残留了 MainHUD，先彻底移除（防止叠加）
		if (MainHUDInstance)
		{
			MainHUDInstance->RemoveFromParent();
			MainHUDInstance = nullptr;
		}
 
		if (PlayerHUDClass && !PlayerHUDInstance)
		{
			PlayerHUDInstance = CreateWidget<UUserWidget>(this, PlayerHUDClass);
			if (PlayerHUDInstance) PlayerHUDInstance->AddToViewport(1);
		}
	}
	// 如果在木屋/基地：只显示 MainHUD (总钱数)
	else if (MapName.Contains(TEXT("Cabin")) || MapName.Contains(TEXT("Main")))
	{
		if (PlayerHUDInstance)
		{
			PlayerHUDInstance->RemoveFromParent();
			PlayerHUDInstance = nullptr;
		}
 
		if (MainHUDClass && !MainHUDInstance)
		{
			MainHUDInstance = CreateWidget<UUserWidget>(this, MainHUDClass);
			if (MainHUDInstance) MainHUDInstance->AddToViewport(0);
		}
	}
}