// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "STypes.h"
#include "DialogueObject.generated.h"

class SDialogueTreeView;
class FDialogueLineRow;
class FDialogueEditor;

USTRUCT()
struct FDialogueTransition
{
	GENERATED_BODY()

public:
	FDialogueTransition() {}
	virtual ~FDialogueTransition() {};

	UPROPERTY()
		int32 LineIndex = INDEX_NONE;
	UPROPERTY()
		bool IsLink = false;

	FORCEINLINE bool operator==(const FDialogueTransition& Other) const
	{
		return LineIndex == Other.LineIndex && IsLink == Other.IsLink;
	}
};

USTRUCT()
struct FDialogueLine
{
	GENERATED_BODY()

public:
	FDialogueLine() {}
	virtual ~FDialogueLine() {};

	//not serialized
	int32 StringID = INDEX_NONE;

	//serialized
	UPROPERTY()
		FString TextSpoken = "";
	UPROPERTY()
		FString TextWheel = "";//TODO
	UPROPERTY()
		bool IsLineSet = false;
	UPROPERTY()
		TArray<FDialogueTransition> TransitionList;
	UPROPERTY()
		bool IsAmbient = false;
	UPROPERTY()
		bool IsComment = false;
	UPROPERTY()
		FName Speaker;
	UPROPERTY()
		FString PlotCondition;
	UPROPERTY()
		FString PlotConditionFlag;
	UPROPERTY()
		bool IsConditionMet = false;
	UPROPERTY()
		FString PlotAction;
	UPROPERTY()
		FString PlotActionFlag;
	UPROPERTY()
		bool IsActionSet = false;

	FORCEINLINE bool operator==(const FDialogueLine& Other) const
	{
		if (TextSpoken != Other.TextSpoken) return false;
		if (TextWheel != Other.TextWheel) return false;
		if (IsLineSet != Other.IsLineSet) return false;
		if (TransitionList.Num() != Other.TransitionList.Num()) return false;
		for (int32 i = 0; i < TransitionList.Num(); i++)
		{
			if (TransitionList[i].LineIndex != Other.TransitionList[i].LineIndex ||
				TransitionList[i].IsLink != Other.TransitionList[i].IsLink) return false;
		}
		if (IsAmbient != Other.IsAmbient) return false;
		if (IsComment != Other.IsComment) return false;
		if (Speaker != Other.Speaker) return false;
		if (PlotCondition != Other.PlotCondition) return false;
		if (PlotConditionFlag != Other.PlotConditionFlag) return false;
		if (IsConditionMet != Other.IsConditionMet) return false;
		if (PlotAction != Other.PlotAction) return false;
		if (PlotActionFlag != Other.PlotActionFlag) return false;
		if (IsActionSet != Other.IsActionSet) return false;

		return true;
	}
};

/**
 *
 */
UCLASS()
class UDialogueObject : public UObject
{
	GENERATED_BODY()

public:
	UDialogueObject();

public:
	//serializable
	UPROPERTY()
		TArray<int32> StartList;
	UPROPERTY()
		TArray<FDialogueLine> NPCLineList;
	UPROPERTY()
		TArray<FDialogueLine> PlayerLineList;

	TArray<TSharedPtr<FDialogueLineRow>> Entries;

public:

	FORCEINLINE bool IsTransacting() const { return bIsTransacting; }
	FORCEINLINE void SetDialogueEditorInstance(TSharedPtr<FDialogueEditor> In) { dialogueEditorInstance = In; }

	FORCEINLINE TArray<FDialogueEditorOperation>& GetOpsHistory() { return OpsHistory; }
	FORCEINLINE void AddNewOp(FDialogueEditorOperation Op)
	{
		OpsHistory.Add(Op);
		CurrentOpIndex = OpsHistory.Num() - 1;
	}

	void SwapLastOp(bool bUndo);

private:
	bool bIsTransacting = false;
	bool bPost = false;
	int32 CurrentOpIndex = INDEX_NONE;
	FString sText;

	TArray<FDialogueEditorOperation> OpsHistory;
	void RestoreOpState(FDialogueEditorOperation Op);

protected:
	TSharedPtr<FDialogueEditor> dialogueEditorInstance;

};
