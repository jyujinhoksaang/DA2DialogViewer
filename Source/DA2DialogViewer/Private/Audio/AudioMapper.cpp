// Copyright Epic Games, Inc. All Rights Reserved.

#include "Audio/AudioMapper.h"
#include "Data/DialogCSVReader.h"
#include "Misc/Paths.h"

FAudioMapper::FAudioMapper()
{
}

FAudioMapper::~FAudioMapper()
{
}

bool FAudioMapper::LoadDialogCSV(const FString& CSVPath)
{
	Clear();

	TArray<TArray<FString>> Rows;
	if (!FDialogCSVReader::ReadCSV(CSVPath, Rows))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load dialog CSV: %s"), *CSVPath);
		return false;
	}

	// Parse each row: dialog_id, gender, audio_file_id, sound_bank
	for (const TArray<FString>& Row : Rows)
	{
		if (Row.Num() < 4)
		{
			continue;
		}

		int32 DialogID = FCString::Atoi(*Row[0]);
		FString Gender = Row[1].TrimStartAndEnd().ToLower();
		FString AudioFileID = Row[2].TrimStartAndEnd();
		FString SoundBank = Row[3].TrimStartAndEnd();

		if (DialogID <= 0 || AudioFileID.IsEmpty())
		{
			continue;
		}

		// Find or create audio info entry
		FDialogAudioInfo* AudioInfo = AudioMap.Find(DialogID);
		if (!AudioInfo)
		{
			FDialogAudioInfo NewInfo;
			NewInfo.DialogID = DialogID;
			NewInfo.SoundBank = SoundBank;
			AudioInfo = &AudioMap.Add(DialogID, NewInfo);
		}

		// Set male or female audio file
		if (Gender == TEXT("m"))
		{
			AudioInfo->MaleAudioFile = AudioFileID + TEXT(".wav");
		}
		else if (Gender == TEXT("f"))
		{
			AudioInfo->FemaleAudioFile = AudioFileID + TEXT(".wav");
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Loaded %d dialog audio mappings from %s"), AudioMap.Num(), *CSVPath);
	return AudioMap.Num() > 0;
}

FString FAudioMapper::GetAudioFile(int32 DialogID, EPlayerGender Gender) const
{
	const FDialogAudioInfo* AudioInfo = AudioMap.Find(DialogID);
	if (!AudioInfo)
	{
		return FString();
	}

	if (Gender == EPlayerGender::Male)
	{
		return AudioInfo->MaleAudioFile;
	}
	else
	{
		return AudioInfo->FemaleAudioFile;
	}
}

FString FAudioMapper::GetAudioFilePath(int32 DialogID, EPlayerGender Gender, const FString& AudioDirectory) const
{
	FString AudioFile = GetAudioFile(DialogID, Gender);
	if (AudioFile.IsEmpty())
	{
		return FString();
	}

	return FPaths::Combine(AudioDirectory, AudioFile);
}

bool FAudioMapper::HasAudioMapping(int32 DialogID) const
{
	return AudioMap.Contains(DialogID);
}

void FAudioMapper::Clear()
{
	AudioMap.Empty();
}
