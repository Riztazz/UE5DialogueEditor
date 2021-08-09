
#include "Slate/SDialogueLineDetails.h"
#include "Slate/SDialogueTreeView.h"
#include "DialogueEditor.h"
#include <Widgets/Layout/SScrollBox.h>
#include <Widgets/Layout/SBorder.h>
#include <Widgets/Text/STextBlock.h>
#include <Widgets/Input/STextComboBox.h>
#include "Widgets/SWidget.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "GameFramework/Character.h"
#include "STypes.h"

void SDialogueLineDetails::Construct(const FArguments& InArgs)
{
	GEditor->RegisterForUndo(this);

	dialogueEditorInstance = InArgs._DialogueEditorInstance;

	if (dialogueEditorInstance.IsValid())
	{
		// do anything you need from Dialogue Editor Instance object
	}

	FSlateApplication::Get().OnFocusChanging().AddSP(this, &SDialogueLineDetails::OnGlobalFocusChanging);

	ChildSlot[
		SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(5.0f)
			.AutoHeight()[
				SNew(SBorder)
					.BorderBackgroundColor(FColor(192, 192, 192, 255))
					.Padding(15.0f)[
						SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.Padding(5.0f)
							.AutoHeight()[
								SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.Padding(25.0f, 10.0f, 25.0f, 10.0f)
									.AutoWidth()[
										SNew(STextBlock)
											.Text(FText::FromString(TEXT("Line")))]
									+ SHorizontalBox::Slot()
											.Padding(5.0f)
											.AutoWidth()[
												SNew(SBorder)
													.BorderBackgroundColor(FColor(192, 192, 192, 255))]
											+ SHorizontalBox::Slot()
													.Padding(25.0f, 10.0f, 25.0f, 10.0f)
													.AutoWidth()[
														SNew(STextBlock)
															.Text(FText::FromString(TEXT("Speaker")))]
													+ SHorizontalBox::Slot()
															.Padding(5.0f)
															.MaxWidth(400.f)[
																SNew(SComboButton)
																	.ContentPadding(FMargin(2.0f))
																	.OnGetMenuContent(this, &SDialogueLineDetails::GenerateDropDownOwner)
																	.IsEnabled_Lambda([this] { return  dialogueEditorInstance.Pin()->DialogueTreeView->GetCurrentlySelectedLine() && !dialogueEditorInstance.Pin()->DialogueTreeView->GetCurrentlySelectedLine()->IsPlayer && dialogueEditorInstance.Pin()->DialogueTreeView->GetCurrentlySelectedLine()->LinkToLineIndex == INDEX_NONE; })
																	.ButtonContent()[
																		SAssignNew(TextBlockSpeaker, STextBlock)
																			.Text(FText::FromName(DialogueLineSpeakerName))]]
															+ SHorizontalBox::Slot()
																			.Padding(5.0f)
																			.AutoWidth()[
																				SNew(SBorder)
																					.BorderBackgroundColor(FColor(192, 192, 192, 255))]
																			+ SHorizontalBox::Slot()
																					.Padding(25.0f, 10.0f, 25.0f, 10.0f)
																					.AutoWidth()[
																						SNew(STextBlock)
																							.Text(FText::FromString(TEXT("Ambient")))]
																					+ SHorizontalBox::Slot()
																							.Padding(5.0f)
																							.AutoWidth()[
																								SAssignNew(CheckBoxAmbient, SCheckBox)
																									.Type(ESlateCheckBoxType::CheckBox)
																									.IsEnabled(this, &SDialogueLineDetails::IsEnabledAmbient)
																									.IsChecked(this, &SDialogueLineDetails::IsCheckedAmbient)
																									.OnCheckStateChanged(this, &SDialogueLineDetails::ToggleAmbient)]
																							+ SHorizontalBox::Slot()
																									.Padding(5.0f)
																									.AutoWidth()[
																										SNew(SBorder)
																											.BorderBackgroundColor(FColor(192, 192, 192, 255))]
																									+ SHorizontalBox::Slot()
																											.Padding(25.0f, 10.0f, 25.0f, 10.0f)
																											.AutoWidth()[
																												SNew(STextBlock)
																													.Text(FText::FromString(TEXT("Comment")))]
																											+ SHorizontalBox::Slot()
																													.Padding(5.0f)
																													.AutoWidth()[
																														SAssignNew(CheckBoxComment, SCheckBox)
																															.Type(ESlateCheckBoxType::CheckBox)
																															.IsEnabled(this, &SDialogueLineDetails::IsEnabledComment)
																															.IsChecked(this, &SDialogueLineDetails::IsCheckedComment)
																															.OnCheckStateChanged(this, &SDialogueLineDetails::ToggleComment)]]
							+ SVerticalBox::Slot()
																															.Padding(5.0f)
																															.AutoHeight()[
																																SAssignNew(SlateEditableTextBoxLine, SEditableTextBox)
																																	.Text(FText::FromString(""))
																																	.OnTextChanged(this, &SDialogueLineDetails::OnTextChangedLine)
																																	.OnTextCommitted(this, &SDialogueLineDetails::OnTextCommittedLine)
																																	.IsReadOnly_Lambda([this]()
																																		{
																																			return !IsSelectedValid();
																																		})]]]
			+ SVerticalBox::Slot()
																																			.Padding(5.0f)
																																			.AutoHeight()[
																																				SNew(SBorder)
																																					.BorderBackgroundColor(FColor(192, 192, 192, 255))
																																					.Padding(15.0f)[
																																						SNew(SVerticalBox)
																																							+ SVerticalBox::Slot()
																																							.Padding(5.0f)
																																							.AutoHeight()[
																																								SNew(SHorizontalBox)
																																									+ SHorizontalBox::Slot()
																																									.Padding(20.0f, 5.0f, 20.0f, 5.f)
																																									.AutoWidth()[
																																										SNew(STextBlock)
																																											.Text(FText::FromString(TEXT("Condition")))]
																																									+ SHorizontalBox::Slot()
																																											.Padding(5.0f)
																																											.AutoWidth()[
																																												SAssignNew(CheckBoxCondition, SCheckBox)
																																													.Type(ESlateCheckBoxType::CheckBox)
																																													.IsEnabled(this, &SDialogueLineDetails::IsEnabledCondition)
																																													.IsChecked(this, &SDialogueLineDetails::IsCheckedCondition)
																																													.OnCheckStateChanged(this, &SDialogueLineDetails::ToggleCondition)]
																																											+ SHorizontalBox::Slot()
																																													.Padding(5.0f)
																																													.AutoWidth()[
																																														SNew(STextBlock)
																																															.Text(FText::FromString(TEXT("true/false flag")))]]
																																							+ SVerticalBox::Slot()
																																															.Padding(5.0f)
																																															.AutoHeight()[
																																																SAssignNew(PlotsComboCondition, STextComboBox)
																																																	.IsEnabled(this, &SDialogueLineDetails::IsSelectedValid)
																																																	.OptionsSource(&dialogueEditorInstance.Pin()->PlotsCondition)
																																																	.InitiallySelectedItem(dialogueEditorInstance.Pin()->PlotsCondition[0])
																																																	.OnSelectionChanged(this, &SDialogueLineDetails::OnSelectedPlotChangedCondition)]
																																															+ SVerticalBox::Slot()
																																																	.Padding(5.0f)
																																																	.AutoHeight()[
																																																		SAssignNew(PlotFlagsComboCondition, STextComboBox)
																																																			.IsEnabled(this, &SDialogueLineDetails::IsSelectedValid)
																																																			.OptionsSource(&dialogueEditorInstance.Pin()->PlotFlagsCondition)
																																																			.InitiallySelectedItem(dialogueEditorInstance.Pin()->PlotFlagsCondition[0])
																																																			.OnSelectionChanged(this, &SDialogueLineDetails::OnSelectedPlotFlagChangedCondition)]]]
																																			+ SVerticalBox::Slot()
																																																			.Padding(5.0f)
																																																			.AutoHeight()[
																																																				SNew(SBorder)
																																																					.BorderBackgroundColor(FColor(192, 192, 192, 255))
																																																					.Padding(15.0f)[
																																																						SNew(SVerticalBox)
																																																							+ SVerticalBox::Slot()
																																																							.Padding(5.0f)
																																																							.AutoHeight()[
																																																								SNew(SHorizontalBox)
																																																									+ SHorizontalBox::Slot()
																																																									.Padding(20.0f, 5.0f, 20.0f, 5.f)
																																																									.AutoWidth()[
																																																										SNew(STextBlock)
																																																											.Text(FText::FromString(TEXT("Action")))]
																																																									+ SHorizontalBox::Slot()
																																																											.Padding(5.0f)
																																																											.AutoWidth()[
																																																												SAssignNew(CheckBoxAction, SCheckBox)
																																																													.Type(ESlateCheckBoxType::CheckBox)
																																																													.IsEnabled(this, &SDialogueLineDetails::IsEnabledAction)
																																																													.IsChecked(this, &SDialogueLineDetails::IsCheckedAction)
																																																													.OnCheckStateChanged(this, &SDialogueLineDetails::ToggleAction)]
																																																											+ SHorizontalBox::Slot()
																																																													.Padding(5.0f)
																																																													.AutoWidth()[
																																																														SNew(STextBlock)
																																																															.Text(FText::FromString(TEXT("set/clear flag")))]]
																																																							+ SVerticalBox::Slot()
																																																															.Padding(5.0f)
																																																															.AutoHeight()[
																																																																SAssignNew(PlotsComboAction, STextComboBox)
																																																																	.IsEnabled(this, &SDialogueLineDetails::IsSelectedValid)
																																																																	.OptionsSource(&dialogueEditorInstance.Pin()->PlotsAction)
																																																																	.InitiallySelectedItem(dialogueEditorInstance.Pin()->PlotsAction[0])
																																																																	.OnSelectionChanged(this, &SDialogueLineDetails::OnSelectedPlotChangedAction)]
																																																															+ SVerticalBox::Slot()
																																																																	.Padding(5.0f)
																																																																	.AutoHeight()[
																																																																		SAssignNew(PlotFlagsComboAction, STextComboBox)
																																																																			.IsEnabled(this, &SDialogueLineDetails::IsSelectedValid)
																																																																			.OptionsSource(&dialogueEditorInstance.Pin()->PlotFlagsAction)
																																																																			.InitiallySelectedItem(dialogueEditorInstance.Pin()->PlotFlagsAction[0])
																																																																			.OnSelectionChanged(this, &SDialogueLineDetails::OnSelectedPlotFlagChangedAction)]]]
	];
}

