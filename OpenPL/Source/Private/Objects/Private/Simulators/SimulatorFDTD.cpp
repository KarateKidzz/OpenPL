/*
  ==============================================================================

    SimulatorFDTD.cpp
    Created: 3 Mar 2021 11:52:13am
    Author:  James Kelly

  ==============================================================================
*/

#include "Simulators/SimulatorFDTD.h"
#include "OpenPLCommonPrivate.h"
#include "PL_SCENE.h"
#include "PL_SYSTEM.h"

void SimulatorFDTD::Simulate(int SimulateVoxelIndex)
{
    if (Lattice == nullptr || Lattice->size() == 0)
    {
        DebugError("Voxel lattice is either null or has no voxels!");
        return;
    }
    
    // Reset all pressure and velocity
    {
        for(auto& Voxel : *Lattice)
        {
            Voxel.AirPressure = 0.0f;
            Voxel.ParticleVelocityX = 0.0f;
            Voxel.ParticleVelocityY = 0.0f;
            Voxel.ParticleVelocityZ = 0.0f;
        }
    }
    
    // Time-stepped FDTD
    for (int CurrentTimeStep = 0; CurrentTimeStep < TimeSteps; CurrentTimeStep++)
    {
        // Pressure grid
        {
            for (int x = 0; x < XSize; x++)
            {
                for (int y = 0; y < YSize; y++)
                {
                    for (int z = 0; z < ZSize; z++)
                    {
                        PLVoxel& CurrentVoxel = (*Lattice)[ThreeDimToOneDim(x, y, z, XSize, YSize)];
                        PLVoxel NextVoxelX = x + 1 >= XSize ? PLVoxel() : (*Lattice)[ThreeDimToOneDim(x + 1, y, z, XSize, YSize)];
                        PLVoxel NextVoxelY = y + 1 >= YSize ? PLVoxel() : (*Lattice)[ThreeDimToOneDim(x, y + 1, z, XSize, YSize)];
                        PLVoxel NextVoxelZ = z + 1 >= ZSize ? PLVoxel() : (*Lattice)[ThreeDimToOneDim(x, y, z + 1, XSize, YSize)];
                        
                        double Beta = static_cast<double>(CurrentVoxel.Beta);
                        
                        const double Divergance = ((NextVoxelX.ParticleVelocityX - CurrentVoxel.ParticleVelocityX) + (NextVoxelY.ParticleVelocityY - CurrentVoxel.ParticleVelocityY) + (NextVoxelZ.ParticleVelocityZ - CurrentVoxel.ParticleVelocityZ));
                        CurrentVoxel.AirPressure = Beta * (CurrentVoxel.AirPressure - UpdateCoefficents * Divergance);
                    }
                }
            }
        }
        
        // Process X Velocity
        {
            for (int x = 1; x < XSize; x++)
            {
                for (int y = 0; y < YSize; y++)
                {
                    for (int z = 0; z < ZSize; z++)
                    {
                        // Basically don't understand any of this!!!
                        
                        const PLVoxel& PreviousVoxel = (*Lattice)[ThreeDimToOneDim(x-1, y, z, XSize, YSize)];
                        
                        const double BetaNext = static_cast<double>(PreviousVoxel.Beta);
                        const double AbsorptionNext = PreviousVoxel.Absorptivity;
                        const double YNext = (1.f - AbsorptionNext) / (1.f + AbsorptionNext);  // What is Y? Yee?
                        
                        PLVoxel& CurrentVoxel = (*Lattice)[ThreeDimToOneDim(x, y, z, XSize, YSize)];
                        
                        const double BetaThis = static_cast<double>(CurrentVoxel.Beta);
                        const double AbsorptionThis = CurrentVoxel.Absorptivity;
                        const double YThis = (1.f - AbsorptionThis) / (1.f + AbsorptionThis);  // What is Y?
                        
                        const double GradientX = (CurrentVoxel.AirPressure - PreviousVoxel.AirPressure);
                        const double AirCellUpdate = CurrentVoxel.ParticleVelocityX - UpdateCoefficents * GradientX;
                        
                        const double YBoundary = BetaThis * YNext + BetaNext * YThis;
                        const double WallCellUpdate = YBoundary * (PreviousVoxel.AirPressure * BetaNext + CurrentVoxel.AirPressure * BetaThis);
                        
                        CurrentVoxel.ParticleVelocityX = BetaThis * BetaNext * AirCellUpdate + (BetaNext - BetaThis) * WallCellUpdate;
                    }
                }
            }
        }
        
        // Process Y Velocity
//        {
//            for (int x = 0; x < XSize; x++)
//            {
//                for (int y = 1; y < YSize; y++)
//                {
//                    for (int z = 0; z < ZSize; z++)
//                    {
//                        // Basically don't understand any of this!!!
//
//                        const PLVoxel& PreviousVoxel = (*Lattice)[ThreeDimToOneDim(x, y-1, z, XSize, YSize)];
//
//                        const double BetaNext = static_cast<double>(PreviousVoxel.Beta);
//                        const double AbsorptionNext = PreviousVoxel.Absorptivity;
//                        const double YNext = (1.f - AbsorptionNext) / (1.f + AbsorptionNext);  // What is Y?
//
//                        PLVoxel& CurrentVoxel = (*Lattice)[ThreeDimToOneDim(x, y, z, XSize, YSize)];
//
//                        const double BetaThis = static_cast<double>(CurrentVoxel.Beta);
//                        const double AbsorptionThis = CurrentVoxel.Absorptivity;
//                        const double YThis = (1.f - AbsorptionThis) / (1.f + AbsorptionThis);  // What is Y?
//
//                        const double GradientY = (CurrentVoxel.AirPressure - PreviousVoxel.AirPressure);
//                        const double AirCellUpdate = CurrentVoxel.ParticleVelocityY - UpdateCoefficents * GradientY;
//
//                        const double YBoundary = BetaThis * YNext + BetaNext * YThis;
//                        const double WallCellUpdate = YBoundary * (PreviousVoxel.AirPressure * BetaNext + CurrentVoxel.AirPressure * BetaThis);
//
//                        CurrentVoxel.ParticleVelocityY = BetaThis * BetaNext * AirCellUpdate + (BetaNext - BetaThis) * WallCellUpdate;
//                    }
//                }
//            }
//        }
        
        // Process Z Velocity
        {
            for (int x = 0; x < XSize; x++)
            {
                for (int y = 0; y < YSize; y++)
                {
                    for (int z = 1; z < ZSize; z++)
                    {
                        // Basically don't understand any of this!!!

                        const PLVoxel& PreviousVoxel = (*Lattice)[ThreeDimToOneDim(x, y, z-1, XSize, YSize)];

                        const double BetaNext = static_cast<double>(PreviousVoxel.Beta);
                        const double AbsorptionNext = PreviousVoxel.Absorptivity;
                        const double YNext = (1.f - AbsorptionNext) / (1.f + AbsorptionNext);  // What is Y?

                        PLVoxel& CurrentVoxel = (*Lattice)[ThreeDimToOneDim(x, y, z, XSize, YSize)];

                        const double BetaThis = static_cast<double>(CurrentVoxel.Beta);
                        const double AbsorptionThis = CurrentVoxel.Absorptivity;
                        const double YThis = (1.f - AbsorptionThis) / (1.f + AbsorptionThis);  // What is Y?

                        const double GradientZ = (CurrentVoxel.AirPressure - PreviousVoxel.AirPressure);
                        const double AirCellUpdate = CurrentVoxel.ParticleVelocityZ - UpdateCoefficents * GradientZ;

                        const double YBoundary = BetaThis * YNext + BetaNext * YThis;
                        const double WallCellUpdate = YBoundary * (PreviousVoxel.AirPressure * BetaNext + CurrentVoxel.AirPressure * BetaThis);

                        CurrentVoxel.ParticleVelocityZ = BetaThis * BetaNext * AirCellUpdate + (BetaNext - BetaThis) * WallCellUpdate;
                    }
                }
            }
        }
        
        // Absorption top/bottom
        {
            for (int i = 0; i < ZSize; ++i)
            {
                const int Index1 = i;
                const int Index2 = XSize * (ZSize + 1) + i;
                
                (*Lattice)[Index1].ParticleVelocityX = -(*Lattice)[Index1].AirPressure;
                (*Lattice)[Index2].ParticleVelocityX = (*Lattice)[Index2 - YSize - 1].AirPressure;
            }
        }
        
        // Absorption left/right
        {
            for (int i = 0; i < XSize; ++i)
            {
                const int Index1 = i * (ZSize + 1);
                const int Index2 = i * (ZSize + 1) + ZSize;
                
                (*Lattice)[Index1].ParticleVelocityZ = -(*Lattice)[Index1].AirPressure;
                (*Lattice)[Index2].ParticleVelocityZ = (*Lattice)[Index2 - 1].AirPressure;
            }
        }
        
        // Add response
        {
            for (int i = 0; i < CubeSize; i++)
            {
                SimulatedLattice[i][CurrentTimeStep] = (*Lattice)[i];
            }
        }
        
        (*Lattice)[SimulateVoxelIndex].AirPressure += Pulse[CurrentTimeStep];
    }
}
