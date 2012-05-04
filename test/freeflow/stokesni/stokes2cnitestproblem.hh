// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2009-2012 by Klaus Mosthaf                                *
 *   Copyright (C) 2007-2008 by Bernd Flemisch                               *
 *   Copyright (C) 2008-2009 by Andreas Lauser                               *
 *   Institute for Modelling Hydraulic and Environmental Systems             *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/**
 * @file
 * @brief  Definition of a simple Stokes problem
 * @author Klaus Mosthaf, Andreas Lauser, Bernd Flemisch
 */
#ifndef DUMUX_STOKES2CNITESTPROBLEM_HH
#define DUMUX_STOKES2CNITESTPROBLEM_HH

#include <dumux/freeflow/stokes2cni/stokes2cnimodel.hh>
#include <dumux/material/fluidsystems/h2oairfluidsystem.hh>

#if HAVE_UG
#include <dune/grid/io/file/dgfparser/dgfug.hh>
#endif
#include <dune/grid/io/file/dgfparser/dgfs.hh>
#include <dune/grid/io/file/dgfparser/dgfyasp.hh>
#include <dune/common/fvector.hh>

namespace Dumux
{

template <class TypeTag>
class Stokes2cniTestProblem;

//////////
// Specify the properties for the stokes problem
//////////
namespace Properties
{
NEW_TYPE_TAG(Stokes2cniTestProblem, INHERITS_FROM(BoxStokes2cni));

// Set the grid type
SET_TYPE_PROP(Stokes2cniTestProblem, Grid, Dune::SGrid<2,2>);

// Set the problem property
SET_TYPE_PROP(Stokes2cniTestProblem, Problem, Stokes2cniTestProblem<TypeTag>);

//! Select the fluid system
SET_TYPE_PROP(Stokes2cniTestProblem, 
              FluidSystem,
              Dumux::FluidSystems::H2OAir<typename GET_PROP_TYPE(TypeTag, Scalar)>);

//! Select the phase to be considered
SET_INT_PROP(Stokes2cniTestProblem,
             StokesPhaseIndex,
             GET_PROP_TYPE(TypeTag, FluidSystem)::gPhaseIdx);

//! Select the phase to be considered by the transport equation
SET_INT_PROP(Stokes2cniTestProblem, 
             StokesComponentIndex,
             GET_PROP_TYPE(TypeTag, FluidSystem)::H2OIdx);

//! a stabilization factor. Set to zero for no stabilization
SET_SCALAR_PROP(Stokes2cniTestProblem, StabilizationAlpha, -1.0);

//! stabilization at the boundaries
SET_SCALAR_PROP(Stokes2cniTestProblem, StabilizationBeta, 0.0);

// Enable gravity
SET_BOOL_PROP(Stokes2cniTestProblem, EnableGravity, true);
}

/*!
 * \ingroup BoxStokes2cniModel
 * \ingroup BoxTestProblems
 * \brief Stokes2cni problem with air (N2) flowing
 *        from the left to the right.
 *
 * The domain is sized 1m times 1m. The boundary conditions for the momentum balances
 * are all set to Dirichlet. The mass balance has outflow boundary conditions, which are
 * replaced in the localresidual by the sum
 * of the two momentum balances. In the middle of the right boundary,
 * one vertex obtains Dirichlet bcs to fix the pressure at one point.
 *
 * This problem uses the \ref BoxStokes2cniModel.
 *
 * This problem is non-stationary and can be simulated until \f$t_{\text{end}} =
 * 100\;s\f$ is reached. A good choice for the initial time step size
 * is \f$t_{\text{inital}} = 1\;s\f$.
 * To run the simulation execute the following line in shell:
 * <tt>./test_stokes2cni  -parameterFile ./test_stokes2cni.input</tt>
 */
template <class TypeTag>
class Stokes2cniTestProblem 
    : public GET_PROP_TYPE(TypeTag, BaseProblem)
{
    typedef typename GET_PROP_TYPE(TypeTag, BaseProblem) ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, TimeManager) TimeManager;
    typedef typename GET_PROP_TYPE(TypeTag, Stokes2cniIndices) Indices;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, RateVector) RateVector;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;

    enum { // Number of equations and grid dimension
        numEq = GET_PROP_VALUE(TypeTag, NumEq),
        dimWorld = GridView::dimensionworld
    };
    enum { // copy some indices for convenience
        massBalanceIdx = Indices::massBalanceIdx,
        momentum0Idx = Indices::momentum0Idx,
        transportIdx = Indices::transportIdx,
        energyIdx = Indices::energyIdx,

        pressureIdx = Indices::pressureIdx,
        velocity0Idx = Indices::velocity0Idx,
        massFracIdx = Indices::massFracIdx,
        temperatureIdx = Indices::temperatureIdx
    };

    typedef typename GridView::ctype CoordScalar;
    typedef Dune::FieldVector<CoordScalar, dimWorld> GlobalPosition;

