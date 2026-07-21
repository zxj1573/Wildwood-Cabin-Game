#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

UCLASS()
class CABINGAME_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()
public:
	AEnemyAIController();
protected:
	UPROPERTY(VisibleAnywhere)
	class UAIPerceptionComponent* AIPerception;

	class UAISenseConfig_Sight* SightConfig;

	UFUNCTION()
	void OnTargetDetected(AActor* Actor, struct FAIStimulus Stimulus);
};