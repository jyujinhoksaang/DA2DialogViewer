// Copyright Epic Games, Inc. All Rights Reserved.

#include "Audio/AudioUtils.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

uint32 FAudioUtils::ComputeFNV32Hash(const FString& String)
{
	// FNV-1a 32-bit hash algorithm
	// Used by Dragon Age 2 for generating audio file IDs

	// FNV-1a prime and offset basis for 32-bit
	const uint32 FNV_Prime = 16777619u;
	const uint32 FNV_OffsetBasis = 2166136261u;

	uint32 Hash = FNV_OffsetBasis;

	// Convert string to lowercase ASCII bytes (DA2 uses lowercase for hashing)
	FString LowerString = String.ToLower();

	// Process each byte
	for (int32 i = 0; i < LowerString.Len(); ++i)
	{
		TCHAR Char = LowerString[i];

		// FNV-1a: XOR with byte, then multiply by prime
		Hash ^= static_cast<uint8>(Char);
		Hash *= FNV_Prime;
	}

	return Hash;
}

uint32 FAudioUtils::ComputeAudioFileID(int32 TLKID, bool bIsMale)
{
	// Build string: "TLKID_M" or "TLKID_F"
	// Example: TLK ID 6000680 with male = "6000680_m"
	FString IDString = FString::Printf(TEXT("%d_%s"), TLKID, bIsMale ? TEXT("m") : TEXT("f"));

	return ComputeFNV32Hash(IDString);
}

bool FAudioUtils::DoesAudioFileExist(const FString& AudioDirectory, uint32 AudioFileID)
{
	FString FilePath = BuildAudioFilePath(AudioDirectory, AudioFileID);
	return IFileManager::Get().FileExists(*FilePath);
}

FString FAudioUtils::BuildAudioFilePath(const FString& AudioDirectory, uint32 AudioFileID)
{
	// Build path like "Data/all_conv_wav/267111449.wav"
	return FPaths::Combine(AudioDirectory, FString::Printf(TEXT("%u.wav"), AudioFileID));
}