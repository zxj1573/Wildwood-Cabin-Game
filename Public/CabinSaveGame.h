#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "Components/WidgetComponent.h" 
#include "CabinSaveGame.generated.h"

UCLASS()
class CABINGAME_API UCabinSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// 存储玩家已经解锁的所有标签
	UPROPERTY()
	TArray<FString> SavedUnlockedTags;
	// 可以在这里添加其他需要永久保存的数据，比如总金币、等级等
	UPROPERTY()
	int32 SavedMaterialCount = 0;
	UPROPERTY()
	FString SaveSlotName = TEXT("Slot1");
	
};