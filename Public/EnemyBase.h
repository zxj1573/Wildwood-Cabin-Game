#pragma once
 
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
// 必须确保 generated.h 是最后一个包含
// 1. 包含接口头文件
#include "CabinCombatInterface.h" 
#include "EnemyBase.generated.h"
 
UCLASS()
class CABINGAME_API AEnemyBase : public ACharacter, public ICabinCombatInterface
{
	GENERATED_BODY()
 
public:
	AEnemyBase();
 
	// 将这些移到 public 或保持 protected（修复编译错误后 protected 也会生效）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.0f;
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float BaseDamage = 15.0f;
	// 3. 实现接口函数声明 (NativeEvent 需要后缀 _Implementation)
	virtual void TakeCabinDamage_Implementation(float DamageAmount, const FHitResult& HitResult, AActor* DamageCauser) override;
protected:
	virtual void BeginPlay() override;
 
	// ReplicatedUsing 必须配合 UFUNCTION 
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;
 
	// 网络属性注册
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 
	// 伤害处理
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
 
	UFUNCTION()
	void OnRep_CurrentHealth();
 
	virtual void Die();
 
	UFUNCTION(BlueprintImplementableEvent)
	void OnDeath();
 
	UFUNCTION(BlueprintImplementableEvent)
	void OnHurt();
	// 感官源：让 AI 能感知到这个敌人（如果是内斗）或玩家
	UPROPERTY(VisibleAnywhere, Category = "AI")
	class UAIPerceptionStimuliSourceComponent* StimuliSource;
	// 死亡动画蒙太奇
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals|Animation")
	class UAnimMontage* DeathMontage;
 
	// 受击动画蒙太奇
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals|Animation")
	class UAnimMontage* HitMontage;
 
	// 攻击动画蒙太奇（之后会用到）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals|Animation")
	class UAnimMontage* AttackMontage;
	// 当感知更新时的处理函数
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, struct FAIStimulus Stimulus);
 
	// 攻击判定：撞到玩家就扣血
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
};