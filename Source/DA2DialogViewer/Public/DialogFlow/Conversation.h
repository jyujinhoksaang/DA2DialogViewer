// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogNode.h"

/**
 * Complete conversation graph loaded from XML
 */
class FConversation
{
public:
	FConversation();
	~FConversation();

	// Conversation name/identifier
	FString ConversationName;

	// Dialog owner tag (extracted from UTC file that references this conversation)
	FString OwnerTag;

	// Entry links to start conversation
	TArray<FDialogEntryLink> EntryLinks;

	// All dialog nodes in conversation
	TArray<FDialogNode> Nodes;

	// Find node by index
	const FDialogNode* FindNode(int32 NodeIndex) const;
	FDialogNode* FindNode(int32 NodeIndex);

	// Get entry node indices
	TArray<int32> GetEntryNodeIndices() const;

	// Clear all data
	void Clear();

	// Debug: Print conversation structure
	void DebugPrint() const;
};
