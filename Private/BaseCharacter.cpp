#include "BaseCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "BaseFacility.h"
#include "CabinGameInstance.h"
#include "CabinPlayerController.h"
#include "PlayerHUDWidget.h"
#include "CabinMainHUD.h"
#include "Engine/DamageEvents.h"
#include "Components/SkeletalMeshComponent.h" // 确保包含此头文件
#include "CabinUpgradeStation.h"
#include "GameFramework/Character.h"
#include "Engine/SkeletalMeshSocket.h"
#include "CabinItemData.h"

ABaseCharacter::ABaseCharacter()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
 
	// 1. 极高频率的网络更新，确保位移包不丢失
	SetNetUpdateFrequency(100.f);      // 替换之前的 NetUpdateFrequency = 100.f;
	SetMinNetUpdateFrequency(33.f);   // 替换之前的 
	NetPriority = 2.0f; 
 
	// 2. 核心修复：解决动画“一顿一顿”和“迈腿慢动作”
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		// 【关键修复】：变量名是 bEnableUpdateRateOptimizations (带s)
		CharacterMesh->bEnableUpdateRateOptimizations = false; 
 
		// 强制始终刷新骨骼：这是解决服务器看客户端“迈腿慢”最核心的设置
		CharacterMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		
		// 开启固定包围盒，防止因关闭 URO 导致的性能大幅下降
		CharacterMesh->bComponentUseFixedSkelBounds = true;
	}
 
	// 3. 核心修复：解决“往回拽”和移动抖动
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// 线性平滑 (Linear) 在高发包频率下表现最稳
		MoveComp->NetworkSmoothingMode = ENetworkSmoothingMode::Linear;
		
		// 允许服务器平滑处理客户端镜像
		MoveComp->bNetworkSkipProxyPredictionOnNetUpdate = false;
 
		// 网格体平滑权重调整，让模型在胶囊体纠偏时更稳
		MoveComp->NetProxyShrinkRadius = 0.01f;
		MoveComp->NetProxyShrinkHalfHeight = 0.01f;
	} 
 
	// 4. 组件与其它设置
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetRootComponent());
	FirstPersonCamera->bUsePawnControlRotation = true;
 
	this->SetNetCullDistanceSquared(900000000.0f); 
	SetReplicateMovement(true); 
	bAlwaysRelevant = true; // 消除距离导致的同步丢失
 
	MaxHealth = 100.f;
	CurrentHealth = MaxHealth;
	CurrentStamina = MaxStamina;
}
// 补上请求函数
void ABaseCharacter::RequestInitialSync()
{
	if (IsLocallyControlled())
	{
		Server_RequestStats(); // 这是一个新的服务器 RPC
	}
}
// 在 .h 中声明并实现这个 RPC
void ABaseCharacter::Server_RequestStats_Implementation()
{
	if (UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance()))
	{
		// 服务器把最新的存档数据推给请求的客户端
		Client_SyncPersistentStats(GI->GlobalMaterialCount, GI->PermanentMaxHealth);
	}
}
void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	// 【核心修复】：如果是本地玩家，等一小会儿确保 UI 构造完成了，再向服务器请求同步
	if (IsLocallyControlled())
	{
		FTimerHandle SyncTimer;
		GetWorldTimerManager().SetTimer(SyncTimer, this, &ABaseCharacter::RequestInitialSync, 0.2f, false);
	}

	if (HasAuthority() && IsPlayerControlled())
	{
		if (UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance()))
		{
			MaxHealth = GI->PermanentMaxHealth;
			CurrentHealth = MaxHealth;
			Client_SyncPersistentStats(GI->GlobalMaterialCount, GI->PermanentMaxHealth);
		}
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ABaseCharacter::Client_SyncPersistentStats_Implementation(int32 ServerMaterials, float ServerMaxHealth)
{
	if (UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance()))
	{
		GI->GlobalMaterialCount = ServerMaterials;
		GI->PermanentMaxHealth = ServerMaxHealth;
		this->MaxHealth = ServerMaxHealth;
		GI->OnMaterialAmountChanged.Broadcast(ServerMaterials);
	}
}

	void ABaseCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
 
    // 只有本地控制的角色才执行射线检测（优化性能，防止服务器替别人检测）
    if (IsLocallyControlled() && FirstPersonCamera)
    {
        FVector Start = FirstPersonCamera->GetComponentLocation();
        FVector End = Start + (FirstPersonCamera->GetForwardVector() * 400.f);
        FHitResult Hit;
        
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);
 
        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
        {
            AActor* HitActor = Hit.GetActor();
            // 检查是否实现了交互接口
            if (HitActor && HitActor->Implements<UInteractionInterface>())
            {
                bool bCanInteract = true;
 
                // 【核心修改】：如果是设施，检查它是否已建成
                // 只有建好了，才显示“按 E 交互”
                if (ABaseFacility* Facility = Cast<ABaseFacility>(HitActor))
                {
                	// 修改为直接访问变量：
                	if (!Facility->bIsBuilt)
                    {
                        bCanInteract = false;
                    }
                }
 
                if (bCanInteract)
                {
                    if (ACabinPlayerController* MyPC = Cast<ACabinPlayerController>(GetController()))
                    {
                        UUserWidget* CurrentHUD = MyPC->GetActiveHUD();
                        FText PromptText = FText::FromString(TEXT("按 [E] 交互"));
 
                        if (UCabinMainHUD* MainHUD = Cast<UCabinMainHUD>(CurrentHUD))
                        {
                            MainHUD->SetPromptText(PromptText);
                        }
                        else if (UPlayerHUDWidget* LevelHUD = Cast<UPlayerHUDWidget>(CurrentHUD))
                        {
                            LevelHUD->SetPromptText(PromptText);
                        }
                    }
                }
                else
                {
                    ClearAllHUDPrompts();
                }
            }
            else
            {
                ClearAllHUDPrompts();
            }
        }
        else
        {
            ClearAllHUDPrompts();
        }
    }
 
    // 体力逻辑保持在服务器运行 (HasAuthority)
    if (HasAuthority() && !bIsEliminated)
    {
        if (bIsSprinting && GetVelocity().Size() > 10.f)
        {
            CurrentStamina = FMath::Clamp(CurrentStamina - (25.f * DeltaTime), 0.f, MaxStamina);
            if (CurrentStamina <= 0.f) 
            { 
                bIsExhausted = true; 
                StopSprinting(); // 确保这个函数内部会修改并同步状态
            }
        }
        else if (CurrentStamina < MaxStamina)
        {
            CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + (12.f * DeltaTime));
            if (bIsExhausted && CurrentStamina >= MaxStamina * 0.2f)
            {
                bIsExhausted = false;
            }
        }
    }
}
// 辅助函数：清除所有可能的 HUD 提示
void ABaseCharacter::ClearAllHUDPrompts()
{
	if (ACabinPlayerController* MyPC = Cast<ACabinPlayerController>(GetController()))
	{
		// 【核心修复】：使用你已经实现的 GetActiveHUD()，它能兼容基地和森林
		UUserWidget* CurrentHUD = MyPC->GetActiveHUD(); 
		if (UCabinMainHUD* MainHUD = Cast<UCabinMainHUD>(CurrentHUD))
			MainHUD->SetPromptText(FText::GetEmpty());
		else if (UPlayerHUDWidget* LevelHUD = Cast<UPlayerHUDWidget>(CurrentHUD))
			LevelHUD->SetPromptText(FText::GetEmpty());
	}
}
void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseCharacter, MaxHealth);
	DOREPLIFETIME(ABaseCharacter, CurrentHealth);
	DOREPLIFETIME(ABaseCharacter, CurrentStamina);
	DOREPLIFETIME(ABaseCharacter, InGameMaterials);
	DOREPLIFETIME(ABaseCharacter, bIsEliminated);
	DOREPLIFETIME(ABaseCharacter, bIsSprinting);
	DOREPLIFETIME(ABaseCharacter, bIsExhausted);
}

