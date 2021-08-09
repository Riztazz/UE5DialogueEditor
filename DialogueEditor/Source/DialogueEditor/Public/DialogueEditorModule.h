// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Toolkits/AssetEditorToolkit.h"

class IDialogueEditor;
class UDialogueObject;

extern const FName DialogueEditorAppIdentifier;

/**
 * Dialogue editor module interface
 */
class IDialogueEditorModule : public IModuleInterface, public IHasMenuExtensibility, public IHasToolBarExtensibility
{
public:
	/**
	 * Creates a new Dialogue editor.
	 */
	virtual TSharedRef<IDialogueEditor> CreateDialogueEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UDialogueObject* Dialogue) = 0;
};