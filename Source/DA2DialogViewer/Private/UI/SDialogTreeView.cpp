// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SDialogTreeView.h"
#include "UI/SDialogWheel.h"
#include "DialogFlow/Conversation.h"
#include "Data/DialogDataManager.h"
#include "Audio/AudioMapper.h"
#include "Audio/AudioUtils.h"
#include "Audio/DialogAudioPlayer.h"
#include "Audio/DialogAudioManager.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/FileManager.h"

// Static member initialization
TSet<int32> FDialogTreeItem::EncounteredPlayerSpeakerIDs;

// Hardcoded player speaker IDs (identified via flip-flop analysis of 704 DA2 conversations)
// These IDs are used >95% of the time for player lines (Hawke)
// Source: analyze_player_speakers_flipflop.py analysis
TSet<int32> FDialogTreeItem::KnownPlayerSpeakerIDs = {
	2, 10, 14, 18, 26, 34, 66, 74, 78, 110, 138, 258, 266, 274, 290, 322
};

/**
 * Tree row widget for a single dialog line
 */
class SDialogTreeRow : public STableRow<TSharedPtr<FDialogTreeItem>>
{
public:
	SLATE_BEGIN_ARGS(SDialogTreeRow)
		{
		}

		SLATE_ARGUMENT(FString, OwnerTag)
		SLATE_ARGUMENT(TSharedPtr<FDialogAudioManager>, AudioManager)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FDialogTreeItem> InItem)
	{
		Item = InItem;
		OwnerTag = InArgs._OwnerTag;
		AudioManager = InArgs._AudioManager;

		// Check if this line has audio available
		// Hide play button for reference nodes (gray stubs) - they're just links to the real occurrence
		// Only show play button on first occurrence with valid spoken line
		bool bHasAudio = !Item->bIsReference && !FDialogTreeItem::IsValidlyEmpty(Item->SpokenText);

		// Build row content
		STableRow<TSharedPtr<FDialogTreeItem>>::Construct(
			STableRow<TSharedPtr<FDialogTreeItem>>::FArguments()
			.Padding(2.0f)
			.Content()
			[
				SNew(SHorizontalBox)

				// Play button (only show if line might have audio)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FCoreStyle::Get(), "NoBorder")
					.OnClicked(this, &SDialogTreeRow::OnPlayAudioClicked)
					.Visibility(bHasAudio ? EVisibility::Visible : EVisibility::Hidden)
					.ToolTipText(FText::FromString(TEXT("Play dialog audio")))
					.ContentPadding(FMargin(2.0f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("\u25B6"))) // Play symbol ▶
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						.ColorAndOpacity(FLinearColor(0.5f, 1.0f, 0.5f))
					]
				]

				// Condition/Action indicator (CA, C, A, or empty)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SBox)
					.WidthOverride(30.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Item->GetIndicatorString()))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(FLinearColor::Yellow)
					]
				]

				// Speaker type
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SBox)
					.WidthOverride(200.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Item->GetSpeakerString(OwnerTag)))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(this, &SDialogTreeRow::GetSpeakerColor)
					]
				]

				// Paraphrase text (short preview)
				+ SHorizontalBox::Slot()
				.FillWidth(0.3f)
				.Padding(2.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Item->ParaphraseText))
					.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
					.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 1.0f, 1.0f))
				]

				// Spoken text (full line)
				+ SHorizontalBox::Slot()
				.FillWidth(0.7f)
				.Padding(2.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Item->SpokenText))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor::White)
					.AutoWrapText(true)
				]

				// Node info (for debugging)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("[%d]"), Item->NodeIndex)))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.ColorAndOpacity(FLinearColor::Gray)
				]
			],
			InOwnerTable
		);
	}