void ABaseCharacter::AddMaterials(int32 Amount)
{
	if (HasAuthority())
	{
		InGameMaterials += Amount;
		OnRep_InGameMaterials();
	}
}

void ABaseCharacter::ServerAddMaterials_Implementation(int32 Amount)
{
	AddMaterials(Amount);
}

void ABaseCharacter::Server_InteractGeneric_Implementation(AActor* TargetActor)
{
	// 核心修复：严禁与自己交互，彻底消除栈溢出 (Stack Overflow)
	if (!TargetActor || TargetActor == this) return;
 
	if (TargetActor->Implements<UInteractionInterface>())
	{
		IInteractionInterface::Execute_Interact(TargetActor, this);
	}
}
 
bool ABaseCharacter::Server_InteractGeneric_Validate(AActor* TargetActor)
{
	return true;
}

void ABaseCharacter::Client_UpdateMaterialUI_Implementation(int32 NewTotal)
{
	if (UCabinGameInstance* GI = Cast<UCabinGameInstance>(GetGameInstance()))
	{
		GI->GlobalMaterialCount = NewTotal;
		GI->OnMaterialAmountChanged.Broadcast(NewTotal);
	}
}

float ABaseCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || bIsEliminated) return 0.f;
 
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.f, MaxHealth);
 
	// 播放受击动画（服务器触发，属性同步会确保客户端逻辑）
	if (ActualDamage > 0 && HitMontage && CurrentHealth > 0)
	{
		// 注意：蒙太奇播放通常在本地或通过 Multicast
		PlayAnimMontage(HitMontage); 
	}
 
	if (CurrentHealth <= 0.f)
	{
		bIsEliminated = true;
		OnRep_IsEliminated(); // 服务器主动调用
	}
	return ActualDamage;
}

