/*****************************************************************************
*   Copyright (C) 2007-2008 by Markus Wolff                                  *
*   Institute of Hydraulic Engineering                                       *
*   University of Stuttgart, Germany                                         *
*   email: <givenname>.<name>@iws.uni-stuttgart.de                           *
*                                                                            *
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
 * \brief Dumux-equivalent for GRUWA (1p-stationary, finite volumes)
 */
#ifndef DUMUX_GROUNDWATER_PROBLEM_HH
#define DUMUX_GROUNDWATER_PROBLEM_HH

#if HAVE_UG
#include <dune/grid/uggrid.hh>
#endif

#include <dune/grid/yaspgrid.hh>
#include <dune/grid/sgrid.hh>

#include <dumux/material/fluidsystems/liquidphase.hh>
#include "pseudoh2o.hh"

#include <dumux/decoupled/1p/diffusion/diffusionproblem1p.hh>
#include <dumux/decoupled/1p/diffusion/fv/fvvelocity1p.hh>

#include "groundwater_spatialparams.hh"

namespace Dumux
{

struct Source
{
	Dune::FieldVector<double,2> globalPos;
	double q;
	int index;
};

struct BoundarySegment
{
	double from, to;
	bool neumann;
	double value;
};

template<class TypeTag>
class GroundwaterProblem;

//////////
// Specify the properties
//////////
namespace Properties
{
NEW_TYPE_TAG(GroundwaterProblem, INHERITS_FROM(DecoupledOneP))
        ;

// Set the grid type
SET_PROP(GroundwaterProblem, Grid)
{//    typedef Dune::YaspGrid<2> type;
    typedef Dune::SGrid<2, 2> type;
};

// Set the wetting phase
SET_PROP(GroundwaterProblem, Fluid)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Scalar)) Scalar;
public:
    typedef Dumux::LiquidPhase<Scalar, Dumux::PseudoH2O<Scalar> > type;
};

// Set the spatial parameters
SET_PROP(GroundwaterProblem, SpatialParameters)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Grid)) Grid;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Scalar)) Scalar;

public:
    typedef Dumux::GroundwaterSpatialParams<TypeTag> type;
};

// Enable gravity
SET_BOOL_PROP(GroundwaterProblem, EnableGravity, false);

// Set the model
SET_TYPE_PROP(GroundwaterProblem, Model, Dumux::FVVelocity1P<TypeTag>);

//Set the problem
SET_TYPE_PROP(GroundwaterProblem, Problem, Dumux::GroundwaterProblem<TTAG(GroundwaterProblem)>);

}

/*!
 * \ingroup IMPETtests
 *
 * \brief test problem for the decoupled one-phase model.
 */
template<class TypeTag = TTAG(GroundwaterProblem)>
class GroundwaterProblem: public DiffusionProblem1P<TypeTag>
{
    typedef DiffusionProblem1P<TypeTag> ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(GridView)) GridView;

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Fluid)) Fluid;

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(PrimaryVariables)) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(BoundaryTypes)) BoundaryTypes;

    enum
    {
        dim = GridView::dimension, dimWorld = GridView::dimensionworld
    };

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Scalar)) Scalar;

    typedef typename GridView::Traits::template Codim<0>::Entity Element;
    typedef typename GridView::Intersection Intersection;
    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GET_PROP(TypeTag, PTAG(ParameterTree)) Params;

