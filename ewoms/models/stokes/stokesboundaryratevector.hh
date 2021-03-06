// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*!
 * \file
 *
 * \copydoc Ewoms::StokesBoundaryRateVector
 */
#ifndef EWOMS_STOKES_BOUNDARY_RATE_VECTOR_HH
#define EWOMS_STOKES_BOUNDARY_RATE_VECTOR_HH

#include <opm/material/densead/Math.hpp>
#include <opm/material/common/Valgrind.hpp>
#include <opm/material/constraintsolvers/NcpFlash.hpp>

#include <dune/common/fvector.hh>

#include "stokesintensivequantities.hh"

namespace Ewoms {

/*!
 * \ingroup StokesModel
 *
 * \brief Implements a boundary vector for the fully implicit (Navier-)Stokes
 *        model.
 */
template <class TypeTag>
class StokesBoundaryRateVector : public GET_PROP_TYPE(TypeTag, RateVector)
{
    typedef typename GET_PROP_TYPE(TypeTag, RateVector) ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Evaluation) Evaluation;
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    enum { numComponents = FluidSystem::numComponents };
    enum { phaseIdx = GET_PROP_VALUE(TypeTag, StokesPhaseIndex) };
    enum { dimWorld = GridView::dimensionworld };

    enum { conti0EqIdx = Indices::conti0EqIdx };
    enum { momentum0EqIdx = Indices::momentum0EqIdx };
    enum { enableEnergy = GET_PROP_VALUE(TypeTag, EnableEnergy) };

    typedef Ewoms::EnergyModule<TypeTag, enableEnergy> EnergyModule;
    typedef Dune::FieldVector<Scalar, dimWorld> DimVector;

public:
    StokesBoundaryRateVector() : ParentType()
    {}

    /*!
     * \copydoc
     * ImmiscibleBoundaryRateVector::ImmiscibleBoundaryRateVector(Scalar)
     */
    StokesBoundaryRateVector(Scalar value) : ParentType(value)
    {}

    /*!
     * \copydoc ImmiscibleBoundaryRateVector::ImmiscibleBoundaryRateVector(const
     * ImmiscibleBoundaryRateVector &)
     */
    StokesBoundaryRateVector(const StokesBoundaryRateVector &value)
        : ParentType(value)
    {}

    /*!
     * \param context The execution context for which the boundary rate should be specified.
     * \param bfIdx The local index of the boundary segment (-> local space index).
     * \param timeIdx The index used by the time discretization.
     * \param velocity The velocity vector [m/s] at the boundary.
     * \param fluidState The repesentation of the thermodynamic state
     *                   of the system on the integration point of the
     *                   boundary segment.
     */
    template <class Context, class FluidState>
    void setFreeFlow(const Context &context, int bfIdx, int timeIdx,
                     const DimVector &velocity, const FluidState &fluidState)
    {
        const auto &stencil = context.stencil(timeIdx);
        const auto &scvf = stencil.boundaryFace(bfIdx);

        int insideScvIdx = context.interiorScvIndex(bfIdx, timeIdx);
        //const auto &insideScv = stencil.subControlVolume(insideScvIdx);
        const auto &insideIntQuants = context.intensiveQuantities(bfIdx, timeIdx);

        // the outer unit normal
        const auto &normal = scvf.normal();

        // distance between the center of the SCV and center of the boundary face
        DimVector distVec = stencil.subControlVolume(insideScvIdx).geometry().center();
        const auto &scvPos = context.element().geometry().corner(insideScvIdx);
        distVec.axpy(-1, scvPos);

        Scalar dist = 0.0;
        for (int dimIdx = 0; dimIdx < dimWorld; ++dimIdx)
            dist += distVec[dimIdx] * normal[dimIdx];
        dist = std::abs(dist);

        DimVector gradv[dimWorld];
        for (int axisIdx = 0; axisIdx < dimWorld; ++axisIdx) {
            // Approximation of the pressure gradient at the boundary
            // segment's integration point.
            gradv[axisIdx] = normal;
            gradv[axisIdx] *= (velocity[axisIdx]
                               - insideIntQuants.velocity()[axisIdx]) / dist;
            Valgrind::CheckDefined(gradv[axisIdx]);
        }

        // specify the mass fluxes over the boundary
        Scalar volumeFlux = 0;
        for (int dimIdx = 0; dimIdx < dimWorld; ++dimIdx)
            volumeFlux += velocity[dimIdx]*normal[dimIdx];

        typename FluidSystem::template ParameterCache<Evaluation> paramCache;
        paramCache.updatePhase(fluidState, phaseIdx);
        Scalar density = FluidSystem::density(fluidState, paramCache, phaseIdx);
        Scalar molarDensity = density / fluidState.averageMolarMass(phaseIdx);
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            (*this)[conti0EqIdx + compIdx] =
                volumeFlux
                * molarDensity
                * fluidState.moleFraction(phaseIdx, compIdx);
        }

