// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Player gender for audio selection
 */
UENUM()
enum class EPlayerGender : uint8
{
	Male = 0	UMETA(DisplayName = "Male"),
	Female = 1	UMETA(DisplayName = "Female")
};

/**
 * Dialog audio mapping entry from dialog.csv
 */
struct FDialogAudioInfo
{
	// Dialog ID (TLK string reference)
	int32 DialogID;

	// Audio file ID for male player
	FString MaleAudioFile;

	// Audio file ID for female player
	FString FemaleAudioFile;

	// Sound bank name
	FString SoundBank;

	FDialogAudioInfo()
		: DialogID(-1)
	{}
};

/**
 * Maps TLK dialog IDs to audio file names
 */
class FAudioMapper
{
public:
	FAudioMapper();
	~FAudioMapper();

	// Load dialog.csv mapping
	bool LoadDialogCSV(const FString& CSVPath);

	// Get audio file for dialog ID and gender
	FString GetAudioFile(int32 DialogID, EPlayerGender Gender) const;

	// Get full path to audio file
	FString GetAudioFilePath(int32 DialogID, EPlayerGender Gender, const FString& AudioDirectory) const;

	// Check if dialog ID has audio mapping
	bool HasAudioMapping(int32 DialogID) const;

	// Clear all mappings
	void Clear();

private:
	// Map from DialogID to audio info
	TMap<int32, FDialogAudioInfo> AudioMap;
};
