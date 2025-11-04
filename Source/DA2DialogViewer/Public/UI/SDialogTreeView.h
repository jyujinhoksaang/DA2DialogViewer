// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

class FConversation;
struct FDialogNode;
class SDialogWheel;
class FDialogDataManager;
class FDialogAudioManager;

/**
 * Speaker type classification for dialog lines
 */
enum class ESpeakerType : uint8
{
	Player, // Blue - Hawke (player character)
	Owner, // Red - Conversation owner (main NPC)
	Henchman, // Green - Party companion
	OtherNPC, // Magenta - Other named NPC (NOT FULLY IMPLEMENTED)
	Reference // Gray - Reference to another node
};

/**
 * Tree item representing a dialog line in the tree
 */
class FDialogTreeItem
{
public:
	// Node index in conversation
	int32 NodeIndex;

	// Parent item (nullptr for root entries)
	TSharedPtr<FDialogTreeItem> Parent;

	// Child items
	TArray<TSharedPtr<FDialogTreeItem>> Children;

	// Cached node data for display
	int32 SpeakerID;
	int32 TLKStringID; // Spoken line TLK ID (from dialog node)
	int32 ParaphraseTLKID; // Paraphrase TLK ID (from dialog link)
	bool bHasCondition;
	bool bHasAction;
	int32 NumLinks;

	// Display text
	FString ParaphraseText; // Short preview (for player choices)
	FString SpokenText; // Full dialog line

	// Reference tracking
	bool bIsReference; // True if this is a reference to an existing node
	int32 ReferencedNodeIndex; // The original node this references (for display)

	// Flip-flop tracking - determines if this should be NPC or Player based on alternation
	bool bIsNPCTurn; // True = NPC turn, False = Player turn

	// Speaker 257 resolution - dynamically determined based on parent party conditions
	FString ResolvedSpeakerName; // "OWNER", "Carver", "Bethany", etc. (empty if not Speaker 257)

	// Static tracking for encountered player speaker IDs (to avoid log spam)
	static TSet<int32> EncounteredPlayerSpeakerIDs;

	// Hardcoded player speaker IDs (identified via flip-flop analysis of all conversations)
	// These speaker IDs are used >95% of the time for player lines
	// Derived from analyzing 704 DA2 conversations with flip-flop logic
	static TSet<int32> KnownPlayerSpeakerIDs;

	FDialogTreeItem()
		: NodeIndex(-1)
		  , SpeakerID(-1)
		  , TLKStringID(-1)
		  , ParaphraseTLKID(-1)
		  , bHasCondition(false)
		  , bHasAction(false)
		  , NumLinks(0)
		  , bIsReference(false)
		  , ReferencedNodeIndex(-1)
		  , bIsNPCTurn(false)
	{
	}

	// Get condition/action indicator string
	FString GetIndicatorString() const
	{
		if (bHasCondition && bHasAction)
			return TEXT("CA");
		else if (bHasCondition)
			return TEXT("C");
		else if (bHasAction)
			return TEXT("A");
		else
			return TEXT("");
	}

	// Check if a text string is "validly empty" (empty, placeholder, or not found)
	static bool IsValidlyEmpty(const FString& Text)
	{
		return Text.IsEmpty() || Text.Contains("[[") || Text.EndsWith(TEXT("Found]"));
	}

	// Check if this line is an ambient line
	// Ambient = has valid spoken text but no children (links)
	bool IsAmbient() const
	{
		return !IsValidlyEmpty(SpokenText) && NumLinks == 0;
	}

	// Validate player line has proper paraphrase/spoken text combination
	void ValidatePlayerLine() const
	{
		// Use the helper to check if text is validly empty (handles all placeholder cases)
		bool bHasValidParaphrase = !IsValidlyEmpty(ParaphraseText);
		bool bHasValidSpokenText = !IsValidlyEmpty(SpokenText);

		// UPDATED VALIDATION LOGIC based on real-world data:
		// Valid combinations:
		//   1. Both paraphrase AND spoken text (normal player choice)
		//   2. Neither paraphrase NOR spoken text (continue node)
		//   3. Paraphrase but NO spoken text (we need to understand this case better)
		// INVALID combination:
		//   - NO paraphrase but HAS spoken text (should never happen!)

		if (!bHasValidParaphrase && bHasValidSpokenText)
		{
			UE_LOG(LogTemp, Error, TEXT("INVALID Player Line: NO paraphrase but HAS spoken text! Paraphrase='%s', Spoken='%s'"),
			       *ParaphraseText, *SpokenText);
			checkf(false, TEXT("Player line has spoken text but no paraphrase - this should not happen!"));
		}
	}

