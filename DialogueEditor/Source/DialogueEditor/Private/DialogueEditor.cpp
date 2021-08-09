
#include "DialogueEditor.h"
#include "EditorStyleSet.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SScrollBox.h"
#include "PropertyEditorModule.h"
#include "DialogueEditorModule.h"
#include "Slate/SDialogueTreeView.h"
#include "Slate/SDialogueLineDetails.h"
#include <AssetRegistry/AssetRegistryModule.h>
#include <Engine/DataTable.h>
#include "STypes.h"

#define LOCTEXT_NAMESPACE "DialogueEditor"

const FName FDialogueEditor::ToolkitFName(TEXT("DialogueEditor"));
const FName FDialogueEditor::TabIdDialogueLineDetails(TEXT("DialogueEditor_TabIdDialogueLine"));
const FName FDialogueEditor::TabIdTreeView(TEXT("DialogueEditor_TabIdTreeView"));

void FDialogueEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_DialogueEditor", "Dialogue Editor"));

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(TabIdTreeView, FOnSpawnTab::CreateSP(this, &FDialogueEditor::SpawnTabTreeView))
		.SetDisplayName(LOCTEXT("DialogueEditor_TreeViewTab", "TreeViewTab"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(TabIdDialogueLineDetails, FOnSpawnTab::CreateSP(this, &FDialogueEditor::SpawnTabDialogueLineDetails))
		.SetDisplayName(LOCTEXT("DialogueEditor_DetailsTab", "DetailsTab"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FDialogueEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(TabIdTreeView);
	InTabManager->UnregisterTabSpawner(TabIdDialogueLineDetails);
}

void FDialogueEditor::SetDialogueDirty()
{
	if (!bLoadingDialogue)
	{
		UE_LOG(LogTemp, Warning, TEXT("FDialogueEditor::SetDialogueDirty()"));

		DialogueTreeView->GetCurrentlySelectedLine().Get()->RowDialogueLine = *DialogueTreeView->GetCurrentDialogueLinePtr();

		check(DialogueTreeView->GetEntries().Num() == 1 && DialogueTreeView->GetEntries()[0]->RowLineParent == nullptr && DialogueTreeView->GetEntries()[0]->LineIndex == INT_MAX);

		Dialogue->MarkPackageDirty();
	}
}

void FDialogueEditor::ExecuteDialoguePathPicked(const FString& InPath)
{
	DialogueTreeView->ProcessXML(InPath);
}

void FDialogueEditor::InitDialogueEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UDialogueObject* InDialogue, TSharedPtr<class FUICommandList> DialogueEditorCommands)
{
	/*FLevelEditorModule& levelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	IDialogueEditorModule& dialogueEditorModule = FModuleManager::LoadModuleChecked<IDialogueEditorModule>("DialogueEditor");
	//levelEditorModule.GetGlobalLevelEditorActions()->Append(DialogueEditorCommands.ToSharedRef());*/

	//Cleanup Plots just in case
	PlotsCondition.Empty();
	PlotFlagsCondition.Empty();
	PlotsCondition.Add(MakeShareable(new FString(StringEmpty)));
	PlotFlagsCondition.Add(MakeShareable(new FString(StringEmpty)));

	PlotsAction.Empty();
	PlotFlagsAction.Empty();
	PlotsAction.Add(MakeShareable(new FString(StringEmpty)));
	PlotFlagsAction.Add(MakeShareable(new FString(StringEmpty)));

	//TODO Async?
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetData;
	AssetRegistryModule.Get().GetAssetsByClass(UDataTable::StaticClass()->GetFName(), AssetData);

	for (const FAssetData& Asset : AssetData)
	{
		UDataTable* DataTable = Cast<UDataTable>(Asset.GetAsset());
		if (DataTable->GetRowStruct()->IsChildOf(FTablePlotFlags::StaticStruct()))
		{
			PlotsCondition.Add(MakeShareable(new FString(DataTable->GetName())));
			PlotsAction.Add(MakeShareable(new FString(DataTable->GetName())));
		}
	}

	/*const bool bIsUpdatable = false;
	const bool bAllowFavorites = true;
	const bool bIsLockable = false;*/

	SetDialogue(InDialogue);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	const FName RandomName = FName(*("Standalone_DialogueEditor_Layout_v" + FDateTime::Now().ToString()));
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout(RandomName)
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter()
				->Split
				(
					FTabManager::NewStack()
					->SetHideTabWell(true)
					->AddTab(TabIdTreeView, ETabState::OpenedTab)
				)
			)
			->Split
			(
				FTabManager::NewSplitter()
				->Split
				(
					FTabManager::NewStack()
					->SetHideTabWell(true)
					->AddTab(TabIdDialogueLineDetails, ETabState::OpenedTab)
				)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;

	DialogueDirtyDelegate.BindSP(this, &FDialogueEditor::SetDialogueDirty);
	DialoguePathPicked.BindSP(this, &FDialogueEditor::ExecuteDialoguePathPicked);

	FAssetEditorToolkit::InitAssetEditor(
		Mode,
		InitToolkitHost,
		DialogueEditorAppIdentifier,
		StandaloneDefaultLayout,
		bCreateDefaultStandaloneMenu,
		bCreateDefaultToolbar,
		(UObject*)InDialogue);
}

FDialogueEditor::~FDialogueEditor()
{
	//TODO reset stuff here

	PlotsCondition.Empty();
	PlotFlagsCondition.Empty();
	PlotsAction.Empty();
	PlotFlagsAction.Empty();

	Dialogue->Entries.Empty();
	DialogueTreeView = nullptr;
	CurrentLineDetails = nullptr;
	DockTabDetails = nullptr;

	Dialogue = nullptr;

	GEngine->ForceGarbageCollection(true);
}

FName FDialogueEditor::GetToolkitFName() const
{
	return ToolkitFName;
}

FText FDialogueEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Dialogue Editor");
}

FText FDialogueEditor::GetToolkitToolTipText() const
{
	return LOCTEXT("ToolTip", "Dialogue Editor");
}

FString FDialogueEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "WorldCentricTabPrefix ").ToString();
}

