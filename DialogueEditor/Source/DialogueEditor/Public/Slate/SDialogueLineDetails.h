#pragma once

class FDialogueEditor;
class STextComboBox;

class SDialogueLineDetails : public SCompoundWidget, public FEditorUndoClient
{
	SLATE_BEGIN_ARGS(SDialogueLineDetails) {}
	SLATE_ARGUMENT(TWeakPtr<class FDialogueEditor>, DialogueEditorInstance);
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void OnGlobalFocusChanging(const FFocusEvent& FocusEvent, const FWeakWidgetPath& OldFocusedWidgetPath, const TSharedPtr<SWidget>& OldFocusedWidget, const FWidgetPath& NewFocusedWidgetPath, const TSharedPtr<SWidget>& NewFocusedWidget);
	void OnSelectedPlotChangedCondition(TSharedPtr<FString> InPlot, ESelectInfo::Type SelectInfo);
	void OnSelectedPlotChangedAction(TSharedPtr<FString> InPlot, ESelectInfo::Type SelectInfo);
	void OnSelectedPlotFlagChangedCondition(TSharedPtr<FString> InPlotFlag, ESelectInfo::Type SelectInfo);
	void OnSelectedPlotFlagChangedAction(TSharedPtr<FString> InPlotFlag, ESelectInfo::Type SelectInfo);

	bool IsEnabledAmbient() const;
	ECheckBoxState IsCheckedAmbient() const;
	void ToggleAmbient(ECheckBoxState NewState);
	bool IsEnabledComment() const;
	ECheckBoxState IsCheckedComment() const;
	void ToggleComment(ECheckBoxState NewState);
	bool IsEnabledCondition() const;
	ECheckBoxState IsCheckedCondition() const;
	void ToggleCondition(ECheckBoxState NewState);
	bool IsEnabledAction() const;
	ECheckBoxState IsCheckedAction() const;
	void ToggleAction(ECheckBoxState NewState);

	bool IsSelectedValid() const;

	void OnTextChangedLine(const FText& InText);
	void OnTextCommittedLine(const FText& InText, ETextCommit::Type InCommitType);

	TSharedRef<SWidget> GenerateDropDownOwner();
	void OnSelectedOwner(const FAssetData& AssetData);

public:
	~SDialogueLineDetails();
	/** FEditorUndoClient interface */
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

public:
	TSharedPtr<STextComboBox> PlotsComboCondition;
	TSharedPtr<STextComboBox> PlotFlagsComboCondition;

	TSharedPtr<STextComboBox> PlotsComboAction;
	TSharedPtr<STextComboBox> PlotFlagsComboAction;

	TSharedPtr<SCheckBox> CheckBoxAmbient;
	TSharedPtr<SCheckBox> CheckBoxComment;
	TSharedPtr<SCheckBox> CheckBoxCondition;
	TSharedPtr<SCheckBox> CheckBoxAction;

	TSharedPtr<SEditableTextBox> SlateEditableTextBoxLine = nullptr;

protected:
	TWeakPtr<FDialogueEditor> dialogueEditorInstance;

private:
	bool bCheckedAmbient = false;
	bool bCheckedComment = false;
	bool bCheckedCondition = false;
	bool bCheckedAction = false;

	TSharedPtr<STextBlock> TextBlockSpeaker = nullptr;
	FName DialogueLineSpeakerName = *StringDefaultSpeaker;

	FScopedTransaction* ScopedTransaction;
	int32 CurrentOpHash = INDEX_NONE;
};