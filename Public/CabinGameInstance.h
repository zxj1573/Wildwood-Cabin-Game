#pragma once
 
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "CabinGameInstance.generated.h"
UENUM(BlueprintType)
enum class ELevelPathType : uint8
{
	Normal      UMETA(DisplayName = "普通路径"),
	Hard        UMETA(DisplayName = "困难路径"),
	Elite       UMETA(DisplayName = "精英关卡")
};
// 在类声明之前定义委托

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaterialAmountChanged, int32, NewTotal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFacilityUnlocked, FString, Tag);
UCLASS()
class CABINGAME_API UCabinGameInstance : public UGameInstance
{
	GENERATED_BODY()
 
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	float CurrentDifficultyMultiplier = 1.0f;
	// 永久保存的最大血量（初始设为 100）
	// 将变量改为更加明确的名称
	UPROPERTY(BlueprintReadWrite, Category = "Persistence")
	float PermanentMaxHealth = 100.f;
	/** 
	 * 当前关卡索引 (例如: 1-10 关)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	int32 CurrentLevelIndex = 0;
	
	UFUNCTION(BlueprintCallable, Category = "Settlement")
	void AddMaterialsToGlobal(int32 Amount);
	UCabinGameInstance();
	UFUNCTION(BlueprintCallable, Exec, Category = "Debug")
	void DeleteSaveData();
	UPROPERTY(BlueprintReadWrite, Category = "Roguelike")
	int32 GlobalMaterialCount = 0;
	// 材料数量变化时用于通知UI的动态委托
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnMaterialAmountChanged OnMaterialAmountChanged;
	// 修改这里：去掉括号，并使用正确的变量名 GlobalMaterialCount
	UFUNCTION(BlueprintCallable, Category = "Roguelike")
	void SetGlobalMaterials(int32 NewTotal) 
	{ 
		GlobalMaterialCount = NewTotal; 
		// 记得广播，这样 UI 才会实时跳数字
		OnMaterialAmountChanged.Broadcast(GlobalMaterialCount);
	}
	// 服务器端更新金库并存档
	void AddToTeamBank(int32 Amount);
	   // 内存中的已解锁列表
	UPROPERTY(BlueprintReadOnly)
	TArray<FString> UnlockedTags;
	// 核心函数：处理设施购买
	// 修改此函数以支持属性提升
	bool PurchaseFacility(FString FacilityTag, int32 Cost);
	
	UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	void HostCabinSession(int32 MaxPlayers = 4);
 
	UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	void FindAndJoinForestSession();
 
	// 存档槽位名称
	const FString SaveSlotName = TEXT("PlayerSaveSlot_0");
 
	// 保存当前进度到磁盘
	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	void SaveProgress();
 
	// 从磁盘加载进度
	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	void LoadProgress();
	// --- 新增：地图池定义 ---
	UPROPERTY(EditAnywhere, Category = "Roguelike")
	TArray<FString> MapPool;
 
	// 获取下一张地图的完整路径
	UFUNCTION(BlueprintCallable, Category = "Roguelike")
	FString GetNextMapPath();
 
	UPROPERTY(EditAnywhere, Category = "Roguelike")
	TArray<FString> NormalMapPool;
 
	UPROPERTY(EditAnywhere, Category = "Roguelike")
	TArray<FString> HardMapPool;
 
	UPROPERTY(EditAnywhere, Category = "Roguelike")
	TArray<FString> EliteMapPool;
 
	// 获取指定类型的随机地图
	FString GetRandomMapFromPool(ELevelPathType PathType);
	
	// 【新增】：世界进度 - 精英怪是否已被击败
	UPROPERTY(BlueprintReadWrite, Category = "Progression")
	bool bEliteEliminated = false;
 
	// 【新增】：设施解锁委托
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnFacilityUnlocked OnFacilityUnlocked;
 

protected:
	virtual void Init() override;
    
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
 
	void ActualCreateSession(int32 MaxPlayers);
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
 
	bool bSessionBusy = false;
	int32 PendingMaxPlayers = 4;
};