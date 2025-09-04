// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldActor.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Landscape.h"
#include "LandscapeEdit.h"

// Sets default values
AWorldActor::AWorldActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AWorldActor::GenerateTerrain()
{
	if (!Heightmap) {
		UE_LOG(LogTemp, Error, TEXT("Keine Heightmap gefunden"));
		return;
	}

	AActor* Actor = UGameplayStatics::GetActorOfClass(GetWorld(), ALandscape::StaticClass());

	if (ALandscape* Landscape = Cast<ALandscape>(Actor)) {

		FLandscapeEditDataInterface LandscapeEdit(Landscape->GetLandscapeInfo());

        const FIntPoint LandscapeVerts = GetLandscapeVertexCounts(Landscape);
        UE_LOG(LogTemp, Log, TEXT("LandscapeVerts:%s" ), *(LandscapeVerts.ToString()));

        UTextureRenderTarget2D* RT = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), LandscapeVerts.X+1, LandscapeVerts.Y+1);

        UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), RT, Heightmap);

        TArray<FLinearColor> OutColor;  TArray<uint16> InHeights;

        UKismetRenderingLibrary::ReadRenderTargetRaw(GetWorld(), RT, OutColor);

        for (FLinearColor Color : OutColor) {

            // Hier Conversion von Color.R in InHeights
            InHeights.Add(uint16(Color.R * 65534.0f));
        }

        LandscapeEdit.SetHeightData(0, 0, LandscapeVerts.X, LandscapeVerts.Y, InHeights.GetData(), 0, true);

	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Keine Landsacpe gefunden"));

	}

}

void AWorldActor::ManipulateHeights(const FVector InLocation, const float InDelta)
{

    AActor* Actor = UGameplayStatics::GetActorOfClass(GetWorld(), ALandscape::StaticClass());

    if (ALandscape* Landscape = Cast<ALandscape>(Actor)) {

        FLandscapeEditDataInterface LandscapeEdit(Landscape->GetLandscapeInfo());

        // Hole aktuellen Wert an (X, Y)
        TArray<uint16> Heights;
        Heights.SetNumUninitialized(1);

        const FIntRect Rect = GetLandscapeRect(InLocation, Landscape);

        LandscapeEdit.GetHeightDataFast(Rect.Min.X, Rect.Min.Y, Rect.Max.X, Rect.Max.Y, Heights.GetData(), 0);

        // Höhe ändern
        Heights[0] = FMath::Clamp<uint16>(Heights[0] + InDelta, 0, 65535);

        // Zurückschreiben
        LandscapeEdit.SetHeightData(Rect.Min.X, Rect.Min.Y, Rect.Max.X, Rect.Max.Y, Heights.GetData(), 0, true);




    }
    else {
        UE_LOG(LogTemp, Error, TEXT("Keine Landsacpe gefunden"));

    }
   
}

// Called when the game starts or when spawned
void AWorldActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWorldActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

AActor* AWorldActor::GenerateBackgroundLandscape(const int64 InHalfSize, const uint32 InScale, const uint32 InMaxHeight, const int32 InMinHeight)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    // Landscape finden
    ALandscape* Landscape = Cast<ALandscape>(
        UGameplayStatics::GetActorOfClass(World, ALandscape::StaticClass()));
    if (!Landscape) return nullptr;

    ULandscapeInfo* Info = Landscape->GetLandscapeInfo();
    if (!Info) return nullptr;

    FLandscapeEditDataInterface LandscapeEdit(Info);

    // MeshDescription vorbereiten
    FMeshDescription MeshDesc;
    FStaticMeshAttributes Attributes(MeshDesc);
    Attributes.Register();

    TMap<FIntPoint, FVertexID> VertexMap;

    const int32 NumVertsX = (InHalfSize * 2) / InScale + 1;
    const int32 NumVertsY = (InHalfSize * 2) / InScale + 1;

    for (int32 y = 0; y < NumVertsY; y++)
    {
        for (int32 x = 0; x < NumVertsX; x++)
        {
            const float WorldX = -InHalfSize + x * InScale;
            const float WorldY = -InHalfSize + y * InScale;

            // Höhe aus Landscape
            TArray<uint16> Heights;
            Heights.SetNumUninitialized(1);

            LandscapeEdit.GetHeightDataFast(WorldX, WorldY, WorldX, WorldY, Heights.GetData(),0);

            //float HeightNorm = (float)Heights / 65535.0f;
            //float WorldZ = FMath::Lerp((float)InMinHeight, (float)InMaxHeight, HeightNorm);

            // Vertex einfügen
            FVertexID VId = MeshDesc.CreateVertex();
            //Attributes.GetVertexPositions()[VId] = FVector(WorldX, WorldY, WorldZ);

            VertexMap.Add(FIntPoint(x, y), VId);
        }
    }

    // Triangles erzeugen
    for (int32 y = 0; y < NumVertsY - 1; y++)
    {
        for (int32 x = 0; x < NumVertsX - 1; x++)
        {
            const FVertexID V0 = VertexMap[FIntPoint(x, y)];
            const FVertexID V1 = VertexMap[FIntPoint(x + 1, y)];
            const FVertexID V2 = VertexMap[FIntPoint(x, y + 1)];
            const FVertexID V3 = VertexMap[FIntPoint(x + 1, y + 1)];

            // Quad → zwei Triangles
           ///// MeshDesc.CreateTriangle({ V0, V2, V1 });
            //MeshDesc.CreateTriangle({ V2, V3, V1 });
        }
    }

    // StaticMesh erstellen
    UStaticMesh* StaticMesh = NewObject<UStaticMesh>(World, UStaticMesh::StaticClass());
   /// StaticMesh->BuildFromMeshDescription(MeshDesc);

    // Actor erzeugen
    AActor* MeshActor = World->SpawnActor<AActor>();
    UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(MeshActor);
    MeshComp->SetStaticMesh(StaticMesh);
    MeshComp->RegisterComponent();
    MeshActor->SetRootComponent(MeshComp);

    return MeshActor;
}