void ABaseCharacter::OnRep_IsEliminated()
{
	if (bIsEliminated)
	{
		// 1. 禁用移动和输入
		GetCharacterMovement()->DisableMovement();
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			DisableInput(PC);
		}
 
		// 2. 播放死亡动画（如果设置了）
		if (DeathMontage)
		{
			PlayAnimMontage(DeathMontage);
		}
		else
		{
			// 没动画时才隐藏，有动画时保持显示直到消除 (Elimination)
			GetMesh()->SetVisibility(false);
		}
 
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        
		// 3. 5秒后可以考虑重连或回到基地，这里先设置生命周期
		SetLifeSpan(5.0f);
	}
}

UPlayerHUDWidget* ABaseCharacter::GetLocalPlayerHUD() const
{
	if (!IsLocallyControlled())
		return nullptr;

	ACabinPlayerController* PC = Cast<ACabinPlayerController>(GetController());
	return PC ? Cast<UPlayerHUDWidget>(PC->GetPlayerHUD()) : nullptr;
}

void ABaseCharacter::OnRep_InGameMaterials()
{
	// 仅本地控制的玩家处理 UI 更新
	if (!IsLocallyControlled()) return;
 
	// 获取本地 HUD 并更新“本局材料”显示（通常是屏幕右下角的小字）
	if (UPlayerHUDWidget* LevelHUD = GetLocalPlayerHUD())
	{
		LevelHUD->UpdateMaterialText(InGameMaterials);
	}
}
void ABaseCharacter::OnRep_CurrentHealth() {}
void ABaseCharacter::OnRep_CurrentStamina() {}

void ABaseCharacter::StartSprinting()
{
	if (CurrentStamina > 10.f && !bIsExhausted)
	{
		bIsSprinting = true;
		// 【关键】：本地立即修改速度
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed; 
		
		// 仍然发 RPC 只是为了同步变量 bIsSprinting 给其他玩家看，
		// 但位置同步不再依赖它，而是依赖移动包里的速度数据。
		if (!HasAuthority())
			Server_SetSprinting(true);
	}
}
 
void ABaseCharacter::StopSprinting()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	if (!HasAuthority())
		Server_SetSprinting(false);
}


// ====================== 修复：弹出提示 ======================
void ABaseCharacter::Client_ShowPopup_Implementation(const FString& Message)
{
	Client_ReceiveUIMessage(Message);
}

