// Copyright Epic Games, Inc. All Rights Reserved.

#include "Plot/ConditionEvaluator.h"
#include "Plot/PlotState.h"

bool FConditionEvaluator::EvaluateCondition(const FPlotReference& Condition, const FPlotState& PlotState)
{
	// Empty plot name = no condition = always true
	if (Condition.PlotName.IsEmpty())
	{
		return true;
	}

	// Flag index -1 = just check if plot exists (always true for now)
	if (Condition.FlagIndex < 0)
	{
		return true;
	}

	// Get current flag value
	int32 CurrentValue = PlotState.GetFlag(Condition.PlotName, Condition.FlagIndex);

	// No comparison type specified (255) = just check if flag is set
	if (Condition.ComparisonType == 255)
	{
		return PlotState.HasFlag(Condition.PlotName, Condition.FlagIndex);
	}

	// Compare based on comparison type
	// Note: We don't have the expected value in the condition structure
	// For now, we'll just check if the flag exists and has a non-zero value
	return CurrentValue != 0;
}

bool FConditionEvaluator::EvaluateLinkCondition(const FDialogLink& Link, const FPlotState& PlotState)
{
	// Check condition flags
	// 4294967295 (0xFFFFFFFF) = no condition
	// 6 = some specific condition we'll treat as "always true" for now
	// 2 = another condition type

	if (Link.ConditionFlags == 4294967295)
	{
		return true; // No condition
	}

	if (Link.ConditionFlags == 6 || Link.ConditionFlags == 2)
	{
		return true; // Common "always true" conditions
	}

	// For now, treat other condition flags as "true"
	// In a full implementation, we'd decode the condition flags
	return true;
}

bool FConditionEvaluator::EvaluateNodeCondition(const FDialogNode& Node, const FPlotState& PlotState)
{
	return EvaluateCondition(Node.Condition, PlotState);
}

bool FConditionEvaluator::CompareFlagValue(int32 ActualValue, int32 ExpectedValue, uint8 ComparisonType)
{
	switch (ComparisonType)
	{
	case 0: // Equal
		return ActualValue == ExpectedValue;
	case 1: // Not equal
		return ActualValue != ExpectedValue;
	case 2: // Less than
		return ActualValue < ExpectedValue;
	case 3: // Less than or equal
		return ActualValue <= ExpectedValue;
	case 4: // Greater than
		return ActualValue > ExpectedValue;
	case 5: // Greater than or equal
		return ActualValue >= ExpectedValue;
	default:
		return false;
	}
}
