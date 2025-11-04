// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Audio utility functions for Dragon Age 2 dialog system
 * Handles FNV32 hashing and audio file path resolution
 */
class FAudioUtils
{
public:
	/**
	 * Compute FNV-1a 32-bit hash of a string
	 * Used by Dragon Age 2 to generate audio file IDs from string identifiers
	 *
	 * @param String Input string to hash
	 * @return 32-bit FNV hash value
	 */
	static uint32 ComputeFNV32Hash(const FString& String);

	/**
	 * Compute audio file ID from TLK string ID with gender suffix
	 * Converts TLK ID to string, appends gender suffix (_M or _F), then computes FNV32 hash
	 *
	 * @param TLKID The TLK string ID
	 * @param bIsMale True for male (_M suffix), false for female (_F suffix)
	 * @return FNV32 hash that matches audio file ID
	 */
	static uint32 ComputeAudioFileID(int32 TLKID, bool bIsMale);

	/**
	 * Check if an audio file exists at the given path
	 *
	 * @param AudioDirectory Base directory for audio files (e.g., "Data/all_conv_wav")
	 * @param AudioFileID Numeric audio file ID
	 * @return True if the .wav file exists
	 */
	static bool DoesAudioFileExist(const FString& AudioDirectory, uint32 AudioFileID);

	/**
	 * Build full path to audio file
	 *
	 * @param AudioDirectory Base directory for audio files
	 * @param AudioFileID Numeric audio file ID
	 * @return Full path like "Data/all_conv_wav/267111449.wav"
	 */
	static FString BuildAudioFilePath(const FString& AudioDirectory, uint32 AudioFileID);
};