#include "MaterialPickup.h"
#include "BaseCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"

AMaterialPickup::AMaterialPickup()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;

	// 初始化 PickupMesh
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	RootComponent = PickupMesh;
	
	PickupMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

// 显式定义 BeginPlay
void AMaterialPickup::BeginPlay()
{
	Super::BeginPlay();
}

void AMaterialPickup::Interact_Implementation(ABaseCharacter* Interactor)
{
	if (!HasAuthority()) return;
	if (ABaseCharacter* Character = Cast<ABaseCharacter>(Interactor))
	{
		// 使用 MaterialAmount
		Character->ServerAddMaterials(MaterialAmount);
		
		UE_LOG(LogTemp, Log, TEXT("Picked up materials: %d"), MaterialAmount);

		// 拾取后消除（Elimination）该物体
		Destroy();
	}
	
}
void AMaterialPickup::Destroyed()
{
	Super::Destroyed();
 
	if (PickupVFX) {
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PickupVFX, GetActorLocation(), GetActorRotation());
	}
	if (PickupSFX) {
		// 关键：在 PlaySoundAtLocation 的参数中加入 PickupAttenuation
		UGameplayStatics::PlaySoundAtLocation(
			this, 
			PickupSFX, 
			GetActorLocation(), 
			FRotator::ZeroRotator, 
			1.0f, // 音量
			1.0f, // 音调
			0.0f, // 开始时间
			AttenuationSettings); // 传入衰减资产
	}
}