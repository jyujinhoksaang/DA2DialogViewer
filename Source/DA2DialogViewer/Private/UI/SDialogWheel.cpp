// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SDialogWheel.h"
#include "UI/SDialogTreeView.h"
#include "Data/DialogDataManager.h"
#include "Plot/ConditionEvaluator.h"
#include "Plot/ActionExecutor.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

void SDialogWheel::Construct(const FArguments& InArgs, TSharedPtr<FDialogDataManager> InDataManager)
{
	DataManager = InDataManager;
	CurrentNode = nullptr;
	HoveredOptionIndex = -1;
}

void SDialogWheel::SetCurrentNode(const FDialogNode* InNode)
{
	CurrentNode = InNode;
	HoveredOptionIndex = -1;

	// Build options from node links
	Options.Empty();

	if (CurrentNode)
	{
		// First pass: collect all valid links grouped by response type
		// This allows us to implement priority/supersede logic for duplicate types
		TMap<EResponseType, TArray<FDialogLink>> LinksByType;

		for (const FDialogLink& Link : CurrentNode->Links)
		{
			// Skip auto-continue links (they're not player choices)
			if (Link.ResponseType == EResponseType::AutoContinue)
			{
				continue;
			}

			// Evaluate link condition
			if (DataManager.IsValid() && FConditionEvaluator::EvaluateLinkCondition(Link, DataManager->GetPlotState()))
			{
				// Check if link has valid displayable text
				FString DialogText;
				if (DataManager.IsValid())
				{
					DialogText = DataManager->GetTLKString(Link.TLKStringID);
					if (DialogText.IsEmpty())
					{
						DialogText = Link.PreviewText;
					}
				}

				// Only consider option if it has valid, non-empty text
				// Filter out empty strings, placeholders like [[CONTINUE]], and "Not Found" entries
				// Use IsValidlyEmpty to check for: empty, [[placeholders]], or [TLK xxx - Not Found]
				if (!DialogText.IsEmpty() && !FDialogTreeItem::IsValidlyEmpty(DialogText))
				{
					// Group by GENUS instead of exact response type
					// This ensures "Tactful with condition" supersedes "Diplomatic without condition"
					// because they belong to the same Diplomatic genus
					EResponseType Genus = GetResponseTypeGenus(Link.ResponseType, Link.IconOverride);
					LinksByType.FindOrAdd(Genus).Add(Link);
				}
			}
		}

		// Second pass: For each response type genus, select the highest priority link
		// Priority: Conditional links (with actual conditions) supersede unconditional links
		// This prevents duplicate response type genuses from appearing on the wheel
		for (auto& Pair : LinksByType)
		{
			EResponseType Type = Pair.Key;
			TArray<FDialogLink>& Links = Pair.Value;

			const FDialogLink* SelectedLink = nullptr;

			if (Links.Num() == 1)
			{
				// Only one link of this type, use it
				SelectedLink = &Links[0];
			}
			else
			{
				// Multiple links of same type - apply priority logic
				// Priority order:
				// 1. Links with conditions (ConditionFlags != 0xFFFFFFFF) - these are specific/contextual
				// 2. Links without conditions (ConditionFlags == 0xFFFFFFFF) - these are fallback/default

				const FDialogLink* ConditionalLink = nullptr;
				const FDialogLink* UnconditionalLink = nullptr;

				for (const FDialogLink& Link : Links)
				{
					const uint32 NoConditionFlag = 4294967295; // 0xFFFFFFFF

					if (Link.ConditionFlags == NoConditionFlag)
					{
						// Unconditional/fallback link
						if (!UnconditionalLink)
						{
							UnconditionalLink = &Link;
						}
					}
					else
					{
						// Conditional link - has higher priority
						if (!ConditionalLink)
						{
							ConditionalLink = &Link;
						}
					}
				}

				// Select conditional link if available, otherwise use unconditional fallback
				SelectedLink = ConditionalLink ? ConditionalLink : UnconditionalLink;
			}

			// Add the selected link to options
			if (SelectedLink)
			{
				FDialogWheelOption Option;
				Option.Link = *SelectedLink;
				Options.Add(Option);
			}
		}

		CalculateOptionPositions();
	}
	else
	{
		// No current node - clear everything
		CurrentNode = nullptr;
	}

	// If we have no valid options, clear the current node and hide the wheel
	if (Options.Num() == 0)
	{
		CurrentNode = nullptr; // Prevent any "Node X" text from showing
	}

	// Update visibility based on whether we have valid options
	SetVisibility(Options.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed);
}

