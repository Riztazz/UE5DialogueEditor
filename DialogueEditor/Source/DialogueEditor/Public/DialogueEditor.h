#pragma once

#include "CoreMinimal.h"
#include "Toolkits/IToolkitHost.h"
#include "Editor/PropertyEditor/Public/PropertyEditorDelegates.h"
#include "IDialogueEditor.h"

class SDockableTab;
class UDialogueObject;
class SDialogueLineDetails;
class SDialogueTreeView;

/**
 * Declares a delegate that is executed when a file was picked in the SFilePathPicker widget.
 *
 * The first parameter will contain the path to the picked file.
 */
DECLARE_DELEGATE_OneParam(FOnPathPicked, const FString& /*PickedPath*/);

//when dialogue is modified
DECLARE_DELEGATE(FSetDialogueDirty);

/**
 *
 */
class FDialogueEditor : public IDialogueEditor
{
public:

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
	void SetDialogueDirty();
	void ExecuteDialoguePathPicked(const FString& InPath);

	/**
	 * Edits the specified asset object
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	InDialogue			The Dialogue to Edit
	 */
	void InitDialogueEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UDialogueObject* InDialogue, TSharedPtr<class FUICommandList> DialogueEditorCommands);

	/** Destructor */
	virtual ~FDialogueEditor();

	/** Begin IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual bool IsPrimaryEditor() const override { return true; }
	/** End IToolkit interface */

	/** Begin IDialogueEditor initerface */
	virtual UDialogueObject* GetDialogue();
	virtual void SetDialogue(UDialogueObject* InDialogue);
	/** End IDialogueEditor initerface */

	FORCEINLINE void SetLoadingDialogue(bool bLoading) { bLoadingDialogue = bLoading; }
	FORCEINLINE bool IsLoadingDialogue() const { return bLoadingDialogue; }

public:
	/** The name given to all instances of this type of editor */
	static const FName ToolkitFName;

	//Plot
	TArray<TSharedPtr<FString>> PlotsCondition;
	TArray<TSharedPtr<FString>> PlotFlagsCondition;
	TArray<TSharedPtr<FString>> PlotsAction;
	TArray<TSharedPtr<FString>> PlotFlagsAction;

	TSharedPtr<SDialogueLineDetails> CurrentLineDetails;
	TSharedPtr<SDialogueTreeView> DialogueTreeView;
	TSharedPtr<SDockTab> DockTabDetails;

	/** Holds a delegate that is executed when a file was picked. */
	FOnPathPicked DialoguePathPicked;
	FSetDialogueDirty DialogueDirtyDelegate;

private:

	//Dialogue Tree View
	/**	The tab ids for all the tabs used */
	static const FName TabIdTreeView;
	TSharedRef<SDockTab> SpawnTabTreeView(const FSpawnTabArgs& Args);

	//Plot
	/**	The tab ids for all the tabs used */
	static const FName TabIdDialogueLineDetails;
	TSharedRef<SDockTab> SpawnTabDialogueLineDetails(const FSpawnTabArgs& Args);

	/** The Dialogue open within this editor */
	UPROPERTY()
		UDialogueObject* Dialogue = nullptr;
	bool bLoadingDialogue = false;
};