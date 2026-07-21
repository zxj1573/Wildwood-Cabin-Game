#pragma once
 
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CabinItemData.generated.h"
 
UENUM(BlueprintType)
enum class EItemType : uint8
{
	Weapon,
	Potion,
	Blessing, // 赐福
	Curse,    // 诅咒
	Gag       // 搞怪道具
};
 
UCLASS(BlueprintType)
class CABINGAME_API UCabinItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()
 
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FText ItemName;
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	UTexture2D* Icon;
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logic")
	EItemType ItemType;
 
	// 掉落权重：数字越大，这一关结束后爆出它的概率越高
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roguelike")
	int32 DropWeight = 10;
 
	// 物理模型：掉在地上时的样子
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	UStaticMesh* PickupMesh;
 
	// 数值强度（如回血量、增伤百分比）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Magnitude = 10.0f;
	/** 【新增】：当物品作为武器被捡起时，要在玩家手中生成的 Actor 类（如 BP_Axe） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logic|Combat")
	TSubclassOf<AActor> ItemActorClass;
 
	/** 【新增】：对应的战斗姿态索引 (0:赤手空拳, 1:近战, 2:远程) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logic|Combat")
	uint8 WeaponStateIndex = 0;
	/** 【新增】：当物品作为武器被捡起时，要在玩家手中生成的 Actor 类 */
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logic|Combat")
	TObjectPtr<UAnimMontage> AttackMontage;
};