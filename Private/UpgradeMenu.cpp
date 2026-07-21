	#include "UpgradeMenu.h"
#include "UpgradeEntry.h"
#include "CabinGameInstance.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
 
void UUpgradeMenu::NativeConstruct()
{
	Super::NativeConstruct();
 
	// 1. 获取 GameInstance 引用
	UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance());
 
	// 2. 核心逻辑合并：在一个循环内完成所有初始化工作
	if (EntryContainer)
	{
		// AllChildren 在这里定义，作用域持续到 if 结束
		TArray<UWidget*> AllChildren = EntryContainer->GetAllChildren();
		
		for (UWidget* Child : AllChildren)
		{
			if (UUpgradeEntry* Entry = Cast<UUpgradeEntry>(Child))
			{
				// A. 将条目加入查找表，方便后续通过广播刷新
				EntryMap.Add(Entry->FacilityTag, Entry);
 
				// B. 处理秘密设施的显示/隐藏逻辑
				// 如果是隐藏设施且未解锁，则将其从 UI 中完全消除（Collapsed）
				if (Entry->bIsSecret && GI)
				{
					if (!GI->UnlockedTags.Contains(Entry->FacilityTag))
					{
						Entry->SetVisibility(ESlateVisibility::Collapsed);
					}
				}
			}
		}
	}
 
	// 3. 绑定解锁广播事件
	if (GI)
	{
		// 当任何设施解锁时，通知此菜单刷新状态
		GI->OnFacilityUnlocked.AddDynamic(this, &UUpgradeMenu::RefreshFacilityStatus);
	}
}
 
void UUpgradeMenu::RefreshFacilityStatus(FString UnlockedTag)
{
	// 检查 Map 中是否存在该设施的标签
	if (EntryMap.Contains(UnlockedTag))
	{
		// 找到对应的 UI 条目，消除锁定状态（材质变彩色）
		EntryMap[UnlockedTag]->SetIconLocked(false);
		
		// 如果该条目之前是隐藏的，解锁后应显示出来
		EntryMap[UnlockedTag]->SetVisibility(ESlateVisibility::Visible);
	}
}