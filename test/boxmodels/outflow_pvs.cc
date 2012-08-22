// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2009-2012 by Andreas Lauser                               *
 *   Copyright (C) 2009 by Karin Erbertseder                                 *
 *   Copyright (C) 2008 by Bernd Flemisch                                    *
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
 * \file
 *
 * \brief test for the compositional PVS box model
 */
#include "config.h"

#include "problems/outflowproblem.hh"

#include <dumux/common/start.hh>
#include <dumux/boxmodels/pvs/pvsmodel.hh>

namespace Dumux {
namespace Properties {
NEW_TYPE_TAG(OutflowProblem, INHERITS_FROM(BoxPvs, OutflowBaseProblem));

// Verbosity of the PVS model (0=silent, 1=medium, 2=chatty)
SET_INT_PROP(OutflowProblem, PvsVerbosity, 1);
}}

int main(int argc, char** argv)
{
    typedef TTAG(OutflowProblem) ProblemTypeTag;
    return Dumux::start<ProblemTypeTag>(argc, argv);
}