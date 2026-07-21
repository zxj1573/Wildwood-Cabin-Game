#include "RewardPickup.h"
#include "Net/UnrealNetwork.h"
#include "Components/SphereComponent.h"
#include "BaseCharacter.h"
#include "CabinItemData.h"

ARewardPickup::ARewardPickup()
{
	bReplicates = true;
	SetReplicateMovement(true);

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	RootComponent = SphereComp;
	SphereComp->SetSphereRadius(80.f);
	SphereComp->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetSimulatePhysics(true); // 开启物理，让它能蹦
}

void ARewardPickup::Initialize(UCabinItemData* InData, int32 InMaterials)
{
	if (HasAuthority())
	{
		ItemData = InData;
		ContainedMaterials = InMaterials;
		OnRep_ItemData();

		// 物理效果：蹦出来（向上且随机偏移的冲量）
		FVector Impulse = FVector(FMath::RandRange(-200, 200), FMath::RandRange(-200, 200), 600.f);
		MeshComp->AddImpulse(Impulse, NAME_None, true);
	}
}

void ARewardPickup::OnRep_ItemData()
{
	if (ItemData && ItemData->PickupMesh)
	{
		MeshComp->SetStaticMesh(ItemData->PickupMesh);
	}
}

void ARewardPickup::NotifyActorBeginOverlap(AActor* OtherActor)
{
	// 在 ARewardPickup::NotifyActorBeginOverlap 中
	if (ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor))
	{
		if (ItemData && ItemData->ItemType == EItemType::Weapon)
		{
			// 自动将捡到的武器装在手上
			Player->EquipWeapon(ItemData);
		}
    
		// ... 增加材料并 Destroy 的逻辑 ...
	}
	if (HasAuthority())
	{
		if (ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor))
		{
			// 应用奖励：如果是武器/赐福等逻辑
			Player->ApplyReward(ItemData);
			// 增加材料
			Player->AddMaterials(ContainedMaterials);
            
			// 拾取后消除 (Elimination)
			Destroy();
		}
	}
}

void ARewardPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARewardPickup, ItemData);
	DOREPLIFETIME(ARewardPickup, ContainedMaterials);
}