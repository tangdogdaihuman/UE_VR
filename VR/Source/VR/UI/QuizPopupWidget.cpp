#include "QuizPopupWidget.h"
#include "QuizComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/BackgroundBlur.h"
#include "Blueprint/WidgetTree.h"

void UQuizPopupWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildUI();
}

void UQuizPopupWidget::BuildUI()
{
    if (!WidgetTree) return;

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
    WidgetTree->RootWidget = Canvas;

    // Dark background
    UBorder* Bg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Bg->SetBrushColor(FLinearColor(0, 0, 0, 0.65f));
    UCanvasPanelSlot* BgSlot = Canvas->AddChildToCanvas(Bg);
    BgSlot->SetAnchors(FAnchors(0, 0, 1, 1));
    BgSlot->SetOffsets(FMargin(0));

    // Centered panel
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Panel->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.08f, 0.95f));
    Panel->SetPadding(FMargin(30, 25));
    UCanvasPanelSlot* PSlot = Canvas->AddChildToCanvas(Panel);
    PSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
    PSlot->SetSize(FVector2D(650, 350));
    PSlot->SetPosition(FVector2D(-325, -175));

    UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Panel->SetContent(VBox);

    // Question
    QuestionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    FSlateFontInfo QFont = QuestionText->GetFont();
    QFont.Size = 26;
    QuestionText->SetFont(QFont);
    QuestionText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    VBox->AddChildToVerticalBox(QuestionText);

    // Options list (text only — answer via keyboard 1/2/3)
    OptionsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    FSlateFontInfo OFont = OptionsText->GetFont();
    OFont.Size = 20;
    OptionsText->SetFont(OFont);
    OptionsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.8f, 1.0f)));
    VBox->AddChildToVerticalBox(OptionsText);

    // Result
    ResultText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    FSlateFontInfo RFont = ResultText->GetFont();
    RFont.Size = 22;
    ResultText->SetFont(RFont);
    ResultText->SetVisibility(ESlateVisibility::Collapsed);
    VBox->AddChildToVerticalBox(ResultText);
}

void UQuizPopupWidget::Setup(UQuizComponent* InQuizComp, const FText& Question, const TArray<FText>& Options)
{
    QuizComp = InQuizComp;

    if (QuestionText)
        QuestionText->SetText(Question);

    if (OptionsText)
    {
        FString OptStr = TEXT("\n");
        for (int32 i = 0; i < Options.Num(); ++i)
        {
            OptStr += FString::Printf(TEXT("  [%d]  %s\n"), i + 1, *Options[i].ToString());
        }
        OptStr += TEXT("\n  Press 1 / 2 / 3 to answer\n");
        OptionsText->SetText(FText::FromString(OptStr));
    }

    if (ResultText)
        ResultText->SetVisibility(ESlateVisibility::Collapsed);
}

void UQuizPopupWidget::ShowResult(const FText& Message, bool bIsSuccess)
{
    if (ResultText)
    {
        ResultText->SetText(Message);
        ResultText->SetColorAndOpacity(FSlateColor(bIsSuccess ? FLinearColor(0.2f, 1.0f, 0.3f) : FLinearColor(1.0f, 0.3f, 0.2f)));
        ResultText->SetVisibility(ESlateVisibility::Visible);
    }
}
