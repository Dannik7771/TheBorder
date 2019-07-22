// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "TheBorderGameMode.h"
#include "TheBorderCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATheBorderGameMode::ATheBorderGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