void SDialogWheel::Clear()
{
	CurrentNode = nullptr;
	Options.Empty();
	HoveredOptionIndex = -1;
}

int32 SDialogWheel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                            const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                            int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Center = AllottedGeometry.GetLocalSize() / 2;

	// Don't draw anything if no options (widget should be collapsed anyway)
	if (Options.Num() == 0)
	{
		return LayerId;
	}

	// Draw center circle
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2f(40, 40), FSlateLayoutTransform(FVector2f(Center - FVector2D(20, 20)))),
		FAppStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		FLinearColor(0.2f, 0.2f, 0.2f, 1.0f)
	);

	// Draw each option
	for (int32 i = 0; i < Options.Num(); ++i)
	{
		DrawOption(Options[i], AllottedGeometry, OutDrawElements, LayerId + 1);
	}

	// Draw center indicator
	if (HoveredOptionIndex >= 0 && Options.IsValidIndex(HoveredOptionIndex))
	{
		// Show hovered option's tone color and label
		const FDialogWheelOption& HoveredOption = Options[HoveredOptionIndex];
		FLinearColor ToneColor = GetResponseTypeColor(HoveredOption.Link.ResponseType, HoveredOption.Link.IconOverride);
		FString ToneLabel = GetResponseTypeLabel(HoveredOption.Link.ResponseType, HoveredOption.Link.IconOverride);

		// Larger center square for better text fit
		const FVector2D SquareSize(100, 40);

		// Redraw center square with tone color
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 2,
			AllottedGeometry.ToPaintGeometry(FVector2f(SquareSize), FSlateLayoutTransform(FVector2f(Center - SquareSize / 2))),
			FAppStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			ToneColor
		);

		// Draw tone label centered in the square
		// Measure text to center it properly
		const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);
		const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		const FVector2D TextSize = FontMeasure->Measure(ToneLabel, LabelFont);

		// Center text within the square
		const FVector2D TextPosition = Center - TextSize / 2;

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 3,
			AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2f(TextPosition))),
			ToneLabel,
			LabelFont,
			ESlateDrawEffect::None,
			FLinearColor::Black
		);
	}
	else if (CurrentNode)
	{
		// Default center display showing node index
		FString CenterText = FString::Printf(TEXT("Node %d"), CurrentNode->NodeIndex);
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 2,
			AllottedGeometry.ToPaintGeometry(FVector2f(60, 10), FSlateLayoutTransform(FVector2f(Center - FVector2D(30, 5)))),
			CenterText,
			FCoreStyle::GetDefaultFontStyle("Regular", 8),
			ESlateDrawEffect::None,
			FLinearColor::White
		);
	}

	return LayerId + 2;
}

void SDialogWheel::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
}

void SDialogWheel::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	HoveredOptionIndex = -1;
	Invalidate(EInvalidateWidgetReason::Paint);
	SCompoundWidget::OnMouseLeave(MouseEvent);
}

FVector2D SDialogWheel::ComputeDesiredSize(float) const
{
	// Wheel needs enough space for radius plus text
	const float FullSize = (WheelRadius + 80.0f) * 2;
	return FVector2D(FullSize, FullSize);
}

FCursorReply SDialogWheel::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(CursorEvent.GetScreenSpacePosition());
	int32 OptionIndex = FindOptionAtPosition(MyGeometry, LocalPosition);

	if (OptionIndex >= 0)
	{
		return FCursorReply::Cursor(EMouseCursor::Hand);
	}

	return FCursorReply::Cursor(EMouseCursor::Default);
}

FReply SDialogWheel::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	int32 NewHoveredIndex = FindOptionAtPosition(MyGeometry, LocalPosition);

	if (NewHoveredIndex != HoveredOptionIndex)
	{
		HoveredOptionIndex = NewHoveredIndex;

		if (HoveredOptionIndex >= 0)
		{
			OnOptionHovered(HoveredOptionIndex);
		}

		// Force widget to repaint on hover change
		const_cast<SDialogWheel*>(this)->Invalidate(EInvalidateWidgetReason::Paint);
	}

	return FReply::Handled();
}

