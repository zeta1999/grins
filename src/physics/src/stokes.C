//-----------------------------------------------------------------------bl-
//--------------------------------------------------------------------------
//
// GRINS - General Reacting Incompressible Navier-Stokes
//
// Copyright (C) 2014-2019 Paul T. Bauman, Roy H. Stogner
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


// This class
#include "grins/stokes.h"

// GRINS
#include "grins_config.h"
#include "grins/generic_ic_handler.h"
#include "grins/assembly_context.h"
#include "grins/inc_nav_stokes_macro.h"
#include "grins/multiphysics_sys.h"

// libMesh
#include "libmesh/fem_context.h"
#include "libmesh/quadrature.h"

namespace GRINS
{

  template<class Mu>
  Stokes<Mu>::Stokes(const std::string& physics_name, const GetPot& input )
    : IncompressibleNavierStokesBase<Mu>(physics_name,
                                         PhysicsNaming::stokes(), /* "core" Physics name */
                                         input),
    _p_pinning(input,physics_name),
    _pin_pressure( input("Physics/"+PhysicsNaming::stokes()+"/pin_pressure", false ) )
  {
    this->_ic_handler = new GenericICHandler( physics_name, input );
  }

  template<class Mu>
  void Stokes<Mu>::auxiliary_init( MultiphysicsSystem& system )
  {
    if( _pin_pressure )
      _p_pinning.check_pin_location(system.get_mesh());
  }

