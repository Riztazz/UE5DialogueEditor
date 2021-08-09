#include "Slate/SDialogueTreeView.h"
#include "Slate/SDialogueLineDetails.h"
#include "SlateOptMacros.h"
#include "ScopedTransaction.h"
#include "XmlParser.h"
#include "DesktopPlatform/Public/DesktopPlatformModule.h"

//SDialogueTreeView
void SDialogueTreeView::Construct(const FArguments& InArgs)
{
	GEditor->RegisterForUndo(this);

	DialogueEditorInstance = InArgs._DialogueEditorInstance;

	if (DialogueEditorInstance.IsValid())
	{
		// do anything you need from Dialogue Editor Instance object
	}

	const auto OnGetChildren = [](TSharedPtr<FDialogueLineRow> InNode, TArray<TSharedPtr<FDialogueLineRow>>& OutChildren)
	{
		OutChildren = InNode->RowLineChildren;
	};

	ChildSlot[
		SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)[
				SAssignNew(TreeView, STreeView<TSharedPtr<FDialogueLineRow>>)
					.TreeItemsSource(&GetEntries())
					.OnGenerateRow(this, &SDialogueTreeView::OnGenerateRow)
					.OnGetChildren_Static(OnGetChildren)
					.OnSelectionChanged(this, &SDialogueTreeView::OnSelectionChanged)
					.OnContextMenuOpening(this, &SDialogueTreeView::OnContextMenuOpening)
					.SelectionMode(ESelectionMode::Single)
					/*.OnSetExpansionRecursive(this, &SDialogueTreeView::OnExpansionRecursiveView)
					.OnExpansionChanged(this, &SDialogueTreeView::OnExpansionChangedView)*/
					.OnMouseButtonDoubleClick(this, &SDialogueTreeView::OnLineDoubleClicked)
					.HeaderRow
					(
						SNew(SHeaderRow)
						+ SHeaderRow::Column(LineColumns::LabelLogic)
						.FillWidth(0.03f)
						.DefaultLabel(NSLOCTEXT("SDialogueTreeView", "Logic", "Logic"))
						+ SHeaderRow::Column(LineColumns::LabelSpeaker)
						.FillWidth(0.05f)
						.DefaultLabel(NSLOCTEXT("SDialogueTreeView", "Speaker", "Speaker"))
						+ SHeaderRow::Column(LineColumns::LabelText)
						.DefaultLabel(NSLOCTEXT("SDialogueTreeView", "Text", "Text"))
					)]
	];

	if (DialogueEditorInstance.Pin().Get()->GetDialogue()->StartList.Num() > 0)
	{
		LoadDialogue();
	}
	else
	{
		NewDialogue();
	}
}

TSharedRef<ITableRow> SDialogueTreeView::OnGenerateRow(TSharedPtr<FDialogueLineRow> Entry, const TSharedRef<STableViewBase>& Table)
{
	TSharedRef<SDialogueTreeViewRow> Row = SNew(SDialogueTreeViewRow, Table)
		.Entry(Entry);
	//.ToolTipText(FText::FromString(Entry->TextSpeaker));

	Entry->RowWidget = Row;

	return Row;
}

void SDialogueTreeView::OnSelectionChanged(TSharedPtr<FDialogueLineRow> Entry, ESelectInfo::Type SelectInfo)
{
	if (!DialogueEditorInstance.Pin().Get()->IsLoadingDialogue())
	{
		if (CurrentlySelectedLine)
		{
			CurrentlySelectedLine->IsSelected = false;
		}

		if (Entry)
		{
			Entry->IsSelected = true;
		}

		PreviouslySelectedLine = CurrentlySelectedLine;
		CurrentlySelectedLine = Entry;

		if (!IsDragging())
		{
			if (CurrentlySelectedLine)
			{
				//update details
				DialogueEditorInstance.Pin()->DockTabDetails->SetContent(
					SNew(SScrollBox)
					+ SScrollBox::Slot()[
						CurrentlySelectedLine->DialogueLineDetails.ToSharedRef()]);
			}
		}
	}
}