private:
	FSlateColor GetSpeakerColor() const
	{
		// Use GetSpeakerType() as single source of truth
		// This ensures text and color are ALWAYS synchronized
		ESpeakerType SpeakerType = Item->GetSpeakerType();

		switch (SpeakerType)
		{
			case ESpeakerType::Player:
				return FLinearColor(0.3f, 0.5f, 1.0f); // Player - Blue

			case ESpeakerType::Owner:
				return FLinearColor(1.0f, 0.3f, 0.3f); // Owner - Red

			case ESpeakerType::Henchman:
				return FLinearColor(0.5f, 1.0f, 0.5f); // Henchman - Green

			case ESpeakerType::OtherNPC:
				// NOTE: This logic is INCOMPLETE and NOT FULLY IMPLEMENTED
				// We cannot currently identify specific named NPCs besides the owner and henchmen
				// This is a known limitation - magenta NPCs are "some NPC in the conversation" but we don't know their identity
				// Future work: Find a way to map speaker IDs to actual NPC names/tags
				return FLinearColor(1.0f, 0.5f, 1.0f); // Other NPC - Magenta (NOT FULLY IMPLEMENTED)

			case ESpeakerType::Reference:
				return FLinearColor(0.6f, 0.6f, 0.6f); // Reference - Gray

			default:
				return FLinearColor::White; // Fallback (should never happen)
		}
	}

	FReply OnPlayAudioClicked()
	{
		if (!AudioManager.IsValid() || !Item.IsValid())
		{
			return FReply::Handled();
		}

		// Play audio only from spoken line (no fallback to paraphrase)
		bool bSuccess = AudioManager->PlayDialogAudio(Item->TLKStringID, -1);

		if (!bSuccess)
		{
			UE_LOG(LogTemp, Warning, TEXT("No audio found for dialog node %d (Spoken TLK: %d)"),
				Item->NodeIndex, Item->TLKStringID);
		}

		return FReply::Handled();
	}

	TSharedPtr<FDialogTreeItem> Item;
	FString OwnerTag;
	TSharedPtr<FDialogAudioManager> AudioManager;
};

void SDialogTreeView::Construct(const FArguments& InArgs, TSharedPtr<FDialogDataManager> InDataManager)
{
	DataManager = InDataManager;
	OnNodeSelectedCallback = InArgs._OnNodeSelected;

	// Initialize audio manager
	AudioManager = MakeShared<FDialogAudioManager>();
	AudioManager->Initialize(DataManager);

	ChildSlot
	[
		SNew(SVerticalBox)

		// Tree view section (fills most of the space)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(TreeView, STreeView<TSharedPtr<FDialogTreeItem>>)
			.TreeItemsSource(&RootItems)
			.OnGenerateRow(this, &SDialogTreeView::OnGenerateRow)
			.OnGetChildren(this, &SDialogTreeView::OnGetChildren)
			.OnSelectionChanged(this, &SDialogTreeView::OnSelectionChanged)
			.OnMouseButtonDoubleClick(this, &SDialogTreeView::OnMouseDoubleClick)
			.OnContextMenuOpening(this, &SDialogTreeView::OnContextMenuOpening)
			.SelectionMode(ESelectionMode::Single)
			.HeaderRow
			(
				SNew(SHeaderRow)

				+ SHeaderRow::Column("Indicator")
				.DefaultLabel(FText::FromString(TEXT("")))
				.FixedWidth(30.0f)

				+ SHeaderRow::Column("Speaker")
				.DefaultLabel(FText::FromString(TEXT("Speaker")))
				.FixedWidth(200.0f)

				+ SHeaderRow::Column("Paraphrase")
				.DefaultLabel(FText::FromString(TEXT("Paraphrase")))
				.FillWidth(0.3f)

				+ SHeaderRow::Column("Spoken")
				.DefaultLabel(FText::FromString(TEXT("Spoken Line")))
				.FillWidth(0.7f)

				+ SHeaderRow::Column("Node")
				.DefaultLabel(FText::FromString(TEXT("Node")))
				.FixedWidth(50.0f)
			)
		]

		// Metadata section (fixed height, split horizontally)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 5.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)

			// Left half: Condition
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(5.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.Padding(5.0f)
				[
					SAssignNew(ConditionTextBlock, STextBlock)
					.Text(FText::FromString(TEXT("Condition:\n(none)")))
					.Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
					.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 1.0f, 1.0f))
				]
			]

			// Right half: Action
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(5.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.Padding(5.0f)
				[
					SAssignNew(ActionTextBlock, STextBlock)
					.Text(FText::FromString(TEXT("Action:\n(none)")))
					.Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
					.ColorAndOpacity(FLinearColor(1.0f, 0.8f, 0.8f, 1.0f))
				]
			]
		]
	];
}

void SDialogTreeView::LoadConversation(TSharedPtr<FConversation> InConversation)
{
	CurrentConversation = InConversation;
	Clear();

	if (CurrentConversation.IsValid())
	{
		// Detect conversation owner using heuristic
		DetectedOwnerSpeakerID = DetectConversationOwner();

		BuildTreeFromConversation();
		TreeView->RequestTreeRefresh();
	}
}

void SDialogTreeView::Clear()
{
	RootItems.Empty();
	SelectedItem.Reset();

	if (TreeView.IsValid())
	{
		TreeView->RequestTreeRefresh();
	}
}

