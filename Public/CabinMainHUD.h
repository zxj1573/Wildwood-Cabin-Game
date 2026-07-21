	#pragma once
 
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CabinMainHUD.generated.h"
 
UCLASS()
class CABINGAME_API UCabinMainHUD : public UUserWidget
{
	GENERATED_BODY()
 
public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* GlobalMaterialText;

	// 共享变量：既是准星提示，也是人数提示
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, OptionalWidget = true))
	class UTextBlock* InteractionText;
 
	// 新增：专门用于集结/人数提示的独立文本框
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, OptionalWidget = true))
	class UTextBlock* PromptText; 
	
	void RefreshGlobalCount(int32 Count);
    
	void ShowGatheringProgress(int32 Current, int32 Total);
	
	virtual void NativeConstruct() override;
	
	void SetPromptText(FText Text);
	// 新增：锁定标记

private:
	void ClearPrompt();
	FTimerHandle TimerHandle_ClearPrompt;
 
	// 逻辑锁：当正在显示人数提示时，准星交互提示暂时失效
	bool bIsShowingGatheringStatus = false;
	bool bIsShowingWarning = false;
};
 