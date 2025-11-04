// Copyright Epic Games, Inc. All Rights Reserved.

#include "Plot/PlotDatabase.h"
#include "Data/DialogCSVReader.h"

FPlotDatabase::FPlotDatabase()
{
}

FPlotDatabase::~FPlotDatabase()
{
}

bool FPlotDatabase::LoadPlotsCSV(const FString& CSVPath)
{
	Clear();

	// Load plot name -> GUID mapping
	if (!FDialogCSVReader::ReadPlotsCSV(CSVPath, PlotToGUID))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load plots CSV: %s"), *CSVPath);
		return false;
	}

	// Build reverse mapping (GUID -> plot name)
	for (const TPair<FString, FString>& Pair : PlotToGUID)
	{
		GUIDToPlot.Add(Pair.Value, Pair.Key);
	}

	UE_LOG(LogTemp, Log, TEXT("Loaded %d plots into database"), PlotToGUID.Num());
	return true;
}

FString FPlotDatabase::GetGUIDForPlot(const FString& PlotName) const
{
	const FString* GUID = PlotToGUID.Find(PlotName);
	return GUID ? *GUID : FString();
}

FString FPlotDatabase::GetPlotForGUID(const FString& GUID) const
{
	const FString* PlotName = GUIDToPlot.Find(GUID);
	return PlotName ? *PlotName : FString();
}

bool FPlotDatabase::HasPlot(const FString& PlotName) const
{
	return PlotToGUID.Contains(PlotName);
}

bool FPlotDatabase::HasGUID(const FString& GUID) const
{
	return GUIDToPlot.Contains(GUID);
}

void FPlotDatabase::Clear()
{
	PlotToGUID.Empty();
	GUIDToPlot.Empty();
}
