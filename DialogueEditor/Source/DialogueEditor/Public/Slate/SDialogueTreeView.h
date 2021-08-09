#pragma once
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "STypes.h"
#include "Widgets/SWidget.h"

class SDialogueLineDetails;
class UDialogueObject;
class SDialogueTreeViewRow;

namespace LineColumns
{
	static const FName LabelText(TEXT("Text"));
	static const FName LabelLogic(TEXT("Logic"));
	static const FName LabelSpeaker(TEXT("Speaker"));
};

class FDialogueLineRow : public TSharedFromThis<FDialogueLineRow>
{
public:
	FDialogueLineRow(const TWeakPtr<FDialogueEditor> InDialogueEditorInstance) :DialogueEditorInstance(InDialogueEditorInstance) {}

	FDialogueLine RowDialogueLine;//<- NOTE: this gets serialized in DialogueObject

	int32 Indent = -1;
	int32 LineIndex = INDEX_NONE;
	int32 LinkToLineIndex = INDEX_NONE;//the LineIndex of the Original in Player or NPC Lists
	int32 IndexAsLink = INDEX_NONE;//as index in Links of Original
	int32 IndexAsChild = INDEX_NONE;//as index in Children of Parent
	bool IsPlayer = false;
	bool IsSelected = false;
	//bool IsRowCollapsed = false;

	TSharedPtr<FDialogueLineRow> RowLineParent = nullptr;//for easy access to rows in table
	TArray<TSharedPtr<FDialogueLineRow>> RowLineChildren;//for visual rows in table, not dialogue line transition links
	TArray<TSharedPtr<FDialogueLineRow>> Links;

	TSharedPtr<SDialogueLineDetails> DialogueLineDetails = nullptr;
	TSharedPtr<SDialogueTreeViewRow> RowWidget = nullptr;
	TWeakPtr<FDialogueEditor> DialogueEditorInstance = nullptr;
};

class SDialogueTreeViewRow : public SMultiColumnTableRow<TSharedPtr<FDialogueLineRow>>
{
public:

	SLATE_BEGIN_ARGS(SDialogueTreeViewRow) { }
	SLATE_ARGUMENT(TSharedPtr<FDialogueLineRow>, Entry)
		SLATE_EVENT(FOnDragDetected, OnDragDetected)
		SLATE_EVENT(FOnTableRowDragEnter, OnDragEnter)
		SLATE_EVENT(FOnTableRowDragLeave, OnDragLeave)
		SLATE_EVENT(FOnTableRowDrop, OnDrop)
		SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

public:
	TSharedPtr<FDialogueLineRow> Entry;

private:
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	bool IsAncestor(TSharedPtr<FDialogueLineRow> Entry1, TSharedPtr<FDialogueLineRow> Entry2);

	EDragType DragType = EDragType::INVALID;
};

//dragdrop class
class FDialogueLineDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FDialogueLineDragDropOp, FDecoratedDragDropOp)

public:
	TSharedPtr<FDialogueLineRow> Entry;

	/** Constructs the drag drop operation */
	static TSharedRef<FDialogueLineDragDropOp> New(TSharedPtr<FDialogueLineRow> InEntry)
	{
		TSharedRef<FDialogueLineDragDropOp> Operation = MakeShareable(new FDialogueLineDragDropOp());
		Operation->Entry = InEntry;
		Operation->DefaultHoverText = FText::FromString("");
		Operation->CurrentHoverText = FText::FromString("");
		Operation->Construct();

		return Operation;
	}
};
//end dragdrop class

class SDialogueTreeView : public SCompoundWidget, public FEditorUndoClient
{
	SLATE_BEGIN_ARGS(SDialogueTreeView) {}
	SLATE_ARGUMENT(TWeakPtr<class FDialogueEditor>, DialogueEditorInstance)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

private:
	FScopedTransaction* ScopedTransaction;
	FDialogueLine ExistingLine;

	TSharedPtr<STreeView<TSharedPtr<FDialogueLineRow>>> TreeView;
	TSharedPtr<FDialogueLineRow> CurrentlySelectedLine;
	TSharedPtr<FDialogueLineRow> PreviouslySelectedLine;//TODO remove? needed for UNDO?

