	#include "EnvironmentSpawner.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
 
AEnvironmentSpawner::AEnvironmentSpawner()
{
	PrimaryActorTick.bCanEverTick = false; // 生成器不需要 Tick，节省性能
}
 
void AEnvironmentSpawner::BeginPlay()
{
	Super::BeginPlay();
 
	// 只有服务器有权生成物体
	if (HasAuthority())
	{
		ExecuteSpawn();
	}
}
 
void AEnvironmentSpawner::ExecuteSpawn()
{
	if (SpawnPool.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnvironmentSpawner: 资产池为空！请在蓝图中指定类。"));
		return;
	}
 
	for (int32 i = 0; i < SpawnCount; i++)
	{
		// 1. 在圆形范围内计算随机位置 (X, Y)
		FVector RandomOffset = UKismetMathLibrary::RandomPointInBoundingBox(FVector::ZeroVector, FVector(SpawnRadius, SpawnRadius, 0.f));
		FVector SpawnLocation = GetActorLocation() + RandomOffset;
		
		// 2. 贴合地面检测 (Line Trace)
		if (bSnapToGround)
		{
			FHitResult Hit;
			FVector Start = SpawnLocation + FVector(0, 0, 500); // 从上方往下投射射线
			FVector End = SpawnLocation - FVector(0, 0, 1000);
			
			if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
			{
				SpawnLocation = Hit.Location;
			}
		}
 
		// 3. 随机选择一个资产进行生成
		int32 RandomIndex = FMath::RandRange(0, SpawnPool.Num() - 1);
		TSubclassOf<AActor> SelectedClass = SpawnPool[RandomIndex];
 
		if (SelectedClass)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			
			GetWorld()->SpawnActor<AActor>(SelectedClass, SpawnLocation, FRotator::ZeroRotator, Params);
		}
	}
}