public:
    GroundwaterProblem(const GridView &gridView) :
        ParentType(gridView)
    {
//        this->spatialParameters().setDelta(delta_);
    	this->spatialParameters().setParameters();

    	// Write input parameters into private variables
        Dune::FieldVector<int,2> resolution = Params::tree().template get<Dune::FieldVector<int,2> >("Geometry.numberOfCells");
        GlobalPosition size = Params::tree().template get<GlobalPosition>("Geometry.domainSize");

        // Read sources
    	std::vector<double> sources = Params::tree().template get<std::vector<double> >("Source.sources");
		int NumberOfSources = std::trunc(sources.size()/3);

		for (int sourceCount=0; sourceCount<NumberOfSources ; sourceCount++)
		{
			Source tempSource;
			tempSource.globalPos[0]=sources[sourceCount*3];
			tempSource.globalPos[1]=sources[sourceCount*3+1];
			tempSource.q=sources[sourceCount*3+2];

			tempSource.index = std::floor(tempSource.globalPos[0]
			   * resolution[0]/size[0])
			   + std::floor(tempSource.globalPos[1]*resolution[1]/size[1])
			   * resolution[0];
			sources_.push_back(tempSource);
		}

		// Read Boundary Conditions
    	std::vector<double> BC = Params::tree().template get<std::vector<double> >("BoundaryConditions.left");
		int NumberOfSegments = std::trunc(BC.size()/4);
		for (int segmentCount=0; segmentCount<NumberOfSegments ; segmentCount++)
		{
			BoundarySegment tempSegment;
			tempSegment.from = BC[segmentCount*4];
			tempSegment.to = BC[segmentCount*4+1];
			tempSegment.neumann = (BC[segmentCount*4+2]!=0);
			tempSegment.value = BC[segmentCount*4+3];
			boundaryConditions_[2].push_back(tempSegment);
		}
    	BC = Params::tree().template get<std::vector<double> >("BoundaryConditions.right");
		NumberOfSegments = std::trunc(BC.size()/4);
		for (int segmentCount=0; segmentCount<NumberOfSegments ; segmentCount++)
		{
			BoundarySegment tempSegment;
			tempSegment.from = BC[segmentCount*4];
			tempSegment.to = BC[segmentCount*4+1];
			tempSegment.neumann = (BC[segmentCount*4+2]!=0);
			tempSegment.value = BC[segmentCount*4+3];
			boundaryConditions_[3].push_back(tempSegment);
		}
    	BC = Params::tree().template get<std::vector<double> >("BoundaryConditions.bottom");
		NumberOfSegments = std::trunc(BC.size()/4);
		for (int segmentCount=0; segmentCount<NumberOfSegments ; segmentCount++)
		{
			BoundarySegment tempSegment;
			tempSegment.from = BC[segmentCount*4];
			tempSegment.to = BC[segmentCount*4+1];
			tempSegment.neumann = (BC[segmentCount*4+2]!=0);
			tempSegment.value = BC[segmentCount*4+3];
			boundaryConditions_[1].push_back(tempSegment);
		}
    	BC = Params::tree().template get<std::vector<double> >("BoundaryConditions.top");
		NumberOfSegments = std::trunc(BC.size()/4);
		for (int segmentCount=0; segmentCount<NumberOfSegments ; segmentCount++)
		{
			BoundarySegment tempSegment;
			tempSegment.from = BC[segmentCount*4];
			tempSegment.to = BC[segmentCount*4+1];
			tempSegment.neumann = (BC[segmentCount*4+2]!=0);
			tempSegment.value = BC[segmentCount*4+3];
			boundaryConditions_[0].push_back(tempSegment);
		}

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
    {
        return "groundwater";
    }

    bool shouldWriteRestartFile() const
    { return false; }

    /*!
    * \brief Returns the temperature within the domain.
    *
    * This problem assumes a temperature of 10 degrees Celsius.
    */
    Scalar temperature(const Element& element) const
    {
        return 273.15 + 10; // -> 10°C
    }

    // \}

    //! Returns the reference pressure for evaluation of constitutive relations
    Scalar referencePressure(const Element& element) const
    {
        return 1e5; // -> 10°C
    }

    //!source term [kg/(m^3 s)] (or in 2D [kg/(m^2 s)]).
    void source(PrimaryVariables& values, const Element& element) const
    {
        values = 0;
        Scalar density=Fluid::density(0,0);
   		Scalar depth = Params::tree().template get<double>("Geometry.depth");
		for (int sourceCount = 0; sourceCount != sources_.size(); sourceCount++)
        {
			if (this->variables().index(element) == sources_[sourceCount].index)
				values+=sources_[sourceCount].q*density/element.geometry().volume()/depth;
        }
    }

    /*!
    * \brief Returns the type of boundary condition.
    *
    * BC can be dirichlet (pressure) or neumann (flux).
    */
    void boundaryTypesAtPos(BoundaryTypes &bcType,
            const GlobalPosition& globalPos) const
    {
       	GlobalPosition size = Params::tree().template get<GlobalPosition>("Geometry.domainSize");
        double coordinate=0;
        int boundaryIndex=0;
        if (globalPos[0]<0.0001)
        {
            //Left boundary
            coordinate=globalPos[1];
            boundaryIndex=2;
        }
        if (globalPos[1]<0.0001)
        {
            //Bottom boundary
            coordinate=globalPos[0];
            boundaryIndex=1;
        }
        if (globalPos[0]> size[0] -0.0001)
        {
            //Right boundary
            coordinate=globalPos[1];
            boundaryIndex=3;
        }
        if (globalPos[1]> size[1] -0.0001)
        {
            //Top boundary
            coordinate=globalPos[0];
            boundaryIndex=0;
        }

    	for (int segmentCount=0; segmentCount<boundaryConditions_[boundaryIndex].size();segmentCount++)
        {
    		if ((boundaryConditions_[boundaryIndex][segmentCount].from < coordinate) &&
    				(coordinate < boundaryConditions_[boundaryIndex][segmentCount].to))
    		{
    			if (boundaryConditions_[boundaryIndex][segmentCount].neumann)
                    bcType.setAllNeumann();
                else
                    bcType.setAllDirichlet();
                return;
            }
        }
        bcType.setAllNeumann();
    }


    //! return dirichlet condition  (pressure, [Pa])
    void dirichletAtPos(PrimaryVariables &values,
                        const GlobalPosition &globalPos) const
    {
      	GlobalPosition size = Params::tree().template get<GlobalPosition>("Geometry.domainSize");
        double coordinate=0;
        int boundaryIndex=0;
        if (globalPos[0]<0.0001)
        {
            //Left boundary
            coordinate=globalPos[1];
            boundaryIndex=2;
        }
        if (globalPos[1]<0.0001)
        {
            //Bottom boundary
            coordinate=globalPos[0];
            boundaryIndex=1;
        }
        if (globalPos[0]> size[0] -0.0001)
        {
            //Right boundary
            coordinate=globalPos[1];
            boundaryIndex=3;
        }
        if (globalPos[1]> size[1] -0.0001)
        {
            //Top boundary
            coordinate=globalPos[0];
            boundaryIndex=0;
        }

    	for (int segmentCount=0; segmentCount<boundaryConditions_[boundaryIndex].size();segmentCount++)
    	{
    		if ((boundaryConditions_[boundaryIndex][segmentCount].from < coordinate) &&
    				(coordinate < boundaryConditions_[boundaryIndex][segmentCount].to))
    		{
                values = boundaryConditions_[boundaryIndex][segmentCount].value*Fluid::density(0,0)*9.81;
                return;
            }
        }
        values = 0;
    }

    //! return neumann condition  (flux, [kg/(m^2 s)])
    void neumannAtPos(PrimaryVariables &values, const GlobalPosition& globalPos) const
    {
    	GlobalPosition size = Params::tree().template get<GlobalPosition>("Geometry.domainSize");
        double coordinate=0;
        int boundaryIndex=0;
        if (globalPos[0]<0.0001)
        {
            //Left boundary
            coordinate=globalPos[1];
            boundaryIndex=2;
        }
        if (globalPos[1]<0.0001)
        {
            //Bottom boundary
            coordinate=globalPos[0];
            boundaryIndex=1;
        }
        if (globalPos[0]> size[0] -0.0001)
        {
            //Right boundary
            coordinate=globalPos[1];
            boundaryIndex=3;
        }
        if (globalPos[1]> size[1] -0.0001)
        {
            //Top boundary
            coordinate=globalPos[0];
            boundaryIndex=0;
        }
        
    	for (int segmentCount=0; segmentCount<boundaryConditions_[boundaryIndex].size();segmentCount++)
    	{
    		if ((boundaryConditions_[boundaryIndex][segmentCount].from < coordinate) &&
    				(coordinate < boundaryConditions_[boundaryIndex][segmentCount].to))
    		{
                values = boundaryConditions_[boundaryIndex][segmentCount].value*Fluid::density(0,0)*(-1);
                return;
            }
        }
        values = 0;
    }

    void writeOutput()
    { //Achtung: Quick und dirty:
//    	switch(problemProps_.plotMode)
//    	{
//    	case 0: //grid-plot (colors only)
//		{
        Dune::FieldVector<int,2> resolution = Params::tree().template get<Dune::FieldVector<int,2> >("Geometry.numberOfCells");
        GlobalPosition size = Params::tree().template get<GlobalPosition>("Geometry.domainSize");
	
	Scalar zmax, zmin;
	zmax=this->variables().pressure()[0]/(Fluid::density(0,0)*9.81);
	zmin=this->variables().pressure()[0]/(Fluid::density(0,0)*9.81);

	for (int i=0; i< resolution[0]*resolution[1]; i++)
	{
		Scalar currentHead= this->variables().pressure()[i]/(Fluid::density(0,0)*9.81);
		zmax = std::max(currentHead,zmax);
		zmin = std::min(currentHead,zmin);			
	}

	std::ofstream dataFile;
	dataFile.open("dumux-out.vgfc");
	dataFile << "Gridplot" << std::endl;
	dataFile << "## This is an DuMuX output for the NumLab Grafics driver. \n";
	dataFile << "## This output file was generated at " << __TIME__ <<", "<< __DATE__<< "\n";
	// the following specifies necessary information for the numLab grafical ouptut
	dataFile << "# x-range 0 "<< size[0] << "\n" ;
	dataFile << "# y-range 0 "<< size[1] << "\n" ;
	dataFile << "# x-count " << resolution[0] << "\n" ;
	dataFile << "# y-count " << resolution[1] << "\n" ;
	if ((zmax-zmin)/zmax>0.01)
		dataFile << "# scale 1 1 "<< sqrt(size[0]*size[1])/(zmax-zmin) << "\n";
	else
		dataFile << "# scale 1 1 1\n";

	dataFile << "# min-color 255 0 0\n";
	dataFile << "# max-color 0 0 255\n";
	dataFile << "# time 0 \n" ;
	dataFile << "# label piezometric head \n";


//			for (int i=problemProps_.resolution[1]-1; i>=0; i--)
	for (int i=0; i< resolution[1]; i++)
	{
		for (int j=0; j<resolution[0]; j++)
		{
			int currentIdx = i*resolution[0]+j;
			dataFile << this->variables().pressure()[currentIdx]/(Fluid::density(0,0)*9.81);
			if(j != resolution[0]-1) // all but last entry
				dataFile << " ";
			else // write the last entry
				dataFile << "\n";
		}
	}
	dataFile.close();
//		}
//		break;
//		case 1: //Piecewise constant with 3d-plotter
//		{
//    		std::ofstream dataFile;
//    		dataFile.open("output.dat");
//
//    	    ElementIterator eItEnd = this->gridView().template end<0> ();
//    	    for (ElementIterator eIt = this->gridView().template begin<0> (); eIt != eItEnd; ++eIt)
//    	    {
//    	    	//Aktuell gibts noch probleme mit der Skalierung.
//    	    	int cellIndex = this->variables().index(*eIt);
//    	        dataFile << eIt->geometry().corner(0)[0] << " "
//    	                 << -1*eIt->geometry().corner(0)[1] << " "
//    	                 << this->variables().pressure()[cellIndex]/(Fluid::density(0,0)*9.81)<< " "
//    	        		 << eIt->geometry().corner(1)[0] << " "
//    	                 << -1*eIt->geometry().corner(1)[1] << " "
//    	                 << this->variables().pressure()[cellIndex]/(Fluid::density(0,0)*9.81)<< " "
//    	        		 << eIt->geometry().corner(2)[0] << " "
//    	                 << -1*eIt->geometry().corner(2)[1] << " "
//    	                 << this->variables().pressure()[cellIndex]/(Fluid::density(0,0)*9.81)<<std::endl;
//
//    	        dataFile << eIt->geometry().corner(3)[0] << " "
//    	                 << -1*eIt->geometry().corner(3)[1] << " "
//    	                 << this->variables().pressure()[cellIndex]/(Fluid::density(0,0)*9.81)<< " "
//    	        		 << eIt->geometry().corner(1)[0] << " "
//    	                 << -1*eIt->geometry().corner(1)[1] << " "
//    	                 << this->variables().pressure()[cellIndex]/(Fluid::density(0,0)*9.81)<< " "
//    	        		 << eIt->geometry().corner(2)[0] << " "
//    	                 << -1*eIt->geometry().corner(2)[1] << " "
//    	                 << this->variables().pressure()[cellIndex]/(Fluid::density(0,0)*9.81)<<std::endl;
//    	    }
//		}
//		break;
//		case 2: //Interpolate between cell centers
//		{
//    		std::ofstream dataFile;
//    		dataFile.open("interpolate.dat");
//
//			for (int i=0; i< resolution[1]-1; i++)
//			{
//				for (int j=0; j< resolution[0]-1; j++)
//				{
//					int currentIdx = i*resolution[0]+j;
//					double dx=size[0]/resolution[0];
//					double dy=size[1]/resolution[1];
//
//	    	        dataFile << dx*(j+0.5) << " "
//	    	                 << -dy*(i+0.5) << " "
//	    	                 << this->variables().pressure()[currentIdx]/(Fluid::density(0,0)*9.81)<< " "
//	    	        		 << dx*(j+1.5) << " "
//	    	                 << -dy*(i+0.5) << " "
//	    	                 << this->variables().pressure()[currentIdx+1]/(Fluid::density(0,0)*9.81)<< " "
//	    	        		 << dx*(j+1.5) << " "
//	    	                 << -dy*(i+1.5) << " "
//	    	                 << this->variables().pressure()[currentIdx+1+resolution[0]]/(Fluid::density(0,0)*9.81)<<std::endl;
//
//	    	        dataFile << dx*(j+0.5) << " "
//	    	                 << -dy*(i+0.5) << " "
//	    	                 << this->variables().pressure()[currentIdx]/(Fluid::density(0,0)*9.81)<< " "
//	    	        		 << dx*(j+0.5) << " "
//	    	                 << -dy*(i+1.5) << " "
//	    	                 << this->variables().pressure()[currentIdx+resolution[0]]/(Fluid::density(0,0)*9.81)<< " "
//	    	        		 << dx*(j+1.5) << " "
//	    	                 << -dy*(i+1.5) << " "
//	    	                 << this->variables().pressure()[currentIdx+1+resolution[0]]/(Fluid::density(0,0)*9.81)<<std::endl;
//				}
//			}
//			dataFile.close();
//		}
//		break;
//		default:
//		{
	    	//Textoutput:
	std::cout << "         x          y          h           v_x           v_y"<<std::endl;
	std::cout << "------------------------------------------------------------"<<std::endl;

    	ElementIterator eItEnd = this->gridView().template end<0> ();
	for (ElementIterator eIt = this->gridView().template begin<0> (); eIt != eItEnd; ++eIt)
	{
		int cellIndex = this->variables().index(*eIt);
	    	double v_x,v_y,piezo,x,y;
	    	v_x= (this->variables().velocity()[cellIndex][0][0]+this->variables().velocity()[cellIndex][1][0])/2;
	    	v_y= (this->variables().velocity()[cellIndex][2][1]+this->variables().velocity()[cellIndex][3][1])/2;

	    	if (std::abs(v_x)<1e-17)
	    		v_x=0;
	    	if (std::abs(v_y)<1e-17)
	    		v_y=0;
	    	piezo=this->variables().pressure()[cellIndex]/(Fluid::density(0,0)*9.81);
	    	x = eIt->geometry().center()[0];
	    	y = eIt->geometry().center()[1];

	    	printf("%10.4g %10.4g %10.4g %13.4g %13.4g\n",x,y,piezo,v_x,v_y);
//		        std::cout << std::setw(10)<< eIt->geometry().center()[0]<<" "
//		        		<< std::setw(10)<< eIt->geometry().center()[1]<<" "
//		        		<< std::setw(10) <<std::setprecision(6) << piezo<<" "
//		        		<< std::setw(11)<< std::setprecision(6) << v_x<<" "
//		        		<< std::setw(11)<< std::setprecision(6) << v_y <<std::endl;
	}
//		}
//    	}
    }

private:
    Scalar exact (const GlobalPosition& globalPos) const
    {
        double pi = 4.0*atan(1.0);

        return (sin(pi*globalPos[0])*sin(pi*globalPos[1]));
    }

    Dune::FieldVector<Scalar,dim> exactGrad (const GlobalPosition& globalPos) const
        {
        Dune::FieldVector<Scalar,dim> grad(0);
        double pi = 4.0*atan(1.0);
        grad[0] = pi*cos(pi*globalPos[0])*sin(pi*globalPos[1]);
        grad[1] = pi*cos(pi*globalPos[1])*sin(pi*globalPos[0]);

        return grad;
        }

    std::vector<Source> sources_;
    std::vector<BoundarySegment> boundaryConditions_[4];
};
} //end namespace

#endif
