// Copyright Epic Games, Inc. All Rights Reserved.

#include "Audio/DialogAudioManager.h"
#include "Audio/AudioUtils.h"
#include "Data/DialogDataManager.h"
#include "HAL/FileManager.h"

FDialogAudioManager::FDialogAudioManager()
{
	AudioPlayer = MakeUnique<FDialogAudioPlayer>();
}

FDialogAudioManager::~FDialogAudioManager()
{
	StopAudio();
}

void FDialogAudioManager::Initialize(TSharedPtr<FDialogDataManager> InDataManager)
{
	DataManager = InDataManager;
}

bool FDialogAudioManager::PlayDialogAudio(int32 SpokenTLKID, int32 ParaphraseTLKID)
{
	if (!DataManager.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("DialogAudioManager: No data manager initialized"));
		return false;
	}

	// Get player gender
	bool bIsMale = (DataManager->GetPlayerGender() == EPlayerGender::Male);

	// Priority 1: Try spoken TLK ID
	if (SpokenTLKID > 0)
	{
		if (TryPlayAudio(SpokenTLKID, bIsMale))
		{
			UE_LOG(LogTemp, Log, TEXT("DialogAudioManager: Playing spoken audio for TLK %d"), SpokenTLKID);
			return true;
		}
	}

	// Priority 2: Try paraphrase TLK ID as fallback
	if (ParaphraseTLKID > 0)
	{
		if (TryPlayAudio(ParaphraseTLKID, bIsMale))
		{
			UE_LOG(LogTemp, Log, TEXT("DialogAudioManager: Playing paraphrase audio for TLK %d (fallback)"), ParaphraseTLKID);
			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("DialogAudioManager: No audio found for spoken TLK %d or paraphrase TLK %d"),
		SpokenTLKID, ParaphraseTLKID);
	return false;
}

void FDialogAudioManager::StopAudio()
{
	if (AudioPlayer.IsValid())
	{
		AudioPlayer->StopAudio();
	}
}

bool FDialogAudioManager::IsPlaying() const
{
	return AudioPlayer.IsValid() && AudioPlayer->IsPlaying();
}

void FDialogAudioManager::SetVolume(float Volume)
{
	if (AudioPlayer.IsValid())
	{
		AudioPlayer->SetVolume(Volume);
	}
}

bool FDialogAudioManager::TryPlayAudio(int32 TLKID, bool bIsMale)
{
	// Try audio mapper first (uses dialog.csv)
	FString AudioDir = DataManager->GetAudioDirectory();
	FString AudioFilePath = DataManager->GetAudioMapper().GetAudioFilePath(TLKID,
		bIsMale ? EPlayerGender::Male : EPlayerGender::Female, AudioDir);

	if (!AudioFilePath.IsEmpty() && IFileManager::Get().FileExists(*AudioFilePath))
	{
		// Found via audio mapper
		return AudioPlayer->PlayAudio(AudioFilePath);
	}

	// Fallback: Try FNV32 hash
	uint32 AudioFileID = FAudioUtils::ComputeAudioFileID(TLKID, bIsMale);
	FString FNVPath = FAudioUtils::BuildAudioFilePath(AudioDir, AudioFileID);

	if (IFileManager::Get().FileExists(*FNVPath))
	{
		// Found via FNV32 hash
		UE_LOG(LogTemp, Log, TEXT("DialogAudioManager: Using FNV32 fallback for TLK %d -> %u"), TLKID, AudioFileID);
		return AudioPlayer->PlayAudio(FNVPath);
	}

	// Not found
	return false;
}