void SDialogueLineDetails::OnGlobalFocusChanging(const FFocusEvent& FocusEvent, const FWeakWidgetPath& OldFocusedWidgetPath, const TSharedPtr<SWidget>& OldFocusedWidget, const FWidgetPath& NewFocusedWidgetPath, const TSharedPtr<SWidget>& NewFocusedWidget)
{
	if (ScopedTransaction)
	{
		delete ScopedTransaction;
		ScopedTransaction = nullptr;
		CurrentOpHash = INDEX_NONE;
	}
}

void SDialogueLineDetails::OnSelectedPlotChangedCondition(TSharedPtr<FString> InPlot, ESelectInfo::Type SelectInfo)
{
	FString sPlot = InPlot ? *InPlot.Get() : StringEmpty;

	if (sPlot != StringEmpty)
	{
		UE_LOG(LogTemp, Warning, TEXT("Selected Condition Plot %s"), *sPlot);
	}

	dialogueEditorInstance.Pin()->PlotFlagsCondition.Empty();
	dialogueEditorInstance.Pin()->PlotFlagsCondition.Add(MakeShareable(new FString(StringEmpty)));

	if (sPlot != StringEmpty)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetData;
		AssetRegistryModule.Get().GetAssetsByClass(UDataTable::StaticClass()->GetFName(), AssetData);

		for (const FAssetData& Asset : AssetData)
		{
			UDataTable* DataTable = Cast<UDataTable>(Asset.GetAsset());
			if (DataTable->GetRowStruct()->IsChildOf(FTablePlotFlags::StaticStruct()) && DataTable->GetName() == sPlot)
			{
				for (const TPair<FName, uint8*>& row : DataTable->GetRowMap())
				{
					dialogueEditorInstance.Pin()->PlotFlagsCondition.Add(MakeShareable(new FString(row.Key.ToString())));
				}
			}
		}
	}

	if (!dialogueEditorInstance.Pin().Get()->IsLoadingDialogue())
	{
		dialogueEditorInstance.Pin().Get()->DialogueTreeView->GetCurrentDialogueLinePtr()->PlotCondition = sPlot == StringEmpty ? "" : sPlot;
	}

	PlotFlagsComboCondition->ClearSelection();
	PlotFlagsComboCondition->RefreshOptions();
	PlotFlagsComboCondition->SetSelectedItem(dialogueEditorInstance.Pin()->PlotFlagsCondition[0]);

	if (SelectInfo != ESelectInfo::Direct)
	{
		dialogueEditorInstance.Pin()->DialogueDirtyDelegate.ExecuteIfBound();
	}
}