TSharedPtr<SWidget> SDialogueTreeView::OnContextMenuOpening()
{
	const bool bCloseAfterSelection = true;

	FMenuBuilder MenuBuilder(bCloseAfterSelection, TSharedPtr<FUICommandList>());

	MenuBuilder.BeginSection(NAME_None);
	{
		if (IsValidToImport())
		{
			const FText ImportLabel = FText::FromString("Import from XML");
			const FText ImportTooltip = FText::FromString("Import from XML (Dragon Age: Origins)");
			MenuBuilder.AddMenuEntry(ImportLabel, ImportTooltip, FSlateIcon("DialogueStyle", "Dialogue.Import"), FUIAction(FExecuteAction::CreateSP(this, &SDialogueTreeView::OnImportFromXML)));
		}

		if (CurrentlySelectedLine && CurrentlySelectedLine->LinkToLineIndex == INDEX_NONE)
		{
			TSharedPtr<FDialogueLineRow> FutureChild = nullptr;
			const FText InsertLineLabelChild = FText::FromString("Insert Line Child");
			const FText InsertLineTooltipChild = FText::FromString("Insert a new line as a child (new level) of the currently selected line");
			MenuBuilder.AddMenuEntry(InsertLineLabelChild, InsertLineTooltipChild, FSlateIcon("DialogueStyle", "Dialogue.InsertLineChild"), FUIAction(FExecuteAction::CreateSP(this, &SDialogueTreeView::OnInsertLineChild, CurrentlySelectedLine, FutureChild)));
		}

		if (CurrentlySelectedLine && CurrentlySelectedLine != GetEntries()[0])
		{
			const FText InsertLineLabelSibling = FText::FromString("Insert Line Sibling");
			const FText InsertLineTooltipSibling = FText::FromString("Insert a new line as a sibling (same level) of the currently selected line");
			MenuBuilder.AddMenuEntry(InsertLineLabelSibling, InsertLineTooltipSibling, FSlateIcon("DialogueStyle", "Dialogue.InsertLineSibling"), FUIAction(FExecuteAction::CreateSP(this, &SDialogueTreeView::OnInsertLineSibling)));
		}

		if (CurrentlySelectedLine && CurrentlySelectedLine != GetEntries()[0])
		{
			const FText InsertLineLabelSibling = FText::FromString("Delete line");
			const FText InsertLineTooltipSibling = FText::FromString("Delete currently selected line and its descendants");
			MenuBuilder.AddMenuEntry(InsertLineLabelSibling, InsertLineTooltipSibling, FSlateIcon("DialogueStyle", "Dialogue.DeleteLine"), FUIAction(FExecuteAction::CreateSP(this, &SDialogueTreeView::OnDeleteLine, CurrentlySelectedLine)));
		}
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SDialogueTreeView::OnImportFromXML()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	if (DesktopPlatform == nullptr)
	{
		return;
	}

	const FString DefaultPath = FPaths::GetPath(FilePath.Get());

	// show the file browse dialog
	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	void* ParentWindowHandle = (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid())
		? ParentWindow->GetNativeWindow()->GetOSWindowHandle()
		: nullptr;

	TArray<FString> OutFiles;

	if (DesktopPlatform->OpenFileDialog(ParentWindowHandle, BrowseTitle.Get().ToString(), DefaultPath, TEXT(""), FileTypeFilter.Get(), EFileDialogFlags::None, OutFiles))
	{
		DialogueEditorInstance.Pin()->DialoguePathPicked.ExecuteIfBound(OutFiles[0]);
	}
}

void SDialogueTreeView::OnInsertLineSibling()
{
	bIsSibling = true;

	if (CurrentlySelectedLine->RowLineParent)
	{
		TreeView->SetSelection(CurrentlySelectedLine->RowLineParent, ESelectInfo::Direct);
		OnInsertLineChild(CurrentlySelectedLine, nullptr);
	}
	else //TODO assume all OK? extra check for StartingList?
	{
		TreeView->SetSelection(GetEntries()[0], ESelectInfo::Direct);
		OnInsertLineChild(GetEntries()[0], nullptr);
	}
}

void SDialogueTreeView::OnDeleteLine(TSharedPtr<FDialogueLineRow> Entry)
{
	if (Entry->RowLineParent)
	{
		int32 nChildIndex = Entry->RowLineParent->RowLineChildren.IndexOfByKey(Entry);
		check(nChildIndex != INDEX_NONE);
		//check(nChildIndex == Entry->IndexAsChild);//TODO doublecheck if needed @sa if (ChildLine->IndexAsChild == INDEX_NONE) in UpdateDialogue
		Entry->RowLineParent->RowLineChildren.RemoveAt(nChildIndex);
		TreeView->SetSelection(Entry->RowLineParent, ESelectInfo::Direct);
	}
	else
	{
		TreeView->SetSelection(GetEntries()[0], ESelectInfo::Direct);
	}

	//handle dialogue object	
	if (Entry->LinkToLineIndex != INDEX_NONE) //if link, remove from original too
	{
		TSharedPtr<FDialogueLineRow> OriginalLine = GetOriginalLine(GetEntries()[0], CurrentlySelectedLine->LinkToLineIndex, CurrentlySelectedLine->IsPlayer);
		check(OriginalLine);
		OriginalLine->Links.RemoveAt(Entry->IndexAsLink);
	}
	else //only non links are in Player/NPC lists
	{
		if (Entry->IsPlayer)
		{
			int32 nLineIndex = DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList.IndexOfByKey(Entry->RowDialogueLine);
			check(nLineIndex != INDEX_NONE);

			//remove from NPC parent
			int32 nTransition = DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList[Entry->RowLineParent->LineIndex].TransitionList.IndexOfByPredicate(
				[nLineIndex](const FDialogueTransition _transition)
				{
					return _transition.LineIndex == nLineIndex;
				});
			check(nTransition != INDEX_NONE);

			DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList[Entry->RowLineParent->LineIndex].TransitionList.RemoveAt(nTransition);

			//update indeces for >
			for (int32 i = 0; i < DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList.Num(); ++i)
			{
				FDialogueLine* LineNPC = &DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList[i];
				for (int32 j = 0; j < LineNPC->TransitionList.Num(); ++j)
				{
					FDialogueTransition* _transition = &LineNPC->TransitionList[j];
					if (_transition->LineIndex > nLineIndex)
					{
						--_transition->LineIndex;
					}
				}
			}

			DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList.RemoveAt(nLineIndex);
		}
		else //NPC
		{
			int32 nLineIndex = DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList.IndexOfByKey(Entry->RowDialogueLine);
			check(nLineIndex != INDEX_NONE);

			//remove from Player parent
			int32 nTransition = DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList[Entry->RowLineParent->LineIndex].TransitionList.IndexOfByPredicate(
				[nLineIndex](const FDialogueTransition _transition)
				{
					return _transition.LineIndex == nLineIndex;
				});
			check(nTransition != INDEX_NONE);

			DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList[Entry->RowLineParent->LineIndex].TransitionList.RemoveAt(nTransition);

			//update indeces for >
			for (int32 i = 0; i < DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList.Num(); ++i)
			{
				FDialogueLine* LinePlayer = &DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList[i];
				for (int32 j = 0; j < LinePlayer->TransitionList.Num(); ++j)
				{
					FDialogueTransition* _transition = &LinePlayer->TransitionList[j];
					if (_transition->LineIndex > nLineIndex)
					{
						--_transition->LineIndex;
					}
				}
			}

			DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList.RemoveAt(nLineIndex);

			//handle StartList too here in NPC
			if (IsStartingBranch(Entry)) //remove from starting branch if needed
			{
				int32 nStart = DialogueEditorInstance.Pin().Get()->GetDialogue()->StartList.IndexOfByKey(Entry->LineIndex);
				check(nStart != INDEX_NONE);

				DialogueEditorInstance.Pin().Get()->GetDialogue()->StartList.RemoveAt(nStart);
			}

			for (int32 i = 0; i < DialogueEditorInstance.Pin().Get()->GetDialogue()->StartList.Num(); ++i)
			{
				int32 n = DialogueEditorInstance.Pin().Get()->GetDialogue()->StartList[i];
				if (n > nLineIndex)
				{
					--n;
					DialogueEditorInstance.Pin().Get()->GetDialogue()->StartList[i] = n;
				}
			}
		}
	}

	TreeView->RequestTreeRefresh();

	DialogueEditorInstance.Pin()->DialogueDirtyDelegate.ExecuteIfBound();
}

void SDialogueTreeView::OnExpansionRecursiveView(TSharedPtr<FDialogueLineRow> Entry, bool bInIsItemExpanded)
{
	UE_LOG(LogTemp, Warning, TEXT("TODO SDialogueTreeView::OnExpansionRecursiveView"));//TODO
}

void SDialogueTreeView::OnExpansionChangedView(TSharedPtr<FDialogueLineRow> Entry, bool bExpanded)
{
	UE_LOG(LogTemp, Warning, TEXT("TODO SDialogueTreeView::OnExpansionChangedView"));//TODO
}

void SDialogueTreeView::OnLineDoubleClicked(TSharedPtr<FDialogueLineRow> Entry)
{
	if (Entry->LinkToLineIndex != INDEX_NONE)
	{
		TSharedPtr<FDialogueLineRow> OriginalLine = GetOriginalLine(GetEntries()[0], CurrentlySelectedLine->LinkToLineIndex, CurrentlySelectedLine->IsPlayer);
		check(OriginalLine);

		TreeView->SetSelection(OriginalLine, ESelectInfo::Direct);
		//TreeView->SetItemExpansion(OriginalLine->RowLineParent, true);//TODO expand ancestors	

		UE_LOG(LogTemp, Warning, TEXT("SDialogueTreeView::OnLineDoubleClicked LINK jump to %s "), *OriginalLine->RowDialogueLine.TextSpoken);
	}
	else
	{
		//expand collapse branch
		UE_LOG(LogTemp, Warning, TEXT("TODO SDialogueTreeView::OnLineDoubleClicked Expand/Collapse node"));//TODO
	}
}

TSharedPtr<FDialogueLineRow> SDialogueTreeView::GetOriginalLine(TSharedPtr<FDialogueLineRow> Entry, int32 LineIndex, bool IsPlayer)
{
	if (Entry->LineIndex == LineIndex && Entry->IsPlayer == IsPlayer && Entry->LinkToLineIndex == INDEX_NONE)
	{
		return Entry;
	}
	else
	{
		TSharedPtr<FDialogueLineRow> OriginalLine = nullptr;
		for (int32 c = 0; c < Entry->RowLineChildren.Num(); ++c)
		{
			TSharedPtr<FDialogueLineRow> Child = Entry->RowLineChildren[c];

			if (Child->LinkToLineIndex == INDEX_NONE)
			{
				OriginalLine = GetOriginalLine(Child, LineIndex, IsPlayer);

				if (OriginalLine)
				{
					return OriginalLine;
				}
			}
		}
	}

	return nullptr;
}

bool SDialogueTreeView::IsValidToImport()
{
	return DialogueEditorInstance.Pin().Get()->GetDialogue()->StartList.Num() == 0 ? true : false;
}

void SDialogueTreeView::NewDialogue()
{
	//initial Root entry
	if (GetEntries().Num() == 0)
	{
		FDialogueLineRow* rootDialogueLine = new FDialogueLineRow(DialogueEditorInstance);
		rootDialogueLine->IsPlayer = true;//as a dummy...
		rootDialogueLine->LineIndex = INT_MAX;

		TSharedPtr<FDialogueLineRow> rootDialogueLinePtr = MakeShareable(rootDialogueLine);
		rootDialogueLinePtr->DialogueLineDetails = SNew(SDialogueLineDetails).DialogueEditorInstance(SharedThis(DialogueEditorInstance.Pin().Get()));
		GetEntries().Add(rootDialogueLinePtr);
		//TreeView->SetSelection(rootDialogueLinePtr, ESelectInfo::Direct);
		CurrentlySelectedLine = rootDialogueLinePtr;//TODO manually, UGLY!!!
	}
}

void SDialogueTreeView::LoadDialogue(bool bImport /*= false*/)
{
	UE_LOG(LogTemp, Warning, TEXT("Generate tree from saved or imported dialogue"));

	TSharedPtr<FDialogueLineRow> rootDialogueLinePtr = nullptr;

	if (!bImport)
	{
		FDialogueLineRow* rootDialogueLine = nullptr;
		rootDialogueLine = new FDialogueLineRow(DialogueEditorInstance);
		rootDialogueLine->IsPlayer = true;//as a dummy...
		rootDialogueLine->LineIndex = INT_MAX;

		rootDialogueLinePtr = MakeShareable(rootDialogueLine);
		rootDialogueLinePtr->DialogueLineDetails = SNew(SDialogueLineDetails).DialogueEditorInstance(SharedThis(DialogueEditorInstance.Pin().Get()));
		GetEntries().Add(rootDialogueLinePtr);
		//TreeView->SetSelection(rootDialogueLinePtr, ESelectInfo::Direct);
		CurrentlySelectedLine = rootDialogueLinePtr;//TODO manually, UGLY!!!
	}
	else
	{
		rootDialogueLinePtr = GetEntries()[0];
	}

	//stupid next frame need AAAAAAAAAARRRRRRRRRRRRRRGGGGGGGGGGGGGHHHHHHHHHHHHHH
	/*if (GEditor)
	{
		GEditor->GetTimerManager()->SetTimerForNextTick([rootDialogueLinePtr, this]()
			{
				//dialogueEditorInstance.Pin().Get()->GetDialogue()
				for (int32 nStarting : dialogueEditorInstance.Pin().Get()->GetDialogue()->StartList)
				{
					ExistingLine = dialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList[nStarting];
					OnInsertLineChild();
					CurrentlySelected = rootDialogueLinePtr;
				}
			});
	}*/

	if (GEditor)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::Editor)
			{
				if (UWorld* World = Context.World())
				{
					FTimerDelegate delayedCallback;
					delayedCallback.BindWeakLambda(World, [rootDialogueLinePtr, this]
						{
							DialogueEditorInstance.Pin().Get()->SetLoadingDialogue(true);

							//dialogueEditorInstance.Pin().Get()->GetDialogue()
							for (int32 nStarting : DialogueEditorInstance.Pin().Get()->GetDialogue()->StartList)
							{
								ExistingLine = DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList[nStarting];
								LineLoadIndex = nStarting;
								OnInsertLineChild(GetEntries()[0], nullptr);
								UpdateLineBranch(GetEntries()[0]->RowLineChildren[GetEntries()[0]->RowLineChildren.Num() - 1], false);//newly minted starting line :D

								//reset to Root for next starting branch
								CurrentlySelectedLine = rootDialogueLinePtr;
								TreeView->SetSelection(CurrentlySelectedLine, ESelectInfo::Direct);
							}

							//handle missed linked transitions
							for (TSharedPtr<FDialogueLineRow> Child : GetEntries()[0]->RowLineChildren)
							{
								UpdateLineBranch(Child, true);
							}

							TreeView->SetItemExpansion(rootDialogueLinePtr, true);
							for (TSharedPtr<FDialogueLineRow> Child : rootDialogueLinePtr->RowLineChildren)
							{
								TreeView->SetItemExpansion(Child, false);
							}

							//select Root when done							
							CurrentlySelectedLine = rootDialogueLinePtr;
							TreeView->SetSelection(rootDialogueLinePtr, ESelectInfo::Direct);

							ExistingLine = FDialogueLine();//reset

							DialogueEditorInstance.Pin().Get()->SetLoadingDialogue(false);
						});
					FTimerHandle unusedHandle;
					World->GetTimerManager().SetTimer(unusedHandle, delayedCallback, 0.25f, false);

					break;
				}
			}
		}
	}
}