	bool bIsSibling = false;
	bool bIsDragging = false;

	//when loading from dialogue object
	int32 LineLoadIndex = INDEX_NONE;

	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FDialogueLineRow> Entry, const TSharedRef<STableViewBase>& Table);
	void OnSelectionChanged(TSharedPtr<FDialogueLineRow> Entry, ESelectInfo::Type SelectInfo);
	TSharedPtr<SWidget> OnContextMenuOpening();

	void OnImportFromXML();
	void OnInsertLineSibling();
	void OnDeleteLine(TSharedPtr<FDialogueLineRow> Entry);

	void OnExpansionRecursiveView(TSharedPtr<FDialogueLineRow> Entry, bool bInIsItemExpanded);
	void OnExpansionChangedView(TSharedPtr<FDialogueLineRow> Entry, bool bExpanded);
	void OnLineDoubleClicked(TSharedPtr<FDialogueLineRow> InClickedEntry);

	TSharedPtr<FDialogueLineRow> GetOriginalLine(TSharedPtr<FDialogueLineRow> Entry, int32 LineIndex, bool IsPlayer);
	bool IsValidToImport();

	void NewDialogue();
	void LoadDialogue(bool bImport = false);
	void UpdateLineBranch(TSharedPtr<FDialogueLineRow> Entry, bool bHandleLinks);

	int32 GetIndexOfByKey(TSharedPtr<FDialogueLineRow> Entry);
	void UpdateLineDetails(TSharedPtr<FDialogueLineRow> Entry);
	void UpdatePlotFlagFromFlagId(FDialogueLine& DialogueLine);

protected:
	TWeakPtr<FDialogueEditor> DialogueEditorInstance;

public:
	FDialogueLine* GetCurrentDialogueLinePtr();
	void UpdateLinksText();
	void ProcessXML(const FString& InFile);
	bool IsStartingBranch(TSharedPtr<FDialogueLineRow> Entry);
	TSharedPtr<FDialogueLineRow> GetNewLinkLine(TSharedPtr<FDialogueLineRow> Entry);
	void OnInsertLineChild(TSharedPtr<FDialogueLineRow> FutureParent, TSharedPtr<FDialogueLineRow> FutureChild = nullptr);
	void UpdateDialogue(TSharedPtr<FDialogueLineRow> ParentLine, TSharedPtr<FDialogueLineRow> ChildLine, bool bRemove = false);

public:
	~SDialogueTreeView();
	/** FEditorUndoClient interface */
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

	FORCEINLINE TSharedPtr<STreeView<TSharedPtr<FDialogueLineRow>>> GetTreeView() const { return TreeView; }
	FORCEINLINE TArray<TSharedPtr<FDialogueLineRow>>& GetEntries() { return DialogueEditorInstance.Pin().Get()->GetDialogue()->Entries; }
	FORCEINLINE TSharedPtr<FDialogueLineRow> GetCurrentlySelectedLine() const { return CurrentlySelectedLine; }
	FORCEINLINE void SetDragging(bool bDrag) { bIsDragging = bDrag; }
	FORCEINLINE bool IsDragging() const { return bIsDragging; }
	FORCEINLINE bool IsSelectedRoot() const { return DialogueEditorInstance.Pin().Get()->GetDialogue()->Entries[0] == CurrentlySelectedLine; }
	FORCEINLINE bool IsSelectedLink() const { return CurrentlySelectedLine->LinkToLineIndex != INDEX_NONE ? true : false; }
	FORCEINLINE bool HasTransitionLinks(FDialogueLine line) const { return line.TransitionList.Num() > 0; }
	FORCEINLINE bool IsDetailsAvailable() const { return !IsDragging() && CurrentlySelectedLine && !IsSelectedRoot() && !IsSelectedLink(); }

	//file picker related
private:

	/** Holds the directory path to browse by default. */
	TAttribute<FString> BrowseDirectory;

	/** Holds the title for the browse dialog window. */
	TAttribute<FText> BrowseTitle;

	/** Holds the currently selected file path. */
	TAttribute<FString> FilePath;

	/** Holds the file type filter string. */
	TAttribute<FString> FileTypeFilter;

	/** Holds the editable text box. */
	TSharedPtr<SEditableTextBox> TextBox;
};