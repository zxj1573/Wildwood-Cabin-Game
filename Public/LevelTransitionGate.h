#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractionInterface.h"
#include "BaseCharacter.h"
#include "Components/BoxComponent.h"
#include "LevelTransitionGate.generated.h"
UENUM(BlueprintType)
enum class EGatePathType : uint8
{
	Normal      UMETA(DisplayName = "普通路径"),
	Hard        UMETA(DisplayName = "困难路径")
};
UCLASS()
class CABINGAME_API ALevelTransitionGate : public AActor, public IInteractionInterface
{
	GENERATED_BODY()

public:
	ALevelTransitionGate();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Travel")
	FString TargetMapPath;

	// 分支难度倍率（普通路1.0，更难的路1.5）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Travel")
	float DifficultyMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Travel")
	int32 RequiredMaterials = 0; // 如果进入森林需要门票则设置

	// 集结触发区域
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* GatheringVolume;

	// 实现交互接口
	virtual void Interact_Implementation(ABaseCharacter* Interactor) override;
	UFUNCTION(Client, Reliable)
	void Client_ShowGatheringWarning(int32 Current, int32 Total);
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnShowGatheringFeedback(int32 Current, int32 Total);
	// 是否已解锁（默认关着）
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Gate")
	bool bIsLocked = true;

	// 服务器解锁函数
	UFUNCTION(BlueprintCallable, Category = "Gate")
	void SetGateLocked(bool bNewLocked);
	// 在编辑器里设置这扇门通往哪里
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Configuration")
	EGatePathType PathType = EGatePathType::Normal;
 
	// 困难路径的额外难度加成系数
	UPROPERTY(EditAnywhere, Category = "Gate Configuration")
	float HardPathMultiplierBonus = 0.5f; 
protected:
	// 记录当前在集结区的玩家
	UPROPERTY()
	TArray<ABaseCharacter*> PlayersInArea;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

	// 全员同步播放进入动画/特效
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayEntryEffects();

	// 供蓝图绑定的动画事件
	UFUNCTION(BlueprintImplementableEvent, Category = "Travel")
	void OnPlayEntryAnimation();

	// 最终跳转逻辑
	void InitiateServerTravel();
};