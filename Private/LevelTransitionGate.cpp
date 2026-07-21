#include "LevelTransitionGate.h"
#include "BaseCharacter.h"
#include "CabinGameInstance.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
// --- 关键包含 ---
#include "CabinMainHUD.h"  // 包含你自己的 UI 类
#include "BaseCharacter.h" // 确保包含了角色类
#include "Net/UnrealNetwork.h" 
#include "Kismet/GameplayStatics.h"

ALevelTransitionGate::ALevelTransitionGate()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

    // 初始化集结区
    GatheringVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("GatheringVolume"));
    RootComponent = GatheringVolume;
    GatheringVolume->SetBoxExtent(FVector(800.f, 800.f, 100.f)); // 约 4x4 米范围
}
// --- 补齐 1: 网络同步必须实现的函数 ---
void ALevelTransitionGate::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 
	// 注册 bIsLocked 变量进行同步
	DOREPLIFETIME(ALevelTransitionGate, bIsLocked);
}
 
// --- 补齐 2: 实现解锁函数 ---
void ALevelTransitionGate::SetGateLocked(bool bNewLocked)
{
	if (HasAuthority())
	{
		bIsLocked = bNewLocked;
		UE_LOG(LogTemp, Warning, TEXT("路牌状态更改: %s"), bIsLocked ? TEXT("锁定") : TEXT("已激活"));
	}
}
void ALevelTransitionGate::NotifyActorBeginOverlap(AActor* OtherActor)
{
	// 仅在服务器处理逻辑
	if (!HasAuthority()) return;
 
	if (ABaseCharacter* Character = Cast<ABaseCharacter>(OtherActor))
	{
		PlayersInArea.AddUnique(Character);
		UE_LOG(LogTemp, Log, TEXT("玩家进入集结区: %s (当前人数: %d)"), *Character->GetName(), PlayersInArea.Num());
	}
}

void ALevelTransitionGate::NotifyActorEndOverlap(AActor* OtherActor)
{
	// 仅在服务器处理逻辑
	if (!HasAuthority()) return;
 
	if (ABaseCharacter* Character = Cast<ABaseCharacter>(OtherActor))
	{
		PlayersInArea.Remove(Character);
		UE_LOG(LogTemp, Warning, TEXT("玩家离开集结区: %s (当前人数: %d)"), *Character->GetName(), PlayersInArea.Num());
	}
}

void ALevelTransitionGate::Interact_Implementation(ABaseCharacter* Interactor)
{
	if (!HasAuthority() || !Interactor) return;
 
	UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance());
	if (!GI) return;
 
	// --- 新增逻辑：检查是否在木屋 ---
	// 如果关卡索引是 0（初始木屋），或者地图名包含 CabinMap，则视为豁免区域
	bool bIsHomeBase = (GI->CurrentLevelIndex == 0) || GetWorld()->GetMapName().Contains(TEXT("CabinMap"),ESearchCase::IgnoreCase);
 
	// 如果不在木屋 且 被锁定了，才提示任务未完成
	if (!bIsHomeBase && bIsLocked)
	{
		Interactor->Client_ShowPopup(TEXT("本区域任务尚未完成，路牌未激活！"));
		return;
	}
 
	// 1. 获取当前世界总玩家数 (在线人数)
	int32 TotalPlayers = GetWorld()->GetNumPlayerControllers();
    
	// 2. 获取区域内已准备人数
	int32 ReadyCount = PlayersInArea.Num();
 
	UE_LOG(LogTemp, Warning, TEXT("尝试开启传送: 准备人数 %d / 总人数 %d"), ReadyCount, TotalPlayers);
 
	// 3. 判定逻辑：人齐了 (ReadyCount >= TotalPlayers)
	if (ReadyCount >= TotalPlayers)
	{
		// 传送逻辑保持不变...
		Multicast_PlayEntryEffects();
		FTimerHandle TravelTimer;
		GetWorldTimerManager().SetTimer(TravelTimer, this, &ALevelTransitionGate::InitiateServerTravel, 1.5f, false);
	}
	else
	{
		Client_ShowGatheringWarning(ReadyCount, TotalPlayers);
	}
}
void ALevelTransitionGate::Client_ShowGatheringWarning_Implementation(int32 Current, int32 Total)
{
	// 1. 获取本地正在使用的控制器（在客户端运行此 RPC 时，这会返回本地玩家）
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
    
	// 关键检查：必须是本地控制的玩家！
	if (!PC || !PC->IsLocalController()) 
	{
		return; 
	}
 
	// 2. 查找 UI
	TArray<UUserWidget*> FoundWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), FoundWidgets, UCabinMainHUD::StaticClass(), false);
 
	for (UUserWidget* Widget : FoundWidgets)
	{
		if (UCabinMainHUD* MainUI = Cast<UCabinMainHUD>(Widget))
		{
			// 3. 再次确认这个 UI 实例属于当前的本地玩家
			if (MainUI->GetOwningPlayer() == PC)
			{
				// 专门打印一条调试，确认找到了“本地”UI
				// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("成功找到本地 HUD 实例！"));
				MainUI->ShowGatheringProgress(Current, Total);
			}
		}
	}
}
void ALevelTransitionGate::Multicast_PlayEntryEffects_Implementation()
{
    // 在所有客户端触发蓝图动画事件
    OnPlayEntryAnimation();
}

void ALevelTransitionGate::InitiateServerTravel()
{
	if (!HasAuthority()) return;
 
	UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance());
	if (!GI) return;
 
	GI->CurrentLevelIndex++;
 
	// 获取地图类型
	ELevelPathType MapTypeToSeek = (PathType == EGatePathType::Hard) ? ELevelPathType::Hard : ELevelPathType::Normal;
	
	// 【核心检查点 1】：获取路径
	FString NextMapPath = GI->GetRandomMapFromPool(MapTypeToSeek);
	
	// 【核心检查点 2】：打印路径到日志
	if (NextMapPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("传送失败！地图池中没有找到路径。请检查 BP_CabinGameInstance 的 MapPool 数组！"));
		return; 
	}
 
	UWorld* World = GetWorld();
	if (World)
	{
		// 拼接 URL。注意路径必须以 /Game/ 开头
		FString TravelURL = NextMapPath + TEXT("?seamless=1");
		
		UE_LOG(LogTemp, Warning, TEXT("=== 正在准备传送 ==="));
		UE_LOG(LogTemp, Warning, TEXT("目标地图路径: %s"), *NextMapPath);
		UE_LOG(LogTemp, Warning, TEXT("最终跳转 URL: %s"), *TravelURL);
 
		World->ServerTravel(TravelURL);
	}
}