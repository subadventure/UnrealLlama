// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuildingComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class EXAMPLE_API UBuildingComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

	friend class AExampleCharacter;

public:	
	// Sets default values for this component's properties
	UBuildingComponent();

	UPROPERTY(EditAnywhere, Category=Building)
	float HalfSize = 500;

	UPROPERTY(EditAnywhere, Category=Building)
	float ProjectingLength = 1000;


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	AExampleCharacter* PlayerCharacter;

};
