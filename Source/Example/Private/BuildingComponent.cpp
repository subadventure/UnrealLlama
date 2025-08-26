// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "ExampleCharacter.h"

// Sets default values for this component's properties
UBuildingComponent::UBuildingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickInterval(.1);
	// ...
	PlayerCharacter = Cast<AExampleCharacter>(GetOwner());

}


// Called when the game starts
void UBuildingComponent::BeginPlay()
{
	Super::BeginPlay();

	UKismetSystemLibrary::PrintString(GetWorld(), PlayerCharacter ? "Player Character is da" : " Owner not valid!");
	// ...
	
}


// Called every frame
void UBuildingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	FHitResult Hit;
	const FVector Direction = PlayerCharacter->GetControlRotation().Vector();
	const FVector From = PlayerCharacter->GetFollowCamera()->GetComponentLocation() + Direction;
	UKismetSystemLibrary::LineTraceSingle(GetWorld(), From , From + Direction * (ProjectingLength + HalfSize), ETraceTypeQuery::TraceTypeQuery1, true, { PlayerCharacter }, EDrawDebugTrace::None, Hit, true);

	const FVector Target = (!Hit.GetActor() ? From + Direction * ProjectingLength : Hit.Location) - (GetStaticMesh() ? GetStaticMesh()->GetBoundingBox().GetExtent() : FVector(0)) -Direction * HalfSize;
	const FVector TargetGridded = FVector(FMath::RoundToFloat(Target.X / 100.0), FMath::RoundToFloat(Target.Y / 100.0), FMath::RoundToFloat(Target.Z / 100.0)) * 100.0;

	SetWorldLocation(TargetGridded);

}

