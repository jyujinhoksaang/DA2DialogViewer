// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Response type for dialog wheel positioning and behavior
 * Maps to conversation_categories.csv from DA2 data files
 */
UENUM()
enum class EResponseType : uint8
{
	Neutral = 0			UMETA(DisplayName = "Neutral"),			// Generic neutral choice (position 2 - top area)
	Aggressive = 1		UMETA(DisplayName = "Aggressive"),		// Aggressive/harsh tone (position 1 - 5 o'clock)
	Diplomatic = 2		UMETA(DisplayName = "Diplomatic"),		// Diplomatic/helpful tone (position 0 - 1 o'clock)
	Humorous = 3		UMETA(DisplayName = "Humorous"),		// Humorous/sarcastic tone (position 4 - 3 o'clock)
	Bonus = 4			UMETA(DisplayName = "Bonus"),			// Special personality-locked choices (position 3 - 7 o'clock)
	Follower = 5		UMETA(DisplayName = "Follower"),		// Companion ability calls (position 2 - top area)
	Choice1 = 6			UMETA(DisplayName = "Choice 1"),		// Generic choice #1 (position 0 - 1 o'clock)
	Choice2 = 7			UMETA(DisplayName = "Choice 2"),		// Generic choice #2 (position 1 - 5 o'clock)
	Choice3 = 8			UMETA(DisplayName = "Choice 3"),		// Generic choice #3 (position 4 - 3 o'clock)
	Choice4 = 9			UMETA(DisplayName = "Choice 4"),		// Generic choice #4 (position 2 - top area)
	Choice5 = 10		UMETA(DisplayName = "Choice 5"),		// Generic choice #5 (position 3 - 7 o'clock)
	Investigate = 11	UMETA(DisplayName = "Investigate"),		// Investigation/inquiry options (position 6 - 9 o'clock)
	AutoContinue = 255	UMETA(DisplayName = "Auto Continue")	// Non-interactive, automatic continuation
};

/**
 * Plot reference for conditions and actions
 */
struct FPlotReference
{
	// Plot name (e.g., "plt_and100pt_tranquility")
	FString PlotName;

	// Plot flag index (-1 = no specific flag)
	int32 FlagIndex;

	// Comparison type
	uint8 ComparisonType;

	FPlotReference()
		: FlagIndex(-1)
		, ComparisonType(255)
	{}
};

/**
 * Link between dialog nodes (LINK in XML)
 */
struct FDialogLink
{
	// Target node index in conversation
	int32 TargetNodeIndex;

	// TLK string ID for link text
	int32 TLKStringID;

	// Response type for dialog wheel
	EResponseType ResponseType;

	// Icon override
	uint8 IconOverride;

	// Condition flags
	uint32 ConditionFlags;

	// Cached preview text
	FString PreviewText;

	FDialogLink()
		: TargetNodeIndex(-1)
		, TLKStringID(-1)
		, ResponseType(EResponseType::AutoContinue)
		, IconOverride(255)
		, ConditionFlags(0)
	{}
};

/**
 * Dialog node representing a LINE in conversation XML
 */
struct FDialogNode
{
	// Node index in conversation
	int32 NodeIndex;

	// Speaker ID (1 = player, 10 = NPC, etc.)
	int32 SpeakerID;

	// TLK string ID for line text
	int32 TLKStringID;

	// Condition to show this node
	FPlotReference Condition;

	// Action to execute when node is shown
	FPlotReference Action;

	// Links to child nodes
	TArray<FDialogLink> Links;

	// Cached debug text
	FString DebugText;

	FDialogNode()
		: NodeIndex(-1)
		, SpeakerID(-1)
		, TLKStringID(-1)
	{}
};

/**
 * Entry link into conversation
 */
struct FDialogEntryLink
{
	// Target node index
	int32 TargetNodeIndex;

	// TLK string ID (usually empty for entries)
	int32 TLKStringID;

	// Icon override
	uint8 IconOverride;

	// Condition flags
	uint32 ConditionFlags;

	FDialogEntryLink()
		: TargetNodeIndex(-1)
		, TLKStringID(-1)
		, IconOverride(255)
		, ConditionFlags(0)
	{}
};
