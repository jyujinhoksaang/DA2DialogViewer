// Copyright Epic Games, Inc. All Rights Reserved.

#include "Audio/DialogAudioPlayer.h"
#include "HAL/FileManager.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <mmsystem.h>
#include "Windows/HideWindowsPlatformTypes.h"

#pragma comment(lib, "Winmm.lib")

FDialogAudioPlayer::FDialogAudioPlayer()
	: CurrentVolume(1.0f)
{
}

FDialogAudioPlayer::~FDialogAudioPlayer()
{
	StopAudio();
}

bool FDialogAudioPlayer::PlayAudio(const FString& AudioFilePath)
{
	// Stop any currently playing audio
	StopAudio();

	// Check if file exists
	if (!IFileManager::Get().FileExists(*AudioFilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("DialogAudioPlayer: Audio file not found: %s"), *AudioFilePath);
		return false;
	}

	CurrentAudioFile = AudioFilePath;

	// Use Windows PlaySound API - simple, works with raw WAV files, no cooking needed
	// SND_ASYNC = play asynchronously
	// SND_FILENAME = first param is a filename, not an alias
	BOOL Result = PlaySoundW(*AudioFilePath, nullptr, SND_ASYNC | SND_FILENAME);

	if (Result)
	{
		UE_LOG(LogTemp, Log, TEXT("DialogAudioPlayer: Playing audio: %s"), *AudioFilePath);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("DialogAudioPlayer: Windows PlaySound failed for: %s"), *AudioFilePath);
		CurrentAudioFile.Empty();
		return false;
	}
}

void FDialogAudioPlayer::StopAudio()
{
	if (!CurrentAudioFile.IsEmpty())
	{
		// Stop Windows PlaySound
		PlaySoundW(nullptr, nullptr, 0);
		UE_LOG(LogTemp, Log, TEXT("DialogAudioPlayer: Stopped audio: %s"), *CurrentAudioFile);
		CurrentAudioFile.Empty();
	}
}

bool FDialogAudioPlayer::IsPlaying() const
{
	// Windows PlaySound doesn't provide a direct way to query playback status
	// We'll just track if we have a current file playing
	return !CurrentAudioFile.IsEmpty();
}

void FDialogAudioPlayer::SetVolume(float Volume)
{
	CurrentVolume = FMath::Clamp(Volume, 0.0f, 1.0f);

	// Note: Windows PlaySound doesn't support volume control per-sound
	// Volume is controlled by system mixer
	// If volume control is needed, would need to use waveOut API instead
}
