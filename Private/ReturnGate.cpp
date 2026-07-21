// Fill out your copyright notice in the Description page of Project Settings.


#include "ReturnGate.h"
#include "BaseCharacter.h"
#include "Kismet/GameplayStatics.h" // 必须包含这个，否则无法解析 UGameplayStatics
#include "CabinGameInstance.h"
#include "CabinGameMode.h"
 
void AReturnGate::Interact_Implementation(ABaseCharacter* Interactor)
{
	if (!HasAuthority()) return;
 
	if (ACabinGameMode* GM = Cast<ACabinGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GM->SettleLevelResources(); // 核心：跳转前先结算
		// 使用 Seamless Travel
		GetWorld()->ServerTravel(TEXT("/Game/Maps/CabinMap?listen"));
	}
}