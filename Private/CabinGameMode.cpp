#include "CabinGameMode.h"
#include "BaseCharacter.h"
#include "CabinGameInstance.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

ACabinGameMode::ACabinGameMode()
{
	// 可以空着，也可以写默认逻辑
}
void ACabinGameMode::SettleLevelResources()
{
	if (!HasAuthority()) return;
	UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance());
	if (!GI) return;
 
	int32 CurrentBatchTotal = 0;
	// 1. 收集所有玩家身上的钱
	for (TActorIterator<ABaseCharacter> It(GetWorld()); It; ++It)
	{
		CurrentBatchTotal += It->InGameMaterials;
		It->InGameMaterials = 0; // 收缴后清空口袋
	}
 
	if (CurrentBatchTotal > 0)
	{
		// 2. 正确累加到银行！(AddToTeamBank 内部执行的是 +=)
		GI->AddToTeamBank(CurrentBatchTotal);
		
		// 3. 通知所有玩家同步最新的银行总额
		for (FConstPlayerControllerIterator ItPC = GetWorld()->GetPlayerControllerIterator(); ItPC; ++ItPC)
		{
			if (ABaseCharacter* Char = Cast<ABaseCharacter>(ItPC->Get()->GetPawn()))
			{
				Char->Client_SyncPersistentStats(GI->GlobalMaterialCount, GI->PermanentMaxHealth);
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("结算完成：累加存入 %d"), CurrentBatchTotal);
	}
}
// ========================== 修复：检查所有玩家是否被淘汰 ==========================
void ACabinGameMode::CheckAllPlayersEliminated()
{
	TArray<APlayerController*> PlayerControllers;

	// UE5.7 正确写法
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get(); // 加 .Get() 修复
		if (PC)
		{
			PlayerControllers.Add(PC);
		}
	}

	bool AllEliminated = true;
	for (APlayerController* PC : PlayerControllers)
	{
		ABaseCharacter* Character = Cast<ABaseCharacter>(PC->GetPawn());
		if (Character && !Character->bIsEliminated)
		{
			AllEliminated = false;
			break;
		}
	}

	if (AllEliminated)
	{
		UE_LOG(LogTemp, Log, TEXT("所有玩家已淘汰！"));
	}
}

void ACabinGameMode::OnEnemyEliminated()
{
	CheckLevelCompletion();
}

void ACabinGameMode::CheckLevelCompletion()
{
}

void ACabinGameMode::UnlockAllTransitionGates()
{
}
// 建议在 GameMode 中重写 PostLogin
void ACabinGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
 
	if (ABaseCharacter* Char = Cast<ABaseCharacter>(NewPlayer->GetPawn()))
	{
		if (UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance()))
		{
			// 强行下发一次当前的资源总量
			Char->Client_SyncPersistentStats(GI->GlobalMaterialCount, GI->PermanentMaxHealth);
		}
	}
}