  template<class Mu>
  void Stokes<Mu>::element_time_derivative
  ( bool compute_jacobian, AssemblyContext & context )
  {
    // The number of local degrees of freedom in each variable.
    const unsigned int n_u_dofs = context.get_dof_indices(this->_flow_vars.u()).size();
    const unsigned int n_p_dofs = context.get_dof_indices(this->_press_var.p()).size();

    // Check number of dofs is same for this->_flow_vars.u(), v_var and w_var.
    libmesh_assert (n_u_dofs == context.get_dof_indices(this->_flow_vars.v()).size());

    if (this->_flow_vars.dim() == 3)
      libmesh_assert (n_u_dofs == context.get_dof_indices(this->_flow_vars.w()).size());

    // We get some references to cell-specific data that
    // will be used to assemble the linear system.

    // Element Jacobian * quadrature weights for interior integration.
    const std::vector<libMesh::Real> &JxW =
      context.get_element_fe(this->_flow_vars.u())->get_JxW();

    // The velocity shape function gradients (in global coords.)
    // at interior quadrature points.
    const std::vector<std::vector<libMesh::RealGradient> >& u_gradphi =
      context.get_element_fe(this->_flow_vars.u())->get_dphi();

    // The pressure shape functions at interior quadrature points.
    const std::vector<std::vector<libMesh::Real> >& p_phi =
      context.get_element_fe(this->_press_var.p())->get_phi();

    // The subvectors and submatrices we need to fill:
    //
    // K_{\alpha \beta} = R_{\alpha},{\beta} = \partial{ R_{\alpha} } / \partial{ {\beta} } (where R denotes residual)
    // e.g., for \alpha = v and \beta = u we get: K{vu} = R_{v},{u}
    // Note that Kpu, Kpv, Kpw and Fp comes as constraint.

    libMesh::DenseSubMatrix<libMesh::Number> &Kuu = context.get_elem_jacobian(this->_flow_vars.u(), this->_flow_vars.u()); // R_{u},{u}
    libMesh::DenseSubMatrix<libMesh::Number> &Kvv = context.get_elem_jacobian(this->_flow_vars.v(), this->_flow_vars.v()); // R_{v},{v}
    libMesh::DenseSubMatrix<libMesh::Number>* Kww = NULL;

    libMesh::DenseSubMatrix<libMesh::Number> &Kup = context.get_elem_jacobian(this->_flow_vars.u(), this->_press_var.p()); // R_{u},{p}
    libMesh::DenseSubMatrix<libMesh::Number> &Kvp = context.get_elem_jacobian(this->_flow_vars.v(), this->_press_var.p()); // R_{v},{p}
    libMesh::DenseSubMatrix<libMesh::Number>* Kwp = NULL;

    libMesh::DenseSubVector<libMesh::Number> &Fu = context.get_elem_residual(this->_flow_vars.u()); // R_{u}
    libMesh::DenseSubVector<libMesh::Number> &Fv = context.get_elem_residual(this->_flow_vars.v()); // R_{v}
    libMesh::DenseSubVector<libMesh::Number>* Fw = NULL;

    if( this->_flow_vars.dim() == 3 )
      {
        Kww = &context.get_elem_jacobian(this->_flow_vars.w(), this->_flow_vars.w()); // R_{w},{w}
        Kwp = &context.get_elem_jacobian(this->_flow_vars.w(), this->_press_var.p()); // R_{w},{p}
        Fw  = &context.get_elem_residual(this->_flow_vars.w()); // R_{w}
      }

    // Now we will build the element Jacobian and residual.
    // Constructing the residual requires the solution and its
    // gradient from the previous timestep.  This must be
    // calculated at each quadrature point by summing the
    // solution degree-of-freedom values by the appropriate
    // weight functions.
    unsigned int n_qpoints = context.get_element_qrule().n_points();

    for (unsigned int qp=0; qp != n_qpoints; qp++)
      {
        // Compute the solution & its gradient at the old Newton iterate.
        libMesh::Number p, u, v, w;
        p = context.interior_value(this->_press_var.p(), qp);
        u = context.interior_value(this->_flow_vars.u(), qp);
        v = context.interior_value(this->_flow_vars.v(), qp);
        if (this->_flow_vars.dim() == 3)
          w = context.interior_value(this->_flow_vars.w(), qp);

        libMesh::Gradient grad_u, grad_v, grad_w;
        grad_u = context.interior_gradient(this->_flow_vars.u(), qp);
        grad_v = context.interior_gradient(this->_flow_vars.v(), qp);
        if (this->_flow_vars.dim() == 3)
          grad_w = context.interior_gradient(this->_flow_vars.w(), qp);

        libMesh::NumberVectorValue Uvec (u,v);
        if (this->_flow_vars.dim() == 3)
          Uvec(2) = w;

        // Compute the viscosity at this qp
        libMesh::Real _mu_qp = this->_mu(context, qp);

        // First, an i-loop over the velocity degrees of freedom.
        // We know that n_u_dofs == n_v_dofs so we can compute contributions
        // for both at the same time.
        for (unsigned int i=0; i != n_u_dofs; i++)
          {
            Fu(i) += JxW[qp] *
              ( p*u_gradphi[i][qp](0)              // pressure term
                -_mu_qp*(u_gradphi[i][qp]*grad_u) ); // diffusion term

            Fv(i) += JxW[qp] *
              ( p*u_gradphi[i][qp](1)              // pressure term
                -_mu_qp*(u_gradphi[i][qp]*grad_v) ); // diffusion term
            if (this->_flow_vars.dim() == 3)
              {
                (*Fw)(i) += JxW[qp] *
                  ( p*u_gradphi[i][qp](2)              // pressure term
                    -_mu_qp*(u_gradphi[i][qp]*grad_w) ); // diffusion term
              }

            if (compute_jacobian)
              {
                for (unsigned int j=0; j != n_u_dofs; j++)
                  {
                    // TODO: precompute some terms like:
                    //   (Uvec*vel_gblgradphivec[j][qp]),
                    //   vel_phi[i][qp]*vel_phi[j][qp],
                    //   (vel_gblgradphivec[i][qp]*vel_gblgradphivec[j][qp])

                    Kuu(i,j) += JxW[qp] * context.get_elem_solution_derivative() *
                      (-_mu_qp*(u_gradphi[i][qp]*u_gradphi[j][qp])); // diffusion term

                    Kvv(i,j) += JxW[qp] * context.get_elem_solution_derivative() *
                      (-_mu_qp*(u_gradphi[i][qp]*u_gradphi[j][qp])); // diffusion term

                    if (this->_flow_vars.dim() == 3)
                      {
                        (*Kww)(i,j) += JxW[qp] * context.get_elem_solution_derivative() *
                          (-_mu_qp*(u_gradphi[i][qp]*u_gradphi[j][qp])); // diffusion term
                      }
                  } // end of the inner dof (j) loop

                // Matrix contributions for the up, vp and wp couplings
                for (unsigned int j=0; j != n_p_dofs; j++)
                  {
                    Kup(i,j) += context.get_elem_solution_derivative() * JxW[qp]*u_gradphi[i][qp](0)*p_phi[j][qp];
                    Kvp(i,j) += context.get_elem_solution_derivative() * JxW[qp]*u_gradphi[i][qp](1)*p_phi[j][qp];
                    if (this->_flow_vars.dim() == 3)
                      (*Kwp)(i,j) += context.get_elem_solution_derivative() * JxW[qp]*u_gradphi[i][qp](2)*p_phi[j][qp];
                  } // end of the inner dof (j) loop

              } // end - if (compute_jacobian && context.get_elem_solution_derivative())

          } // end of the outer dof (i) loop
      } // end of the quadrature point (qp) loop
  }

  template<class Mu>
  void Stokes<Mu>::element_constraint
  ( bool compute_jacobian, AssemblyContext & context )
  {
    this->element_constraint_impl(compute_jacobian,context);

    // Pin p = p_value at p_point
    if( _pin_pressure )
      _p_pinning.pin_value( context, compute_jacobian, this->_press_var.p() );
  }

} // namespace GRINS

// Instantiate
INSTANTIATE_INC_NS_SUBCLASS(Stokes);
