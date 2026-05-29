#include "ControlHUDWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"

void UControlHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildUI();
}

void UControlHUDWidget::BuildUI()
{
    if (!WidgetTree) return;

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
    WidgetTree->RootWidget = Canvas;

    UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UCanvasPanelSlot* VBoxSlot = Canvas->AddChildToCanvas(VBox);
    VBoxSlot->SetAnchors(FAnchors(0, 0, 0, 0));
    VBoxSlot->SetPosition(FVector2D(20, 20));
    VBoxSlot->SetAutoSize(true);

    StatusLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    FSlateFontInfo Font = StatusLabel->GetFont();
    Font.Size = 22;
    StatusLabel->SetFont(Font);
    StatusLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    VBox->AddChildToVerticalBox(StatusLabel);

    CueLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Font.Size = 14;
    CueLabel->SetFont(Font);
    CueLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
    VBox->AddChildToVerticalBox(CueLabel);
}

void UControlHUDWidget::SetStatus(const FString& StatusText)
{
    if (StatusLabel) StatusLabel->SetText(FText::FromString(StatusText));
}

void UControlHUDWidget::SetCue(const FString& CueText)
{
    if (CueLabel) CueLabel->SetText(FText::FromString(CueText));
}
