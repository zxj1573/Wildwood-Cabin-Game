	#include "PlayerHUDWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "TimerManager.h"
 
void UPlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (NotificationText) NotificationText->SetVisibility(ESlateVisibility::Hidden);
	if (InteractionText) InteractionText->SetVisibility(ESlateVisibility::Hidden);
}
// 注意这里添加了 _Implementation 后缀
void UPlayerHUDWidget::UpdateMaterialText_Implementation(int32 NewAmount)
{
	if (InGameMaterialText)
	{
		InGameMaterialText->SetText(FText::AsNumber(NewAmount));
	}
}
void UPlayerHUDWidget::SetPromptText(FText TextToDisplay)
{
	if (InteractionText)
	{
		InteractionText->SetText(TextToDisplay);
		InteractionText->SetVisibility(TextToDisplay.IsEmpty() ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
	}
}
 
void UPlayerHUDWidget::UpdateStatus(float HPPercent, float SPPercent)
{
	if (HealthBar) HealthBar->SetPercent(HPPercent);
	if (StaminaBar) StaminaBar->SetPercent(SPPercent);
}
 

void UPlayerHUDWidget::ShowNotification(const FString& Message)
{
	if (NotificationText)
	{
		NotificationText->SetText(FText::FromString(Message));
		NotificationText->SetVisibility(ESlateVisibility::Visible);
		GetWorld()->GetTimerManager().ClearTimer(NotificationTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(NotificationTimerHandle, this, &UPlayerHUDWidget::HideNotification, 2.0f, false);
	}
}
 
void UPlayerHUDWidget::HideNotification() { if (NotificationText) NotificationText->SetVisibility(ESlateVisibility::Hidden); }