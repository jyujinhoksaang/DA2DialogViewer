// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogFlow/Conversation.h"

class FXmlFile;
class FXmlNode;

/**
 * Parser for DA2 conversation XML files
 */
class FConversationParser
{
public:
	/**
	 * Parse conversation XML file
	 * @param FilePath Path to conversation XML file
	 * @param OutConversation Output conversation object
	 * @return True if parsing succeeded
	 */
	static bool ParseConversation(const FString& FilePath, FConversation& OutConversation);

private:
	// Parse entry links (label 30001)
	static void ParseEntryLinks(const FXmlNode* ConvNode, FConversation& OutConversation);

	// Parse dialog lines (label 30002)
	static void ParseDialogLines(const FXmlNode* ConvNode, FConversation& OutConversation);

	// Parse a single LINK struct
	static FDialogLink ParseLink(const FXmlNode* LinkNode);

	// Parse a single LINE struct
	static FDialogNode ParseLine(const FXmlNode* LineNode);

	// Parse plot reference
	static FPlotReference ParsePlotReference(const FXmlNode* PlotNode);

	// Find child node by label
	static const FXmlNode* FindNodeByLabel(const FXmlNode* ParentNode, const FString& Label);

	// Get uint16 value from node
	static uint16 GetUInt16Value(const FXmlNode* Node, uint16 DefaultValue = 0);

	// Get uint32 value from node
	static uint32 GetUInt32Value(const FXmlNode* Node, uint32 DefaultValue = 0);

	// Get sint32 value from node
	static int32 GetSInt32Value(const FXmlNode* Node, int32 DefaultValue = 0);

	// Get uint8 value from node
	static uint8 GetUInt8Value(const FXmlNode* Node, uint8 DefaultValue = 0);

	// Get string value from node
	static FString GetStringValue(const FXmlNode* Node, const FString& DefaultValue = FString());

	// Get TLK string ID from tlkstring node
	static int32 GetTLKStringID(const FXmlNode* TLKNode);
};
