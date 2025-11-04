// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Runtime plot flag state tracker
 * Stores current values of all plot flags during dialog playback
 */
class FPlotState
{
public:
	FPlotState();
	~FPlotState();

	// Set plot flag value
	void SetFlag(const FString& PlotName, int32 FlagIndex, int32 Value);

	// Get plot flag value (returns 0 if not set)
	int32 GetFlag(const FString& PlotName, int32 FlagIndex) const;

	// Check if plot flag exists
	bool HasFlag(const FString& PlotName, int32 FlagIndex) const;

	// Clear all flags
	void Clear();

	// Reset to default state
	void Reset();

	// Debug: Print all flags
	void DebugPrint() const;

private:
	// Generate unique key for plot + flag index
	FString MakeKey(const FString& PlotName, int32 FlagIndex) const;

	// Map from plot+flag to value
	TMap<FString, int32> FlagValues;
};
