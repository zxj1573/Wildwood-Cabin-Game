	#include "UpgradeEntry.h"
#include "CabinGameInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/Image.h"
 
// 注意：必须有 UUpgradeEntry:: 前缀！
void UUpgradeEntry::NativeConstruct()
{
	Super::NativeConstruct();
 
	if (IconImage)
	{
		// 自动创建动态材质实例
		DynamicMaterial = IconImage->GetDynamicMaterial();
	}
	if (CostText) CostText->SetText(FText::AsNumber(Cost));
	if (PurchaseButton)
	{
		PurchaseButton->OnClicked.AddDynamic(this, &UUpgradeEntry::OnPurchaseClicked);
	}
 
	// 初始化状态：检查自己是否已经解锁
	if (UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance()))
	{
		bool bUnlocked = GI->UnlockedTags.Contains(FacilityTag);
		SetIconLocked(!bUnlocked); // 如果没解锁，就设为阴影（1.0）
	}
}
 
// 注意：必须有 UUpgradeEntry:: 前缀！
void UUpgradeEntry::SetIconLocked(bool bIsLocked)
{
	if (DynamicMaterial)
	{
		// 1.0 = 阴影, 0.0 = 彩色
		float ParameterValue = bIsLocked ? 1.0f : 0.0f;
		DynamicMaterial->SetScalarParameterValue(FName("IsLocked"), ParameterValue);
	}
}

 
void UUpgradeEntry::OnPurchaseClicked()
{
	UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance());
	if (GI && GI->PurchaseFacility(FacilityTag, Cost))
	{
		// 成功扣钱后，GameInstance 会广播 OnFacilityUnlocked
		// 你的 UpgradeMenu 已经在监听这个广播并会调用 RefreshFacilityStatus
		// 这里可以加个成功音效
	}
}