// ====================== 修复：应用奖励 ======================
void ABaseCharacter::ApplyReward(UCabinItemData* ItemData)
{
	if (!ItemData)
		return;

	// 这里可以扩展生命、攻击、材料等奖励
	Client_ReceiveUIMessage(FString::Printf(TEXT("已获得：%s"), *ItemData->ItemName.ToString()));
}

void ABaseCharacter::Server_SetSprinting_Implementation(bool bNewSprinting)
{
	bIsSprinting = bNewSprinting;
	// 【核心修复】：服务器必须同步修改其对应的移动组件速度
	GetCharacterMovement()->MaxWalkSpeed = bNewSprinting ? SprintSpeed : WalkSpeed;
}

void ABaseCharacter::OnInteractPressed()
{
	if (!FirstPersonCamera) return;
 
	FVector S = FirstPersonCamera->GetComponentLocation();
	FVector E = S + (FirstPersonCamera->GetForwardVector() * 400.f);
	FHitResult H;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
 
	if (GetWorld()->LineTraceSingleByChannel(H, S, E, ECC_Visibility, Params))
	{
		AActor* Target = H.GetActor();
		if (Target && Target->Implements<UInteractionInterface>())
		{
			// 发送到通用服务器 RPC
			Server_InteractGeneric(Target);
		}
	}
}
void ABaseCharacter::Client_ReceiveUIMessage_Implementation(const FString& Message)
{
	// 如果收到的消息是“材料不足”，触发抖动反馈
	if (Message.Contains(TEXT("材料不足")))
	{
		OnMaterialShortageFeedback();
	}
	if (UPlayerHUDWidget* HUD = GetLocalPlayerHUD())
		HUD->ShowNotification(Message);
}

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABaseCharacter::Move);
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABaseCharacter::Look);
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &ABaseCharacter::OnInteractPressed);
		EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &ABaseCharacter::StartSprinting);
		EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &ABaseCharacter::StopSprinting);
		EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		// 【新增】：绑定攻击按键 (对应你在 IMC 中设置的鼠标左键)
		EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &ABaseCharacter::Attack);
	}
	// 强制绑定 K 键进行自损测试
	PlayerInputComponent->BindKey(EKeys::K, IE_Pressed, this, &ABaseCharacter::OnKKeyPressed);
}
void ABaseCharacter::OnKKeyPressed()
{
	// 本地打印：确认按键被触发了
	UE_LOG(LogTemp, Warning, TEXT("K键已按下，正在请求服务器扣血..."));
    
	// 调用服务器 RPC
	Server_SuicideTest();
}
 
