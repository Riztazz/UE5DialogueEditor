// Fill out your copyright notice in the Description page of Project Settings.

#include "DialogueObjectFactory.h"
#include "DialogueObject.h"

UDialogueObjectFactory::UDialogueObjectFactory()
{
	// Provide the factory with information about how to handle our asset
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UDialogueObject::StaticClass();
}

UObject* UDialogueObjectFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	// Create and return a new instance of our DialogueObject object
	UDialogueObject* DialogueObject = NewObject<UDialogueObject>(InParent, Class, Name, Flags);
	return DialogueObject;
}
