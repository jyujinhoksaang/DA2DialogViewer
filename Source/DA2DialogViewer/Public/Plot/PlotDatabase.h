// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Database for plot name <-> GUID mappings
 */
class FPlotDatabase
{
public:
	FPlotDatabase();
	~FPlotDatabase();

	// Load plots.csv
	bool LoadPlotsCSV(const FString& CSVPath);

	// Get GUID for plot name
	FString GetGUIDForPlot(const FString& PlotName) const;

	// Get plot name for GUID
	FString GetPlotForGUID(const FString& GUID) const;

	// Check if plot exists
	bool HasPlot(const FString& PlotName) const;

	// Check if GUID exists
	bool HasGUID(const FString& GUID) const;

	// Clear database
	void Clear();

	// Get total plot count
	int32 GetPlotCount() const { return PlotToGUID.Num(); }

private:
	// Plot name -> GUID
	TMap<FString, FString> PlotToGUID;

	// GUID -> Plot name (reverse lookup)
	TMap<FString, FString> GUIDToPlot;
};
