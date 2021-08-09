// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "DialogueEditorModule.h"
#include "DialogueObject.h"
#include "Modules/ModuleManager.h"
#include "Toolkits/IToolkit.h"
#include "DialogueEditor.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "AssetTypeActions_DialogueObject.h"
#include <LevelEditor.h>
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "ClassIconFinder.h"

#define LOCTEXT_NAMESPACE "DialogueEditorActions"

class FDialogueEditorCommands : public TCommands<FDialogueEditorCommands>
{
public:
	FDialogueEditorCommands() : TCommands<FDialogueEditorCommands>
		(
			"DialogueEditor", // Context name for fast lookup
			NSLOCTEXT("DialogueEditor", "DialogueEditor", "Dialogue Editor"), // Localized context name for displaying
			NAME_None, // Parent
			FEditorStyle::GetStyleSetName() // Icon Style Set
			)
	{
	}

	PRAGMA_DISABLE_OPTIMIZATION
		virtual void RegisterCommands() override
	{
		UI_COMMAND(InsertLine, "InsertLine", "Adds new line", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Alt | EModifierKey::Control, EKeys::P));
	}
	PRAGMA_ENABLE_OPTIMIZATION

		TSharedPtr<FUICommandInfo> InsertLine;
};

#undef LOCTEXT_NAMESPACE

const FName DialogueEditorAppIdentifier = FName(TEXT("DialogueEditor"));

#define LOCTEXT_NAMESPACE "FDialogueEditorModule"

/**
 * StaticMesh editor module
 */
class FDialogueEditorModule : public IDialogueEditorModule
{
private:
	TSharedPtr<class FUICommandList> DialogueEditorCommands;

public:
	/** Constructor */
	FDialogueEditorModule() { }

	static void InsertLineTest()
	{
		UE_LOG(LogTemp, Warning, TEXT("InsertLine shortcut pressed!"));
	}

	/**
	* Called right after the module DLL has been loaded and the module object has been created
	*/
	virtual void StartupModule() override
	{
		MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);
		ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_DialogueObject()));

		//Commands
		FDialogueEditorCommands::Register();
		DialogueEditorCommands = MakeShareable(new FUICommandList);
		const FDialogueEditorCommands& Actions = FDialogueEditorCommands::Get();

		DialogueEditorCommands->MapAction(
			Actions.InsertLine,
			FExecuteAction::CreateStatic(&InsertLineTest));

		//slate
		TSharedRef<FSlateStyleSet> StyleSet = MakeShareable(new FSlateStyleSet("DialogueStyle"));

		StyleSet->Set("Dialogue.InsertLineChild", new FSlateImageBrush(FPaths::ProjectPluginsDir() / "DialogueEditor/Content/Slate/Icons/Dialogue_insert_child_16x.png", FVector2D(16.0f, 16.0f)));
		StyleSet->Set("Dialogue.InsertLineSibling", new FSlateImageBrush(FPaths::ProjectPluginsDir() / "DialogueEditor/Content/Slate/Icons/Dialogue_insert_sibling_16x.png", FVector2D(16.0f, 16.0f)));

		FSlateStyleRegistry::RegisterSlateStyle(StyleSet.Get());
	}

	/**
	* Called before the module is unloaded, right before the module object is destroyed.
	*/
	virtual void ShutdownModule() override
	{
		MenuExtensibilityManager.Reset();
		ToolBarExtensibilityManager.Reset();

		if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			// Unregister our created dialogue from the AssetTools
			IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
			for (int32 i = 0; i < CreatedAssetTypeActions.Num(); ++i)
			{
				AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[i].ToSharedRef());
			}
		}

		CreatedAssetTypeActions.Empty();

		//Commands
		FDialogueEditorCommands::Unregister();
	}

	/**
	* Creates a new Dialogue editor for a DialogueObject
	*/
	virtual TSharedRef<IDialogueEditor> CreateDialogueEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UDialogueObject* Dialogue) override
	{
		TSharedRef<FDialogueEditor> NewDialogueEditor(new FDialogueEditor());
		NewDialogueEditor->InitDialogueEditor(Mode, InitToolkitHost, Dialogue, DialogueEditorCommands);
		return NewDialogueEditor;
	}

	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
	{
		AssetTools.RegisterAssetTypeActions(Action);
		CreatedAssetTypeActions.Add(Action);
	}

	/** Gets the extensibility managers for outside entities to extend static mesh editor's menus and toolbars */
	virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager() override { return MenuExtensibilityManager; }
	virtual TSharedPtr<FExtensibilityManager> GetToolBarExtensibilityManager() override { return ToolBarExtensibilityManager; }

private:
	TSharedPtr<FExtensibilityManager> MenuExtensibilityManager;
	TSharedPtr<FExtensibilityManager> ToolBarExtensibilityManager;

	TArray<TSharedPtr<IAssetTypeActions>> CreatedAssetTypeActions;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_GAME_MODULE(FDialogueEditorModule, DialogueEditor);