FLinearColor FDialogueEditor::GetWorldCentricTabColorScale() const
{
	return FColor::Red;
}

UDialogueObject* FDialogueEditor::GetDialogue()
{
	return Dialogue;
}

void FDialogueEditor::SetDialogue(UDialogueObject* InDialogue)
{
	Dialogue = InDialogue;
	Dialogue->SetDialogueEditorInstance(MakeShareable(this));
}

TSharedRef<SDockTab> FDialogueEditor::SpawnTabTreeView(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == TabIdTreeView);

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("GenericEditor.Tabs.Properties"))
		.Label(LOCTEXT("GenericDetailsTitle", "Tab"))
		.TabRole(ETabRole::DocumentTab)
		.TabColorScale(GetTabColorScale())[
			SNew(SScrollBox)
				+ SScrollBox::Slot()[
					SAssignNew(DialogueTreeView, SDialogueTreeView)
						.DialogueEditorInstance(SharedThis(this))]
		];
}

TSharedRef<SDockTab> FDialogueEditor::SpawnTabDialogueLineDetails(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == TabIdDialogueLineDetails);

	DockTabDetails = SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("GenericEditor.Tabs.Properties"))
		.Label(LOCTEXT("GenericDetailsTitle", "Tab"))
		.TabRole(ETabRole::DocumentTab)
		.TabColorScale(GetTabColorScale());

	DockTabDetails->SetContent(
		SNew(SScrollBox)
		+ SScrollBox::Slot()[
			DialogueTreeView->GetCurrentlySelectedLine()->DialogueLineDetails.ToSharedRef()]);

	return MakeShareable(DockTabDetails.Get());
}

#undef LOCTEXT_NAMESPACE
