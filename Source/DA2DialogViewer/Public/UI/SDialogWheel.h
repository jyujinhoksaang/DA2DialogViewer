// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "DialogFlow/DialogNode.h"

class FDialogDataManager;
class SDialogTreeView;

/**
 * Dialog wheel option data
 */
struct FDialogWheelOption
{
	FDialogLink Link;
	FVector2D Position;
	float Angle;
	bool bIsHovered;

	FDialogWheelOption()
		: Position(),
		  Angle(0.0f)
		  , bIsHovered(false)
	{
	}
};

/**
 * Interactive dialog wheel widget
 * Displays player response options in radial layout
 */
class SDialogWheel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogWheel)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FDialogDataManager> InDataManager);

	// Set current dialog node
	void SetCurrentNode(const FDialogNode* InNode);

	// Clear wheel
	void Clear();

	// Set tree view reference for navigation
	void SetTreeView(TSharedPtr<SDialogTreeView> InTreeView) { TreeView = InTreeView; }

private:
	// Paint the wheel
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	                      const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                      int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// Compute desired size for hit testing
	virtual FVector2D ComputeDesiredSize(float) const override;

	// Handle mouse move for hover
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// Handle mouse click
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// Set cursor when hovering over options
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

	// Calculate option positions
	void CalculateOptionPositions();

	// Get angle for response type
	float GetAngleForResponseType(EResponseType Type) const;

	// Get label for response type (with icon override refinement)
	FString GetResponseTypeLabel(EResponseType Type, uint8 IconOverride = 255) const;

	// Get color for response type (with icon override refinement)
	FLinearColor GetResponseTypeColor(EResponseType Type, uint8 IconOverride = 255) const;

	// Find option at position
	int32 FindOptionAtPosition(const FGeometry& Geometry, const FVector2D& LocalPosition) const;

	// Draw single option
	void DrawOption(const FDialogWheelOption& Option, const FGeometry& AllottedGeometry,
	                FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

	// Handle option clicked
	void OnOptionClicked(int32 OptionIndex);

	// Handle option hovered
	void OnOptionHovered(int32 OptionIndex);

	// Get preview text for link
	FString GetLinkPreviewText(const FDialogLink& Link) const;

	// Get response type genus (grouping similar tones together)
	// Maps icon overrides to their parent EResponseType family
	static EResponseType GetResponseTypeGenus(EResponseType Type, uint8 IconOverride);

private:
	// Data manager reference
	TSharedPtr<FDialogDataManager> DataManager;

	// Tree view reference
	TSharedPtr<SDialogTreeView> TreeView;

	// Current dialog node
	const FDialogNode* CurrentNode = nullptr;

	// Wheel options
	TArray<FDialogWheelOption> Options;

	// Currently hovered option
	int32 HoveredOptionIndex = INDEX_NONE;

	// Wheel radius
	static constexpr float WheelRadius = 150.0f;

	// Option button radius
	static constexpr float OptionRadius = 40.0f;
};
