#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"

class UDialogueObject;

/**
 * Public interface to Dialogue Editor
 */
class IDialogueEditor : public FAssetEditorToolkit
{
public:
	/** Retrieves the current Dialogue. */
	virtual UDialogueObject* GetDialogue() = 0;

	/** Set the current Dialogue. */
	virtual void SetDialogue(UDialogueObject* InDialogue) = 0;
};