	#pragma once
 
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
// --- 必须包含以下头文件，否则编译器不认识 UButton 和 UTextBlock ---
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "UpgradeEntry.generated.h"
 
UCLASS()
class CABINGAME_API UUpgradeEntry : public UUserWidget
{
	GENERATED_BODY()
 
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FString FacilityTag;
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 Cost = 100;
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bIsSecret = false; 
 
	// 使用 class 关键字或直接包含头文件
	UPROPERTY(meta = (BindWidget))
	class UButton* PurchaseButton;
 
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* CostText;
 
	UPROPERTY(meta = (BindWidget))
	class UImage* IconImage;
 
	UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
	void SetIconLocked(bool bIsLocked);
 
protected:
	UPROPERTY()
	class UMaterialInstanceDynamic* DynamicMaterial;
 
	virtual void NativeConstruct() override;
 
	UFUNCTION()
	void OnPurchaseClicked();
};