FReply SDialogWheel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		int32 ClickedIndex = FindOptionAtPosition(MyGeometry, LocalPosition);

		if (ClickedIndex >= 0)
		{
			OnOptionClicked(ClickedIndex);
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

void SDialogWheel::CalculateOptionPositions()
{
	// Static layout system for dialog wheel with aesthetic positioning
	// Clock positions: 1=60°, 3=0°, 5=300°, 7=240°, 9=180°, 11=120°

	const int32 NumOptions = Options.Num();
	if (NumOptions == 0 || NumOptions > 6)
	{
		return;
	}

	// Clock angles mapping (1, 3, 5, 7, 9, 11 o'clock)
	const float ClockAngles[6] = { 60.0f, 0.0f, 300.0f, 240.0f, 180.0f, 120.0f };

	// Define static layout patterns based on choice count
	// Each pattern defines which clock positions to use
	TArray<int32> LayoutPattern;

	switch (NumOptions)
	{
		case 2:
			// Horizontal symmetry: 1 and 5 o'clock
			LayoutPattern = { 0, 2 }; // indices into ClockAngles
			break;

		case 3:
		{
			// Check if we have any flavored response types (personality-affecting choices)
			bool bHasFlavored = Options.ContainsByPredicate([](const FDialogWheelOption& Opt)
			{
				return Opt.Link.ResponseType == EResponseType::Diplomatic ||
				       Opt.Link.ResponseType == EResponseType::Humorous ||
				       Opt.Link.ResponseType == EResponseType::Aggressive;
			});

			if (bHasFlavored)
			{
				// Flavored choices: use right side (1, 3, 5)
				LayoutPattern = { 0, 1, 2 };
			}
			else
			{
				// All neutral: triangular symmetry (1, 5, 9)
				LayoutPattern = { 0, 2, 4 };
			}
			break;
		}

		case 4:
			// Right side: 1, 3, 5 + Left side: 9 (single for balance)
			LayoutPattern = { 0, 1, 2, 4 };
			break;

		case 5:
			// Right side: 1, 3, 5 + Left side: 7, 11 (horizontal symmetry)
			LayoutPattern = { 0, 1, 2, 3, 5 };
			break;

		case 6:
			// All positions clockwise: 1, 3, 5, 7, 9, 11
			LayoutPattern = { 0, 1, 2, 3, 4, 5 };
			break;
	}

	// Create assignment array to track which option goes to which position
	TArray<int32> PositionAssignments;
	PositionAssignments.SetNum(NumOptions);
	for (int32 i = 0; i < NumOptions; ++i)
	{
		PositionAssignments[i] = -1; // Unassigned
	}

	// Track which layout positions have been used
	TArray<bool> UsedPositions;
	UsedPositions.SetNum(LayoutPattern.Num());
	for (int32 i = 0; i < UsedPositions.Num(); ++i)
	{
		UsedPositions[i] = false;
	}

	// First pass: Assign flavored options to their preferred positions
	// Diplomatic -> 1 o'clock (position index 0 in pattern) - peaceful/tactful choices
	// Humorous -> 3 o'clock (position index 1 in pattern) - witty/sarcastic choices
	// Aggressive -> 5 o'clock (position index 2 in pattern) - harsh/direct choices
	// Note: If a preferred position is already taken (duplicate type), the option
	// will be assigned in the second pass to maintain consistency

	for (int32 i = 0; i < NumOptions; ++i)
	{
		const FDialogWheelOption& Option = Options[i];
		int32 PreferredPosition = -1;

		switch (Option.Link.ResponseType)
		{
			case EResponseType::Diplomatic:
				PreferredPosition = 0; // 1 o'clock (peaceful)
				break;
			case EResponseType::Humorous:
				PreferredPosition = 1; // 3 o'clock (witty)
				break;
			case EResponseType::Aggressive:
				PreferredPosition = 2; // 5 o'clock (hostile)
				break;
			default:
				// Neutral/generic options will be assigned in second pass
				break;
		}

		// Assign if position is valid and available in the layout
		// If position is already taken (e.g., duplicate Humorous), skip this option
		// and it will be assigned to next available position in second pass
		if (PreferredPosition >= 0 && PreferredPosition < LayoutPattern.Num() && !UsedPositions[PreferredPosition])
		{
			PositionAssignments[i] = PreferredPosition;
			UsedPositions[PreferredPosition] = true;
		}
	}

	// Second pass: Assign neutral/helpful options to remaining positions in order
	int32 NextAvailablePosition = 0;
	for (int32 i = 0; i < NumOptions; ++i)
	{
		if (PositionAssignments[i] == -1) // Not yet assigned
		{
			// Find next available position
			while (NextAvailablePosition < LayoutPattern.Num() && UsedPositions[NextAvailablePosition])
			{
				NextAvailablePosition++;
			}

			if (NextAvailablePosition < LayoutPattern.Num())
			{
				PositionAssignments[i] = NextAvailablePosition;
				UsedPositions[NextAvailablePosition] = true;
				NextAvailablePosition++;
			}
		}
	}

	// Apply the calculated positions to options
	for (int32 i = 0; i < NumOptions; ++i)
	{
		FDialogWheelOption& Option = Options[i];

		int32 LayoutIndex = PositionAssignments[i];
		if (LayoutIndex >= 0 && LayoutIndex < LayoutPattern.Num())
		{
			int32 ClockIndex = LayoutPattern[LayoutIndex];
			float TrigAngle = ClockAngles[ClockIndex];

			Option.Angle = TrigAngle;
			float Radians = FMath::DegreesToRadians(TrigAngle);

			// Calculate position (no negation needed with proper angle mapping)
			Option.Position = FVector2D(
				FMath::Cos(Radians) * WheelRadius,
				-FMath::Sin(Radians) * WheelRadius // Negate Y for screen coordinates
			);
		}
	}
}

EResponseType SDialogWheel::GetResponseTypeGenus(EResponseType Type, uint8 IconOverride)
{
	// Map icon overrides to their genus (parent EResponseType family)
	// This groups similar tones together (e.g., Tactful/Helpful → Diplomatic genus)
	// GENUS GROUPINGS:
	//   - Aggressive genus: Harsh, Direct, Intimidate → all map to EResponseType::Aggressive
	//   - Diplomatic genus: Tactful, Helpful, Peaceful → all map to EResponseType::Diplomatic
	//   - Humorous genus: Witty, Charming, Sarcastic → all map to EResponseType::Humorous

	if (IconOverride != 255)
	{
		switch (IconOverride)
		{
			// Aggressive genus (icon overrides 5, 10, 17)
			case 5:  // Intimidate (old)
			case 10: // Harsh
			case 17: // Direct
				return EResponseType::Aggressive;

			// Diplomatic genus (icon overrides 6, 11, 18)
			case 6:  // Diplomacy (old)
			case 11: // Tactful
			case 18: // Helpful
				return EResponseType::Diplomatic;

			// Humorous genus (icon overrides 7, 12, 19)
			case 7:  // Charm (old)
			case 12: // Witty
			case 19: // Charming
				return EResponseType::Humorous;
		}
	}

	// No icon override or unrecognized - return the base type as-is
	return Type;
}

FString SDialogWheel::GetResponseTypeLabel(EResponseType Type, uint8 IconOverride) const
{
	// Icon override provides refinement within response type families
	// conversation_icons.csv defines these mappings
	if (IconOverride != 255)
	{
		switch (IconOverride)
		{
			// Special icons
			case 3: return TEXT("End Romance");
			case 4:
			case 9: return TEXT("Flirt");
			case 8: return TEXT("Lie");
			case 13: return TEXT("No");
			case 14: return TEXT("Yes");
			case 15: return TEXT("Investigate");
			case 16: return TEXT("Special");

			// Aggressive family variations
			case 5:  // Intimidate (old)
			case 10: return TEXT("Harsh");
			case 17: return TEXT("Direct");

			// Diplomatic family variations
			case 6:  // Diplomacy (old)
			case 11: return TEXT("Tactful");
			case 18: return TEXT("Helpful");

			// Humorous family variations
			case 7:  // Charm (old)
			case 12: return TEXT("Witty");
			case 19: return TEXT("Charming");
		}
	}

	// Base labels by response type
	switch (Type)
	{
		case EResponseType::Neutral:		return TEXT("Neutral");
		case EResponseType::Aggressive:		return TEXT("Aggressive");
		case EResponseType::Diplomatic:		return TEXT("Diplomatic");
		case EResponseType::Humorous:		return TEXT("Humorous");
		case EResponseType::Bonus:			return TEXT("Bonus");
		case EResponseType::Follower:		return TEXT("Follower");
		case EResponseType::Choice1:		return TEXT("Choice 1");
		case EResponseType::Choice2:		return TEXT("Choice 2");
		case EResponseType::Choice3:		return TEXT("Choice 3");
		case EResponseType::Choice4:		return TEXT("Choice 4");
		case EResponseType::Choice5:		return TEXT("Choice 5");
		case EResponseType::Investigate:	return TEXT("Investigate");
		default:							return TEXT("Unknown");
	}
}

FLinearColor SDialogWheel::GetResponseTypeColor(EResponseType Type, uint8 IconOverride) const
{
	// Icon override can refine color shades within families
	// For example: Harsh (darker red) vs Direct (lighter red)

	// Check for icon-specific color refinements
	if (IconOverride != 255)
	{
		switch (IconOverride)
		{
			// Aggressive family - shades of red
			case 5:  // Intimidate
			case 10: return FLinearColor(0.8f, 0.1f, 0.1f); // Harsh - dark red
			case 17: return FLinearColor(1.0f, 0.3f, 0.2f); // Direct - bright red-orange

			// Diplomatic family - shades of blue/green
			case 6:  // Diplomacy
			case 11: return FLinearColor(0.2f, 0.5f, 0.9f); // Tactful - blue
			case 18: return FLinearColor(0.3f, 0.8f, 0.5f); // Helpful - green

			// Humorous family - shades of purple/orange
			case 7:  // Charm (old)
			case 12: return FLinearColor(1.0f, 0.5f, 0.0f); // Witty - orange
			case 19: return FLinearColor(0.8f, 0.4f, 0.9f); // Charming - purple

			// Special colors
			case 4:
			case 9:  return FLinearColor(1.0f, 0.2f, 0.6f); // Flirt - pink
			case 8:  return FLinearColor(0.6f, 0.3f, 0.1f); // Lie - brown
			case 15: return FLinearColor(0.7f, 0.7f, 0.4f); // Investigate - yellow
			case 16: return FLinearColor(1.0f, 0.8f, 0.2f); // Special - gold
		}
	}

	// Base colors by response type
	switch (Type)
	{
		case EResponseType::Neutral:		return FLinearColor(0.6f, 0.6f, 0.6f); // Gray
		case EResponseType::Aggressive:		return FLinearColor(0.9f, 0.2f, 0.2f); // Red
		case EResponseType::Diplomatic:		return FLinearColor(0.3f, 0.6f, 0.9f); // Blue
		case EResponseType::Humorous:		return FLinearColor(0.9f, 0.5f, 0.2f); // Orange
		case EResponseType::Bonus:			return FLinearColor(1.0f, 0.8f, 0.2f); // Gold
		case EResponseType::Follower:		return FLinearColor(0.4f, 0.7f, 0.4f); // Light green
		case EResponseType::Choice1:
		case EResponseType::Choice2:
		case EResponseType::Choice3:
		case EResponseType::Choice4:
		case EResponseType::Choice5:		return FLinearColor(0.7f, 0.7f, 0.7f); // Light gray (neutral choices)
		case EResponseType::Investigate:	return FLinearColor(0.7f, 0.7f, 0.4f); // Yellow
		default:							return FLinearColor(0.5f, 0.5f, 0.5f); // Dark gray
	}
}

int32 SDialogWheel::FindOptionAtPosition(const FGeometry& Geometry, const FVector2D& LocalPosition) const
{
	FVector2D Center = Geometry.GetLocalSize() / 2;
	FVector2D RelativePos = LocalPosition - Center;

	// Use larger hit area for text-based options (120x40 text box)
	const float HitAreaWidth = 120.0f;
	const float HitAreaHeight = 40.0f;

	for (int32 i = 0; i < Options.Num(); ++i)
	{
		FVector2D OptionPos = Options[i].Position;
		FVector2D OptionTopLeft = OptionPos - FVector2D(HitAreaWidth / 2, HitAreaHeight / 2);
		FVector2D OptionBottomRight = OptionPos + FVector2D(HitAreaWidth / 2, HitAreaHeight / 2);

		// Check if mouse is within rectangular bounds
		if (RelativePos.X >= OptionTopLeft.X && RelativePos.X <= OptionBottomRight.X &&
			RelativePos.Y >= OptionTopLeft.Y && RelativePos.Y <= OptionBottomRight.Y)
		{
			return i;
		}
	}

	return -1;
}

void SDialogWheel::DrawOption(const FDialogWheelOption& Option, const FGeometry& AllottedGeometry,
                              FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	FVector2D Center = AllottedGeometry.GetLocalSize() / 2;
	FVector2D OptionCenter = Center + Option.Position;

	// Determine if hovered
	bool bIsHovered = (Options.IndexOfByPredicate([&](const FDialogWheelOption& Opt)
	{
		return &Opt == &Option;
	}) == HoveredOptionIndex);

	// Get actual dialog text from TLK string or use preview text
	FString DialogText;
	if (DataManager.IsValid())
	{
		DialogText = DataManager->GetTLKString(Option.Link.TLKStringID);
		if (DialogText.IsEmpty())
		{
			DialogText = Option.Link.PreviewText;
		}
		if (DialogText.IsEmpty())
		{
			DialogText = FString::Printf(TEXT("[TLK %d]"), Option.Link.TLKStringID);
		}
	}
	else
	{
		DialogText = FString::Printf(TEXT("[TLK %d]"), Option.Link.TLKStringID);
	}

	// Truncate long text for display
	if (DialogText.Len() > 40)
	{
		DialogText = DialogText.Left(37) + TEXT("...");
	}

	// Measure text to center it properly
	const FSlateFontInfo TextFont = FCoreStyle::GetDefaultFontStyle("Regular", 9);
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2D TextSize = FontMeasure->Measure(DialogText, TextFont);

	// Draw background for hovered option for better contrast
	if (bIsHovered)
	{
		// 50% wider background than text for better coverage
		FVector2D BackgroundSize(180, 40);

		// Slightly lighter background than default dark gray
		FLinearColor HoverBackground(0.3f, 0.3f, 0.3f, 0.9f);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2f(BackgroundSize), FSlateLayoutTransform(FVector2f(OptionCenter - BackgroundSize / 2))),
			FAppStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			HoverBackground
		);
	}

	// Text color (brighter white for hover, neutral gray otherwise)
	FLinearColor TextColor = bIsHovered ? FLinearColor(1.0f, 1.0f, 1.0f) : FLinearColor(0.7f, 0.7f, 0.7f);

	// Center text at option position
	const FVector2D TextPosition = OptionCenter - TextSize / 2;

	// Draw dialog text centered
	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2f(TextPosition))),
		DialogText,
		TextFont,
		ESlateDrawEffect::None,
		TextColor
	);
}

