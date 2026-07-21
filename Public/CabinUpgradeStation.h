	#pragma once
 
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractionInterface.h"
#include "Components/WidgetComponent.h" 
#include "CabinUpgradeStation.generated.h"
 
class USoundBase;
class UNiagaraSystem;
USTRUCT(BlueprintType)
struct FUnlockStage
{
	GENERATED_BODY()
 
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString StageName;
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString UnlockTag;
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString InternalPrerequisite;
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> GlobalRequirements;
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Cost = 10;
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName LevelName;
};
 
UCLASS()
class CABINGAME_API ACabinUpgradeStation : public AActor, public IInteractionInterface
{
	GENERATED_BODY()
 
public:
	ACabinUpgradeStation();
	// 在类中添加一个 Client RPC
	UFUNCTION(Client, Reliable)
	void Client_OpenUpgradeMenu();
 
	// 在 UPROPERTY 区域添加 Widget 类引用
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> UpgradeMenuClass;
	// --- 新增：神秘 UI 组件 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* InfoWidget;
 
	// --- 新增：获取模糊化信息的函数 ---
	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	bool GetNextStageInfo(FString& OutName, int32& OutCost);
	
	UPROPERTY(EditAnywhere, Category = "Unlock Settings")
	TArray<FUnlockStage> UnlockStages;
	
	virtual void Interact_Implementation(ABaseCharacter* Interactor) override;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentStageIndex)
	int32 CurrentStageIndex = -1;
 
	UFUNCTION()
	void OnRep_CurrentStageIndex();
 
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 

 
	// 服务器执行函数 - 确保参数是 AActor* 且在类作用域内
	void ExecuteServerInteraction(AActor* Interactor);
 
protected:
	virtual void BeginPlay() override;
	void ExecuteStageUnlock(const FUnlockStage& Stage);
	/** 升级成功时的音效 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* UpgradeSound;
 
	/** 升级成功时的 Niagara 特效 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* UpgradeEffect;
};