const FDialogNode* SDialogTreeView::GetSelectedNode() const
{
	if (SelectedItem.IsValid() && CurrentConversation.IsValid())
	{
		return CurrentConversation->FindNode(SelectedItem->NodeIndex);
	}
	return nullptr;
}

void SDialogTreeView::NavigateToNode(int32 NodeIndex)
{
	TSharedPtr<FDialogTreeItem> Item = FindTreeItem(NodeIndex);
	if (Item.IsValid() && TreeView.IsValid())
	{
		TreeView->SetSelection(Item);
		TreeView->RequestScrollIntoView(Item);
	}
}

void SDialogTreeView::NavigateToPlayerChoice(int32 PlayerNodeIndex)
{
	// Find the player LINE (the choice that was clicked in the wheel)
	TSharedPtr<FDialogTreeItem> PlayerItem = FindTreeItem(PlayerNodeIndex);
	if (!PlayerItem.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("NavigateToPlayerChoice: Could not find player node %d"), PlayerNodeIndex);
		return;
	}

	// Check if player line has valid spoken text
	bool bHasValidSpokenText = !FDialogTreeItem::IsValidlyEmpty(PlayerItem->SpokenText);

	if (bHasValidSpokenText)
	{
		// Case 1: Player line has spoken text
		// Navigate to player line and play player audio
		UE_LOG(LogTemp, Log, TEXT("Player choice has spoken text, navigating to player line %d"), PlayerNodeIndex);

		if (TreeView.IsValid())
		{
			TreeView->SetSelection(PlayerItem);
			TreeView->RequestScrollIntoView(PlayerItem);
		}

		// Play player audio
		if (AudioManager.IsValid())
		{
			AudioManager->PlayDialogAudio(PlayerItem->TLKStringID, -1);
		}
	}
	else
	{
		// Case 2: Player line has NO spoken text (silent choice like "Let brother answer")
		// Navigate to first LINK (child) and play that audio
		UE_LOG(LogTemp, Log, TEXT("Player choice has NO spoken text, navigating to first child of node %d"), PlayerNodeIndex);

		if (PlayerItem->Children.Num() > 0)
		{
			TSharedPtr<FDialogTreeItem> FirstChild = PlayerItem->Children[0];

			if (TreeView.IsValid())
			{
				TreeView->SetSelection(FirstChild);
				TreeView->RequestScrollIntoView(FirstChild);
			}

			// Play first child's audio
			if (AudioManager.IsValid())
			{
				AudioManager->PlayDialogAudio(FirstChild->TLKStringID, -1);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Player choice has no spoken text and no children!"));
		}
	}
}

void SDialogTreeView::ExpandAll()
{
	if (!TreeView.IsValid())
		return;

	for (TSharedPtr<FDialogTreeItem>& RootItem : RootItems)
	{
		ExpandBranch(RootItem);
	}
}

void SDialogTreeView::CollapseAll()
{
	if (!TreeView.IsValid())
		return;

	for (TSharedPtr<FDialogTreeItem>& RootItem : RootItems)
	{
		CollapseBranch(RootItem);
	}
}

void SDialogTreeView::ExpandBranch(TSharedPtr<FDialogTreeItem> Item)
{
	if (!Item.IsValid() || !TreeView.IsValid())
		return;

	TreeView->SetItemExpansion(Item, true);

	for (TSharedPtr<FDialogTreeItem>& Child : Item->Children)
	{
		ExpandBranch(Child);
	}
}

void SDialogTreeView::CollapseBranch(TSharedPtr<FDialogTreeItem> Item)
{
	if (!Item.IsValid() || !TreeView.IsValid())
		return;

	TreeView->SetItemExpansion(Item, false);

	for (TSharedPtr<FDialogTreeItem>& Child : Item->Children)
	{
		CollapseBranch(Child);
	}
}

TSharedRef<ITableRow> SDialogTreeView::OnGenerateRow(TSharedPtr<FDialogTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SDialogTreeRow, OwnerTable, Item)
		.OwnerTag(CurrentConversation.IsValid() ? CurrentConversation->OwnerTag : TEXT(""))
		.AudioManager(AudioManager);
}

void SDialogTreeView::OnGetChildren(TSharedPtr<FDialogTreeItem> Item, TArray<TSharedPtr<FDialogTreeItem>>& OutChildren)
{
	if (Item.IsValid())
	{
		OutChildren = Item->Children;
	}
}

