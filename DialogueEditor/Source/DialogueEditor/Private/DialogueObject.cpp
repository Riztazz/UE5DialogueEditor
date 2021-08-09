// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueObject.h"

UDialogueObject::UDialogueObject()
{
	CurrentOpIndex = OpsHistory.Num() - 1;
}

void UDialogueObject::SwapLastOp(bool bUndo)
{
	//it's doing it twice and messes up, 1 OK, 2 NOT OK
	if (GEditor)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::Editor)
			{
				if (UWorld* World = Context.World())
				{
					FTimerDelegate delayedCallback;
					delayedCallback.BindWeakLambda(World, [this]
						{
							bPost = false;
						});
					FTimerHandle unusedHandle;
					World->GetTimerManager().SetTimer(unusedHandle, delayedCallback, 0.1f, false);

					break;
				}
			}
		}
	}

	if (!bPost)
	{
		if (bUndo)//from Undo To Redo
		{
			FDialogueEditorOperation* OpPtr = &OpsHistory[CurrentOpIndex];
			OpPtr->OpUndo = true;

			check(CurrentOpIndex >= 0 && CurrentOpIndex < OpsHistory.Num());
			RestoreOpState(OpsHistory[CurrentOpIndex]);

			CurrentOpIndex--;
		}
		else//from Redo To Undo
		{
			CurrentOpIndex++;

			FDialogueEditorOperation* OpPtr = &OpsHistory[CurrentOpIndex];
			OpPtr->OpUndo = false;

			check(CurrentOpIndex >= 0 && CurrentOpIndex < OpsHistory.Num());
			RestoreOpState(OpsHistory[CurrentOpIndex]);
		}

		bPost = true;
	}
}

void UDialogueObject::RestoreOpState(FDialogueEditorOperation Op)
{
	FString sBool = Op.OpUndo == true ? "UNDO" : "REDO";

	switch (Op.OpType)
	{
	case EDialogueOperationType::TEXTEDIT:
	{
		bIsTransacting = true;

		FDialogueLine* dialogueLine = nullptr;
		bool IsPlayer = dialogueEditorInstance->DialogueTreeView->GetCurrentlySelectedLine().Get()->IsPlayer;

		if (IsPlayer)
		{
			dialogueLine = &PlayerLineList[dialogueEditorInstance->DialogueTreeView->GetCurrentlySelectedLine().Get()->LineIndex];
		}
		else //NPC
		{
			dialogueLine = &NPCLineList[dialogueEditorInstance->DialogueTreeView->GetCurrentlySelectedLine().Get()->LineIndex];
		}

		dialogueEditorInstance->DialogueTreeView->GetCurrentlySelectedLine().Get()->DialogueLineDetails->SlateEditableTextBoxLine->SetText(FText::FromString(dialogueLine->TextSpoken));

		bIsTransacting = false;

		break;
	}
	case EDialogueOperationType::INSERTLINE:
	{
		break;
	}
	case EDialogueOperationType::START:
	{
		Entries.RemoveAt(Entries.Num() - 1);
		dialogueEditorInstance->DialogueTreeView->GetTreeView().Get()->RequestTreeRefresh();
		dialogueEditorInstance->DialogueTreeView->GetTreeView().Get()->SetSelection(Entries[0], ESelectInfo::Direct);

		UE_LOG(LogTemp, Warning, TEXT("back to START"));
		break;//all OK here though :D
	}
	case EDialogueOperationType::INVALID:
	default:
	{
		check(Op.OpType == EDialogueOperationType::INVALID);

		break;
	}
	}
}
