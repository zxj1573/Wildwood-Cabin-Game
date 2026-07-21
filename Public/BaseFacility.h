#pragma once
 
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractionInterface.h"
#include "GameplayTagContainer.h"
#include "Sound/SoundBase.h"
#include "NiagaraSystem.h"
#include "BaseFacility.generated.h"
 
UCLASS()
class CABINGAME_API ABaseFacility : public AActor, public IInteractionInterface
{
	GENERATED_BODY()
 
public:
	ABaseFacility();
 
protected:
	// 【核心优化】：消除定时器句柄和 CheckUnlockStatus 声明
	// FTimerHandle TimerHandle_CheckUnlock; -> 已删除
	// void CheckUnlockStatus(); -> 已删除
 
	// 【核心优化】：新增解锁事件的回调函数
	UFUNCTION()
	void HandleFacilityUnlocked(FString UnlockedTag);
 
	virtual void BeginPlay() override;
 

 
	UFUNCTION()
	void OnRep_IsBuilt();
 
	virtual void Interact_Implementation(ABaseCharacter* Interactor) override;
 
public:
	// --- 配置参数 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility")
	FString FacilityTag; // 该设施的唯一标识标签
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility | Effects")
	UNiagaraSystem* BuildEffect; // 显现时的粒子特效
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility | Effects")
	USoundBase* BuildSound; // 显现时的音效
 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* BoardMesh;
	// --- 属性同步与交互 ---
	UPROPERTY(ReplicatedUsing = OnRep_IsBuilt, EditAnywhere, BlueprintReadWrite, Category = "Facility")
	bool bIsBuilt;
	// --- 虚函数重写 ---
	virtual void ExecuteServerInteraction(class ABaseCharacter* PlayerCharacter);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};