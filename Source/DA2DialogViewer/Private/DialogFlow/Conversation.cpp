// Copyright Epic Games, Inc. All Rights Reserved.

#include "DialogFlow/Conversation.h"

FConversation::FConversation()
{
}

FConversation::~FConversation()
{
}

const FDialogNode* FConversation::FindNode(int32 NodeIndex) const
{
	for (const FDialogNode& Node : Nodes)
	{
		if (Node.NodeIndex == NodeIndex)
		{
			return &Node;
		}
	}
	return nullptr;
}

FDialogNode* FConversation::FindNode(int32 NodeIndex)
{
	for (FDialogNode& Node : Nodes)
	{
		if (Node.NodeIndex == NodeIndex)
		{
			return &Node;
		}
	}
	return nullptr;
}

TArray<int32> FConversation::GetEntryNodeIndices() const
{
	TArray<int32> EntryIndices;
	for (const FDialogEntryLink& Entry : EntryLinks)
	{
		EntryIndices.Add(Entry.TargetNodeIndex);
	}
	return EntryIndices;
}

void FConversation::Clear()
{
	ConversationName.Empty();
	EntryLinks.Empty();
	Nodes.Empty();
}

void FConversation::DebugPrint() const
{
	UE_LOG(LogTemp, Log, TEXT("=== Conversation: %s ==="), *ConversationName);
	UE_LOG(LogTemp, Log, TEXT("Entry Links: %d"), EntryLinks.Num());
	for (int32 i = 0; i < EntryLinks.Num(); ++i)
	{
		const FDialogEntryLink& Entry = EntryLinks[i];
		UE_LOG(LogTemp, Log, TEXT("  Entry %d -> Node %d"), i, Entry.TargetNodeIndex);
	}

	UE_LOG(LogTemp, Log, TEXT("Nodes: %d"), Nodes.Num());
	for (const FDialogNode& Node : Nodes)
	{
		UE_LOG(LogTemp, Log, TEXT("  Node %d: Speaker=%d, TLK=%d, Links=%d"),
			Node.NodeIndex, Node.SpeakerID, Node.TLKStringID, Node.Links.Num());

		for (const FDialogLink& Link : Node.Links)
		{
			UE_LOG(LogTemp, Log, TEXT("    -> Node %d (Type=%d)"),
				Link.TargetNodeIndex, (uint8)Link.ResponseType);
		}
	}
}
