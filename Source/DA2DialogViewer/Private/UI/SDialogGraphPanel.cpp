// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SDialogGraphPanel.h"
#include "UI/SDialogWheel.h"
#include "DialogFlow/Conversation.h"
#include "Rendering/DrawElements.h"

void SDialogGraphPanel::Construct(const FArguments& InArgs)
{
	OnNodeSelectedCallback = InArgs._OnNodeSelected;
	SelectedNode = nullptr;
}

void SDialogGraphPanel::LoadConversation(TSharedPtr<FConversation> InConversation)
{
	CurrentConversation = InConversation;
	SelectedNode = nullptr;

	if (CurrentConversation.IsValid())
	{
		CalculateNodePositions();
	}
}

void SDialogGraphPanel::Clear()
{
	CurrentConversation.Reset();
	NodePositions.Empty();
	SelectedNode = nullptr;
}

void SDialogGraphPanel::NavigateToNode(int32 NodeIndex)
{
	if (!CurrentConversation.IsValid())
	{
		return;
	}

	const FDialogNode* Node = CurrentConversation->FindNode(NodeIndex);
	if (Node)
	{
		SelectedNode = Node;

		// Update dialog wheel with new node
		if (DialogWheel.IsValid())
		{
			DialogWheel->SetCurrentNode(Node);
		}

		if (OnNodeSelectedCallback.IsBound())
		{
			OnNodeSelectedCallback.Execute();
		}
	}
}

int32 SDialogGraphPanel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                 const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                 int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BackgroundLayer = LayerId;
	const int32 ConnectionLayer = LayerId + 1;
	const int32 NodeLayer = LayerId + 2;

	if (!CurrentConversation.IsValid())
	{
		// Draw "No conversation loaded" text
		FSlateDrawElement::MakeText(
			OutDrawElements,
			NodeLayer,
			AllottedGeometry.ToPaintGeometry(),
			FText::FromString(TEXT("No conversation loaded. Click 'Load Conversation' to begin.")),
			FCoreStyle::GetDefaultFontStyle("Regular", 14),
			ESlateDrawEffect::None,
			FLinearColor::White
		);
		return NodeLayer;
	}

	// Draw connections first (under nodes)
	for (const FDialogNode& Node : CurrentConversation->Nodes)
	{
		FVector2D StartPos = GetNodePosition(Node.NodeIndex);
		StartPos += FVector2D(NodeWidth / 2, NodeHeight / 2); // Center of node

		for (const FDialogLink& Link : Node.Links)
		{
			FVector2D EndPos = GetNodePosition(Link.TargetNodeIndex);
			EndPos += FVector2D(NodeWidth / 2, NodeHeight / 2);

			FLinearColor ConnectionColor = GetResponseTypeColor(Link);
			DrawConnection(StartPos, EndPos, AllottedGeometry, OutDrawElements, ConnectionLayer, ConnectionColor);
		}
	}

	// Draw nodes
	for (const FDialogNode& Node : CurrentConversation->Nodes)
	{
		bool bIsSelected = (SelectedNode && SelectedNode->NodeIndex == Node.NodeIndex);
		DrawNode(Node, AllottedGeometry, OutDrawElements, NodeLayer, bIsSelected);
	}

	return NodeLayer;
}

