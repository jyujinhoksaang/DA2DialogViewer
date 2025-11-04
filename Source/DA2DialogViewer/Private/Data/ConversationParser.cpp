// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/ConversationParser.h"
#include "XmlFile.h"

bool FConversationParser::ParseConversation(const FString& FilePath, FConversation& OutConversation)
{
	OutConversation.Clear();

	// Load XML file
	TSharedPtr<FXmlFile> XmlFile = MakeShared<FXmlFile>(FilePath, EConstructMethod::ConstructFromFile);
	if (!XmlFile->IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse XML file: %s"), *FilePath);
		return false;
	}

	// Get root node
	const FXmlNode* RootNode = XmlFile->GetRootNode();
	if (!RootNode)
	{
		UE_LOG(LogTemp, Error, TEXT("XML file has no root node: %s"), *FilePath);
		return false;
	}

	// Find CONV struct
	const FXmlNode* ConvNode = nullptr;
	for (const FXmlNode* Child : RootNode->GetChildrenNodes())
	{
		if (Child->GetTag() == TEXT("struct") && Child->GetAttribute(TEXT("name")) == TEXT("CONV"))
		{
			ConvNode = Child;
			break;
		}
	}

	if (!ConvNode)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find CONV struct in XML: %s"), *FilePath);
		return false;
	}

	// Extract conversation name from file path
	OutConversation.ConversationName = FPaths::GetBaseFilename(FilePath);

	// Parse entry links (label 30001)
	ParseEntryLinks(ConvNode, OutConversation);

	// Parse dialog lines (label 30002)
	ParseDialogLines(ConvNode, OutConversation);

	UE_LOG(LogTemp, Log, TEXT("Parsed conversation: %s (%d entries, %d nodes)"),
		*OutConversation.ConversationName, OutConversation.EntryLinks.Num(), OutConversation.Nodes.Num());

	return true;
}

void FConversationParser::ParseEntryLinks(const FXmlNode* ConvNode, FConversation& OutConversation)
{
	// Find struct_list with label 30001
	const FXmlNode* EntryListNode = FindNodeByLabel(ConvNode, TEXT("30001"));
	if (!EntryListNode)
	{
		return;
	}

	// Parse each LINK struct
	for (const FXmlNode* LinkNode : EntryListNode->GetChildrenNodes())
	{
		if (LinkNode->GetTag() == TEXT("struct") && LinkNode->GetAttribute(TEXT("name")) == TEXT("LINK"))
		{
			FDialogEntryLink Entry;

			// label 30100 = target node index
			const FXmlNode* TargetNode = FindNodeByLabel(LinkNode, TEXT("30100"));
			if (TargetNode)
			{
				Entry.TargetNodeIndex = GetUInt16Value(TargetNode);
			}

			// label 30101 = TLK string (usually empty)
			const FXmlNode* TLKNode = FindNodeByLabel(LinkNode, TEXT("30101"));
			if (TLKNode)
			{
				Entry.TLKStringID = GetTLKStringID(TLKNode);
			}

			// label 30301 = icon override
			const FXmlNode* IconNode = FindNodeByLabel(LinkNode, TEXT("30301"));
			if (IconNode)
			{
				Entry.IconOverride = GetUInt8Value(IconNode, 255);
			}

			// label 30303 = condition flags
			const FXmlNode* CondNode = FindNodeByLabel(LinkNode, TEXT("30303"));
			if (CondNode)
			{
				Entry.ConditionFlags = GetUInt32Value(CondNode);
			}

			OutConversation.EntryLinks.Add(Entry);
		}
	}
}

void FConversationParser::ParseDialogLines(const FXmlNode* ConvNode, FConversation& OutConversation)
{
	// Find struct_list with label 30002
	const FXmlNode* LinesListNode = FindNodeByLabel(ConvNode, TEXT("30002"));
	if (!LinesListNode)
	{
		return;
	}

	// Parse each LINE struct
	int32 NodeIndex = 0;
	for (const FXmlNode* LineNode : LinesListNode->GetChildrenNodes())
	{
		if (LineNode->GetTag() == TEXT("struct") && LineNode->GetAttribute(TEXT("name")) == TEXT("LINE"))
		{
			FDialogNode Node = ParseLine(LineNode);
			Node.NodeIndex = NodeIndex++;
			OutConversation.Nodes.Add(Node);
		}
	}
}

FDialogNode FConversationParser::ParseLine(const FXmlNode* LineNode)
{
	FDialogNode Node;

	// label 30200 = speaker ID
	const FXmlNode* SpeakerNode = FindNodeByLabel(LineNode, TEXT("30200"));
	if (SpeakerNode)
	{
		Node.SpeakerID = GetUInt16Value(SpeakerNode);
	}

	// label 30201 = line text (TLK string)
	const FXmlNode* TextNode = FindNodeByLabel(LineNode, TEXT("30201"));
	if (TextNode)
	{
		Node.TLKStringID = GetTLKStringID(TextNode);
	}

	// label 30202 = condition plot
	const FXmlNode* ConditionNode = FindNodeByLabel(LineNode, TEXT("30202"));
	if (ConditionNode)
	{
		Node.Condition = ParsePlotReference(ConditionNode);
	}

	// label 30203 = action plot
	const FXmlNode* ActionNode = FindNodeByLabel(LineNode, TEXT("30203"));
	if (ActionNode)
	{
		Node.Action = ParsePlotReference(ActionNode);
	}

	// label 30204 = child links
	const FXmlNode* LinksListNode = FindNodeByLabel(LineNode, TEXT("30204"));
	if (LinksListNode)
	{
		for (const FXmlNode* LinkNode : LinksListNode->GetChildrenNodes())
		{
			if (LinkNode->GetTag() == TEXT("struct") && LinkNode->GetAttribute(TEXT("name")) == TEXT("LINK"))
			{
				FDialogLink Link = ParseLink(LinkNode);
				Node.Links.Add(Link);
			}
		}
	}

	return Node;
}