void SDialogueLineDetails::OnSelectedPlotFlagChangedCondition(TSharedPtr<FString> InPlotFlag, ESelectInfo::Type SelectInfo)
{
	if (InPlotFlag)
	{
		if (SelectInfo != ESelectInfo::Direct)
		{
			FString sPlotFlag = *InPlotFlag.Get();

			dialogueEditorInstance.Pin().Get()->DialogueTreeView->GetCurrentDialogueLinePtr()->PlotConditionFlag = sPlotFlag == StringEmpty ? "" : sPlotFlag;

			UE_LOG(LogTemp, Warning, TEXT("Selected Condition Plot Flag is %s"), *sPlotFlag);
			dialogueEditorInstance.Pin()->DialogueDirtyDelegate.ExecuteIfBound();
		}
	}
}

void SDialogueLineDetails::OnSelectedPlotChangedAction(TSharedPtr<FString> InPlot, ESelectInfo::Type SelectInfo)
{
	if (SelectInfo != ESelectInfo::Direct)
	{
		FString sPlot = InPlot ? *InPlot.Get() : StringEmpty;
		UE_LOG(LogTemp, Warning, TEXT("Selected Action Plot %s"), *sPlot);

		dialogueEditorInstance.Pin()->PlotFlagsAction.Empty();
		dialogueEditorInstance.Pin()->PlotFlagsAction.Add(MakeShareable(new FString(StringEmpty)));

		if (sPlot != StringEmpty)
		{
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			TArray<FAssetData> AssetData;
			AssetRegistryModule.Get().GetAssetsByClass(UDataTable::StaticClass()->GetFName(), AssetData);

			for (const FAssetData& Asset : AssetData)
			{
				UDataTable* DataTable = Cast<UDataTable>(Asset.GetAsset());
				if (DataTable->GetRowStruct()->IsChildOf(FTablePlotFlags::StaticStruct()) && DataTable->GetName() == sPlot)
				{
					for (const TPair<FName, uint8*>& row : DataTable->GetRowMap())
					{
						dialogueEditorInstance.Pin()->PlotFlagsAction.Add(MakeShareable(new FString(row.Key.ToString())));
					}
				}
			}
		}

		if (!dialogueEditorInstance.Pin().Get()->IsLoadingDialogue())
		{
			dialogueEditorInstance.Pin().Get()->DialogueTreeView->GetCurrentDialogueLinePtr()->PlotAction = sPlot == StringEmpty ? "" : sPlot;
		}

		PlotFlagsComboAction->ClearSelection();
		PlotFlagsComboAction->RefreshOptions();
		PlotFlagsComboAction->SetSelectedItem(dialogueEditorInstance.Pin()->PlotFlagsAction[0]);

		dialogueEditorInstance.Pin()->DialogueDirtyDelegate.ExecuteIfBound();
	}
}

