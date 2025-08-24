// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/MultiLineEditableTextBox.h"
#include "UserInterface.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class EXAMPLE_API UUserInterface : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, Category = llama)
	void TextChanged(const FString& InText);

	UFUNCTION(BlueprintImplementableEvent, Category = llama)
	void DownloadProgress(const float Progress);

	UFUNCTION(BlueprintImplementableEvent, Category = llama)
	void DownloadFinished();


};