void SDialogTreeView::OnSelectionChanged(TSharedPtr<FDialogTreeItem> Item, ESelectInfo::Type SelectInfo)
{
	SelectedItem = Item;

	if (Item.IsValid() && CurrentConversation.IsValid())
	{
		const FDialogNode* Node = CurrentConversation->FindNode(Item->NodeIndex);
		if (Node)
		{
			// Update dialog wheel
			if (DialogWheel.IsValid())
			{
				DialogWheel->SetCurrentNode(Node);
			}

			// Update condition metadata display
			if (ConditionTextBlock.IsValid())
			{
				if (!Node->Condition.PlotName.IsEmpty())
				{
					// VALIDATION: Check for explicit true (operation byte == 1)
					if (Node->Condition.ComparisonType == 1)
					{
						UE_LOG(LogTemp, Warning, TEXT("FOUND Op=1 (explicit TRUE check): Plot=%s, Flag=%d, Node=%d"),
						       *Node->Condition.PlotName, Node->Condition.FlagIndex, Item->NodeIndex);
					}

					// VALIDATION: Check for unexpected operation bytes (not 0, not 1, not 255)
					if (Node->Condition.ComparisonType != 0 && Node->Condition.ComparisonType != 1 && Node->Condition.ComparisonType != 255)
					{
						UE_LOG(LogTemp, Error, TEXT("UNEXPECTED Op=%d (not 0/1/255): Plot=%s, Flag=%d, Node=%d"),
						       Node->Condition.ComparisonType, *Node->Condition.PlotName, Node->Condition.FlagIndex, Item->NodeIndex);
						checkf(false, TEXT("Unexpected operation byte %d in condition (expected 0, 1, or 255)"), Node->Condition.ComparisonType);
					}

					// Format operation byte for human readability
					FString OpDescription;
					switch (Node->Condition.ComparisonType)
					{
						case 255: // 0xFF - Implicit true (default/no condition)
							OpDescription = TEXT("True (implicitly)");
							break;
						case 1: // Explicit true check
							OpDescription = TEXT("True (explicitly)");
							break;
						case 0: // Explicit false check
							OpDescription = TEXT("False (explicitly)");
							break;
						default: // Unexpected value
							OpDescription = FString::Printf(TEXT("%d (unknown)"), Node->Condition.ComparisonType);
							break;
					}

					FString ConditionText = FString::Printf(
						TEXT("Condition:\nPlot: %s\nFlag: %d\nOp: %s"),
						*Node->Condition.PlotName,
						Node->Condition.FlagIndex,
						*OpDescription
					);
					ConditionTextBlock->SetText(FText::FromString(ConditionText));
				}
				else
				{
					ConditionTextBlock->SetText(FText::FromString(TEXT("Condition:\n(none)")));
				}
			}

			// Update action metadata display
			if (ActionTextBlock.IsValid())
			{
				if (!Node->Action.PlotName.IsEmpty())
				{
					// VALIDATION: Check for explicit true (operation byte == 1)
					if (Node->Action.ComparisonType == 1)
					{
						UE_LOG(LogTemp, Warning, TEXT("FOUND Op=1 (explicit TRUE check) in ACTION: Plot=%s, Flag=%d, Node=%d"),
						       *Node->Action.PlotName, Node->Action.FlagIndex, Item->NodeIndex);
					}

					// VALIDATION: Check for unexpected operation bytes (not 0, not 1, not 255)
					if (Node->Action.ComparisonType != 0 && Node->Action.ComparisonType != 1 && Node->Action.ComparisonType != 255)
					{
						UE_LOG(LogTemp, Error, TEXT("UNEXPECTED Op=%d (not 0/1/255) in ACTION: Plot=%s, Flag=%d, Node=%d"),
						       Node->Action.ComparisonType, *Node->Action.PlotName, Node->Action.FlagIndex, Item->NodeIndex);
						checkf(false, TEXT("Unexpected operation byte %d in action (expected 0, 1, or 255)"), Node->Action.ComparisonType);
					}

					// Format operation byte for human readability
					FString OpDescription;
					switch (Node->Action.ComparisonType)
					{
						case 255: // 0xFF - Implicit true (default/no action)
							OpDescription = TEXT("True (implicitly)");
							break;
						case 1: // Explicit true
							OpDescription = TEXT("True (explicitly)");
							break;
						case 0: // Explicit false
							OpDescription = TEXT("False (explicitly)");
							break;
						default: // Unexpected value
							OpDescription = FString::Printf(TEXT("%d (unknown)"), Node->Action.ComparisonType);
							break;
					}

					FString ActionText = FString::Printf(
						TEXT("Action:\nPlot: %s\nFlag: %d\nOp: %s"),
						*Node->Action.PlotName,
						Node->Action.FlagIndex,
						*OpDescription
					);
					ActionTextBlock->SetText(FText::FromString(ActionText));
				}
				else
				{
					ActionTextBlock->SetText(FText::FromString(TEXT("Action:\n(none)")));
				}
			}
		}

		if (OnNodeSelectedCallback.IsBound())
		{
			OnNodeSelectedCallback.Execute();
		}
	}
	else
	{
		// Clear metadata when nothing is selected
		if (ConditionTextBlock.IsValid())
		{
			ConditionTextBlock->SetText(FText::FromString(TEXT("Condition:\n(none)")));
		}
		if (ActionTextBlock.IsValid())
		{
			ActionTextBlock->SetText(FText::FromString(TEXT("Action:\n(none)")));
		}
	}
}

