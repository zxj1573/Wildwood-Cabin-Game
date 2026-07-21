	#include "EnemyBase.h"
#include "Net/UnrealNetwork.h" // 必须包含！
#include "GameFramework/CharacterMovementComponent.h"
#include "CabinGameInstance.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "EnemyAIController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/DamageEvents.h"
#include "Components/CapsuleComponent.h"
 
AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
	StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
	StimuliSource->RegisterWithPerceptionSystem();
}
void AEnemyBase::TakeCabinDamage_Implementation(float DamageAmount, const FHitResult& HitResult, AActor* DamageCauser)
{
	// 这里将接口收到的伤害直接传给虚幻原生的 TakeDamage
	// 这样就能触发你写好的 CurrentHealth 扣减逻辑和 OnRep 逻辑
	FPointDamageEvent DamageEvent(DamageAmount, HitResult, HitResult.ImpactNormal, nullptr);
	TakeDamage(DamageAmount, DamageEvent, DamageCauser ? DamageCauser->GetInstigatorController() : nullptr, DamageCauser);
    
	// 可以在这里产生一些即时特效，比如火花
	if (HitResult.GetActor())
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), nullptr, HitResult.ImpactPoint); // 之后可以换成具体的粒子
	}
}
void AEnemyBase::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	// 服务器判定伤害
	if (HasAuthority() && OtherActor->ActorHasTag(TEXT("Player")))
	{
		UGameplayStatics::ApplyDamage(OtherActor, BaseDamage, GetController(), this, UDamageType::StaticClass());
	}
}
void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();
 
	if (HasAuthority())
	{
		if (UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance()))
		{
			float Multiplier = GI->CurrentDifficultyMultiplier;
			MaxHealth *= Multiplier;
			CurrentHealth = MaxHealth;
			
			// 强化属性
			BaseDamage *= (1.0f + (Multiplier - 1.0f) * 0.5f);
			GetCharacterMovement()->MaxWalkSpeed *= (1.0f + (Multiplier - 1.0f) * 0.2f);
		}
	}
}
 
// 属性同步注册
void AEnemyBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 
	// 使用 DOREPLIFETIME 注册
	DOREPLIFETIME(AEnemyBase, CurrentHealth);
}
 
float AEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || CurrentHealth <= 0) return 0.0f;
 
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	if (ActualDamage > 0.f)
	{
		CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
		
		// 服务器手动调用一次以确保逻辑触发
		OnRep_CurrentHealth();
		if (HitMontage && CurrentHealth > 0.f)
		{
			PlayAnimMontage(HitMontage);
		}
		if (CurrentHealth <= 0.f)
		{
			Die();
		}
	}
	return ActualDamage;
}
 
void AEnemyBase::OnRep_CurrentHealth()
{
	// 触发受击反馈
	OnHurt();
}
 
void AEnemyBase::Die()
{
	if (!HasAuthority()) return;
 
	OnDeath();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// 如果有死亡动画，就播放它
	if (DeathMontage)
	{
		// 注意：死亡动画通常返回动画长度，我们可以根据动画长度来动态设置销毁时间
		float Duration = PlayAnimMontage(DeathMontage);
		SetLifeSpan(Duration > 0 ? Duration : 2.0f);
	}
	else
	{
		SetLifeSpan(2.0f);
	}
 
	OnDeath();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 停止移动，防止尸体继续滑行
	GetCharacterMovement()->StopMovementImmediately();
}
void AEnemyBase::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// 这里写你的逻辑，没有逻辑也必须保留空函数体
	if (!Actor) return;

	// 示例：感知到目标后的逻辑
	UE_LOG(LogTemp, Log, TEXT("感知到对象：%s"), *Actor->GetName());
}