void SDialogueLineDetails::OnSelectedPlotFlagChangedAction(TSharedPtr<FString> InPlotFlag, ESelectInfo::Type SelectInfo)
{
	if (InPlotFlag)
	{
		if (SelectInfo != ESelectInfo::Direct)
		{
			FString sPlotFlag = *InPlotFlag.Get();

			dialogueEditorInstance.Pin().Get()->DialogueTreeView->GetCurrentDialogueLinePtr()->PlotActionFlag = sPlotFlag == StringEmpty ? "" : sPlotFlag;

			UE_LOG(LogTemp, Warning, TEXT("Selected Action Plot Flag is %s"), *sPlotFlag);
			dialogueEditorInstance.Pin()->DialogueDirtyDelegate.ExecuteIfBound();
		}
	}
}

bool SDialogueLineDetails::IsEnabledAmbient() const
{
	return IsSelectedValid() && !dialogueEditorInstance.Pin()->DialogueTreeView->GetCurrentlySelectedLine().Get()->IsPlayer;
}

ECheckBoxState SDialogueLineDetails::IsCheckedAmbient() const
{
	return bCheckedAmbient ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SDialogueLineDetails::ToggleAmbient(ECheckBoxState NewState)
{
	bCheckedAmbient = NewState == ECheckBoxState::Checked ? true : false;
	dialogueEditorInstance.Pin()->DialogueDirtyDelegate.ExecuteIfBound();
}

bool SDialogueLineDetails::IsEnabledComment() const
{
	return IsSelectedValid() && !dialogueEditorInstance.Pin()->DialogueTreeView->GetCurrentlySelectedLine().Get()->IsPlayer;
}

ECheckBoxState SDialogueLineDetails::IsCheckedComment() const
{
	return bCheckedComment ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SDialogueLineDetails::ToggleComment(ECheckBoxState NewState)
{
	bCheckedComment = NewState == ECheckBoxState::Checked ? true : false;
	dialogueEditorInstance.Pin()->DialogueDirtyDelegate.ExecuteIfBound();
}

bool SDialogueLineDetails::IsEnabledCondition() const
{
	return IsSelectedValid() && PlotFlagsComboCondition->GetSelectedItem() != dialogueEditorInstance.Pin()->PlotFlagsCondition[0];
}

ECheckBoxState SDialogueLineDetails::IsCheckedCondition() const
{
	return bCheckedCondition ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SDialogueLineDetails::ToggleCondition(ECheckBoxState NewState)
{
	bCheckedCondition = NewState == ECheckBoxState::Checked ? true : false;
	dialogueEditorInstance.Pin()->DialogueDirtyDelegate.ExecuteIfBound();
}

bool SDialogueLineDetails::IsEnabledAction() const
{
	return IsSelectedValid() && PlotFlagsComboAction->GetSelectedItem() != dialogueEditorInstance.Pin()->PlotFlagsAction[0];
}

ECheckBoxState SDialogueLineDetails::IsCheckedAction() const
{
	return bCheckedAction ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SDialogueLineDetails::ToggleAction(ECheckBoxState NewState)
{
	bCheckedAction = NewState == ECheckBoxState::Checked ? true : false;
	dialogueEditorInstance.Pin()->DialogueDirtyDelegate.ExecuteIfBound();
}

bool SDialogueLineDetails::IsSelectedValid() const
{
	return dialogueEditorInstance.Pin().Get()->GetDialogue()->Entries.Num() > 0 && dialogueEditorInstance.Pin()->DialogueTreeView->IsDetailsAvailable();
}

void SDialogueLineDetails::OnTextChangedLine(const FText& InText)
{
	if (!dialogueEditorInstance.Pin().Get()->GetDialogue()->IsTransacting() && //from undo/redo
		!dialogueEditorInstance.Pin().Get()->IsLoadingDialogue())//from import XML or loading dialogue

	{
		int32 OpIndex = INDEX_NONE;

		if (!ScopedTransaction)
		{
			ScopedTransaction = new FScopedTransaction(NSLOCTEXT("DialogueLineDetailsTab", "OnTextChangedLine", "On Text Changed Line"));
			dialogueEditorInstance.Pin().Get()->GetDialogue()->Modify();

			FDialogueEditorOperation Op = FDialogueEditorOperation(EDialogueOperationType::TEXTEDIT);
			//Op.PayloadStrings.Add("EditableText", InText.ToString());//see done below
			Op.PayloadBools.Add("IsPlayer", dialogueEditorInstance.Pin()->DialogueTreeView->GetCurrentlySelectedLine().Get()->IsPlayer);
			Op.PayloadInts.Add("LineIndex", dialogueEditorInstance.Pin()->DialogueTreeView->GetCurrentlySelectedLine().Get()->LineIndex);

			CurrentOpHash = Op.OpHash;

			dialogueEditorInstance.Pin()->GetDialogue()->AddNewOp(Op);
		}

		OpIndex = dialogueEditorInstance.Pin()->GetDialogue()->GetOpsHistory().IndexOfByPredicate(
			[&](const FDialogueEditorOperation _op)
			{
				return _op.OpHash == CurrentOpHash;//return _op.IsPlayer == Op.IsPlayer && _op.LineIndex == Op.LineIndex;
			});

		check(OpIndex != INDEX_NONE);

		FDialogueEditorOperation* OpPtr = &dialogueEditorInstance.Pin()->GetDialogue()->GetOpsHistory()[OpIndex];
		OpPtr->PayloadStrings.Add("EditableText", InText.ToString());

		dialogueEditorInstance.Pin().Get()->DialogueTreeView->GetCurrentDialogueLinePtr()->TextSpoken = SlateEditableTextBoxLine->GetText().ToString();
		
		dialogueEditorInstance.Pin()->DialogueTreeView->UpdateLinksText();

		if (!dialogueEditorInstance.Pin().Get()->IsLoadingDialogue())
		{
			dialogueEditorInstance.Pin()->DialogueDirtyDelegate.ExecuteIfBound();
		}
	}
}

void SDialogueLineDetails::OnTextCommittedLine(const FText& InText, ETextCommit::Type InCommitType)
{
	UE_LOG(LogTemp, Warning, TEXT("SDialogueLineDetails::OnTextCommittedLine"));
}

TSharedRef<SWidget> SDialogueLineDetails::GenerateDropDownOwner()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SDialogueLineDetails::OnSelectedOwner);
	AssetPickerConfig.bAllowNullSelection = true;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
	AssetPickerConfig.bFocusSearchBoxWhenOpened = false;
	AssetPickerConfig.bShowBottomToolbar = false;
	AssetPickerConfig.SelectionMode = ESelectionMode::Single;

	return ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig);

	//return SNullWidget::NullWidget;
}

void SDialogueLineDetails::OnSelectedOwner(const FAssetData& AssetData)
{
	if (AssetData.GetAsset())
	{
		UBlueprint* BP = Cast<UBlueprint>(AssetData.GetAsset());
		bool bValid = BP->ParentClass->IsChildOf(ACharacter::StaticClass());

		if (bValid)
		{
			DialogueLineSpeakerName = AssetData.GetAsset()->GetFName();
		}
		else
		{
			DialogueLineSpeakerName = *StringInvalid;
		}
	}
	else
	{
		DialogueLineSpeakerName = *StringDefaultSpeaker;
	}

	TextBlockSpeaker->SetText(FText::FromName(DialogueLineSpeakerName));

	FSlateApplication::Get().DismissAllMenus();

	if (!dialogueEditorInstance.Pin().Get()->IsLoadingDialogue())
	{
		dialogueEditorInstance.Pin().Get()->DialogueTreeView->GetCurrentDialogueLinePtr()->Speaker = DialogueLineSpeakerName;
		dialogueEditorInstance.Pin()->DialogueDirtyDelegate.ExecuteIfBound();
	}
}

SDialogueLineDetails::~SDialogueLineDetails()
{
	CurrentOpHash = INDEX_NONE;
	GEditor->UnregisterForUndo(this);
}

void SDialogueLineDetails::PostUndo(bool bSuccess)
{
	dialogueEditorInstance.Pin().Get()->GetDialogue()->SwapLastOp(true);
}

void SDialogueLineDetails::PostRedo(bool bSuccess)
{
	dialogueEditorInstance.Pin().Get()->GetDialogue()->SwapLastOp(false);
}
