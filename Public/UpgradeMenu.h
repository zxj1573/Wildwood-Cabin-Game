	#pragma once
 
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UpgradeEntry.h" // 必须包含，因为 TMap 需要完整类型
#include "UpgradeMenu.generated.h"
 
/**
 * UUpgradeMenu 
 * 升级系统的总图鉴界面
 */
UCLASS()
class CABINGAME_API UUpgradeMenu : public UUserWidget
{
	GENERATED_BODY()
 
public:
	/** 当设施解锁广播发出时调用 */
	UFUNCTION()
	void RefreshFacilityStatus(FString UnlockedTag);
 
protected:
	/** 蓝图中的条目容器（例如 WrapBox 或 VerticalBox） */
	UPROPERTY(meta = (BindWidget))
	class UPanelWidget* EntryContainer;
 
	/** 存储 Tag 到 UI 指针的映射，方便快速变色 */
	UPROPERTY()
	TMap<FString, UUpgradeEntry*> EntryMap;
 
	virtual void NativeConstruct() override;
};