FDialogLink FConversationParser::ParseLink(const FXmlNode* LinkNode)
{
	FDialogLink Link;

	// label 30100 = target node index
	const FXmlNode* TargetNode = FindNodeByLabel(LinkNode, TEXT("30100"));
	if (TargetNode)
	{
		Link.TargetNodeIndex = GetUInt16Value(TargetNode);
	}

	// label 30101 = link text (TLK string)
	const FXmlNode* TextNode = FindNodeByLabel(LinkNode, TEXT("30101"));
	if (TextNode)
	{
		Link.TLKStringID = GetTLKStringID(TextNode);
	}

	// label 30300 = response type
	const FXmlNode* TypeNode = FindNodeByLabel(LinkNode, TEXT("30300"));
	if (TypeNode)
	{
		uint8 TypeValue = GetUInt8Value(TypeNode, 255);
		Link.ResponseType = static_cast<EResponseType>(TypeValue);
	}

	// label 30301 = icon override
	const FXmlNode* IconNode = FindNodeByLabel(LinkNode, TEXT("30301"));
	if (IconNode)
	{
		Link.IconOverride = GetUInt8Value(IconNode, 255);
	}

	// label 30303 = condition flags
	const FXmlNode* CondNode = FindNodeByLabel(LinkNode, TEXT("30303"));
	if (CondNode)
	{
		Link.ConditionFlags = GetUInt32Value(CondNode);
	}

	return Link;
}

FPlotReference FConversationParser::ParsePlotReference(const FXmlNode* PlotNode)
{
	FPlotReference PlotRef;

	// label 30400 = plot name
	const FXmlNode* NameNode = FindNodeByLabel(PlotNode, TEXT("30400"));
	if (NameNode)
	{
		PlotRef.PlotName = GetStringValue(NameNode);
	}

	// label 30401 = flag index
	const FXmlNode* FlagNode = FindNodeByLabel(PlotNode, TEXT("30401"));
	if (FlagNode)
	{
		PlotRef.FlagIndex = GetSInt32Value(FlagNode, -1);
	}

	// label 30402 = comparison type
	const FXmlNode* CompNode = FindNodeByLabel(PlotNode, TEXT("30402"));
	if (CompNode)
	{
		PlotRef.ComparisonType = GetUInt8Value(CompNode, 255);
	}

	return PlotRef;
}

const FXmlNode* FConversationParser::FindNodeByLabel(const FXmlNode* ParentNode, const FString& Label)
{
	if (!ParentNode)
	{
		return nullptr;
	}

	for (const FXmlNode* Child : ParentNode->GetChildrenNodes())
	{
		FString LabelAttr = Child->GetAttribute(TEXT("label"));
		if (LabelAttr == Label)
		{
			return Child;
		}
	}

	return nullptr;
}

uint16 FConversationParser::GetUInt16Value(const FXmlNode* Node, uint16 DefaultValue)
{
	if (!Node)
	{
		return DefaultValue;
	}

	FString Content = Node->GetContent();
	if (Content.IsEmpty())
	{
		return DefaultValue;
	}

	return (uint16)FCString::Atoi(*Content);
}

uint32 FConversationParser::GetUInt32Value(const FXmlNode* Node, uint32 DefaultValue)
{
	if (!Node)
	{
		return DefaultValue;
	}

	FString Content = Node->GetContent();
	if (Content.IsEmpty())
	{
		return DefaultValue;
	}

	return (uint32)FCString::Atoi64(*Content);
}

int32 FConversationParser::GetSInt32Value(const FXmlNode* Node, int32 DefaultValue)
{
	if (!Node)
	{
		return DefaultValue;
	}

	FString Content = Node->GetContent();
	if (Content.IsEmpty())
	{
		return DefaultValue;
	}

	return FCString::Atoi(*Content);
}

uint8 FConversationParser::GetUInt8Value(const FXmlNode* Node, uint8 DefaultValue)
{
	if (!Node)
	{
		return DefaultValue;
	}

	FString Content = Node->GetContent();
	if (Content.IsEmpty())
	{
		return DefaultValue;
	}

	return (uint8)FCString::Atoi(*Content);
}

FString FConversationParser::GetStringValue(const FXmlNode* Node, const FString& DefaultValue)
{
	if (!Node)
	{
		return DefaultValue;
	}

	return Node->GetContent();
}

int32 FConversationParser::GetTLKStringID(const FXmlNode* TLKNode)
{
	if (!TLKNode || TLKNode->GetTag() != TEXT("tlkstring"))
	{
		return -1;
	}

	// Find uint32 child node
	for (const FXmlNode* Child : TLKNode->GetChildrenNodes())
	{
		if (Child->GetTag() == TEXT("uint32"))
		{
			return GetSInt32Value(Child, -1);
		}
	}

	return -1;
}
