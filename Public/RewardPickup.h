	#pragma once
 
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RewardPickup.generated.h"
 
UCLASS()
class CABINGAME_API ARewardPickup : public AActor
{
	GENERATED_BODY()
 
public:
	ARewardPickup();
 
	// 由服务器初始化奖励内容
	void Initialize(class UCabinItemData* InData, int32 InMaterials);
 
protected:
	UPROPERTY(VisibleAnywhere)
	class USphereComponent* SphereComp;
 
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* MeshComp;
 
	// 网络同步数据
	UPROPERTY(ReplicatedUsing = OnRep_ItemData)
	class UCabinItemData* ItemData;
 
	UPROPERTY(Replicated)
	int32 ContainedMaterials;
 
	UFUNCTION()
	void OnRep_ItemData();
 
	// 拾取逻辑
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};