void SDialogWheel::OnOptionClicked(int32 OptionIndex)
{
	if (!Options.IsValidIndex(OptionIndex))
	{
		return;
	}

	const FDialogWheelOption& Option = Options[OptionIndex];

	UE_LOG(LogTemp, Log, TEXT("Dialog option clicked: %s -> Node %d"),
	       *GetResponseTypeLabel(Option.Link.ResponseType), Option.Link.TargetNodeIndex);

	// Execute action if current node has one
	if (CurrentNode && DataManager.IsValid())
	{
		FActionExecutor::ExecuteNodeAction(*CurrentNode, DataManager->GetPlotState());
	}

	// Navigate to player choice and auto-play audio
	if (TreeView.IsValid())
	{
		TreeView->NavigateToPlayerChoice(Option.Link.TargetNodeIndex);
	}
}

void SDialogWheel::OnOptionHovered(int32 OptionIndex)
{
	if (!Options.IsValidIndex(OptionIndex))
	{
		return;
	}

	const FDialogWheelOption& Option = Options[OptionIndex];

	UE_LOG(LogTemp, Verbose, TEXT("Dialog option hovered: %s"),
	       *GetResponseTypeLabel(Option.Link.ResponseType));

	// TODO: Play audio preview here
}

FString SDialogWheel::GetLinkPreviewText(const FDialogLink& Link) const
{
	// For now, just return TLK ID
	// In full implementation, would look up actual text from TLK files
	return FString::Printf(TEXT("TLK %d"), Link.TLKStringID);
}
