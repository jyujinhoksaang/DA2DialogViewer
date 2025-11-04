// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundWave.h"

/**
 * Dialog audio player for WAV file playback
 * Uses GEditor->PlayPreviewSound() for editor-safe audio playback
 */
class FDialogAudioPlayer
{
public:
	FDialogAudioPlayer();
	~FDialogAudioPlayer();

	/**
	 * Load and play audio file
	 * @param AudioFilePath Full path to WAV file
	 * @return True if playback started
	 */
	bool PlayAudio(const FString& AudioFilePath);

	/**
	 * Stop current audio playback
	 */
	void StopAudio();

	/**
	 * Check if audio is currently playing
	 */
	bool IsPlaying() const;

	/**
	 * Set playback volume (0.0 - 1.0)
	 */
	void SetVolume(float Volume);

private:
	/** Current volume (not used by Windows PlaySound) */
	float CurrentVolume;

	/** Currently playing audio file path */
	FString CurrentAudioFile;
};
