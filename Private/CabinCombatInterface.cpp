#include "CabinNotify_WeaponHitbox.h"
#include "Kismet/KismetSystemLibrary.h"
#include "CabinCombatInterface.h"

void UCabinNotify_WeaponHitbox::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	AlreadyHitActors.Empty(); // 每次挥砍开始时清空记录
}

void UCabinNotify_WeaponHitbox::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	// 1. 获取玩家手上挂载的武器（即 BP_Axe）
	TArray<AActor*> AttachedActors;
	Owner->GetAttachedActors(AttachedActors);
	
	for (AActor* WeaponActor : AttachedActors)
	{
		UStaticMeshComponent* WeaponMesh = WeaponActor->FindComponentByClass<UStaticMeshComponent>();
		if (WeaponMesh)
		{
			// 2. 在斧头的 Socket 之间做球体扫描（假设你在斧头 Mesh 里加了 Start 和 End 两个 Socket）
			FVector Start = WeaponMesh->GetSocketLocation("Start");
			FVector End = WeaponMesh->GetSocketLocation("End");
			
			FHitResult HitResult;
			TArray<AActor*> ActorsToIgnore;
			ActorsToIgnore.Add(Owner);
			ActorsToIgnore.Add(WeaponActor);

			bool bHit = UKismetSystemLibrary::SphereTraceSingle(
				Owner, Start, End, TraceRadius, 
				UEngineTypes::ConvertToTraceType(ECC_Pawn), 
				false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, 
				HitResult, true
			);

			// 3. 如果撞到了东西且对方实现了战斗接口
			if (bHit && HitResult.GetActor() && !AlreadyHitActors.Contains(HitResult.GetActor()))
			{
				if (HitResult.GetActor()->Implements<UCabinCombatInterface>())
				{
					// 执行“消除 (Elimination)”相关的伤害逻辑
					ICabinCombatInterface::Execute_TakeCabinDamage(HitResult.GetActor(), DamageBase, HitResult, Owner);
					AlreadyHitActors.Add(HitResult.GetActor());
				}
			}
		}
	}
}