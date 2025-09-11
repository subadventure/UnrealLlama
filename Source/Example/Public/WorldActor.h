// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Landscape.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldActor.generated.h"

UCLASS()
class EXAMPLE_API AWorldActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWorldActor();
	
	UFUNCTION(BlueprintCallable, Category=World)
	void GenerateTerrain(UMaterialInterface* InMaterial);

	UFUNCTION(CallInEditor, BlueprintCallable, Category=World)
	void ManipulateHeights(const FVector InLocation, const float InDelta);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual bool ShouldTickIfViewportsOnly() const override {return true;}

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:

	// ---------------------------------------- HELPER FUNCTIONS ------------------------------------------------ //
	
	const FIntPoint GetLandscapeVertexCounts(ALandscape* LandscapeActor);
	const FIntRect GetLandscapeRect(const FVector& InWorldLocation, ALandscape* Landscape);
	AActor* GenerateBackgroundLandscape(const int64 InHalfSize, const uint32 InScale, const uint32 InMaxHeight, const int32 InMinHeight);


};