void SDialogueTreeView::UpdateLineBranch(TSharedPtr<FDialogueLineRow> Entry, bool bHandleLinks)
{
	if (Entry->RowDialogueLine.TransitionList.Num() > 0)
	{
		for (int32 j = 0; j < Entry.Get()->RowDialogueLine.TransitionList.Num(); ++j)
		{
			FDialogueTransition _transition = Entry.Get()->RowDialogueLine.TransitionList[j];

			if (Entry->IsPlayer)
			{
				ExistingLine = DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList[_transition.LineIndex];
			}
			else
			{
				ExistingLine = DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList[_transition.LineIndex];
			}

			LineLoadIndex = _transition.LineIndex;

			if (!bHandleLinks)
			{
				if (!_transition.IsLink)
				{
					OnInsertLineChild(Entry, nullptr);
					UpdateLineBranch(Entry->RowLineChildren[Entry->RowLineChildren.Num() - 1], false);
				}
			}
			else
			{
				if (_transition.IsLink)
				{
					TSharedPtr<FDialogueLineRow> OriginalLine = GetOriginalLine(GetEntries()[0], _transition.LineIndex, !Entry->IsPlayer);
					check(OriginalLine);

					TSharedPtr<FDialogueLineRow> NewLineToLink = GetNewLinkLine(OriginalLine);
					NewLineToLink->IndexAsChild = j;//current transition index
					OnInsertLineChild(Entry, NewLineToLink);
				}
			}
		}
	}

	if (bHandleLinks)
	{
		for (TSharedPtr<FDialogueLineRow> Child : Entry->RowLineChildren)
		{
			UpdateLineBranch(Child, true);
		}
	}
}

int32 SDialogueTreeView::GetIndexOfByKey(TSharedPtr<FDialogueLineRow> Entry)
{
	for (int32 i = 0; i < GetEntries().Num(); ++i)
	{
		TSharedPtr<FDialogueLineRow> Curr = GetEntries()[i];

		if (Curr->LineIndex == Entry->LineIndex &&
			Curr->IsPlayer == Entry->IsPlayer &&
			Curr->LinkToLineIndex == Entry->LinkToLineIndex &&
			Curr->RowLineParent == Entry->RowLineParent &&
			Curr->RowDialogueLine.TextSpoken == Entry->RowDialogueLine.TextSpoken)
		{
			return i;
		}
	}

	return INDEX_NONE;
}

void SDialogueTreeView::UpdateLineDetails(TSharedPtr<FDialogueLineRow> Entry)
{
	Entry->DialogueLineDetails->SlateEditableTextBoxLine->SetText(FText::FromString(Entry->RowDialogueLine.TextSpoken));

	//ambient
	Entry->DialogueLineDetails->ToggleAmbient(Entry->RowDialogueLine.IsAmbient ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);

	//comment
	Entry->DialogueLineDetails->ToggleComment(Entry->RowDialogueLine.IsComment ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);

	//condition plot plot flag true/false
	Entry->DialogueLineDetails->PlotsComboCondition->SetSelectedItem(Entry->RowDialogueLine.PlotCondition.IsEmpty() ? MakeShareable(new FString(StringEmpty)) : MakeShareable(new FString(Entry->RowDialogueLine.PlotCondition)));
	Entry->DialogueLineDetails->PlotFlagsComboCondition->SetSelectedItem(Entry->RowDialogueLine.PlotConditionFlag.IsEmpty() ? MakeShareable(new FString(StringEmpty)) : MakeShareable(new FString(Entry->RowDialogueLine.PlotConditionFlag)));
	Entry->DialogueLineDetails->ToggleCondition(Entry->RowDialogueLine.IsConditionMet ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);

	//action plot plot flag set/clear
	Entry->DialogueLineDetails->PlotsComboAction->SetSelectedItem(Entry->RowDialogueLine.PlotAction.IsEmpty() ? MakeShareable(new FString(StringEmpty)) : MakeShareable(new FString(Entry->RowDialogueLine.PlotAction)));
	Entry->DialogueLineDetails->PlotFlagsComboAction->SetSelectedItem(Entry->RowDialogueLine.PlotActionFlag.IsEmpty() ? MakeShareable(new FString(StringEmpty)) : MakeShareable(new FString(Entry->RowDialogueLine.PlotActionFlag)));
	Entry->DialogueLineDetails->ToggleAction(Entry->RowDialogueLine.IsActionSet ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);

	//TODO

	//speaker
}