public:
    Stokes2cniTestProblem(TimeManager &timeManager)
        : ParentType(timeManager, GET_PROP_TYPE(TypeTag, GridCreator)::grid().leafView())
    {
        eps_ = 1e-6;

        // initialize the tables of the fluid system
        FluidSystem::init();
    }

    /*!
     * \name Problem parameters
     */
    // \{

    /*!
     * \brief The problem name.
     *
     * This is used as a prefix for files generated by the simulation.
     */
    const char *name() const
    { return "stokes2cni"; }

    // \}

    /*!
     * \name Boundary conditions
     */
    // \{

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment.
     *
     * \param values The boundary types for the conservation equations
     * \param vertex The vertex on the boundary for which the
     *               conditions needs to be specified
     */
    template <class Context>
    void boundaryTypes(BoundaryTypes &values, 
                       const Context &context,
                       int spaceIdx, int timeIdx) const
    {
        const GlobalPosition globalPos = context.pos(spaceIdx, timeIdx);

        values.setAllDirichlet();

        // the mass balance has to be of type outflow
        values.setOutflow(massBalanceIdx);

        if (onUpperBoundary_(globalPos) &&
            !onLeftBoundary_(globalPos) && 
            !onRightBoundary_(globalPos))
        {
            values.setAllOutflow();
        }

        // set pressure at one point
        if (onUpperBoundary_(globalPos) &&
            !onLeftBoundary_(globalPos) && 
            !onRightBoundary_(globalPos))
        {
            values.setDirichlet(massBalanceIdx);
        }
    }

    /*!
     * \brief Evaluate the boundary conditions for a dirichlet
     *        control volume.
     *
     * \param values The dirichlet values for the primary variables
     * \param vertex The vertex representing the "half volume on the boundary"
     *
     * For this method, the \a values parameter stores primary variables.
     */
    template <class Context>
    void dirichlet(PrimaryVariables &values,
                   const Context &context,
                   int spaceIdx, int timeIdx) const
    { initial(values, context, spaceIdx, timeIdx); }

    /*!
     * \brief Evaluate the boundary conditions for a neumann
     *        boundary segment.
     *
     * For this method, the \a values parameter stores the mass flux
     * in normal direction of each phase. Negative values mean influx.
     */
    template <class Context>
    void neumann(RateVector &values,
                 const Context &context,
                 int spaceIdx, int timeIdx) const
    { values = 0.0; }

    // \}

    /*!
     * \name Volume terms
     */
    // \{

    /*!
     * \brief Evaluate the source term for all phases within a given
     *        sub-control-volume.
     *
     * For this method, the \a values parameter stores the rate mass
     * generated or annihilate per volume unit. Positive values mean
     * that mass is created, negative ones mean that it vanishes.
     */
    template <class Context>
    void source(RateVector &values,
                const Context &context,
                int spaceIdx, int timeIdx) const
    {
        // ATTENTION: The source term of the mass balance has to be chosen as
        // div (q_momentum) in the problem file
        values = Scalar(0.0);
    }

    /*!
     * \brief Evaluate the initial value for a control volume.
     *
     * For this method, the \a values parameter stores primary
     * variables.
     */
    template <class Context>
    void initial(PrimaryVariables &values,
                 const Context &context,
                 int spaceIdx, int timeIdx) const
    {
        const GlobalPosition &globalPos = context.pos(spaceIdx, timeIdx);

        const Scalar v1 = 0.5;
        values[velocity0Idx + 0] = 0.0;
        values[velocity0Idx + 1] = v1*(globalPos[0] - this->bboxMin()[0])*(this->bboxMax()[0] - globalPos[0])
                                   / (0.25*(this->bboxMax()[0] - this->bboxMin()[0])*(this->bboxMax()[0] - this->bboxMin()[0]));
        values[pressureIdx] = 1e5 - 1.189*this->gravity()[1]*globalPos[1];
        values[massFracIdx] = 1e-4;
        values[temperatureIdx] = 283.15;

        if (globalPos[0]<0.75 && globalPos[0]>0.25 &&
            globalPos[1]<0.75 && globalPos[1]>0.25)
        {
            values[massFracIdx] = 0.9e-4;
            values[temperatureIdx] = 284.15;
        }
    }

    // \}

private:
    bool onLeftBoundary_(const GlobalPosition &globalPos) const
    { return globalPos[0] < this->bboxMin()[0] + eps_; }

    bool onRightBoundary_(const GlobalPosition &globalPos) const
    { return globalPos[0] > this->bboxMax()[0] - eps_; }

    bool onLowerBoundary_(const GlobalPosition &globalPos) const
    { return globalPos[1] < this->bboxMin()[1] + eps_; }

    bool onUpperBoundary_(const GlobalPosition &globalPos) const
    { return globalPos[1] > this->bboxMax()[1] - eps_; }

    bool onBoundary_(const GlobalPosition &globalPos) const
    {
        return onLeftBoundary_(globalPos)
            || onRightBoundary_(globalPos)
            || onLowerBoundary_(globalPos)
            || onUpperBoundary_(globalPos);
    }

    Scalar eps_;
};
} //end namespace

#endif