// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Audio/AudioMapper.h"
#include "Audio/DialogAudioPlayer.h"

class FDialogDataManager;

/**
 * Dialog audio manager
 * Handles audio file lookup with fallback logic (spoken → paraphrase)
 * Manages audio player instance
 */
class FDialogAudioManager
{
public:
	FDialogAudioManager();
	~FDialogAudioManager();

	/**
	 * Initialize with data manager reference
	 * @param InDataManager Dialog data manager for gender selection
	 */
	void Initialize(TSharedPtr<FDialogDataManager> InDataManager);

	/**
	 * Play audio for dialog line with fallback logic
	 * Priority: 1) Spoken TLK ID, 2) Paraphrase TLK ID
	 *
	 * @param SpokenTLKID Spoken line TLK ID (-1 if none)
	 * @param ParaphraseTLKID Paraphrase TLK ID (-1 if none)
	 * @return True if audio was found and playback started
	 */
	bool PlayDialogAudio(int32 SpokenTLKID, int32 ParaphraseTLKID);

	/**
	 * Stop current audio playback
	 */
	void StopAudio();

	/**
	 * Check if audio is currently playing
	 */
	bool IsPlaying() const;

	/**
	 * Set playback volume
	 */
	void SetVolume(float Volume);

private:
	/**
	 * Try to find and play audio for a given TLK ID
	 * @param TLKID TLK string ID
	 * @param bIsMale Player gender
	 * @return True if audio file was found and playback started
	 */
	bool TryPlayAudio(int32 TLKID, bool bIsMale);

	/** Data manager reference for gender selection */
	TSharedPtr<FDialogDataManager> DataManager;

	/** Audio player instance */
	TUniquePtr<FDialogAudioPlayer> AudioPlayer;
};