#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "STypes.generated.h"

static const FString StringEmpty = "(None)";
static const FString StringInvalid = "(Invalid)";
static const FString StringDefaultSpeaker = "[[OWNER]]";

UENUM()
enum class EDialogueOperationType : uint8
{
	INVALID,
	START,
	DETAILS,
	DROP,
	TEXTEDIT,
	INSERTLINE
};

UENUM()
enum class EDragType : uint8
{
	INVALID,
	MOVE,
	LINK
};

USTRUCT()
struct FDialogueEditorOperation
{
	GENERATED_BODY()

public:
	FDialogueEditorOperation() { SetDialogueLineHash(); }
	FDialogueEditorOperation(const EDialogueOperationType _eOpType) :OpType(_eOpType) { SetDialogueLineHash(); }

	EDialogueOperationType OpType = EDialogueOperationType::INVALID;
	TMap<FName, FString> PayloadStrings;
	TMap<FName, int32> PayloadInts;
	TMap<FName, bool> PayloadBools;

	int32 OpHash = INDEX_NONE;
	bool OpUndo = false;

private:
	void SetDialogueLineHash()
	{
		//FNV32 hash
		int32 _hash = 0x811c9dc5;

		FString sString = FDateTime::Now().ToString();
		for (char cc : sString.GetCharArray())
		{
			_hash *= 0x1000193;
			_hash ^= cc;
		}

		OpHash = _hash;
	}
};

USTRUCT(BlueprintType)
struct FTablePlotFlags : public FTableRowBase
{
	GENERATED_BODY()

public:
	//FLAG_NAME is Row Name by default in table

	//int32, MAIN 0-255, DEFINED > 255
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Table")
		int32 FLAG_ID;
};