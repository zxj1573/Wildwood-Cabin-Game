	#include "CabinUpgradeStation.h"
#include "CabinGameInstance.h"
#include "BaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
 
const int32 UPGRADE_COST_HEALTH = 100;
const float HEALTH_INCREMENT_VAL = 20.f;
const float HEALTH_MAX_CAP = 300.f;
 
ACabinUpgradeStation::ACabinUpgradeStation()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
	// 【核心修复】：必须有 Root 且有物理形状，射线才能撞上它
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;
	InfoWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InfoWidget"));
	InfoWidget->SetupAttachment(RootComponent);
}
// ========================== 获取下一个阶段信息 ==========================
bool ACabinUpgradeStation::GetNextStageInfo(FString& OutStageName, int32& OutCost)
{
	// 默认返回假，你后面可以改成你的升级逻辑
	OutStageName = TEXT("下一阶段");
	OutCost = 100;
	return false;
}
	 
void ACabinUpgradeStation::ExecuteServerInteraction(AActor* Interactor)
{
	// 仅在服务器执行逻辑
	if (!HasAuthority()) return;
 
	UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance());
	ABaseCharacter* Character = Cast<ABaseCharacter>(Interactor);
	if (!GI || !Character) return;
 
	// --- 查找下一个未解锁的设施阶段 ---
	FUnlockStage* NextStage = nullptr;
	for (int32 i = 0; i < UnlockStages.Num(); ++i)
	{
		// 如果存档中不包含这个标签，说明找到了下一个要解锁的项目
		if (!GI->UnlockedTags.Contains(UnlockStages[i].UnlockTag))
		{
			NextStage = &UnlockStages[i];
			break;
		}
	}
 
	if (NextStage)
	{
		// 检查玩家余额是否足够
		if (GI->GlobalMaterialCount >= NextStage->Cost)
		{
			// 1. 扣除费用
			GI->GlobalMaterialCount -= NextStage->Cost;
			
			// 2. 将标签存入已解锁名单
			GI->UnlockedTags.AddUnique(NextStage->UnlockTag);
 
			// 3. 【核心修复】：如果是属性升级（如血量），在此处应用效果
			if (NextStage->UnlockTag.Contains(TEXT("Health")))
			{
				GI->PermanentMaxHealth = FMath::Min(GI->PermanentMaxHealth + HEALTH_INCREMENT_VAL, HEALTH_MAX_CAP);
			}
            
			// 4. 【核心优化】：广播解锁事件。
			// 木屋里的设施不再需要每0.5秒查一次，听到这个广播会瞬间“长出来”
			if (GI->OnFacilityUnlocked.IsBound())
			{
				GI->OnFacilityUnlocked.Broadcast(NextStage->UnlockTag);
			}
 
			// 5. 立即保存进度，防止掉线
			GI->SaveProgress();
 
			// 6. 同步最新的属性给客户端 UI
			Character->Client_SyncPersistentStats(GI->GlobalMaterialCount, GI->PermanentMaxHealth);
            
			// 7. 【解耦关键】：消除强制跳转。
			// 不再调用 ExecuteStageUnlock，让玩家留在木屋自由行动
			Character->Client_ReceiveUIMessage(FString::Printf(TEXT("解锁成功：%s"), *NextStage->StageName));
		}
		else
		{
			Character->Client_ReceiveUIMessage(TEXT("材料不足！"));
		}
	}
	else
	{
		// 找不到 NextStage 说明所有配置的阶段都买完了
		Character->Client_ReceiveUIMessage(TEXT("所有基地设施已满级！"));
	}
}
 
void ACabinUpgradeStation::ExecuteStageUnlock(const FUnlockStage& Stage)
{
	if (!HasAuthority()) return;
 
	// 【核心修复】：防止跳转空地图导致的黑屏
	if (Stage.LevelName.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("错误：阶段 %s 没有设置 LevelName！"), *Stage.StageName);
		return;
	}
 
	// 只有在路径有效时才跳转
	FString URL = Stage.LevelName.ToString() + TEXT("?listen");
	GetWorld()->ServerTravel(URL);
}
 

void ACabinUpgradeStation::Interact_Implementation(ABaseCharacter* Interactor)
{
	// 逻辑：如果这个站是用来打开菜单的，就通知互动的玩家
	if (Interactor && Interactor->IsLocallyControlled())
	{
		Client_OpenUpgradeMenu();
	}
	else if (HasAuthority()) // 如果是服务器接收到，强制发回给客户端
	{
		// 假设 Interactor 有获取 Controller 的方法
		Client_OpenUpgradeMenu(); 
	}
}
 
void ACabinUpgradeStation::Client_OpenUpgradeMenu_Implementation()
{
	if (!UpgradeMenuClass) return;
 
	APlayerController* PC = GEngine->GetFirstLocalPlayerController(GetWorld());
	if (PC)
	{
		// 创建 Widget 时建议使用具体的类或 UUserWidget
		UUserWidget* Menu = CreateWidget<UUserWidget>(PC, UpgradeMenuClass);
		if (Menu)
		{
			Menu->AddToViewport();
			
			FInputModeUIOnly InputMode;
			// --- 修复点：消除 GetSafeWidget 错误，改用 TakeWidget() ---
			InputMode.SetWidgetToFocus(Menu->TakeWidget()); 
			
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
	}
}
void ACabinUpgradeStation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACabinUpgradeStation, CurrentStageIndex);
}
 
void ACabinUpgradeStation::OnRep_CurrentStageIndex() { if (UnlockStages.IsValidIndex(CurrentStageIndex)) { /* 播放音效 */ } }
void ACabinUpgradeStation::BeginPlay() { Super::BeginPlay(); }