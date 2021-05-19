// Fill out your copyright notice in the Description page of Project Settings.


#include "RuntimeOpenPL.h"
#include "OpenPropagationLibrary.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "OpenPLUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "FMODAudioComponent.h"

using namespace OpenPL;

// Sets default values
ARuntimeOpenPL::ARuntimeOpenPL()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ARuntimeOpenPL::BeginPlay()
{
	Super::BeginPlay();
	
    FOpenPropagationLibraryModule& OpenPLModule = FOpenPropagationLibraryModule::Get();
    Scene = TUniquePtr<PLScene>(OpenPLModule.CreateScene());
    
    if (!Scene)
    {
        return;
    }
    
    Scene->CreateVoxels(PLVector(10,10,10), 1);
    
    for (AStaticMeshActor* MeshActor : StaticMeshes)
    {
        TArray<PLVector> Vertices;
        TArray<int> ActualIndices;
        
        UStaticMeshComponent* StaticMeshComponent = MeshActor->GetStaticMeshComponent();
        UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
        
        const int32 LOD = 0;
        
        if (StaticMesh->HasValidRenderData(true, LOD))
        {
            const auto& RenderData = StaticMesh->GetLODForExport(LOD);
            const FPositionVertexBuffer& VertexBuffer = RenderData.VertexBuffers.PositionVertexBuffer;
            
            /*GetCopy
             (
                 TArray < uint32 >& OutIndices
             )
            */
            
            
            
            const FRawStaticIndexBuffer& IndexBuffer = RenderData.IndexBuffer;
            TArray<uint32> Indices;
            IndexBuffer.GetCopy(Indices);
            auto TriangleCount = RenderData.GetNumTriangles();
            auto VertexCount = VertexBuffer.GetNumVertices();
            
            for (int i = 0; i < VertexCount; i++)
            {
                const FVector& VertexPos = VertexBuffer.VertexPosition(i);
                const FVector& VertexWorld = MeshActor->GetTransform().TransformPosition(VertexPos);
                Vertices.Add(PLVector(VertexWorld.X / 100, VertexWorld.Y / 100, VertexWorld.Z));
            }
            
            for (int i = 0; i < Indices.Num(); i++)
            {
                ActualIndices.Add(static_cast<int>(Indices[i]));
            }

            int32 AddedMeshIndex = -1;
            Scene->AddMesh(ConvertUnrealVectorToPL(MeshActor->GetActorLocation()), ConvertUnrealVectorToPL4(MeshActor->GetActorRotation().Euler()), ConvertUnrealVectorToPL(MeshActor->GetActorScale()), Vertices.GetData(), VertexCount, ActualIndices.GetData(), TriangleCount, &AddedMeshIndex);
        }
    }
    
    Scene->FillVoxelsWithGeometry();
    
    int VoxelCount;
    Scene->GetVoxelsCount(&VoxelCount);
    
    Scene->Simulate(PLVector(0,0,0));
        
    Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
}

// Called every frame
void ARuntimeOpenPL::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    if (Player)
    {
        FVector ListenerLocation = Player->GetActorLocation();
        FVector EmitterLocation = FMODEvent->GetActorLocation();
        EmitterLocation.Z = ListenerLocation.Z;
        
        Scene->Simulate(ConvertUnrealVectorToPL(ListenerLocation));
        
        float OutOcclusion;
        Scene->GetOcclusion(ConvertUnrealVectorToPL(EmitterLocation), &OutOcclusion);
        
        UFMODAudioComponent* AudioComponent = FMODEvent->AudioComponent;
        
        if (OutOcclusion > 1)
        {
            OutOcclusion = 1;
        }
        if (OutOcclusion < 0)
        {
            OutOcclusion = 0;
        }
        AudioComponent->SetParameter("Occlusion", 1 - OutOcclusion);
    }
}