void SDialogueTreeView::UpdatePlotFlagFromFlagId(FDialogueLine& DialogueLine)
{
	if (DialogueLine.PlotCondition != StringEmpty)
	{
		int32 nPlotConditionFlag = FCString::Atoi(*DialogueLine.PlotConditionFlag);
		check(nPlotConditionFlag != INDEX_NONE);

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetData;
		AssetRegistryModule.Get().GetAssetsByClass(UDataTable::StaticClass()->GetFName(), AssetData);

		for (const FAssetData& Asset : AssetData)
		{
			UDataTable* DataTable = Cast<UDataTable>(Asset.GetAsset());
			if (DataTable->GetRowStruct()->IsChildOf(FTablePlotFlags::StaticStruct()) && DataTable->GetName() == DialogueLine.PlotCondition)
			{
				static const FString ContextString(TEXT("SDialogueTreeView::UpdatePlotFlagFromFlagId"));

				TArray<FTablePlotFlags*> FlagTableRows;
				DataTable->GetAllRows<FTablePlotFlags>(ContextString, FlagTableRows);

				int32 nIndex = INDEX_NONE;

				for (int32 i = 0; i < FlagTableRows.Num(); ++i)
				{
					FTablePlotFlags* row = FlagTableRows[i];
					for (TFieldIterator<FProperty> PropIt(row->StaticStruct()); PropIt; ++PropIt)
					{
						FProperty* Property = *PropIt;
						if (Property->GetName() == "FLAG_ID")
						{
							FIntProperty* IntProp = CastField<FIntProperty>(Property);
							if (IntProp)
							{
								int32 nPtr = IntProp->GetPropertyValue_InContainer(row);

								if (nPtr == nPlotConditionFlag)
								{
									nIndex = i;
									break;
								}
							}
						}
					}
				}

				check(nIndex != INDEX_NONE);

				int32 nInc = 0;
				for (const TPair<FName, uint8*>& row : DataTable->GetRowMap())
				{
					if (nInc == nIndex)
					{
						DialogueLine.PlotConditionFlag = row.Key.ToString();
						nInc = 0;
						break;
					}

					nInc++;
				}
			}
		}
	}

	if (DialogueLine.PlotAction != StringEmpty)
	{
		int32 nPlotActionFlag = FCString::Atoi(*DialogueLine.PlotActionFlag);
		check(nPlotActionFlag != INDEX_NONE);

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetData;
		AssetRegistryModule.Get().GetAssetsByClass(UDataTable::StaticClass()->GetFName(), AssetData);

		for (const FAssetData& Asset : AssetData)
		{
			UDataTable* DataTable = Cast<UDataTable>(Asset.GetAsset());
			if (DataTable->GetRowStruct()->IsChildOf(FTablePlotFlags::StaticStruct()) && DataTable->GetName() == DialogueLine.PlotAction)
			{
				static const FString ContextString(TEXT("SDialogueTreeView::UpdatePlotFlagFromFlagId"));

				TArray<FTablePlotFlags*> FlagTableRows;
				DataTable->GetAllRows<FTablePlotFlags>(ContextString, FlagTableRows);

				int32 nIndex = INDEX_NONE;

				for (int32 i = 0; i < FlagTableRows.Num(); ++i)
				{
					FTablePlotFlags* row = FlagTableRows[i];
					for (TFieldIterator<FProperty> PropIt(row->StaticStruct()); PropIt; ++PropIt)
					{
						FProperty* Property = *PropIt;
						if (Property->GetName() == "FLAG_ID")
						{
							FIntProperty* IntProp = CastField<FIntProperty>(Property);
							if (IntProp)
							{
								int32 nPtr = IntProp->GetPropertyValue_InContainer(row);

								if (nPtr == nPlotActionFlag)
								{
									nIndex = i;
									break;
								}
							}
						}
					}
				}

				check(nIndex != INDEX_NONE);

				int32 nInc = 0;
				for (const TPair<FName, uint8*>& row : DataTable->GetRowMap())
				{
					if (nInc == nIndex)
					{
						DialogueLine.PlotActionFlag = row.Key.ToString();
						nInc = 0;
						break;
					}

					nInc++;
				}
			}
		}
	}
}

FDialogueLine* SDialogueTreeView::GetCurrentDialogueLinePtr()
{
	if (CurrentlySelectedLine->LineIndex == INT_MAX)
	{
		return nullptr;
	}

	if (CurrentlySelectedLine->IsPlayer)
	{
		return &DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList[CurrentlySelectedLine->LineIndex];
	}
	else
	{
		return &DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList[CurrentlySelectedLine->LineIndex];
	}

	return nullptr;
}

//public
void SDialogueTreeView::UpdateLinksText()
{
	for (TSharedPtr<FDialogueLineRow> ChildLink : CurrentlySelectedLine->Links)
	{
		ChildLink->RowDialogueLine.TextSpoken = CurrentlySelectedLine->RowDialogueLine.TextSpoken;
	}
}