TSharedPtr<SWidget> SDialogTreeView::OnContextMenuOpening()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.BeginSection("TreeOperations", FText::FromString(TEXT("Tree Operations")));
	{
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Expand All")),
			FText::FromString(TEXT("Expand all branches in the tree")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SDialogTreeView::ExpandAll))
		);

		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Collapse All")),
			FText::FromString(TEXT("Collapse all branches in the tree")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SDialogTreeView::CollapseAll))
		);
	}
	MenuBuilder.EndSection();

	if (SelectedItem.IsValid())
	{
		MenuBuilder.BeginSection("BranchOperations", FText::FromString(TEXT("Branch Operations")));
		{
			MenuBuilder.AddMenuEntry(
				FText::FromString(TEXT("Expand This Branch")),
				FText::FromString(TEXT("Expand this branch and all children")),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SDialogTreeView::ExpandBranch, SelectedItem))
			);

			MenuBuilder.AddMenuEntry(
				FText::FromString(TEXT("Collapse This Branch")),
				FText::FromString(TEXT("Collapse this branch and all children")),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SDialogTreeView::CollapseBranch, SelectedItem))
			);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

void SDialogTreeView::BuildTreeFromConversation()
{
	if (!CurrentConversation.IsValid())
		return;

	RootItems.Empty();
	TMap<int32, TSharedPtr<FDialogTreeItem>> FirstOccurrences;

	// Build tree starting from entry links
	for (const FDialogEntryLink& Entry : CurrentConversation->EntryLinks)
	{
		TSharedPtr<FDialogTreeItem> RootItem = MakeShared<FDialogTreeItem>();
		BuildTreeRecursive(Entry.TargetNodeIndex, RootItem, FirstOccurrences);

		if (RootItem->Children.Num() > 0)
		{
			// Entry links are invisible, just add their children as roots
			for (TSharedPtr<FDialogTreeItem>& Child : RootItem->Children)
			{
				Child->Parent.Reset();
				RootItems.Add(Child);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("DialogTreeView: Built tree with %d root items"), RootItems.Num());
}

void SDialogTreeView::BuildTreeRecursive(int32 NodeIndex, TSharedPtr<FDialogTreeItem> ParentItem, TMap<int32, TSharedPtr<FDialogTreeItem>>& FirstOccurrences)
{
	if (!CurrentConversation.IsValid())
		return;

	const FDialogNode* Node = CurrentConversation->FindNode(NodeIndex);
	if (!Node)
		return;

	TSharedPtr<FDialogTreeItem> Item = MakeShared<FDialogTreeItem>();
	Item->NodeIndex = NodeIndex;
	Item->Parent = ParentItem;
	Item->SpeakerID = Node->SpeakerID;
	Item->TLKStringID = Node->TLKStringID;
	Item->bHasCondition = !Node->Condition.PlotName.IsEmpty();
	Item->bHasAction = !Node->Action.PlotName.IsEmpty();
	Item->NumLinks = Node->Links.Num();

	// Set flip-flop state based on parent
	if (ParentItem.IsValid())
	{
		// Flip the state from parent
		Item->bIsNPCTurn = !ParentItem->bIsNPCTurn;
	}
	else
	{
		// Root always starts with NPC
		Item->bIsNPCTurn = true;
	}

	// PRIORITY 1: Check if THIS node has a party condition - party conditions supersede everything
	// This identifies which companion is speaking based on their party flag
	if (!Node->Condition.PlotName.IsEmpty() && Node->Condition.PlotName.Contains(TEXT("party"), ESearchCase::IgnoreCase))
	{
		// This node's condition checks for a party member - resolve to that companion
		Item->ResolvedSpeakerName = ResolveCompanionFromPartyFlag(Node->Condition.FlagIndex);

		UE_LOG(LogTemp, Log, TEXT("Speaker %d resolved to %s based on own party condition (plot: %s, flag: %d)"),
		       Node->SpeakerID, *Item->ResolvedSpeakerName, *Node->Condition.PlotName, Node->Condition.FlagIndex);
	}
	// PRIORITY 2: For Speaker 257, check parent's party condition (hysteresis logic)
	// This handles cases where Speaker 257's response follows a party-gated choice
	else if (Node->SpeakerID == 257 && ParentItem.IsValid())
	{
		// Get parent node to check for party conditions
		const FDialogNode* ParentNode = CurrentConversation->FindNode(ParentItem->NodeIndex);
		if (ParentNode && !ParentNode->Condition.PlotName.IsEmpty())
		{
			// Check if parent has party condition
			if (ParentNode->Condition.PlotName.Contains(TEXT("party"), ESearchCase::IgnoreCase))
			{
				// Resolve companion from party flag
				Item->ResolvedSpeakerName = ResolveCompanionFromPartyFlag(ParentNode->Condition.FlagIndex);

				UE_LOG(LogTemp, Log, TEXT("Speaker 257 resolved to %s based on parent party condition (plot: %s, flag: %d)"),
				       *Item->ResolvedSpeakerName, *ParentNode->Condition.PlotName, ParentNode->Condition.FlagIndex);
			}
			// else: Parent has non-party condition, Speaker 257 remains OWNER (leave ResolvedSpeakerName empty)
		}
		// else: Parent has no condition, Speaker 257 is OWNER (leave ResolvedSpeakerName empty)
	}

	// Check if this is a reference to an existing node
	if (FirstOccurrences.Contains(NodeIndex))
	{
		// This is a reference node - mark it and set reference data
		Item->bIsReference = true;
		Item->ReferencedNodeIndex = NodeIndex;

		// Copy basic data from the original node for display
		TSharedPtr<FDialogTreeItem> OriginalItem = FirstOccurrences[NodeIndex];
		if (OriginalItem.IsValid())
		{
			Item->SpokenText = FString::Printf(TEXT("→ %s"), *OriginalItem->SpokenText);
			// Also copy the paraphrase text so linked lines show their paraphrases
			Item->ParaphraseText = OriginalItem->ParaphraseText;
		}
		else
		{
			Item->SpokenText = FString::Printf(TEXT("→ See Node %d"), NodeIndex);
		}

		// References don't continue recursing to avoid infinite loops
		if (ParentItem.IsValid())
		{
			ParentItem->Children.Add(Item);
		}
		return;
	}

	// This is the first occurrence - mark it
	FirstOccurrences.Add(NodeIndex, Item);
	Item->bIsReference = false;

	// Set display text from TLK strings
	if (DataManager.IsValid())
	{
		// Get the spoken line text (lineTalk)
		FString SpokenLine = DataManager->GetTLKString(Node->TLKStringID);

		// Check if this is actually "not found" vs legitimately empty
		// Look for the specific format: [TLK xxxxx - Not Found]
		bool bIsNotFound = SpokenLine.StartsWith(TEXT("[TLK ")) && SpokenLine.EndsWith(TEXT(" - Not Found]"));

		// Handle empty/special case lines
		if (SpokenLine.IsEmpty() || SpokenLine == TEXT("-1") || bIsNotFound)
		{
			if (Node->Links.Num() == 0)
			{
				// Empty line with no children = end conversation
				Item->SpokenText = TEXT("[[END DIALOG]]");
			}
			else
			{
				// Empty line with children = continue/connector node
				Item->SpokenText = TEXT("[[CONTINUE]]");
			}
		}
		else
		{
			// Add TLK ID prefix for easier sleuthing
			Item->SpokenText = FString::Printf(TEXT("[TLK %d] %s"), Node->TLKStringID, *SpokenLine);
		}

		Item->ParaphraseText = TEXT("");
	}
	else
	{
		// Fallback if no data manager
		Item->ParaphraseText = TEXT("");
		Item->SpokenText = FString::Printf(TEXT("TLK %d"), Node->TLKStringID);
	}

	// Add to parent's children
	if (ParentItem.IsValid())
	{
		ParentItem->Children.Add(Item);
	}

	// Recursively build children
	for (const FDialogLink& Link : Node->Links)
	{
		BuildTreeRecursive(Link.TargetNodeIndex, Item, FirstOccurrences);

		// After building the child, set its paraphrase text from the link
		if (!Item->Children.IsEmpty() && DataManager.IsValid())
		{
			TSharedPtr<FDialogTreeItem> LastChild = Item->Children.Last();
			if (LastChild.IsValid() && !LastChild->bIsReference)
			{
				// Store the paraphrase TLK ID for audio lookup
				LastChild->ParaphraseTLKID = Link.TLKStringID;

				// Get paraphrase from link's TLK (not node's TLK)
				FString ParaphraseText = DataManager->GetTLKString(Link.TLKStringID);

				// Only set paraphrase if it's actually valid text (not empty, not placeholder, not "Not Found")
				// If it's validly empty, leave ParaphraseText blank
				if (!FDialogTreeItem::IsValidlyEmpty(ParaphraseText))
				{
					// SANITY CHECK: Paraphrase text should ONLY exist on player turns
					if (LastChild->bIsNPCTurn)
					{
						UE_LOG(LogTemp, Error, TEXT("ERROR: Found paraphrase text on NPC turn! Node %d has paraphrase: %s"),
						       LastChild->NodeIndex, *ParaphraseText);
						checkf(false, TEXT("Paraphrase text should only exist on player turns (found on NPC node %d)"),
						       LastChild->NodeIndex);
					}

					// Add TLK ID prefix for easier sleuthing
					LastChild->ParaphraseText = FString::Printf(TEXT("[TLK %d] %s"), Link.TLKStringID, *ParaphraseText);
				}
				// else: ParaphraseText remains empty which is what we want for the UI
			}
		}
	}
}

TSharedPtr<FDialogTreeItem> SDialogTreeView::FindTreeItem(int32 NodeIndex)
{
	for (TSharedPtr<FDialogTreeItem>& RootItem : RootItems)
	{
		TSharedPtr<FDialogTreeItem> Found = FindTreeItemRecursive(NodeIndex, RootItem);
		if (Found.IsValid())
			return Found;
	}
	return nullptr;
}

TSharedPtr<FDialogTreeItem> SDialogTreeView::FindTreeItemRecursive(int32 NodeIndex, TSharedPtr<FDialogTreeItem> Item)
{
	if (!Item.IsValid())
		return nullptr;

	if (Item->NodeIndex == NodeIndex)
		return Item;

	for (TSharedPtr<FDialogTreeItem>& Child : Item->Children)
	{
		TSharedPtr<FDialogTreeItem> Found = FindTreeItemRecursive(NodeIndex, Child);
		if (Found.IsValid())
			return Found;
	}

	return nullptr;
}

TSharedPtr<FDialogTreeItem> SDialogTreeView::FindFirstOccurrence(int32 NodeIndex)
{
	for (TSharedPtr<FDialogTreeItem>& RootItem : RootItems)
	{
		TSharedPtr<FDialogTreeItem> Found = FindFirstOccurrenceRecursive(NodeIndex, RootItem);
		if (Found.IsValid())
			return Found;
	}
	return nullptr;
}

TSharedPtr<FDialogTreeItem> SDialogTreeView::FindFirstOccurrenceRecursive(int32 NodeIndex, TSharedPtr<FDialogTreeItem> Item)
{
	if (!Item.IsValid())
		return nullptr;

	// Only return if this is the first occurrence (not a reference)
	if (Item->NodeIndex == NodeIndex && !Item->bIsReference)
		return Item;

	for (TSharedPtr<FDialogTreeItem>& Child : Item->Children)
	{
		TSharedPtr<FDialogTreeItem> Found = FindFirstOccurrenceRecursive(NodeIndex, Child);
		if (Found.IsValid())
			return Found;
	}

	return nullptr;
}

void SDialogTreeView::OnMouseDoubleClick(TSharedPtr<FDialogTreeItem> Item)
{
	// Only handle double-click for reference nodes
	if (!Item.IsValid() || !Item->bIsReference)
		return;

	// Find the first occurrence of the referenced node
	TSharedPtr<FDialogTreeItem> FirstOccurrence = FindFirstOccurrence(Item->ReferencedNodeIndex);

	if (FirstOccurrence.IsValid() && TreeView.IsValid())
	{
		// Navigate to the first occurrence
		TreeView->SetSelection(FirstOccurrence);
		TreeView->RequestScrollIntoView(FirstOccurrence);

		UE_LOG(LogTemp, Log, TEXT("Jumped from reference to first occurrence of node %d"), Item->ReferencedNodeIndex);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not find first occurrence of node %d"), Item->ReferencedNodeIndex);
	}
}

int32 SDialogTreeView::DetectConversationOwner() const
{
	if (!CurrentConversation.IsValid())
		return -1;

	// Count speaker IDs, excluding known player IDs
	TMap<int32, int32> NPCSpeakerCounts;

	for (const FDialogNode& Node : CurrentConversation->Nodes)
	{
		// Skip player speakers (hardcoded list from flip-flop analysis)
		if (FDialogTreeItem::KnownPlayerSpeakerIDs.Contains(Node.SpeakerID))
			continue;

		// Count this NPC speaker
		int32& Count = NPCSpeakerCounts.FindOrAdd(Node.SpeakerID, 0);
		Count++;
	}

	// Find most frequent NPC speaker
	int32 MostFrequentSpeaker = -1;
	int32 MaxCount = 0;

	for (const auto& Pair : NPCSpeakerCounts)
	{
		if (Pair.Value > MaxCount)
		{
			MaxCount = Pair.Value;
			MostFrequentSpeaker = Pair.Key;
		}
	}

	if (MostFrequentSpeaker != -1)
	{
		UE_LOG(LogTemp, Log, TEXT("Detected conversation owner: Speaker %d (%d lines)"),
			MostFrequentSpeaker, MaxCount);
	}

	return MostFrequentSpeaker;
}

FSlateColor SDialogTreeView::GetSpeakerColor(int32 SpeakerID) const
{
	if (SpeakerID == 1)
		return FLinearColor(0.3f, 0.5f, 1.0f); // Player - Blue
	else if (SpeakerID == 10)
		return FLinearColor(0.5f, 1.0f, 0.5f); // NPC - Green
	else
		return FLinearColor::Gray;
}

FString SDialogTreeView::ResolveCompanionFromPartyFlag(int32 FlagIndex)
{
	// ============================================================================
	// PARTY FLAG REFERENCE MAP (plt_gen00pt_party)
	// ============================================================================
	// Based on bytecode analysis of gen00pt_party.txt and conversation XML usage
	//
	// INDIVIDUAL COMPANION FLAGS (256-271, 276-277):
	//   256/257 = Carver      (odd=257 is primary, reversed from others)
	//   258/259 = Bethany
	//   260/261 = Varric
	//   262/263 = Aveline
	//   264/265 = Isabela
	//   266/267 = Merrill
	//   268/269 = Anders
	//   270/271 = Fenris
	//   276/277 = Sebastian
	//
	// SPECIAL COMPOSITE FLAGS (272-275):
	//   272 = Any female companions in party OR player is female
	//         Checks: Bethany || Merrill || Isabela || GetCreatureGender(Hero) == 2
	//
	//   273 = Party contains mage/s (NPC dialog context)
	//         Checks: Bethany || Merrill || Anders
	//         Used by NPCs talking TO the player about mages (e.g., Grace in mag101)
	//
	//   274 = Party contains mage/s (companion banter context)
	//         Checks: Bethany || Merrill || Anders
	//         Used in companion-to-companion banter (follower_banter.xml)
	//         Functionally identical to 273, different context
	//
	//   275 = Player is alone (no active companions)
	//         Checks: GetParty().size == 1
	//         Used for solo-only dialog options and combat barks
	// ============================================================================

	// Special composite flags (272-275)
	if (FlagIndex == 272)
	{
		return TEXT("[Party: Female/s or Female Player]");
	}
	else if (FlagIndex == 273 || FlagIndex == 274)
	{
		return TEXT("[Party: Contains Mage/s]");
	}
	else if (FlagIndex == 275)
	{
		return TEXT("[Party: Solo Player]");
	}

	// Individual companion flags (256-271, 276-277)
	// Carver is unique: uses ODD flag 257 as primary (reversed from others)
	if (FlagIndex == 257 || FlagIndex == 256)
	{
		return TEXT("Carver");
	}

	// For all other companions, use even flags (256-278 range, even = in party)
	// Note: Normalize to base even flag
	int32 BaseFlag = (FlagIndex % 2 == 0) ? FlagIndex : (FlagIndex - 1);

	switch (BaseFlag)
	{
	case 258: return TEXT("Bethany");
	case 260: return TEXT("Varric");
	case 262: return TEXT("Aveline");
	case 264: return TEXT("Isabela");
	case 266: return TEXT("Merrill");
	case 268: return TEXT("Anders");
	case 270: return TEXT("Fenris");
	case 276: return TEXT("Sebastian");
	default:
		UE_LOG(LogTemp, Warning, TEXT("Unknown party flag: %d"), FlagIndex);
		return FString::Printf(TEXT("Unknown Flag %d"), FlagIndex);
	}
}
