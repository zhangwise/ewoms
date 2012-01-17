// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2008 by Klaus Mosthaf, Andreas Lauser, Bernd Flemisch     *
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
/*!
 * \ingroup Properties
 * \ingroup BoxProperties
 * \ingroup TwoPTwoCModel
 * \file
 *
 * \brief Defines default values for most properties required by the
 *        2p2c box model.
 */
#ifndef DUMUX_2P2C_PROPERTY_DEFAULTS_HH
#define DUMUX_2P2C_PROPERTY_DEFAULTS_HH

#include "2p2cindices.hh"

#include "2p2cmodel.hh"
#include "2p2cproblem.hh"
#include "2p2cindices.hh"
#include "2p2cproperties.hh"
#include "2p2cnewtoncontroller.hh"

#include "2p2cprimaryvariables.hh"
#include "2p2cratevector.hh"
#include "2p2cvolumevariables.hh"
#include "2p2cfluxvariables.hh"

#include <dumux/material/heatconduction/dummyheatconductionlaw.hh>

namespace Dumux
{

namespace Properties {
//////////////////////////////////////////////////////////////////
// Property values
//////////////////////////////////////////////////////////////////

/*!
 * \brief Set the property for the number of components.
 *
 * We just forward the number from the fluid system and use an static
 * assert to make sure it is 2.
 */
SET_PROP(BoxTwoPTwoC, NumComponents)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;

public:
    static const int value = FluidSystem::numComponents;

    static_assert(value == 2,
                  "Only fluid systems with 2 components are supported by the 2p-2c model!");
};

/*!
 * \brief Set the property for the number of fluid phases.
 *
 * We just forward the number from the fluid system and use an static
 * assert to make sure it is 2.
 */
SET_PROP(BoxTwoPTwoC, NumPhases)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;

public:
    static const int value = FluidSystem::numPhases;
    static_assert(value == 2,
                  "Only fluid systems with 2 phases are supported by the 2p-2c model!");
};

SET_INT_PROP(BoxTwoPTwoC, NumEq, 2); //!< set the number of equations to 2

//! Set the default formulation to pl-Sg
SET_INT_PROP(BoxTwoPTwoC,
             Formulation,
             TwoPTwoCFormulation::plSg);

/*!
 * \brief Set the property for the material parameters by extracting
 *        it from the material law.
 */
SET_TYPE_PROP(BoxTwoPTwoC,
              MaterialLawParams, 
              typename GET_PROP_TYPE(TypeTag, MaterialLaw)::Params);

//! set the heat conduction law to a dummy one by default
SET_TYPE_PROP(BoxTwoPTwoC,
              HeatConductionLaw,
              Dumux::DummyHeatConductionLaw<typename GET_PROP_TYPE(TypeTag, Scalar)>);

//! extract the type parameter objects for the heat conduction law
//! from the law itself
SET_TYPE_PROP(BoxTwoPTwoC,
              HeatConductionLawParams,
              typename GET_PROP_TYPE(TypeTag, HeatConductionLaw)::Params);

//! Use the 2p2c local jacobian operator for the 2p2c model
SET_TYPE_PROP(BoxTwoPTwoC,
              LocalResidual,
              TwoPTwoCLocalResidual<TypeTag>);

//! Use the 2p2c specific newton controller for the 2p2c model
SET_TYPE_PROP(BoxTwoPTwoC, NewtonController, TwoPTwoCNewtonController<TypeTag>);

//! the Model property
SET_TYPE_PROP(BoxTwoPTwoC, Model, TwoPTwoCModel<TypeTag>);

//! The type of the base base class for actual problems
SET_TYPE_PROP(BoxTwoPTwoC, BaseProblem, TwoPTwoCProblem<TypeTag>);

//! the PrimaryVariables property
SET_TYPE_PROP(BoxTwoPTwoC, PrimaryVariables, TwoPTwoCPrimaryVariables<TypeTag>);

//! the PrimaryVariables property
SET_TYPE_PROP(BoxTwoPTwoC, RateVector, TwoPTwoCRateVector<TypeTag>);

//! the VolumeVariables property
SET_TYPE_PROP(BoxTwoPTwoC, VolumeVariables, TwoPTwoCVolumeVariables<TypeTag>);

//! the FluxVariables property
SET_TYPE_PROP(BoxTwoPTwoC, FluxVariables, TwoPTwoCFluxVariables<TypeTag>);

//! The indices required by the isothermal 2p2c model
SET_PROP(BoxTwoPTwoC,
         TwoPTwoCIndices)
{ private:
    enum { Formulation = GET_PROP_VALUE(TypeTag, Formulation) };
public:
    typedef TwoPTwoCIndices<TypeTag, Formulation, 0> type;
};
SET_TYPE_PROP(BoxTwoPTwoC, Indices, typename GET_PROP(TypeTag, TwoPTwoCIndices));
// disable the smooth upwinding method by default
SET_BOOL_PROP(BoxTwoPTwoC, EnableSmoothUpwinding, false);
}

}

#endif
