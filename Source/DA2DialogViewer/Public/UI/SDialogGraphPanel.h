// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FConversation;
struct FDialogNode;
struct FDialogLink;
class SDialogWheel;

/**
 * Dialog graph visualization panel
 * Displays conversation flow as node graph
 */
class SDialogGraphPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogGraphPanel) {}
		SLATE_EVENT(FSimpleDelegate, OnNodeSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Load conversation into graph
	void LoadConversation(TSharedPtr<FConversation> InConversation);

	// Clear graph
	void Clear();

	// Get currently selected node
	const FDialogNode* GetSelectedNode() const { return SelectedNode; }

	// Set dialog wheel reference for interaction
	void SetDialogWheel(TSharedPtr<SDialogWheel> InDialogWheel) { DialogWheel = InDialogWheel; }

	// Navigate to node by index
	void NavigateToNode(int32 NodeIndex);

private:
	// Paint the graph
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// Handle mouse button down
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// Calculate node positions
	void CalculateNodePositions();

	// Get node position for index
	FVector2D GetNodePosition(int32 NodeIndex) const;

	// Find node at screen position
	const FDialogNode* FindNodeAtPosition(const FGeometry& Geometry, const FVector2D& ScreenPosition) const;

	// Draw a single node
	void DrawNode(const FDialogNode& Node, const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bIsSelected) const;

	// Draw connection between nodes
	void DrawConnection(const FVector2D& StartPos, const FVector2D& EndPos,
		const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FLinearColor& Color) const;

	// Get color for response type
	FLinearColor GetResponseTypeColor(const FDialogLink& Link) const;

	// Get speaker color
	FLinearColor GetSpeakerColor(int32 SpeakerID) const;

private:
	// Current conversation
	TSharedPtr<FConversation> CurrentConversation;

	// Node positions (node index -> position)
	TMap<int32, FVector2D> NodePositions;

	// Selected node
	const FDialogNode* SelectedNode;

	// Dialog wheel reference
	TSharedPtr<SDialogWheel> DialogWheel;

	// Node selection callback
	FSimpleDelegate OnNodeSelectedCallback;

	// Node size
	static constexpr float NodeWidth = 150.0f;
	static constexpr float NodeHeight = 80.0f;

	// Spacing between nodes
	static constexpr float HorizontalSpacing = 200.0f;
	static constexpr float VerticalSpacing = 120.0f;
};