void ABaseCharacter::Server_SuicideTest_Implementation()
{
	// 只有服务器有权修改血量
	if (HasAuthority())
	{
		FDamageEvent DE;
		// 强制扣除 10 点血。TakeDamage 会处理 CurrentHealth 的减法并触发同步
		this->TakeDamage(10.f, DE, GetController(), this);
        
		UE_LOG(LogTemp, Warning, TEXT("服务器：已执行自损扣血。当前血量: %f"), CurrentHealth);
	}
}
	void ABaseCharacter::Move(const FInputActionValue& Value)
{
	// 1. 获取输入向量 (X 是左右，Y 是前后)
	FVector2D MovementVector = Value.Get<FVector2D>();
 
	if (Controller != nullptr)
	{
		// 2. 找到当前控制器的旋转方向（只取偏航角 Yaw，忽略俯仰和翻滚，防止移动时入地或上天）
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
 
		// 3. 获取该方向下的“前”和“右”单位向量
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
 
		// 4. 应用前后移动 (Y)
		AddMovementInput(ForwardDirection, MovementVector.Y);
 
		// 5. 应用左右移动 (X) —— 这就是你之前缺失的部分
		AddMovementInput(RightDirection, MovementVector.X);
	}
}
void ABaseCharacter::Look(const FInputActionValue& Value)
{
	// 1. 从输入值中提取二维向量 (Vector2D)
	// X 代表鼠标的水平移动 (Yaw)，Y 代表鼠标的垂直移动 (Pitch)
	FVector2D LookAxisVector = Value.Get<FVector2D>();
 
	if (Controller != nullptr)
	{
		// 2. 将 X 轴输入应用到控制器的偏航角 (左右旋转)
		AddControllerYawInput(LookAxisVector.X);
 
		// 3. 将 Y 轴输入应用到控制器的俯仰角 (上下旋转)
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
void ABaseCharacter::EquipWeapon(UCabinItemData* WeaponData)
{
	if (!HasAuthority() || !WeaponData || !WeaponData->ItemActorClass) return;
 
	// 1. 如果手上已经有武器，先销毁旧的
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
	}
 
	// 2. 生成武器 Actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
    
	EquippedWeapon = GetWorld()->SpawnActor<AActor>(WeaponData->ItemActorClass, GetActorTransform(), SpawnParams);
 
	// 3. 将武器挂载到右手插槽 (Socket)
	// 假设你在骨骼上创建了一个叫 "Hand_R_Socket" 的插槽
	const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(FName("Hand_R_Socket"));
	if (HandSocket && EquippedWeapon)
	{
		// 关键函数：将 Actor 吸附到组件的特定插槽
		FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
		EquippedWeapon->AttachToComponent(GetMesh(), AttachmentRules, FName("Hand_R_Socket"));
	}
 
	// 4. 更新状态，让动画蓝图变样
	CombatState = ECharacterCombatState::Melee;
}
 
void ABaseCharacter::OnRep_EquippedWeapon()
{
	// 客户端也需要执行一次挂载逻辑，保证视觉同步
	if (EquippedWeapon)
	{
		FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
		EquippedWeapon->AttachToComponent(GetMesh(), AttachmentRules, FName("Hand_R_Socket"));
	}
}
void ABaseCharacter::Attack()
{
	// 1. 基础状态检查：如果已消除、正在播放蒙太奇、或者手里没武器，就不触发
	if (bIsEliminated || !GetMesh() || GetMesh()->GetAnimInstance()->IsAnyMontagePlaying()) return;
 
	// 2. 根据战斗状态播放对应的动画
	if (CombatState == ECharacterCombatState::Melee && MeleeAttackMontage)
	{
		// 播放近战挥砍动画
		PlayAnimMontage(MeleeAttackMontage);
	}
}
void ABaseCharacter::HitCheck()
{
	// 3. 伤害检测必须由服务器执行
	if (!HasAuthority()) return;
 
	// 获取检测的起点（角色前方 50 厘米）和终点（再往前延申 80 厘米）
	FVector Start = GetActorLocation() + GetActorForwardVector() * 50.f;
	FVector End = Start + GetActorForwardVector() * 80.f;
	float Radius = 60.f; // 挥砍范围半径
 
	TArray<FHitResult> OutHits;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this); // 忽略攻击者自己
 
	// 使用球体检测 (Sphere Sweep)，覆盖范围比射线大，手感更好
	bool bHit = GetWorld()->SweepMultiByChannel(
		OutHits, 
		Start, 
		End, 
		FQuat::Identity, 
		ECC_Pawn, // 只检测 Pawn 类型的物体（敌人）
		FCollisionShape::MakeSphere(Radius), 
		Params
	);
 
	if (bHit)
	{
		for (const FHitResult& Hit : OutHits)
		{
			if (AActor* HitActor = Hit.GetActor())
			{
				// 对命中的目标应用伤害。这会触发目标的 TakeDamage 函数
				UGameplayStatics::ApplyDamage(
					HitActor, 
					20.f * DamageMultiplier, // 基础伤害 * 你的伤害倍率
					GetController(), 
					this, 
					UDamageType::StaticClass()
				);
			}
		}
	}
 
	// 调试绘图：在编辑器运行模式下显示检测球，方便调整距离
#if !UE_BUILD_SHIPPING
	DrawDebugSphere(GetWorld(), Start, Radius, 12, FColor::Orange, false, 1.0f);
#endif
}