void SDialogueTreeView::ProcessXML(const FString& InFile)
{
	const FXmlFile xmlFile(InFile);
	const FXmlNode* xmlRoot = xmlFile.GetRootNode();
	const FXmlNode* xmlActualRoot = xmlRoot->GetChildrenNodes()[0];
	const TArray<FXmlNode*> xmlRootChildren = xmlActualRoot->GetChildrenNodes();

	for (FXmlNode* node : xmlRootChildren)
	{
		if (node->GetTag().EndsWith("List"))
		{
			//NPC/Player lines
			if (node->GetTag().EndsWith("LineList"))
			{
				if (node->GetTag().StartsWith("NPC"))
				{
					const TArray<FXmlNode*> NPCLinesListChildren = node->GetChildrenNodes();

					for (FXmlNode* nodeNPCLinesAgent : NPCLinesListChildren)
					{
						const TArray<FXmlNode*> NPCLinesAgentChildren = nodeNPCLinesAgent->GetChildrenNodes();

						FDialogueLine ConvNPCLine;
						ConvNPCLine.IsLineSet = true;

						FString sComment = "";

						for (FXmlNode* nodeNPCLinesAgentChild : NPCLinesAgentChildren)
						{
							if (nodeNPCLinesAgentChild->GetTag().EndsWith("List"))
							{
								if (nodeNPCLinesAgentChild->GetTag() == "TransitionList")
								{
									const TArray<FXmlNode*> TransitionListChildrenNPC = nodeNPCLinesAgentChild->GetChildrenNodes();

									if (TransitionListChildrenNPC.Num() > 0)
									{
										for (FXmlNode* nodeTransitionListAgent : TransitionListChildrenNPC)
										{
											const TArray<FXmlNode*> TransitionAgentChildrenNPC = nodeTransitionListAgent->GetChildrenNodes();

											FDialogueTransition TransitionNPC;

											for (FXmlNode* nTransitionAgentChild : TransitionAgentChildrenNPC)
											{
												if (nTransitionAgentChild->GetTag() == "LineIndex")
												{
													TransitionNPC.LineIndex = FCString::Atoi(*nTransitionAgentChild->GetContent());
												}
												else if (nTransitionAgentChild->GetTag() == "IsLink")
												{
													TransitionNPC.IsLink = nTransitionAgentChild->GetContent() == "False" ? false : true;
												}
											}

											ConvNPCLine.TransitionList.Add(TransitionNPC);
										}
									}
								}
							}
							else
							{
								if (nodeNPCLinesAgentChild->GetTag() == "StringID")
									ConvNPCLine.StringID = FCString::Atoi(*nodeNPCLinesAgentChild->GetContent());
								else if (nodeNPCLinesAgentChild->GetTag() == "Comment")
									sComment = nodeNPCLinesAgentChild->GetContent();
								else if (nodeNPCLinesAgentChild->GetTag() == "ConditionPlotURI")
									ConvNPCLine.PlotCondition = nodeNPCLinesAgentChild->GetContent();
								else if (nodeNPCLinesAgentChild->GetTag() == "ConditionPlotFlag")
									ConvNPCLine.PlotConditionFlag = ConvNPCLine.PlotCondition == "" ? "" : nodeNPCLinesAgentChild->GetContent();// FCString::Atoi(*nodeNPCLinesAgentChild->GetContent());
								else if (nodeNPCLinesAgentChild->GetTag() == "ConditionResult")
									ConvNPCLine.IsConditionMet = nodeNPCLinesAgentChild->GetContent() == "False" ? false : true;
								else if (nodeNPCLinesAgentChild->GetTag() == "ActionPlotURI")
									ConvNPCLine.PlotAction = nodeNPCLinesAgentChild->GetContent();
								else if (nodeNPCLinesAgentChild->GetTag() == "ActionPlotFlag")
									ConvNPCLine.PlotActionFlag = ConvNPCLine.PlotAction == "" ? "" : nodeNPCLinesAgentChild->GetContent();// FCString::Atoi(*nodeNPCLinesAgentChild->GetContent());
								else if (nodeNPCLinesAgentChild->GetTag() == "ActionResult")
									ConvNPCLine.IsActionSet = nodeNPCLinesAgentChild->GetContent() == "False" ? false : true;
								else if (nodeNPCLinesAgentChild->GetTag() == "text")
									ConvNPCLine.TextSpoken = nodeNPCLinesAgentChild->GetContent();
								else if (nodeNPCLinesAgentChild->GetTag() == "Speaker")
									ConvNPCLine.Speaker = *nodeNPCLinesAgentChild->GetContent();
								else if (nodeNPCLinesAgentChild->GetTag() == "Ambient")
									ConvNPCLine.IsAmbient = nodeNPCLinesAgentChild->GetContent() == "False" ? false : true;
							}
						}

						if (ConvNPCLine.PlotCondition == "")
						{
							ConvNPCLine.IsConditionMet = false;
						}

						if (ConvNPCLine.PlotAction == "")
						{
							ConvNPCLine.IsActionSet = false;
						}

						if (ConvNPCLine.PlotCondition != "" || ConvNPCLine.PlotAction != "")
						{
							UpdatePlotFlagFromFlagId(ConvNPCLine);
						}

						if (!sComment.IsEmpty() && ConvNPCLine.TextSpoken.IsEmpty())
						{
							ConvNPCLine.IsComment = true;
							ConvNPCLine.TextSpoken = sComment;
						}

						DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList.Add(ConvNPCLine);
					}
				}
				else if (node->GetTag().StartsWith("Player"))
				{
					const TArray<FXmlNode*> PlayerLinesListChildren = node->GetChildrenNodes();

					for (FXmlNode* nodePlayerLinesAgent : PlayerLinesListChildren)
					{
						const TArray<FXmlNode*> PlayerLinesAgentChildren = nodePlayerLinesAgent->GetChildrenNodes();

						FDialogueLine ConvPlayerLine;
						ConvPlayerLine.IsLineSet = true;

						for (FXmlNode* nodePlayerLinesAgentChild : PlayerLinesAgentChildren)
						{
							if (nodePlayerLinesAgentChild->GetTag().EndsWith("List"))
							{
								if (nodePlayerLinesAgentChild->GetTag() == "TransitionList")
								{
									const TArray<FXmlNode*> TransitionListChildrenPlayer = nodePlayerLinesAgentChild->GetChildrenNodes();

									if (TransitionListChildrenPlayer.Num() > 0)
									{
										for (FXmlNode* nodeTransitionListAgent : TransitionListChildrenPlayer)
										{
											const TArray<FXmlNode*> TransitionAgentChildrenPlayer = nodeTransitionListAgent->GetChildrenNodes();

											FDialogueTransition TransitionPlayer;

											for (FXmlNode* nTransitionAgentChild : TransitionAgentChildrenPlayer)
											{
												if (nTransitionAgentChild->GetTag() == "LineIndex")
												{
													TransitionPlayer.LineIndex = FCString::Atoi(*nTransitionAgentChild->GetContent());
												}
												else if (nTransitionAgentChild->GetTag() == "IsLink")
												{
													TransitionPlayer.IsLink = nTransitionAgentChild->GetContent() == "False" ? false : true;
												}
											}

											ConvPlayerLine.TransitionList.Add(TransitionPlayer);
										}
									}
								}
							}
							else
							{
								if (nodePlayerLinesAgentChild->GetTag() == "StringID")
									ConvPlayerLine.StringID = FCString::Atoi(*nodePlayerLinesAgentChild->GetContent());
								else if (nodePlayerLinesAgentChild->GetTag() == "ConditionPlotURI")
									ConvPlayerLine.PlotCondition = nodePlayerLinesAgentChild->GetContent();
								else if (nodePlayerLinesAgentChild->GetTag() == "ConditionPlotFlag")
									ConvPlayerLine.PlotConditionFlag = ConvPlayerLine.PlotCondition == "" ? "" : nodePlayerLinesAgentChild->GetContent();// FCString::Atoi(*nodePlayerLinesAgentChild->GetContent());
								else if (nodePlayerLinesAgentChild->GetTag() == "ConditionResult")
									ConvPlayerLine.IsConditionMet = nodePlayerLinesAgentChild->GetContent() == "False" ? false : true;
								else if (nodePlayerLinesAgentChild->GetTag() == "ActionPlotURI")
									ConvPlayerLine.PlotAction = nodePlayerLinesAgentChild->GetContent();
								else if (nodePlayerLinesAgentChild->GetTag() == "ActionPlotFlag")
									ConvPlayerLine.PlotActionFlag = ConvPlayerLine.PlotAction == "" ? "" : nodePlayerLinesAgentChild->GetContent();// FCString::Atoi(*nodePlayerLinesAgentChild->GetContent());
								else if (nodePlayerLinesAgentChild->GetTag() == "ActionResult")
									ConvPlayerLine.IsActionSet = nodePlayerLinesAgentChild->GetContent() == "False" ? false : true;
								else if (nodePlayerLinesAgentChild->GetTag() == "text")
									ConvPlayerLine.TextSpoken = nodePlayerLinesAgentChild->GetContent();
								else if (nodePlayerLinesAgentChild->GetTag() == "Speaker")
									ConvPlayerLine.Speaker = *nodePlayerLinesAgentChild->GetContent();
								else if (nodePlayerLinesAgentChild->GetTag() == "Ambient")
									ConvPlayerLine.IsAmbient = nodePlayerLinesAgentChild->GetContent() == "False" ? false : true;
							}
						}

						if (ConvPlayerLine.PlotCondition == "")
						{
							ConvPlayerLine.IsConditionMet = false;
						}

						if (ConvPlayerLine.PlotAction == "")
						{
							ConvPlayerLine.IsActionSet = false;
						}

						if (ConvPlayerLine.PlotCondition != "" || ConvPlayerLine.PlotAction != "")
						{
							UpdatePlotFlagFromFlagId(ConvPlayerLine);
						}

						DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList.Add(ConvPlayerLine);
					}
				}
			}
			else
			{
				if (node->GetTag() == "StartList")
				{
					const TArray<FXmlNode*> StartListChildren = node->GetChildrenNodes();

					for (FXmlNode* nodeStartAgent : StartListChildren)
					{
						const TArray<FXmlNode*> StartAgentChildren = nodeStartAgent->GetChildrenNodes();

						int32 nLineIndex = -1;

						for (FXmlNode* nodeStartAgentChild : StartAgentChildren)
						{
							if (nodeStartAgentChild->GetTag() == "LineIndex")
							{
								nLineIndex = FCString::Atoi(*nodeStartAgentChild->GetContent());
							}
						}

						check(nLineIndex != INDEX_NONE);

						DialogueEditorInstance.Pin().Get()->GetDialogue()->StartList.Add(nLineIndex);
					}
				}
			}
		}
	}

	LoadDialogue(true);

	UE_LOG(LogTemp, Warning, TEXT("SDialogueTreeView::ProcessXML"));
}

bool SDialogueTreeView::IsStartingBranch(TSharedPtr<FDialogueLineRow> Entry)
{
	if (Entry->IsPlayer)//player lines can't be starting branches
	{
		return false;
	}
	else if (Entry->LinkToLineIndex != INDEX_NONE)//links to are not starting branches
	{
		return false;
	}
	else
	{
		return DialogueEditorInstance.Pin().Get()->GetDialogue()->StartList.Contains(Entry->LineIndex);
	}

	return false;
}

TSharedPtr<FDialogueLineRow> SDialogueTreeView::GetNewLinkLine(TSharedPtr<FDialogueLineRow> Entry)
{
	FDialogueLineRow* NewLink = new FDialogueLineRow(DialogueEditorInstance);
	TSharedPtr<FDialogueLineRow> NewLinkPtr = MakeShareable(NewLink);

	NewLinkPtr->LineIndex = Entry->LineIndex;
	NewLinkPtr->IsPlayer = Entry->IsPlayer;
	NewLinkPtr->LinkToLineIndex = Entry->LineIndex;
	check(!Entry->Links.Contains(NewLinkPtr));
	Entry->Links.Add(NewLinkPtr);
	NewLinkPtr->IndexAsLink = Entry->Links.Num() - 1;
	NewLinkPtr->RowDialogueLine = FDialogueLine();//reset it for links, not needed
	NewLinkPtr->DialogueLineDetails = Entry->DialogueLineDetails;

	return NewLinkPtr;
}