FReply SDialogGraphPanel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FDialogNode* ClickedNode = FindNodeAtPosition(MyGeometry, LocalPosition);

		if (ClickedNode)
		{
			NavigateToNode(ClickedNode->NodeIndex);
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

void SDialogGraphPanel::CalculateNodePositions()
{
	NodePositions.Empty();

	if (!CurrentConversation.IsValid() || CurrentConversation->Nodes.Num() == 0)
	{
		return;
	}

	// Simple layout: arrange nodes in a grid based on depth-first traversal
	// For now, just lay them out in rows

	int32 NodesPerRow = 5;
	float StartX = 50.0f;
	float StartY = 50.0f;

	for (int32 i = 0; i < CurrentConversation->Nodes.Num(); ++i)
	{
		int32 Row = i / NodesPerRow;
		int32 Col = i % NodesPerRow;

		FVector2D Position(
			StartX + Col * HorizontalSpacing,
			StartY + Row * VerticalSpacing
		);

		NodePositions.Add(i, Position);
	}
}

FVector2D SDialogGraphPanel::GetNodePosition(int32 NodeIndex) const
{
	const FVector2D* Position = NodePositions.Find(NodeIndex);
	return Position ? *Position : FVector2D::ZeroVector;
}

const FDialogNode* SDialogGraphPanel::FindNodeAtPosition(const FGeometry& Geometry, const FVector2D& ScreenPosition) const
{
	if (!CurrentConversation.IsValid())
	{
		return nullptr;
	}

	for (const FDialogNode& Node : CurrentConversation->Nodes)
	{
		FVector2D NodePos = GetNodePosition(Node.NodeIndex);
		FVector2D NodeSize(NodeWidth, NodeHeight);

		if (ScreenPosition.X >= NodePos.X && ScreenPosition.X <= NodePos.X + NodeSize.X &&
			ScreenPosition.Y >= NodePos.Y && ScreenPosition.Y <= NodePos.Y + NodeSize.Y)
		{
			return &Node;
		}
	}

	return nullptr;
}

void SDialogGraphPanel::DrawNode(const FDialogNode& Node, const FGeometry& AllottedGeometry,
                                 FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bIsSelected) const
{
	FVector2D Position = GetNodePosition(Node.NodeIndex);
	FVector2D Size(NodeWidth, NodeHeight);

	// Node background color
	FLinearColor BackgroundColor = GetSpeakerColor(Node.SpeakerID);
	if (bIsSelected)
	{
		BackgroundColor = FLinearColor::Yellow;
	}

	// Draw node box
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Position))),
		FAppStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		BackgroundColor
	);

	// Draw node border
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2f(Size - FVector2D(4, 4)), FSlateLayoutTransform(FVector2f(Position + FVector2D(2, 2)))),
		FAppStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		FLinearColor(0.1f, 0.1f, 0.1f, 1.0f)
	);

	// Draw node text
	FString NodeText = FString::Printf(TEXT("Node %d\nSpeaker: %d\nTLK: %d"),
	                                   Node.NodeIndex, Node.SpeakerID, Node.TLKStringID);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId + 2,
		AllottedGeometry.ToPaintGeometry(FVector2f(Size - FVector2D(10, 10)), FSlateLayoutTransform(FVector2f(Position + FVector2D(5, 5)))),
		NodeText,
		FCoreStyle::GetDefaultFontStyle("Regular", 8),
		ESlateDrawEffect::None,
		FLinearColor::White
	);
}

void SDialogGraphPanel::DrawConnection(const FVector2D& StartPos, const FVector2D& EndPos,
                                       const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements,
                                       int32 LayerId, const FLinearColor& Color) const
{
	TArray<FVector2D> LinePoints;
	LinePoints.Add(StartPos);
	LinePoints.Add(EndPos);

	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		LinePoints,
		ESlateDrawEffect::None,
		Color,
		true,
		2.0f
	);
}

FLinearColor SDialogGraphPanel::GetResponseTypeColor(const FDialogLink& Link) const
{
	switch (Link.ResponseType)
	{
	case EResponseType::Neutral:
		return FLinearColor::Gray;
	case EResponseType::Diplomatic:
		return FLinearColor::Blue;
	case EResponseType::Aggressive:
		return FLinearColor::Red;
	case EResponseType::Humorous:
		return FLinearColor(1.0f, 0.5f, 0.0f); // Orange
	case EResponseType::Bonus:
		return FLinearColor(1.0f, 0.8f, 0.2f); // Gold
	case EResponseType::Follower:
		return FLinearColor(0.4f, 0.7f, 0.4f); // Light green
	case EResponseType::Choice1:
	case EResponseType::Choice2:
	case EResponseType::Choice3:
	case EResponseType::Choice4:
	case EResponseType::Choice5:
		return FLinearColor(0.7f, 0.7f, 0.7f); // Light gray
	case EResponseType::Investigate:
		return FLinearColor(0.7f, 0.7f, 0.4f); // Yellow
	case EResponseType::AutoContinue:
		return FLinearColor::Gray;
	default:
		return FLinearColor::White;
	}
}

FLinearColor SDialogGraphPanel::GetSpeakerColor(int32 SpeakerID) const
{
	if (SpeakerID == 1)
	{
		// Player
		return FLinearColor(0.2f, 0.4f, 0.8f, 1.0f);
	}
	else if (SpeakerID == 10)
	{
		// NPC
		return FLinearColor(0.4f, 0.6f, 0.4f, 1.0f);
	}
	else
	{
		// Other
		return FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
	}
}
