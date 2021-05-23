/*******************************************************************************
The content of the files in this repository include portions of the
AUDIOKINETIC Wwise Technology released in source code form as part of the SDK
package.

Commercial License Usage

Licensees holding valid commercial licenses to the AUDIOKINETIC Wwise Technology
may use these files in accordance with the end user license agreement provided
with the software or, alternatively, in accordance with the terms contained in a
written agreement between you and Audiokinetic Inc.

Copyright (c) 2021 Audiokinetic Inc.
*******************************************************************************/

// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2020
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 4.0.2019.08.13

#pragma once

#include <Mathematics/Logger.h>
#include <Mathematics/Vector2.h>

// Linear interpolation of a network of triangles whose vertices are of the
// form (x,y,f(x,y)).  The function samples are F[i] and represent
// f(x[i],y[i]), where i is the index of the input vertex (x[i],y[i]) to
// Delaunay2.
//
// The TriangleMesh interface must support the following:
//   bool GetIndices(int, std::array<int, 3>&) const;
//   bool GetBarycentrics(int, Vector2<Real> const&,
//       std::array<Real, 3>&) const;
//   int GetContainingTriangle(Vector2<Real> const&) const;

namespace gte
{
    template <typename Real, typename TriangleMesh>
    class IntpLinearNonuniform2
    {
    public:
        // Construction.
        IntpLinearNonuniform2(TriangleMesh const& mesh, Real const* F)
            :
            mMesh(&mesh),
            mF(F)
        {
            LogAssert(mF != nullptr, "Invalid input.");
        }

        // Linear interpolation.  The return value is 'true' if and only if
        // the input point P is in the convex hull of the input vertices, in
        // which case the interpolation is valid.
        bool operator()(Vector2<Real> const& P, Real& F) const
        {
            int t = mMesh->GetContainingTriangle(P);
            if (t == -1)
            {
                // The point is outside the triangulation.
                return false;
            }

            // Get the barycentric coordinates of P with respect to the triangle,
            // P = b0*V0 + b1*V1 + b2*V2, where b0 + b1 + b2 = 1.
            std::array<Real, 3> bary;
            if (!mMesh->GetBarycentrics(t, P, bary))
            {
                // TODO: Throw an exception or allow this as valid behavior?
                // P is in a needle-like or degenerate triangle.
                return false;
            }

            // The result is a barycentric combination of function values.
            std::array<int, 3> indices;
            mMesh->GetIndices(t, indices);
            F = bary[0] * mF[indices[0]] + bary[1] * mF[indices[1]] + bary[2] * mF[indices[2]];
            return true;
        }

    private:
        TriangleMesh const* mMesh;
        Real const* mF;
    };
}