void SDialogueTreeView::OnInsertLineChild(TSharedPtr<FDialogueLineRow> FutureParent, TSharedPtr<FDialogueLineRow> FutureChild /*= nullptr*/)
{
	bool IsLink = FutureChild && FutureChild->LinkToLineIndex != INDEX_NONE;

	if (!ExistingLine.IsLineSet)//if not loading
	{
		if (!ScopedTransaction)
		{
			ScopedTransaction = new FScopedTransaction(NSLOCTEXT("DialogueTreeViewTab", "OnInsertNewLine", "On Insert New Line"));
			DialogueEditorInstance.Pin().Get()->GetDialogue()->Modify();
		}
	}

	check(GetEntries().Num() == 1 && GetEntries()[0]->RowLineParent == nullptr && GetEntries()[0]->LineIndex == INT_MAX);

	FDialogueLineRow* childLine = new FDialogueLineRow(DialogueEditorInstance);

	if (FutureChild == nullptr)
	{
		FutureChild = MakeShareable(childLine);
		FutureChild->RowDialogueLine = ExistingLine;
	}

	FutureChild->Indent = FutureParent->Indent + 1;
	FutureChild->IsPlayer = !FutureParent->IsPlayer;

	if (!FutureChild->DialogueLineDetails)
	{
		FutureChild->DialogueLineDetails = SNew(SDialogueLineDetails).DialogueEditorInstance(SharedThis(DialogueEditorInstance.Pin().Get()));
	}

	UpdateDialogue(FutureParent, FutureChild);

	//has to be after any UpdateDialogue, as the Player/NPC Lists are populated
	if (ExistingLine.IsLineSet)//loading
	{
		FutureChild->LineIndex = LineLoadIndex;
	}
	else if (!IsLink)//last available
	{
		check(FutureChild->LineIndex == INDEX_NONE);
		FutureChild->LineIndex = FutureChild->IsPlayer ?
			DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList.Num() - 1 :
			DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList.Num() - 1;
	}

	check(FutureChild->LineIndex > INDEX_NONE);

	if (IsStartingBranch(FutureChild))
	{
		check(FutureChild->RowLineParent == nullptr);
		TreeView->SetItemExpansion(GetEntries()[0], true);
	}
	else
	{
		check(FutureChild->RowLineParent != nullptr);
		TreeView->SetItemExpansion(FutureParent, true);
	}

	TreeView->RequestTreeRefresh();
	TreeView->SetSelection(FutureChild, ESelectInfo::Direct);

	if (DialogueEditorInstance.Pin().Get()->IsLoadingDialogue())
	{
		if (FutureChild->LinkToLineIndex != INDEX_NONE)
		{
			TSharedPtr<FDialogueLineRow>OriginalLine = GetOriginalLine(GetEntries()[0], FutureChild->LinkToLineIndex, FutureChild->IsPlayer);
			check(OriginalLine);

			FutureChild->RowDialogueLine.TextSpoken = OriginalLine->RowDialogueLine.TextSpoken;
			FutureChild->DialogueLineDetails = OriginalLine->DialogueLineDetails;
		}
		else
		{
			UpdateLineDetails(FutureChild);
		}
	}

	//reset
	ExistingLine = FDialogueLine();
	bIsSibling = false;

	if (!DialogueEditorInstance.Pin().Get()->IsLoadingDialogue())
	{
		DialogueEditorInstance.Pin()->DialogueDirtyDelegate.ExecuteIfBound();
	}

	if (!ExistingLine.IsLineSet)//if not loading
	{
		if (ScopedTransaction)
		{
			delete ScopedTransaction;
			ScopedTransaction = nullptr;

			//add an op
			FDialogueEditorOperation Op = FDialogueEditorOperation(EDialogueOperationType::INSERTLINE);
			Op.PayloadInts.Add("LineIndexChild", FutureChild->LineIndex);
			Op.PayloadBools.Add("ChildIsLink", IsLink);
			DialogueEditorInstance.Pin()->GetDialogue()->AddNewOp(Op);
		}
	}
}

void SDialogueTreeView::UpdateDialogue(TSharedPtr<FDialogueLineRow> ParentLine, TSharedPtr<FDialogueLineRow> ChildLine, bool bRemove /*= false*/)
{
	check(ParentLine->IsPlayer == !ChildLine->IsPlayer);

	if (GetEntries()[0] == ParentLine)//add to StartList
	{
		if (!bRemove)
		{
			if (!ExistingLine.IsLineSet)
			{
				ChildLine->RowDialogueLine.IsLineSet = true;
				DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList.Add(ChildLine->RowDialogueLine);
				ChildLine->RowDialogueLine = DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList[DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList.Num() - 1];
				DialogueEditorInstance.Pin().Get()->GetDialogue()->StartList.Add(DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList.Num() - 1);
			}

			ParentLine->RowLineChildren.Add(ChildLine);
		}
		else
		{
			check(ParentLine == ChildLine);//fake, just to trigger
		}
	}
	else
	{
		int32 TransitionIndex = INDEX_NONE;

		if (ChildLine->IsPlayer)
		{
			int32 nLinePlayer = INDEX_NONE;

			if (ChildLine->LinkToLineIndex == INDEX_NONE)
			{
				nLinePlayer = ChildLine->LineIndex;
			}
			else //LinkTo
			{
				nLinePlayer = ChildLine->LinkToLineIndex;
			}

			if (!bRemove)
			{
				if (ExistingLine.IsLineSet)
				{
					TransitionIndex = LineLoadIndex;
				}
				else if (nLinePlayer == INDEX_NONE)//Add new
				{
					ChildLine->RowDialogueLine.IsLineSet = true;
					DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList.Add(ChildLine->RowDialogueLine);
					ChildLine->RowDialogueLine = DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList[DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList.Num() - 1];

					TransitionIndex = DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList.Num() - 1;
				}
				else //point to existing
				{
					TransitionIndex = nLinePlayer;
				}
			}
			else
			{
				check(nLinePlayer != INDEX_NONE);
			}
		}
		else //NPC
		{
			int32 nLineNPC = INDEX_NONE;

			if (ChildLine->LinkToLineIndex == INDEX_NONE)
			{
				nLineNPC = ChildLine->LineIndex;
			}
			else //LinkTo
			{
				nLineNPC = ChildLine->LinkToLineIndex;
			}

			if (!bRemove)
			{
				if (ExistingLine.IsLineSet)
				{
					TransitionIndex = LineLoadIndex;
				}
				else if (nLineNPC == INDEX_NONE)//Add new
				{
					ChildLine->RowDialogueLine.IsLineSet = true;
					DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList.Add(ChildLine->RowDialogueLine);
					ChildLine->RowDialogueLine = DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList[DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList.Num() - 1];

					TransitionIndex = DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList.Num() - 1;
				}
				else //point to existing
				{
					TransitionIndex = nLineNPC;
				}
			}
			else
			{
				check(nLineNPC != INDEX_NONE);
			}
		}

		FDialogueLine* line = nullptr;

		if (ParentLine->IsPlayer)
		{
			line = &DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList[ParentLine->LineIndex];
		}
		else //NPC
		{
			line = &DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList[ParentLine->LineIndex];
		}

		check(line);

		if (!bRemove)
		{
			check(TransitionIndex != INDEX_NONE);

			if (!ExistingLine.IsLineSet)
			{
				int32 nIndex = line->TransitionList.IndexOfByPredicate(
					[TransitionIndex](const FDialogueTransition _transition)
					{
						return _transition.LineIndex == TransitionIndex;
					});

				check(nIndex == INDEX_NONE);

				FDialogueTransition NewLink;
				NewLink.LineIndex = TransitionIndex;
				NewLink.IsLink = ChildLine->LinkToLineIndex != INDEX_NONE ? true : false;
				ParentLine->RowDialogueLine.TransitionList.Add(NewLink);

				check(ParentLine->LinkToLineIndex == INDEX_NONE);

				//update Lists
				if (ParentLine->IsPlayer)
				{
					DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList[ParentLine->LineIndex].TransitionList.Add(NewLink);
				}
				else //NPC
				{
					DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList[ParentLine->LineIndex].TransitionList.Add(NewLink);
				}
			}

			ChildLine->RowLineParent = ParentLine;

			if (ChildLine->IndexAsChild == INDEX_NONE)
			{
				ParentLine->RowLineChildren.Add(ChildLine);
			}
			else
			{
				ParentLine->RowLineChildren.Insert(ChildLine, ChildLine->IndexAsChild);
			}
		}
		else //remove
		{
			check(ParentLine->LinkToLineIndex == INDEX_NONE);

			int32 nIndexTransition = INDEX_NONE;//for TransitionList
			int32 nIndexList = ChildLine->LineIndex;//for Player/NPC list

			nIndexTransition = line->TransitionList.IndexOfByPredicate(
				[nIndexList](const FDialogueTransition _transition)
				{
					return _transition.LineIndex == nIndexList;
				});

			check(nIndexTransition != INDEX_NONE);
			check(nIndexList != INDEX_NONE);

			ParentLine->RowDialogueLine.TransitionList.RemoveAt(nIndexTransition);

			//update Lists
			if (ParentLine->IsPlayer)
			{
				DialogueEditorInstance.Pin().Get()->GetDialogue()->PlayerLineList[ParentLine->LineIndex].TransitionList.RemoveAt(nIndexTransition);
			}
			else //NPC
			{
				DialogueEditorInstance.Pin().Get()->GetDialogue()->NPCLineList[ParentLine->LineIndex].TransitionList.RemoveAt(nIndexTransition);
			}

			ChildLine->RowLineParent = nullptr;
			ParentLine->RowLineChildren.Remove(ChildLine);
		}
	}
}

