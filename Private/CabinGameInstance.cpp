          #include "CabinGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "BaseFacility.h"
#include "CabinSaveGame.h"
UCabinGameInstance::UCabinGameInstance()
{
    // 初始化默认值
    GlobalMaterialCount = 0;
}

FString UCabinGameInstance::GetRandomMapFromPool(ELevelPathType PathType)
{
    TArray<FString>* SelectedPool = nullptr;
    FString PoolName = TEXT("Unknown");
 
    // 1. 根据传入的枚举选择池子
    switch (PathType)
    {
    case ELevelPathType::Normal: 
        SelectedPool = &NormalMapPool; 
        PoolName = TEXT("NormalMapPool");
        break;
    case ELevelPathType::Hard:   
        SelectedPool = &HardMapPool;   
        PoolName = TEXT("HardMapPool");
        break;
    case ELevelPathType::Elite:  
        SelectedPool = &EliteMapPool;  
        PoolName = TEXT("EliteMapPool");
        break;
    default:                    
        SelectedPool = &NormalMapPool; 
        PoolName = TEXT("Default(Normal)");
        break;
    }
 
    // 2. 打印当前池子的状态（关键调试！）
    if (SelectedPool)
    {
        UE_LOG(LogTemp, Log, TEXT("正在从 %s 抽取地图，池内数量: %d"), *PoolName, SelectedPool->Num());
    }
 
    // 3. 安全检查
    if (!SelectedPool || SelectedPool->Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("错误：%s 为空！请检查 BP_CabinGameInstance 蓝图配置。"), *PoolName);
        // 不要返回木屋！返回空字符串，让调用者知道出事了
        return FString(); 
    }
 
    // 4. 随机抽取
    int32 RandomIdx = FMath::RandRange(0, SelectedPool->Num() - 1);
    FString ChosenMap = (*SelectedPool)[RandomIdx];
 
    UE_LOG(LogTemp, Warning, TEXT("成功抽取地图: %s"), *ChosenMap);
    return ChosenMap;
}
void UCabinGameInstance::HostCabinSession(int32 MaxPlayers)
{
    if (!SessionInterface.IsValid() || bSessionBusy) return;
    bSessionBusy = true;
    FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
    PendingMaxPlayers = MaxPlayers;
    if (ExistingSession)
    {
        SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
            FOnDestroySessionCompleteDelegate::CreateUObject(this, &UCabinGameInstance::OnDestroySessionComplete));
        SessionInterface->DestroySession(NAME_GameSession);
        return;
    }
    ActualCreateSession(PendingMaxPlayers);
}
 
void UCabinGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    ActualCreateSession(PendingMaxPlayers);
}
 
void UCabinGameInstance::ActualCreateSession(int32 MaxPlayers)
{
    FOnlineSessionSettings Settings;
    Settings.bIsLANMatch = false;
    Settings.NumPublicConnections = MaxPlayers;
    Settings.bAllowJoinInProgress = true;
    Settings.bUsesPresence = true;
    Settings.bShouldAdvertise = true;
    Settings.bUseLobbiesIfAvailable = true;
 
    SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &UCabinGameInstance::OnCreateSessionComplete));
    SessionInterface->CreateSession(0, NAME_GameSession, Settings);
}
 
void UCabinGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    bSessionBusy = false;
    if (bWasSuccessful)
        GetWorld()->ServerTravel(TEXT("/Game/Maps/CabinMap?listen"));
}
 
void UCabinGameInstance::FindAndJoinForestSession()
{
    if (!SessionInterface.IsValid()) return;
    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    SessionSearch->bIsLanQuery = false;
    SessionSearch->QuerySettings.Set(FName(TEXT("SEARCH_PRESENCE")), true, EOnlineComparisonOp::Equals);
    SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
        FOnFindSessionsCompleteDelegate::CreateUObject(this, &UCabinGameInstance::OnFindSessionsComplete));
    SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}
 
void UCabinGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
    if (bWasSuccessful && SessionSearch->SearchResults.Num() > 0)
    {
        SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
            FOnJoinSessionCompleteDelegate::CreateUObject(this, &UCabinGameInstance::OnJoinSessionComplete));
        SessionInterface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[0]);
    }
}
 
void UCabinGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if (Result == EOnJoinSessionCompleteResult::Success)
    {
        if (APlayerController* PC = GetFirstLocalPlayerController())
        {
            FString ConnectString;
            if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
                PC->ClientTravel(ConnectString, TRAVEL_Absolute);
        }
    }
}

void UCabinGameInstance::DeleteSaveData()
{
    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
    {
        UGameplayStatics::DeleteGameInSlot(SaveSlotName, 0);
        
        // 同时清除当前内存中的标签，防止本局游戏继续持有旧状态
        UnlockedTags.Reset();
        
        UE_LOG(LogTemp, Warning, TEXT("存档文件已成功消除（Elimination）！"));
    }
}
 
bool UCabinGameInstance::PurchaseFacility(FString FacilityTag, int32 Cost)
{
    if (GlobalMaterialCount >= Cost)
    {
        GlobalMaterialCount -= Cost;
        
        // 1. 记录解锁数据
        UnlockedTags.AddUnique(FacilityTag); 
        
        // 2. ！！！新增：发送实时解锁广播 ！！！
        // 这行代码会立刻通知场景中所有监听该标签的 BaseFacility
        OnFacilityUnlocked.Broadcast(FacilityTag);
        
        // 3. 通知 UI 更新金钱显示
        OnMaterialAmountChanged.Broadcast(GlobalMaterialCount);
        
        // 4. 强制存盘
        SaveProgress(); 
        
        UE_LOG(LogTemp, Log, TEXT("设施购买成功并已广播信号: %s"), *FacilityTag);
        return true;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("余额不足，无法购买: %s"), *FacilityTag);
    return false;
}
void UCabinGameInstance::AddMaterialsToGlobal(int32 Amount)
{
    // 这里可以根据难度或天赋添加倍率逻辑，比如：Amount * 1.2f
    GlobalMaterialCount += Amount;
    
    // 可以在这里调用保存存档的逻辑
    UE_LOG(LogTemp, Log, TEXT("结算成功：存入 %d，当前总资产: %d"), Amount, GlobalMaterialCount);
}

 
void UCabinGameInstance::Init() 
{
    Super::Init();
 
    // 【关键修复】：必须在这里初始化 SessionInterface，否则按钮点击后逻辑会直接 Return
    if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
    {
        SessionInterface = Subsystem->GetSessionInterface();
        UE_LOG(LogTemp, Warning, TEXT("GameInstance: Online Subsystem Initialized."));
    }
 
    LoadProgress();
}
 
void UCabinGameInstance::AddToTeamBank(int32 Amount)
{
    GlobalMaterialCount += Amount;
    OnMaterialAmountChanged.Broadcast(GlobalMaterialCount);
    SaveProgress();
}
 
void UCabinGameInstance::SaveProgress()
{
    if (UCabinSaveGame* SaveObj = Cast<UCabinSaveGame>(UGameplayStatics::CreateSaveGameObject(UCabinSaveGame::StaticClass())))
    {
        SaveObj->SavedMaterialCount = GlobalMaterialCount;
        SaveObj->SavedUnlockedTags = UnlockedTags; // 保存标签
        UGameplayStatics::SaveGameToSlot(SaveObj, SaveSlotName, 0);
    }
}
 
void UCabinGameInstance::LoadProgress()
{
    if (UCabinSaveGame* Loaded = Cast<UCabinSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0)))
    {
        GlobalMaterialCount = Loaded->SavedMaterialCount;
        UnlockedTags = Loaded->SavedUnlockedTags; // 加载标签
        OnMaterialAmountChanged.Broadcast(GlobalMaterialCount);
    }
}
FString UCabinGameInstance::GetNextMapPath()
{
    if (MapPool.Num() == 0) return TEXT("/Game/Maps/DefaultMap");
 
    // 逻辑：按顺序循环地图池，每完成一轮，难度自动提升
    int32 MapIndex = CurrentLevelIndex % MapPool.Num();
    
    // 如果玩家转了一圈又回来了，可以额外增加难度奖励感
    if (CurrentLevelIndex > 0 && MapIndex == 0)
    {
        CurrentDifficultyMultiplier += 0.5f; // 每一轮额外变难
    }
 
    return MapPool[MapIndex];
}