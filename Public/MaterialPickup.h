#pragma once
 
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractionInterface.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "MaterialPickup.generated.h"
 
// 前向声明
class UNiagaraSystem;
class USoundBase;

UCLASS()
class CABINGAME_API AMaterialPickup : public AActor, public IInteractionInterface
{
	GENERATED_BODY()
public:
	AMaterialPickup();
 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	int32 MaterialAmount = 10;
 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* PickupMesh;
 
	virtual void Interact_Implementation(ABaseCharacter* Interactor) override;
 
	
	virtual void Destroyed() override;
	
	UPROPERTY(EditDefaultsOnly, Category="Effects")
	class UNiagaraSystem* PickupVFX;
 
	UPROPERTY(EditDefaultsOnly, Category="Effects")
	class USoundBase* PickupSFX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundAttenuation* AttenuationSettings; // 空指针时无范围限制，有值则只在范围内听到
protected:
	virtual void BeginPlay() override;
};