	// Check if this line is from a henchman (based on party plot conditions)
	bool IsHenchman() const
	{
		// Henchman is ONLY identified when ResolvedSpeakerName is a specific companion name (FOLLOWER_STATE_ACTIVE check)
		// NOT for composite party flags like "[Party: Solo Player]", "[Party: Contains Mage/s]", etc.

		// Individual companion names (these indicate FOLLOWER_STATE_ACTIVE):
		// "Carver", "Bethany", "Varric", "Aveline", "Isabela", "Merrill", "Anders", "Fenris", "Sebastian"

		// Composite party flags (these do NOT indicate a henchman is speaking):
		// "[Party: Female/s or Female Player]", "[Party: Contains Mage/s]", "[Party: Solo Player]"

		// Check if ResolvedSpeakerName is set and does NOT start with "[Party:"
		if (!ResolvedSpeakerName.IsEmpty() && !ResolvedSpeakerName.StartsWith(TEXT("[Party:")))
		{
			return true; // This is an individual companion speaking (green color)
		}

		return false; // Either empty or composite flag (not a henchman speaking)
	}

	// Determine speaker type - SINGLE SOURCE OF TRUTH for both text and color
	// This method consolidates all speaker identification logic in one place
	ESpeakerType GetSpeakerType() const
	{
		// PRIORITY 1 (HIGHEST): Reference nodes
		if (bIsReference)
		{
			return ESpeakerType::Reference;
		}

		// PRIORITY 2: Ambient line detection
		// Ambient lines (spoken text with no children) ALWAYS use the conversation owner
		if (IsAmbient())
		{
			return ESpeakerType::Owner;
		}

		// PRIORITY 3: Player detection via hardcoded speaker IDs
		// These IDs were identified via flip-flop analysis of all DA2 conversations
		if (KnownPlayerSpeakerIDs.Contains(SpeakerID))
		{
			return ESpeakerType::Player;
		}

		// PRIORITY 4: Empty/Continue/End Dialog lines
		// These are always OWNER (red) even if they have party conditions
		if (SpokenText.Contains(TEXT("[[CONTINUE]]")) || SpokenText.Contains(TEXT("[[END DIALOG]]")))
		{
			return ESpeakerType::Owner;
		}

		// PRIORITY 5: NPC type identification
		// Now we know it's an NPC turn with actual dialogue - determine which type

		// Henchman (party member)
		if (IsHenchman())
		{
			return ESpeakerType::Henchman;
		}

		// Speaker 257 defaults to OWNER
		if (SpeakerID == 257)
		{
			return ESpeakerType::Owner;
		}

		// Other named NPC (NOT FULLY IMPLEMENTED)
		return ESpeakerType::OtherNPC;
	}

	// Get speaker type string using flip-flop logic and conversation context
	FString GetSpeakerString(const FString& OwnerTag = TEXT("")) const
	{
		// PRIORITY 1 (HIGHEST): Ambient line detection
		// Ambient lines (spoken text with no children) ALWAYS use the conversation owner
		// This supersedes even player detection because ambient lines are always NPC lines
		if (IsAmbient())
		{
			// Validate that the conversation has an owner defined
			// If not, this is a data integrity issue (possibly cut content)
			checkf(!OwnerTag.IsEmpty(),
			       TEXT("Ambient line detected (Node %d, Speaker %d, TLK %d) but conversation has NO OWNER! This may be cut content or a data error."),
			       NodeIndex, SpeakerID, TLKStringID);

			UE_LOG(LogTemp, Log, TEXT("Ambient line detected: Node %d, Speaker %d -> treating as OWNER (%s)"),
			       NodeIndex, SpeakerID, *OwnerTag);

			return TEXT("OWNER");
		}

		// PRIORITY 2: Player detection via flip-flop
		// This supersedes EVERYTHING (except ambient) - if flip-flop says player, it's PLAYER (blue)
		if (!bIsNPCTurn)
		{
			// Log unique speaker IDs for player lines (only once per ID)
			if (!EncounteredPlayerSpeakerIDs.Contains(SpeakerID))
			{
				EncounteredPlayerSpeakerIDs.Add(SpeakerID);
				UE_LOG(LogTemp, Warning, TEXT("Found PLAYER with Speaker ID: %d"), SpeakerID);
			}

			return TEXT("PLAYER");
		}

		// PRIORITY 3: Empty/Continue/End Dialog lines
		// If this is an NPC line but has no actual dialogue, treat as OWNER
		// Owner name is displayed in static header, so just show "OWNER"
		if (SpokenText.Contains(TEXT("[[CONTINUE]]")) || SpokenText.Contains(TEXT("[[END DIALOG]]")))
		{
			return TEXT("OWNER");
		}

		// PRIORITY 4: NPC type identification
		// Now we know it's an NPC turn with actual dialogue - determine which type

		// Check if this is a party henchman (based on resolved name from party conditions)
		if (!ResolvedSpeakerName.IsEmpty())
		{
			// WORKAROUND: Composite party flags (e.g., "[Party: Solo Player]") should display as OWNER
			// These are NOT individual companions speaking, they're just party-related conditions
			// NOTE: This logic is incomplete - we don't fully understand all speaker ID patterns yet
			// For now, we assume composite party flags with NPC turn = OWNER
			if (ResolvedSpeakerName.StartsWith(TEXT("[Party:")))
			{
				return TEXT("OWNER"); // Composite party flag → OWNER
			}
			else
			{
				return ResolvedSpeakerName; // Individual companion name (e.g., "Carver", "Bethany")
			}
		}

		// Check if henchman (fallback detection)
		if (IsHenchman())
		{
			return TEXT("HENCHMAN");
		}

		// Speaker 257 defaults to OWNER
		// Owner name is displayed in static header, so just show "OWNER"
		if (SpeakerID == 257)
		{
			return TEXT("OWNER");
		}

		// Other named NPC
		// NOTE: Incomplete logic - some other speaker IDs may also mean OWNER or other named NPCs
		// We don't have enough information to properly identify all speaker types yet
		UE_LOG(LogTemp, VeryVerbose, TEXT("TODO: Identify named NPC with Speaker ID: %d"), SpeakerID);
		return FString::Printf(TEXT("Speaker %d"), SpeakerID);
	}
};

