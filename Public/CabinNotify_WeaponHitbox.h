#pragma once
 
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "CabinNotify_WeaponHitbox.generated.h"
 
UCLASS()
class CABINGAME_API UCabinNotify_WeaponHitbox : public UAnimNotifyState
{
	GENERATED_BODY()
 
public:
	// 注意：增加了 EventReference 参数
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
 
	UPROPERTY(EditAnywhere, Category = "Combat")
	float TraceRadius = 15.0f;
 
	UPROPERTY(EditAnywhere, Category = "Combat")
	float DamageBase = 20.0f;
 
private:
	UPROPERTY()
	TArray<AActor*> AlreadyHitActors;
};