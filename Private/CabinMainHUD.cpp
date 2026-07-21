#include "CabinMainHUD.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "Engine/Engine.h" // 用于调试打印
 
void UCabinMainHUD::SetPromptText(FText Text)
{
    // 如果“警告锁定”开启，直接跳过，不让准星逻辑覆盖文字
    if (bIsShowingWarning) return;
 
    if (InteractionText)
    {
        InteractionText->SetText(Text);
        // 确保颜色是默认白色
        InteractionText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        InteractionText->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }
}
 
 
void UCabinMainHUD::ShowGatheringProgress(int32 Current, int32 Total)
{
    // 改为操作 PromptText，不影响 InteractionText
    if (PromptText) 
    {
        FText Message = FText::Format(
            NSLOCTEXT("UI", "Gathering", "目前集结人数: {0} / {1}"),
            FText::AsNumber(Current),
            FText::AsNumber(Total)
        );
 
        PromptText->SetText(Message);
        PromptText->SetVisibility(ESlateVisibility::Visible);
 
        // 3秒后自动清除
        GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ClearPrompt);
        GetWorld()->GetTimerManager().SetTimer(TimerHandle_ClearPrompt, this, &UCabinMainHUD::ClearPrompt, 3.0f, false);
    }
}
 
void UCabinMainHUD::ClearPrompt()
{
    if (PromptText)
    {
        PromptText->SetVisibility(ESlateVisibility::Collapsed);
    }
} 

 
void UCabinMainHUD::RefreshGlobalCount(int32 Count)
{
    if (GlobalMaterialText)
    {
        GlobalMaterialText->SetText(FText::AsNumber(Count));
    }
}
// CabinMainHUD.cpp
void UCabinMainHUD::NativeConstruct()
{
    Super::NativeConstruct();
    
    // 强制初始化，消除随机内存导致的逻辑锁定
    bIsShowingWarning = false; 
    bIsShowingGatheringStatus = false;
    
    if (InteractionText)
    {
        InteractionText->SetText(FText::GetEmpty());
        InteractionText->SetVisibility(ESlateVisibility::Collapsed);
    }
}