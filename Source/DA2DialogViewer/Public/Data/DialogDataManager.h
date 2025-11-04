// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Plot/PlotDatabase.h"
#include "Plot/PlotState.h"
#include "Audio/AudioMapper.h"
#include "DialogFlow/Conversation.h"

/**
 * Central data manager for dialog system
 * Singleton that manages all data loading and access
 */
class FDialogDataManager : public TSharedFromThis<FDialogDataManager>
{
public:
	FDialogDataManager();
	~FDialogDataManager();

	/** Initialize data manager - load plots.csv and dialog.csv */
	bool Initialize(const FString& DataDirectory);

	/** Load a conversation from XML file */
	bool LoadConversation(const FString& ConversationPath);

	/** Get current conversation */
	TSharedPtr<FConversation> GetCurrentConversation() const { return CurrentConversation; }

	/** Get plot database */
	const FPlotDatabase& GetPlotDatabase() const { return PlotDatabase; }

	/** Get plot state */
	FPlotState& GetPlotState() { return PlotState; }
	const FPlotState& GetPlotState() const { return PlotState; }

	/** Get audio mapper */
	const FAudioMapper& GetAudioMapper() const { return AudioMapper; }

	/** Get data directory */
	const FString& GetDataDirectory() const { return DataDirectory; }

	/** Get audio directory */
	FString GetAudioDirectory() const;

	/** Reset plot state to default */
	void ResetPlotState();

	/** Set player gender for audio selection */
	void SetPlayerGender(EPlayerGender Gender) { PlayerGender = Gender; }

	/** Get player gender */
	EPlayerGender GetPlayerGender() const { return PlayerGender; }

	/** Get TLK string by ID */
	FString GetTLKString(int32 TLKID) const;

	/** Process TLK string for rich text and special markers */
	FString ProcessTLKString(const FString& RawString) const;

	/** Find owner tag from UTC files that reference this conversation */
	FString FindOwnerTagForConversation(const FString& ConversationName) const;

private:
	/** Data directory path */
	FString DataDirectory;

	/** Plot database (plots.csv) */
	FPlotDatabase PlotDatabase;

	/** Current plot state */
	FPlotState PlotState;

	/** Audio mapper (dialog.csv) */
	FAudioMapper AudioMapper;

	/** Currently loaded conversation */
	TSharedPtr<FConversation> CurrentConversation;

	/** Player gender for audio selection */
	EPlayerGender PlayerGender;

	/** TLK string map (TLK ID -> localized text) */
	TMap<int32, FString> TLKStrings;

	/** Is initialized */
	bool bIsInitialized;
};
