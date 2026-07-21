#include "BaseFacility.h"
#include "Net/UnrealNetwork.h"
#include "BaseCharacter.h"
#include "CabinGameInstance.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"

ABaseFacility::ABaseFacility()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;

	BoardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoardMesh"));
	RootComponent = BoardMesh;

	// 初始状态：不可见且不产生物理阻挡（但射线检测可见，以便显示交互提示）
	BoardMesh->SetVisibility(false);
	BoardMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly); 
	BoardMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoardMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); 
}

void ABaseFacility::BeginPlay()
{
	Super::BeginPlay();

	UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance());
	if (GI)
	{
		// 1. 检查存档：如果该设施已经购买过
		if (GI->UnlockedTags.Contains(FacilityTag))
		{
			bIsBuilt = true;
			OnRep_IsBuilt(); // 直接显现
		}
		else
		{
			// 2. 【核心优化】：消除定时器，改为订阅 GameInstance 的解锁事件
			// 当你在主控台购买设施并执行 Broadcast 时，这里会立即响应
			GI->OnFacilityUnlocked.AddDynamic(this, &ABaseFacility::HandleFacilityUnlocked);
		}
	}

	// 客户端初次加载时确保显示状态同步
	OnRep_IsBuilt();
}

// 【新增】：处理解锁信号的回调
void ABaseFacility::HandleFacilityUnlocked(FString UnlockedTag)
{
	// 校验广播出来的标签是否属于本设施
	if (UnlockedTag == FacilityTag)
	{
		bIsBuilt = true;
		
		// 立即刷新显示（服务器执行）
		OnRep_IsBuilt();

		// 既然设施已经建成，就消除 (Eliminate) 订阅，节省性能开销
		if (UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance()))
		{
			GI->OnFacilityUnlocked.RemoveDynamic(this, &ABaseFacility::HandleFacilityUnlocked);
		}
		
		UE_LOG(LogTemp, Log, TEXT("设施 [%s]：检测到解锁信号，已成功建造。"), *FacilityTag);
	}
}

void ABaseFacility::OnRep_IsBuilt()
{
	if (!BoardMesh) return;

	// 记录当前是否处于隐藏状态，用于判断是否需要播放“刚建好”的特效
	bool bWasHidden = !BoardMesh->IsVisible();
    
	BoardMesh->SetVisibility(bIsBuilt);

	if (bIsBuilt)
	{
		// 解锁后开启完整的物理碰撞
		BoardMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		BoardMesh->SetCollisionResponseToAllChannels(ECR_Block);

		// 仅在显现的那一瞬间播放视觉和音效反馈（不在专用服务器执行）
		if (bWasHidden && GetNetMode() != NM_DedicatedServer)
		{
			if (BuildEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BuildEffect, GetActorLocation());
			}
			if (BuildSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, BuildSound, GetActorLocation());
			}
		}
	}
	else
	{
		// 未解锁状态：维持半透明阴影/隐藏状态且无物理阻挡
		BoardMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BoardMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		BoardMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
}

void ABaseFacility::Interact_Implementation(ABaseCharacter* PlayerCharacter)
{
	// 如果已经建成，子类可以扩展交互逻辑（如回血、补弹）
	if (!PlayerCharacter || bIsBuilt) return;

	// 这里的服务器逻辑仅作为备份购买方式
	if (HasAuthority())
	{
		UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance());
		if (GI && GI->PurchaseFacility(FacilityTag, 100))
		{
			bIsBuilt = true;
			OnRep_IsBuilt(); 
			GI->SaveProgress();
		}
	}
}

void ABaseFacility::ExecuteServerInteraction(ABaseCharacter* PlayerCharacter)
{
	// 由子类重写具体的逻辑（如：武器箱给枪，医疗站加血）
}

void ABaseFacility::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// 强制通知，防止客户端因加载顺序问题错过同步
	DOREPLIFETIME_CONDITION_NOTIFY(ABaseFacility, bIsBuilt, COND_None, REPNOTIFY_Always);
}