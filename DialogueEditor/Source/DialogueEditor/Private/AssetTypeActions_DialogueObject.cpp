
#include "AssetTypeActions_DialogueObject.h"
#include "DialogueObject.h"
#include "DialogueEditorModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_DialogueObject"

FText FAssetTypeActions_DialogueObject::GetName() const
{
	return NSLOCTEXT("AssetTypeActions_DialogueObject", "AssetTypeActions_DialogueObject", "DialogueObject");
}

FColor FAssetTypeActions_DialogueObject::GetTypeColor() const
{
	return FColor::Magenta;
}

UClass* FAssetTypeActions_DialogueObject::GetSupportedClass() const
{
	return UDialogueObject::StaticClass();
}

void FAssetTypeActions_DialogueObject::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor /*= TSharedPtr<IToolkitHost>()*/)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto DialogueObject = Cast<UDialogueObject>(*ObjIt);
		if (DialogueObject != NULL)
		{
			IDialogueEditorModule* DialogueEditorModule = &FModuleManager::LoadModuleChecked<IDialogueEditorModule>("DialogueEditor");
			DialogueEditorModule->CreateDialogueEditor(Mode, EditWithinLevelEditor, DialogueObject);
		}
	}
}

uint32 FAssetTypeActions_DialogueObject::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

#undef LOCTEXT_NAMESPACE
