// Copyright Epic Games, Inc. All Rights Reserved.

#include "Plot/PlotState.h"

FPlotState::FPlotState()
{
}

FPlotState::~FPlotState()
{
}

void FPlotState::SetFlag(const FString& PlotName, int32 FlagIndex, int32 Value)
{
	if (PlotName.IsEmpty() || FlagIndex < 0)
	{
		return;
	}

	FString Key = MakeKey(PlotName, FlagIndex);
	FlagValues.Add(Key, Value);

	UE_LOG(LogTemp, Verbose, TEXT("PlotState: Set %s[%d] = %d"), *PlotName, FlagIndex, Value);
}

int32 FPlotState::GetFlag(const FString& PlotName, int32 FlagIndex) const
{
	if (PlotName.IsEmpty() || FlagIndex < 0)
	{
		return 0;
	}

	FString Key = MakeKey(PlotName, FlagIndex);
	const int32* Value = FlagValues.Find(Key);
	return Value ? *Value : 0;
}

bool FPlotState::HasFlag(const FString& PlotName, int32 FlagIndex) const
{
	if (PlotName.IsEmpty() || FlagIndex < 0)
	{
		return false;
	}

	FString Key = MakeKey(PlotName, FlagIndex);
	return FlagValues.Contains(Key);
}

void FPlotState::Clear()
{
	FlagValues.Empty();
}

void FPlotState::Reset()
{
	Clear();
}

void FPlotState::DebugPrint() const
{
	UE_LOG(LogTemp, Log, TEXT("=== Plot State (%d flags) ==="), FlagValues.Num());
	for (const TPair<FString, int32>& Pair : FlagValues)
	{
		UE_LOG(LogTemp, Log, TEXT("  %s = %d"), *Pair.Key, Pair.Value);
	}
}

FString FPlotState::MakeKey(const FString& PlotName, int32 FlagIndex) const
{
	return FString::Printf(TEXT("%s:%d"), *PlotName, FlagIndex);
}
