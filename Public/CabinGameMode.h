#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CabinGameMode.generated.h"

class UCabinGameInstance;
class ABaseCharacter;

UCLASS()
class CABINGAME_API ACabinGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACabinGameMode();

	/** 结算关卡资源：收缴所有玩家的 InGameMaterials 并存入全局银行 */
	UFUNCTION(BlueprintCallable, Category = "Roguelike")
	void SettleLevelResources();
	virtual void PostLogin(APlayerController* NewPlayer) override;
	/** 检查是否所有玩家都被淘汰 */
	UFUNCTION(BlueprintCallable, Category = "Roguelike")
	void CheckAllPlayersEliminated();

	/** 被 AEnemyBase 在死亡时调用 */
	void OnEnemyEliminated();

protected:
	void CheckLevelCompletion();
	void UnlockAllTransitionGates();
};