SDialogueTreeView::~SDialogueTreeView()
{
	GEditor->UnregisterForUndo(this);
}

void SDialogueTreeView::PostUndo(bool bSuccess)
{

}

void SDialogueTreeView::PostRedo(bool bSuccess)
{

}

//end public

//end SDialogueTreeView

//SDialogueTreeViewRow
void SDialogueTreeViewRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	Entry = InArgs._Entry;

	SMultiColumnTableRow< TSharedPtr<FDialogueLineRow> >::Construct(
		FSuperRowType::FArguments()
		.OnDragDetected(InArgs._OnDragDetected)
		.OnDragEnter(InArgs._OnDragEnter)
		.OnDragLeave(InArgs._OnDragLeave)
		.OnDrop(InArgs._OnDrop),
		InOwnerTableView
	);
}

TSharedRef<SWidget> SDialogueTreeViewRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	//lambdas
	auto GetColorForEntry = [this]() -> FLinearColor
	{
		if (Entry->IsSelected)
		{
			return FLinearColor::White;
		}
		else if (Entry->RowDialogueLine.IsComment)
		{
			return FLinearColor(0, 1, 0);
		}
		else if (Entry->DialogueLineDetails->IsCheckedAmbient() == ECheckBoxState::Checked)
		{
			check(!Entry->IsPlayer);
			return FLinearColor(1, 0.075, 0.075);
		}
		else if (Entry->LineIndex == INT_MAX)
		{
			return FLinearColor(0, 0.5f, 0);
		}
		else if (Entry->LinkToLineIndex != INDEX_NONE)
		{
			return FLinearColor::Gray;
		}
		else if (Entry->IsPlayer)
		{
			return FLinearColor(0, 0.25, 1);
		}

		return FLinearColor(1, 0.025, 0.025);
	};

	auto GetTextForEntry = [this]() -> FText
	{
		if (Entry->LineIndex != INT_MAX)
		{
			if (Entry->RowDialogueLine.TextSpoken != "")
			{
				if (Entry->RowDialogueLine.IsComment)
				{
					return FText::FromString("[COMMENT] " + Entry->RowDialogueLine.TextSpoken);
				}
				if (Entry->RowDialogueLine.IsAmbient)
				{
					return FText::FromString("[AMBIENT] " + Entry->RowDialogueLine.TextSpoken);
				}
				else if (Entry->RowLineChildren.Num() > 0 || Entry->LinkToLineIndex != INDEX_NONE)
				{
					return FText::FromString(Entry->RowDialogueLine.TextSpoken);
				}
				else
				{
					return FText::FromString(Entry->RowDialogueLine.TextSpoken + " [END DIALOGUE]");
				}
			}
			else
			{
				if (Entry->IsPlayer)
				{
					if (Entry->RowLineChildren.Num() > 0)
					{
						return FText::FromString("[CONTINUE]");
					}
					else
					{
						return FText::FromString("[END DIALOGUE]");
					}
				}
			}
		}

		return FText::FromString("");
	};

	auto GetTextLogicForEntry = [this]() -> FText
	{
		if (Entry->RowDialogueLine.PlotConditionFlag != "" && Entry->RowDialogueLine.PlotActionFlag != "")
		{
			return FText::FromString(" AC ");
		}
		else if (Entry->RowDialogueLine.PlotConditionFlag != "")
		{
			return FText::FromString("  C ");
		}
		else if (Entry->RowDialogueLine.PlotActionFlag != "")
		{
			return FText::FromString(" A  ");
		}

		return FText::FromString("    ");
	};

	auto GetTextSpeakerForEntry = [this]() -> FText
	{
		if (Entry->LineIndex == INT_MAX)
		{
			return FText::FromString("Root");
		}
		else
		{
			if (!Entry->IsPlayer)
			{
				if (Entry->RowDialogueLine.Speaker == "")
				{
					return FText::FromString(StringDefaultSpeaker);
				}
			}
			else
			{
				return FText::FromString("");
			}
		}

		return FText::FromName(Entry->RowDialogueLine.Speaker);
	};

	if (ColumnName == LineColumns::LabelLogic)
	{
		return SNew(STextBlock)
			.Text_Lambda(GetTextLogicForEntry)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
			.ColorAndOpacity_Lambda(GetColorForEntry);
	}
	else if (ColumnName == LineColumns::LabelSpeaker)
	{
		return SNew(STextBlock)
			.Text_Lambda(GetTextSpeakerForEntry)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
			.ColorAndOpacity_Lambda(GetColorForEntry);
	}
	else if (ColumnName == LineColumns::LabelText)
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()[
				SNew(SExpanderArrow, SharedThis(this))
					.IndentAmount(16)
					.ShouldDrawWires(true)]
			+ SHorizontalBox::Slot()
					.FillWidth(1.0f)[
						SNew(STextBlock)
							.Text_Lambda(GetTextForEntry)
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
							.ColorAndOpacity_Lambda(GetColorForEntry)];
	}

	return SNullWidget::NullWidget;
}

