// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseFacility.h"
#include "ReturnGate.generated.h"

/**
 * 
 */
class ABaseCharacter; 
UCLASS()
class CABINGAME_API AReturnGate : public ABaseFacility
{
	GENERATED_BODY()
public:
	virtual void Interact_Implementation(ABaseCharacter* Interactor) override;
	
};