/**
 * Hierarchical tree view for dialog lines
 * Replaces the visual node graph with a collapsible/expandable list
 */
class SDialogTreeView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogTreeView)
		{
		}

		SLATE_EVENT(FSimpleDelegate, OnNodeSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FDialogDataManager> InDataManager);

	// Load conversation into tree
	void LoadConversation(TSharedPtr<FConversation> InConversation);

	// Clear tree
	void Clear();

	// Get currently selected node
	const FDialogNode* GetSelectedNode() const;

	// Set dialog wheel reference for interaction
	void SetDialogWheel(TSharedPtr<SDialogWheel> InDialogWheel) { DialogWheel = InDialogWheel; }

	// Navigate to node by index
	void NavigateToNode(int32 NodeIndex);

	// Navigate to player choice and auto-play audio
	// If player line has valid spoken text: navigate to player line and play audio
	// If player line has no spoken text: navigate to first child (LINK) and play that audio
	void NavigateToPlayerChoice(int32 PlayerNodeIndex);

	// Expand/collapse operations
	void ExpandAll();
	void CollapseAll();
	void ExpandBranch(TSharedPtr<FDialogTreeItem> Item);
	void CollapseBranch(TSharedPtr<FDialogTreeItem> Item);

private:
	// Generate tree row widget
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FDialogTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable);

	// Get children for tree item
	void OnGetChildren(TSharedPtr<FDialogTreeItem> Item, TArray<TSharedPtr<FDialogTreeItem>>& OutChildren);

	// Selection changed
	void OnSelectionChanged(TSharedPtr<FDialogTreeItem> Item, ESelectInfo::Type SelectInfo);

	// Double-click handler - jump to original node for references
	void OnMouseDoubleClick(TSharedPtr<FDialogTreeItem> Item);

	// Context menu
	TSharedPtr<SWidget> OnContextMenuOpening();

	// Build tree from conversation
	void BuildTreeFromConversation();

	// Recursively build tree items
	void BuildTreeRecursive(int32 NodeIndex, TSharedPtr<FDialogTreeItem> ParentItem, TMap<int32, TSharedPtr<FDialogTreeItem>>& FirstOccurrences);

	// Find tree item by node index
	TSharedPtr<FDialogTreeItem> FindTreeItem(int32 NodeIndex);
	TSharedPtr<FDialogTreeItem> FindTreeItemRecursive(int32 NodeIndex, TSharedPtr<FDialogTreeItem> Item);

	// Find first (non-reference) occurrence of a node
	TSharedPtr<FDialogTreeItem> FindFirstOccurrence(int32 NodeIndex);
	TSharedPtr<FDialogTreeItem> FindFirstOccurrenceRecursive(int32 NodeIndex, TSharedPtr<FDialogTreeItem> Item);

	// Get color for speaker
	FSlateColor GetSpeakerColor(int32 SpeakerID) const;

	// Resolve companion name from party flag
	static FString ResolveCompanionFromPartyFlag(int32 FlagIndex);

private:
	// Detect conversation owner using heuristic: most frequent NPC speaker (excluding player IDs)
	int32 DetectConversationOwner() const;

private:
	// Data manager reference
	TSharedPtr<FDialogDataManager> DataManager;

	// Current conversation
	TSharedPtr<FConversation> CurrentConversation;

	// Detected owner speaker ID for current conversation (using heuristic)
	int32 DetectedOwnerSpeakerID;

	// Tree view widget
	TSharedPtr<STreeView<TSharedPtr<FDialogTreeItem>>> TreeView;

	// Root items (entry points)
	TArray<TSharedPtr<FDialogTreeItem>> RootItems;

	// Currently selected item
	TSharedPtr<FDialogTreeItem> SelectedItem;

	// Dialog wheel reference
	TSharedPtr<SDialogWheel> DialogWheel;

	// Node selection callback
	FSimpleDelegate OnNodeSelectedCallback;

	// Metadata display text blocks
	TSharedPtr<STextBlock> ConditionTextBlock;
	TSharedPtr<STextBlock> ActionTextBlock;

	// Audio manager for playback
	TSharedPtr<FDialogAudioManager> AudioManager;
};
