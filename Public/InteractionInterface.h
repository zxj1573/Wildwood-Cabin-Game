	#pragma once
 
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionInterface.generated.h"
 
// 前向声明，防止循环包含
class ABaseCharacter;
 
UINTERFACE(BlueprintType)
class CABINGAME_API UInteractionInterface : public UInterface
{
	GENERATED_BODY()
};
 
class CABINGAME_API IInteractionInterface
{
	GENERATED_BODY()
public:
	// 所有的交互函数现在都必须接收 ABaseCharacter 类型的指针
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(ABaseCharacter* PlayerCharacter);
};