//-----------------------------------------------------------------------bl-
//--------------------------------------------------------------------------
// 
// GRINS - General Reacting Incompressible Navier-Stokes 
//
// Copyright (C) 2010-2013 The PECOS Development Team
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the Version 2.1 GNU Lesser General
// Public License as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc. 51 Franklin Street, Fifth Floor,
// Boston, MA  02110-1301  USA
//
//-----------------------------------------------------------------------el-


#ifndef GRINS_REACTING_LOW_MACH_NAVIER_STOKES_H
#define GRINS_REACTING_LOW_MACH_NAVIER_STOKES_H

// GRINS
#include "grins/reacting_low_mach_navier_stokes_base.h"

namespace GRINS
{
  template<typename Mixture, typename Evaluator>
  class ReactingLowMachNavierStokes : public ReactingLowMachNavierStokesBase
  {
  public:

    ReactingLowMachNavierStokes(const PhysicsName& physics_name, const GetPot& input);
    ~ReactingLowMachNavierStokes();
    
    //! Read options from GetPot input file.
    virtual void read_input_options( const GetPot& input );

    // Context initialization
    virtual void init_context( AssemblyContext& context );

    // Time dependent part(s)
    virtual void element_time_derivative( bool compute_jacobian,
					  AssemblyContext& context,
					  CachedValues& cache );

    virtual void side_time_derivative( bool compute_jacobian,
				       AssemblyContext& context,
				       CachedValues& cache );

    // Mass matrix part(s)
    virtual void mass_residual( bool compute_jacobian,
				AssemblyContext& context,
				CachedValues& cache );

    virtual void compute_element_time_derivative_cache( const libMesh::FEMContext& context, 
							CachedValues& cache );

    virtual void compute_side_time_derivative_cache( const libMesh::FEMContext& context, 
						     CachedValues& cache );

    virtual void compute_element_cache( const libMesh::FEMContext& context,
					const std::vector<libMesh::Point>& points,
					CachedValues& cache );

    const Mixture& gas_mixture() const;

    virtual libMesh::Real cp_mix( const libMesh::Real T,
                                  const std::vector<libMesh::Real>& Y );

    virtual libMesh::Real mu( const libMesh::Real T,
                              const std::vector<libMesh::Real>& Y );

    virtual libMesh::Real k( const libMesh::Real T,
                             const std::vector<libMesh::Real>& Y );

    virtual void D( const libMesh::Real rho, const libMesh::Real cp,
                    const libMesh::Real k,
                    std::vector<libMesh::Real>& D );

  protected:

    void assemble_mass_time_deriv(AssemblyContext& c, 
				  unsigned int qp,
				  const CachedValues& cache);

    void assemble_species_time_deriv(AssemblyContext& c, 
				     unsigned int qp,
				     const CachedValues& cache);

    void assemble_momentum_time_deriv(AssemblyContext& c, 
				      unsigned int qp,
				      const CachedValues& cache);

    void assemble_energy_time_deriv(AssemblyContext& c, 
				    unsigned int qp,
				    const CachedValues& cache);

    Mixture _gas_mixture;

    //! Enable pressure pinning
    bool _pin_pressure;
    
    PressurePinning _p_pinning;

  private:

    ReactingLowMachNavierStokes();

  };

  template<typename Mixture, typename Evaluator>
  inline
  const Mixture& ReactingLowMachNavierStokes<Mixture,Evaluator>::gas_mixture() const
  {
    return _gas_mixture;
  }

} // namespace GRINS

#endif //GRINS_REACTING_LOW_MACH_NAVIER_STOKES_H