void SDialogueTreeViewRow::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	DragType = FSlateApplication::Get().GetModifierKeys().IsShiftDown() ? EDragType::LINK : EDragType::MOVE;

	UE_LOG(LogTemp, Warning, TEXT("SDialogueTreeViewRow::OnRowDragEnter"));

	bool bInvalid = false;
	TSharedPtr<SWidget> _row = nullptr;
	TSharedPtr<SWidget> _treeView = nullptr;
	TSharedPtr<SDialogueTreeViewRow> HoveredRow = nullptr;

	FWidgetPath Path = FSlateApplication::Get().LocateWindowUnderMouse(DragDropEvent.GetScreenSpacePosition(), FSlateApplication::Get().GetInteractiveTopLevelWindows(), false, DragDropEvent.GetUserIndex());

	for (int32 PathIndex = Path.Widgets.Num() - 1; PathIndex >= 0; PathIndex--)
	{
		TSharedRef<SWidget> PathWidget = Path.Widgets[PathIndex].Widget;

		//TODO ugly!!!
		if (PathWidget.Get().GetTypeAsString().StartsWith("SDialogueTreeViewRow"))
		{
			_row = PathWidget;
		}
		else if (PathWidget.Get().GetTypeAsString().StartsWith("SDialogueTreeView"))
		{
			_treeView = PathWidget;
		}
	}

	if (_row && _treeView)
	{
		HoveredRow = StaticCastSharedPtr<SDialogueTreeViewRow>(_row);

		TSharedPtr<FDialogueLineDragDropOp> DragDropOp = StaticCastSharedPtr<FDialogueLineDragDropOp>(DragDropEvent.GetOperation());

		if (!HoveredRow)
		{
			UE_LOG(LogTemp, Error, TEXT("SDialogueTreeViewRow::OnRowDragEnter No HoverOverLine!"));
			bInvalid = true;
		}
		else if (DragDropOp->Entry->RowLineParent == HoveredRow->Entry)
		{
			UE_LOG(LogTemp, Error, TEXT("SDialogueTreeViewRow::OnRowDragEnter Can't drop on same parent"));
			bInvalid = true;
		}
		else if (DragDropOp->Entry->IsPlayer == HoveredRow->Entry->IsPlayer)
		{
			UE_LOG(LogTemp, Error, TEXT("SDialogueTreeViewRow::OnRowDragEnter Can't drop on same line type e.g. Player on Player, NPC on NPC"));
			bInvalid = true;
		}
		else if (IsAncestor(DragDropOp->Entry, HoveredRow->Entry) && DragType != EDragType::LINK)
		{
			UE_LOG(LogTemp, Error, TEXT("SDialogueTreeViewRow::OnRowDragEnter move on descendants BAD, link GOOD, currently moving, no can do..."));
			bInvalid = true;
		}
		else if (DragDropOp->Entry->IsPlayer && HoveredRow->Entry->LineIndex == INT_MAX)
		{
			UE_LOG(LogTemp, Error, TEXT("SDialogueTreeViewRow::OnRowDragEnter drop player line on Root BAD :D"));
			bInvalid = true;
		}
		else if (DragDropOp->Entry == HoveredRow->Entry)
		{
			UE_LOG(LogTemp, Error, TEXT("SDialogueTreeViewRow::OnRowDragEnter Can't drop on same line"));
			bInvalid = true;
		}
		else if (DragDropOp->Entry->DialogueEditorInstance.Pin().Get()->DialogueTreeView->IsStartingBranch(DragDropOp->Entry) && HoveredRow->Entry->LineIndex == INT_MAX)
		{
			UE_LOG(LogTemp, Warning, TEXT("TODO SDialogueTreeViewRow::OnRowDragEnter shuffle starting branches, for now invalid..."));
			bInvalid = true;
		}
	}
	else
	{
		bInvalid = true;
	}

	if (DragDropEvent.GetOperation()->IsOfType<FDialogueLineDragDropOp>())
	{
		TSharedPtr<FDialogueLineDragDropOp> DragDropOp = StaticCastSharedPtr<FDialogueLineDragDropOp>(DragDropEvent.GetOperation());

		if (bInvalid)
		{
			DragDropOp->CurrentHoverText = FText::FromString(StringInvalid);
		}
		else
		{
			check(DragType != EDragType::INVALID);
			FString sDragType = DragType == EDragType::LINK ? "Link To: " : "Move: ";

			FString sDragDrop = DragDropOp->Entry->RowDialogueLine.TextSpoken;

			DragDropOp->CurrentHoverText = FText::FromString(sDragType + sDragDrop);
			TSharedRef<STreeView<TSharedPtr<FDialogueLineRow>>> TreeView = StaticCastSharedRef<STreeView<TSharedPtr<FDialogueLineRow>>>(OwnerTablePtr.Pin()->AsWidget());
			TreeView->SetSelection(HoveredRow->Entry, ESelectInfo::Direct);
		}
	}
}

void SDialogueTreeViewRow::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	DragType = FSlateApplication::Get().GetModifierKeys().IsShiftDown() ? EDragType::LINK : EDragType::MOVE;

	UE_LOG(LogTemp, Warning, TEXT("SDialogueTreeViewRow::OnRowDragLeave"));

	TSharedPtr<SWidget> _treeView = nullptr;

	FWidgetPath Path = FSlateApplication::Get().LocateWindowUnderMouse(DragDropEvent.GetScreenSpacePosition(), FSlateApplication::Get().GetInteractiveTopLevelWindows(), false, DragDropEvent.GetUserIndex());

	for (int32 PathIndex = Path.Widgets.Num() - 1; PathIndex >= 0; PathIndex--)
	{
		TSharedRef<SWidget> PathWidget = Path.Widgets[PathIndex].Widget;

		//TODO ugly!!!
		if (PathWidget.Get().GetTypeAsString().StartsWith("SDialogueTreeView"))
		{
			_treeView = PathWidget;
			break;
		}
	}

	if (!_treeView)
	{
		if (DragDropEvent.GetOperation()->IsOfType<FDialogueLineDragDropOp>())
		{
			TSharedPtr<FDialogueLineDragDropOp> DragDropOp = StaticCastSharedPtr<FDialogueLineDragDropOp>(DragDropEvent.GetOperation());
			DragDropOp->CurrentHoverText = FText::FromString(StringInvalid);
		}

		UE_LOG(LogTemp, Error, TEXT("SDialogueTreeViewRow::OnRowDragLeave INVALID Leave"));
	}
}

FReply SDialogueTreeViewRow::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (Entry->LineIndex == INT_MAX)
	{
		UE_LOG(LogTemp, Warning, TEXT("SDialogueTreeViewRow::OnRowDragDetected Can't drag Root"));
		return FReply::Unhandled();
	}

	UE_LOG(LogTemp, Warning, TEXT("SDialogueTreeViewRow::OnRowDragDetected"));

	DragType = MouseEvent.GetModifierKeys().IsShiftDown() ? EDragType::LINK : EDragType::MOVE;

	Entry->DialogueEditorInstance.Pin().Get()->DialogueTreeView->SetDragging(true);

	return FReply::Handled().BeginDragDrop(FDialogueLineDragDropOp::New(Entry));
}

FReply SDialogueTreeViewRow::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDialogueLineDragDropOp> DragDropOp = StaticCastSharedPtr<FDialogueLineDragDropOp>(DragDropEvent.GetOperation());

	//if invalid drop, revert to original selection
	if (DragDropOp->CurrentHoverText.ToString() == StringInvalid)
	{
		DragDropOp->Entry->DialogueEditorInstance.Pin().Get()->DialogueTreeView->GetTreeView()->SetSelection(DragDropOp->Entry, ESelectInfo::Direct);
	}
	else
	{
		check(DragType != EDragType::INVALID);

		if (DragType == EDragType::LINK)
		{
			UE_LOG(LogTemp, Warning, TEXT("SDialogueTreeViewRow::OnRowDragDropped LINK"));

			//line as Future Parent for CurrentlySelected as linked Child
			DragDropOp->Entry->DialogueEditorInstance.Pin().Get()->DialogueTreeView->OnInsertLineChild(DragDropOp->Entry,
				DragDropOp->Entry->DialogueEditorInstance.Pin().Get()->DialogueTreeView->GetNewLinkLine(DragDropOp->Entry->DialogueEditorInstance.Pin().Get()->DialogueTreeView->GetCurrentlySelectedLine()));
		}
		else if (DragType == EDragType::MOVE)
		{
			UE_LOG(LogTemp, Warning, TEXT("SDialogueTreeViewRow::OnRowDragDropped MOVE"));

			//line as future child for CurrentlySelected as Parent
			int32 nOldIndent = DragDropOp->Entry->RowLineParent->Indent - 1;
			int32 nNewIndent = DragDropOp->Entry->DialogueEditorInstance.Pin().Get()->DialogueTreeView->GetCurrentlySelectedLine()->Indent - 1;

			DragDropOp->Entry->DialogueEditorInstance.Pin().Get()->DialogueTreeView->UpdateDialogue(DragDropOp->Entry->RowLineParent, DragDropOp->Entry, true);
			DragDropOp->Entry->DialogueEditorInstance.Pin().Get()->DialogueTreeView->UpdateDialogue(DragDropOp->Entry->DialogueEditorInstance.Pin().Get()->DialogueTreeView->GetCurrentlySelectedLine(), DragDropOp->Entry);

			//GetEntries().RemoveAt(nIndex);

			DragDropOp->Entry->DialogueEditorInstance.Pin().Get()->DialogueTreeView->GetTreeView()->RequestTreeRefresh();
			DragDropOp->Entry->DialogueEditorInstance.Pin().Get()->DialogueTreeView->GetTreeView()->SetSelection(DragDropOp->Entry, ESelectInfo::Direct);
			DragDropOp->Entry->DialogueEditorInstance.Pin().Get()->DialogueDirtyDelegate.ExecuteIfBound();
		}
	}

	DragDropOp->Entry->DialogueEditorInstance.Pin().Get()->DialogueTreeView->GetTreeView()->SetItemExpansion(DragDropOp->Entry->RowLineParent, true);
	DragDropOp->Entry->DialogueEditorInstance.Pin().Get()->DialogueTreeView->SetDragging(false);

	DragType = EDragType::INVALID;
	return FReply::Handled();
}

bool SDialogueTreeViewRow::IsAncestor(TSharedPtr<FDialogueLineRow> Entry1, TSharedPtr<FDialogueLineRow> Entry2)
{
	if (Entry2->RowLineParent == nullptr)
		return false;
	else if (Entry1 == Entry2->RowLineParent)
		return true;
	else
	{
		return IsAncestor(Entry1, Entry2->RowLineParent);
	}
}
//end SDialogueTreeViewRow

