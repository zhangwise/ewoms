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
 * \copydoc Ewoms::BlackOilRateVector
 */
#ifndef EWOMS_BLACK_OIL_RATE_VECTOR_HH
#define EWOMS_BLACK_OIL_RATE_VECTOR_HH

#include <dune/common/fvector.hh>

#include <opm/material/common/Valgrind.hpp>
#include <opm/material/constraintsolvers/NcpFlash.hpp>

#include "blackoilintensivequantities.hh"

namespace Ewoms {

/*!
 * \ingroup BlackOilModel
 *
 * \brief Implements a vector representing mass, molar or volumetric rates for
 *        the black oil model.
 *
 * This class is basically a Dune::FieldVector which can be set using
 * either mass, molar or volumetric rates.
 */
template <class TypeTag>
class BlackOilRateVector
    : public Dune::FieldVector<typename GET_PROP_TYPE(TypeTag, Evaluation),
                               GET_PROP_VALUE(TypeTag, NumEq)>
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Evaluation) Evaluation;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    enum { numEq = GET_PROP_VALUE(TypeTag, NumEq) };
    enum { numComponents = GET_PROP_VALUE(TypeTag, NumComponents) };
    enum { conti0EqIdx = Indices::conti0EqIdx };

    typedef Opm::MathToolbox<Evaluation> Toolbox;
    typedef Dune::FieldVector<Evaluation, numEq> ParentType;

public:
    BlackOilRateVector() : ParentType()
    { Valgrind::SetUndefined(*this); }

    /*!
     * \copydoc ImmiscibleRateVector::ImmiscibleRateVector(Scalar)
     */
    BlackOilRateVector(Scalar value) : ParentType(Toolbox::createConstant(value))
    {}

    template <class Eval = Evaluation>
    BlackOilRateVector(const typename std::enable_if<std::is_same<Eval, Evaluation>::value, Evaluation>::type& value) : ParentType(value)
    {}

    /*!
     * \copydoc ImmiscibleRateVector::ImmiscibleRateVector(const
     * ImmiscibleRateVector &)
     */
    BlackOilRateVector(const BlackOilRateVector &value) : ParentType(value)
    {}

    /*!
     * \copydoc ImmiscibleRateVector::setMassRate
     */
    void setMassRate(const ParentType &value)
    { ParentType::operator=(value); }

    /*!
     * \copydoc ImmiscibleRateVector::setMolarRate
     */
    void setMolarRate(const ParentType &value)
    {
        // first, assign molar rates
        ParentType::operator=(value);

        // then, convert them to mass rates
        for (int compIdx = 0; compIdx < numComponents; ++compIdx)
            (*this)[conti0EqIdx + compIdx] *= FluidSystem::molarMass(compIdx);
    }

    /*!
     * \copydoc ImmiscibleRateVector::setVolumetricRate
     */
    template <class FluidState, class RhsEval>
    void setVolumetricRate(const FluidState &fluidState,
                           int phaseIdx,
                           const RhsEval& volume)
    {
        for (int compIdx = 0; compIdx < numComponents; ++compIdx)
            (*this)[conti0EqIdx + compIdx] =
                fluidState.density(phaseIdx)
                * fluidState.massFraction(phaseIdx, compIdx)
                * volume;
    }

    /*!
     * \brief Assignment operator from a scalar or a function evaluation
     */
    template <class RhsEval>
    BlackOilRateVector& operator=(const RhsEval& value)
    {
        for (unsigned i=0; i < this->size(); ++i)
            (*this)[i] = value;
        return *this;
    }

    /*!
     * \brief Assignment operator from another rate vector
     */
    BlackOilRateVector& operator=(const BlackOilRateVector& other)
    {
        for (unsigned i=0; i < this->size(); ++i)
            (*this)[i] = other[i];
        return *this;
    }
};

} // namespace Ewoms

#endif
