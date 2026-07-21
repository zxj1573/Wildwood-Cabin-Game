	#pragma once
 
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CabinCombatInterface.generated.h"
 
// 必须带有 BlueprintType，才能在蓝图类设置里看到它
UINTERFACE(BlueprintType)
class UCabinCombatInterface : public UInterface
{
	GENERATED_BODY()
};
 
class CABINGAME_API ICabinCombatInterface
{
	GENERATED_BODY()
 
public:
	/** 
	 * 核心伤害函数
	 * @param DamageAmount 伤害数值
	 * @param HitResult 碰撞信息（用于产生火花特效的位置）
	 * @param DamageCauser 谁发出的伤害（玩家）
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat")
	void TakeCabinDamage(float DamageAmount, const FHitResult& HitResult, AActor* DamageCauser);
};