        // calculate the momentum flux over the boundary
        for (int axisIdx = 0; axisIdx < dimWorld; ++axisIdx) {
            // calculate a row of grad v + (grad v)^T
            DimVector tmp(0.0);
            for (int j = 0; j < dimWorld; ++j) {
                tmp[j] = gradv[axisIdx][j] + gradv[j][axisIdx];
            }

            // the momentum flux due to viscous forces
            Scalar tmp2 = 0.0;
            for (int dimIdx = 0; dimIdx < dimWorld; ++dimIdx)
                tmp2 += tmp[dimIdx]*normal[dimIdx];

            (*this)[momentum0EqIdx + axisIdx] = -insideIntQuants.fluidState().viscosity(phaseIdx) * tmp2;
        }

        EnergyModule::setEnthalpyRate(*this, fluidState, phaseIdx, volumeFlux);
    }

    /*!
     * \brief Set a in-flow boundary in the (Navier-)Stoke model
     *
     * \param context The execution context for which the boundary rate should
     *                be specified.
     * \param bfIdx The local space index of the boundary segment.
     * \param timeIdx The index used by the time discretization.
     * \param velocity The velocity vector [m/s] at the boundary.
     * \param fluidState The repesentation of the thermodynamic state
     *                   of the system on the integration point of the
     *                   boundary segment.
     */
    template <class Context, class FluidState>
    void setInFlow(const Context &context, int bfIdx, int timeIdx,
                   const DimVector &velocity, const FluidState &fluidState)
    {
        const auto &intQuants = context.intensiveQuantities(bfIdx, timeIdx);

        setFreeFlow(context, bfIdx, timeIdx, velocity, fluidState);

        // don't let mass flow out
        for (int compIdx = 0; compIdx < numComponents; ++compIdx)
            (*this)[conti0EqIdx + compIdx] = std::min<Scalar>(0.0, (*this)[conti0EqIdx + compIdx]);

        // don't let momentum flow out
        for (int axisIdx = 0; axisIdx < dimWorld; ++axisIdx)
            (*this)[momentum0EqIdx + axisIdx] = std::min<Scalar>(0.0, (*this)[momentum0EqIdx + axisIdx]);
    }

    /*!
     * \brief Set a out-flow boundary in the (Navier-)Stoke model
     *
     * \copydoc Doxygen::contextParams
     */
    template <class Context>
    void setOutFlow(const Context &context, int spaceIdx, int timeIdx)
    {
        const auto &intQuants = context.intensiveQuantities(spaceIdx, timeIdx);

        DimVector velocity = intQuants.velocity();
        const auto &fluidState = intQuants.fluidState();

        setFreeFlow(context, spaceIdx, timeIdx, velocity, fluidState);

        // don't let mass flow in
        for (int compIdx = 0; compIdx < numComponents; ++compIdx)
            (*this)[conti0EqIdx + compIdx] = std::max<Scalar>(0.0, (*this)[conti0EqIdx + compIdx]);

        // don't let momentum flow in
        for (int axisIdx = 0; axisIdx < dimWorld; ++axisIdx)
            (*this)[momentum0EqIdx + axisIdx] = std::max<Scalar>(0.0, (*this)[momentum0EqIdx + axisIdx]);
    }

    /*!
     * \brief Set a no-flow boundary in the (Navier-)Stoke model
     *
     * \copydoc Doxygen::contextParams
     */
    template <class Context>
    void setNoFlow(const Context &context, int spaceIdx, int timeIdx)
    {
        static DimVector v0(0.0);

        const auto &intQuants = context.intensiveQuantities(spaceIdx, timeIdx);
        const auto &fluidState = intQuants.fluidState(); // don't care

        // no flow of mass and no slip for the momentum
        setFreeFlow(context, spaceIdx, timeIdx,
                    /*velocity = */ v0, fluidState);
    }
};

} // namespace Ewoms

#endif
