#pragma once
 
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "CabinPlayerController.generated.h"
 
UCLASS()
class CABINGAME_API ACabinPlayerController : public APlayerController
{
	GENERATED_BODY()
 
public:
	// 新增 Getter
	UFUNCTION(BlueprintCallable, Category = "UI")
	UUserWidget* GetMainHUD() const { return MainHUDInstance; }
	// 在编辑器里指定你的 WBP_MainHUD 类
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> MainHUDClass;
	UFUNCTION(BlueprintCallable, Category = "UI")
	UUserWidget* GetActiveHUD() const 
	{ 
		return MainHUDInstance ? MainHUDInstance : PlayerHUDInstance; 
	}
	// 在编辑器里指定你的 WBP_PlayerHUD 类
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> PlayerHUDClass;
	// 增加这个函数
	UFUNCTION(BlueprintCallable, Category = "UI")
	UUserWidget* GetPlayerHUD() const { return PlayerHUDInstance; }
protected:
	virtual void BeginPlay() override;
 
private:
	UPROPERTY()
	UUserWidget* MainHUDInstance;
 
	UPROPERTY()
	UUserWidget* PlayerHUDInstance;
};