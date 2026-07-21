	#pragma once
 
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUDWidget.generated.h"
 
UCLASS()
class CABINGAME_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()
 
public:
	// 必须添加 BlueprintReadOnly，否则蓝图会报错“不对蓝图可见”
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* InteractionText; 
 
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* NotificationText;
 
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* InGameMaterialText;
 
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UProgressBar* HealthBar;
 
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UProgressBar* StaminaBar;
 
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetPromptText(FText TextToDisplay);
 
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowNotification(const FString& Message);
 
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateStatus(float HPPercent, float SPPercent);
 
	/** 
	 * 使用 BlueprintNativeEvent 允许蓝图重写逻辑。
	 * 它是此类的第一个定义，不要在 _Implementation 中使用 override。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI")
	void UpdateMaterialText(int32 NewAmount);
	
	// 注意：没有 override，因为 UUserWidget 里没有这个函数
	virtual void UpdateMaterialText_Implementation(int32 NewAmount);
 
protected:
	virtual void NativeConstruct() override;
	FTimerHandle NotificationTimerHandle;
	void HideNotification();
};