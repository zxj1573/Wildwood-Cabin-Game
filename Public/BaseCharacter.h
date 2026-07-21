#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InteractionInterface.h"
#include "BaseCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIMessage, const FString&, Message);
// 确保这个委托定义在类上方
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaterialAmountChangedSignature, int32, NewAmount);
// 1. 定义角色手持状态枚举
UENUM(BlueprintType)
enum class ECharacterCombatState : uint8
{
	Unarmed     UMETA(DisplayName = "赤手空拳"),
	Melee       UMETA(DisplayName = "近战武器"),
	Ranged      UMETA(DisplayName = "远端武器")
};
 
UCLASS()
class CABINGAME_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseCharacter();

	// --- 联机核心同步接口 ---
	UFUNCTION(Client, Reliable)
	void Client_SyncPersistentStats(int32 ServerMaterials, float ServerMaxHealth);

	UFUNCTION(Client, Reliable)
	void Client_UpdateMaterialUI(int32 NewTotal);

	UFUNCTION(Client, Reliable)
	void Client_ReceiveUIMessage(const FString& Message);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_InteractGeneric(AActor* TargetActor);
	// 这是给蓝图 Bind Event 用的“发射源”
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMaterialAmountChangedSignature OnMaterialAmountChanged;
	UFUNCTION(Server, Reliable)
	void ServerAddMaterials(int32 Amount);

	UFUNCTION(Server, Reliable)
	void Server_SetSprinting(bool bNewSprinting);
	/** 核心修复：请求服务器同步数据 */
	void RequestInitialSync();
 
	/** 核心修复：服务器 RPC 声明 */
	UFUNCTION(Server, Reliable)
	void Server_RequestStats();
	// --- 修复缺失函数 ---
	UFUNCTION(Client, Reliable, BlueprintCallable)
	void Client_ShowPopup(const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "Rewards")
	void ApplyReward(class UCabinItemData* ItemData);
	UPROPERTY(EditAnywhere, Category = "Attributes")
	float WalkSpeed = 200.f; // 基础走路速度
 
	UPROPERTY(EditAnywhere, Category = "Attributes")
	float SprintSpeed = 600.f; // 冲刺固定速度
	// --- 属性与状态 ---
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, BlueprintReadOnly, Category = "Attributes")
	float CurrentHealth;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentStamina, BlueprintReadOnly, Category = "Attributes")
	float CurrentStamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	float MaxStamina = 100.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Attributes")
	bool bIsExhausted = false;

	// 必须有 (Replicated) 标记！
	// 修改：添加 ReplicatedUsing，这样金钱变动时客户端会自动刷 UI
	// 在 ABaseCharacter 类内修改
	UPROPERTY(ReplicatedUsing = OnRep_InGameMaterials, BlueprintReadOnly, Category = "Inventory")
	int32 InGameMaterials;
	// 在 ABaseCharacter 类中
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnMaterialShortageFeedback(); // 给蓝图实现的“反馈”事件
	UPROPERTY(ReplicatedUsing = OnRep_IsEliminated, BlueprintReadOnly, Category = "Status")
	bool bIsEliminated;
	// 必须有 (Replicated) 标记！
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats")
	float DamageMultiplier = 1.0f;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnUIMessage OnUIMessage;

	// --- 辅助函数 ---
	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetHealthPercent() const { return MaxHealth > 0 ? CurrentHealth / MaxHealth : 0.f; }

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetStaminaPercent() const { return MaxStamina > 0 ? CurrentStamina / MaxStamina : 0.f; }

	UFUNCTION(BlueprintPure, Category = "UI")
	class UPlayerHUDWidget* GetLocalPlayerHUD() const;

	void AddMaterials(int32 Amount);
	// 1. 声明本地按键响应
	void OnKKeyPressed();
 
	// 2. 声明服务器扣血 RPC
	UFUNCTION(Server, Reliable)
	void Server_SuicideTest();
	// 捡起武器的核心接口
	void EquipWeapon(UCabinItemData* WeaponData);
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	void ClearAllHUDPrompts();
	UFUNCTION() void OnRep_CurrentHealth();
	UFUNCTION() void OnRep_CurrentStamina();
	UFUNCTION() void OnRep_InGameMaterials();
	UFUNCTION() void OnRep_IsEliminated();

	void Move(const struct FInputActionValue& Value);
	void Look(const struct FInputActionValue& Value);
	void OnInteractPressed();
	void StartSprinting();
	void StopSprinting();

	UPROPERTY(EditAnywhere, Category = "Attributes")
	float SprintSpeedMultiplier = 1.5f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Status")
	bool bIsSprinting = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals|Animation")
	class UAnimMontage* HitMontage;
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals|Animation")
	class UAnimMontage* DeathMontage;
	// 当前手持状态（用于驱动动画蓝图）
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Combat")
	ECharacterCombatState CombatState = ECharacterCombatState::Unarmed;
 
	// 当前装备在手上的武器 Actor
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AActor* EquippedWeapon;
 
	UFUNCTION()
	void OnRep_EquippedWeapon();
	// --- 动画相关 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	class UAnimMontage* MeleeAttackMontage;
 
	// 攻击函数
	void Attack();

private:
	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* FirstPersonCamera;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* JumpAction;	UPROPERTY(EditAnywhere, Category = "Input")
	
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* SprintAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* AttackAction;
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void HitCheck();
};