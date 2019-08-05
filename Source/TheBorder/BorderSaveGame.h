// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BorderSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class THEBORDER_API UBorderSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UBorderSaveGame();
	UPROPERTY(EditAnywhere)
		FVector PlayerLocation;
	UPROPERTY(EditAnywhere)
		FRotator PlayerRotation;



	
};