// -------------------------------------- HELPER FUNCTIONS -------------------------------------------------- //
const FIntPoint AWorldActor::GetLandscapeVertexCounts(ALandscape* LandscapeActor)
{
    if (!LandscapeActor)
    {
        return FIntPoint::ZeroValue;
    }

    // Wir nehmen die erste Component als Referenz
    if (LandscapeActor->LandscapeComponents.Num() == 0)
    {
        return FIntPoint::ZeroValue;
    }

    const ULandscapeComponent* FirstComp = LandscapeActor->LandscapeComponents[0];

    // Größe einer einzelnen Komponente in Quads
    const int32 ComponentSizeQuads = FirstComp->ComponentSizeQuads;
    const int32 NumSubsections = FirstComp->NumSubsections;
    const int32 SubsectionSize = FirstComp->SubsectionSizeQuads;

    // Anzahl Komponenten in X und Y bestimmen
    int32 MinX = INT32_MAX, MinY = INT32_MAX, MaxX = INT32_MIN, MaxY = INT32_MIN;
    for (const ULandscapeComponent* Comp : LandscapeActor->LandscapeComponents)
    {
        if (!Comp) continue;
        MinX = FMath::Min(MinX, Comp->SectionBaseX);
        MinY = FMath::Min(MinY, Comp->SectionBaseY);
        MaxX = FMath::Max(MaxX, Comp->SectionBaseX);
        MaxY = FMath::Max(MaxY, Comp->SectionBaseY);
    }

    // Anzahl Komponenten in jeder Richtung
    const int32 NumComponentsX = ((MaxX - MinX) / ComponentSizeQuads) + 1;
    const int32 NumComponentsY = ((MaxY - MinY) / ComponentSizeQuads) + 1;

    // Gesamtanzahl der Vertices:
    // Jeder Component hat (ComponentSizeQuads+1) Vertices pro Seite
    const int32 TotalVertsX = NumComponentsX * ComponentSizeQuads + 1;
    const int32 TotalVertsY = NumComponentsY * ComponentSizeQuads + 1;

    return FIntPoint(TotalVertsX, TotalVertsY);
    
}

const FIntRect AWorldActor::GetLandscapeRect(const FVector& InWorldLocation, ALandscape* Landscape)
{
    FIntRect Result(0, 0, 0, 0);

    if (!Landscape) return Result;

    // 1. Landschaftsinfo abrufen
    ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
    if (!LandscapeInfo) return Result;

    // 2. World-Position in Landscape-Koordinaten umrechnen
    // Landscape->GetTransform() enthält Scale / Translation
    const FVector LocalPos = Landscape->GetTransform().InverseTransformPosition(InWorldLocation);

    // 3. X/Y Vertex-Koordinaten berechnen
    int32 VertexX = FMath::RoundToInt(LocalPos.X / Landscape->GetActorScale().X);
    int32 VertexY = FMath::RoundToInt(LocalPos.Y / Landscape->GetActorScale().Y);

    // 4. Landscape-Vertex-Range ermitteln (optional: einschränken auf gültige Vertices)
    const FIntPoint VertexCounts = GetLandscapeVertexCounts(Landscape); // deine Funktion von früher
    VertexX = FMath::Clamp(VertexX, 0, VertexCounts.X - 1);
    VertexY = FMath::Clamp(VertexY, 0, VertexCounts.Y - 1);

    // 5. FIntRect erstellen (hier zentriert auf ein 1x1 Vertex-Quad, kann angepasst werden)
    Result.Min = FIntPoint(VertexX, VertexY);
    Result.Max = FIntPoint(VertexX, VertexY);

    return Result;
}