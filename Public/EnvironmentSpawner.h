	#pragma once
 
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnvironmentSpawner.generated.h"
 
UCLASS()
class CABINGAME_API AEnvironmentSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	AEnvironmentSpawner();
 
protected:
	virtual void BeginPlay() override;
 
	// --- 配置参数 ---
 
	/** 随机生成的资产池（可以在编辑器里放入：宝箱BP、敌人BP、材料包BP） */
	UPROPERTY(EditAnywhere, Category = "Spawner|Settings")
	TArray<TSubclassOf<AActor>> SpawnPool;
 
	/** 一次生成多少个物体 */
	UPROPERTY(EditAnywhere, Category = "Spawner|Settings")
	int32 SpawnCount = 5;
 
	/** 生成半径 */
	UPROPERTY(EditAnywhere, Category = "Spawner|Settings")
	float SpawnRadius = 1000.0f;
 
	/** 是否尝试贴合地面（防止生成在半空中） */
	UPROPERTY(EditAnywhere, Category = "Spawner|Settings")
	bool bSnapToGround = true;
 
private:
	/** 核心生成逻辑 */
	void ExecuteSpawn();
};