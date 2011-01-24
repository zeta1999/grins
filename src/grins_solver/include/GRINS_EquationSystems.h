//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//
// Copyright (C) 2008-2010 The PECOS Development Team
//
// Please see http://pecos.ices.utexas.edu for more information.
//
// This file is part of GRINS.
//
// GRINS is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GRINS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GRINS.  If not, see <http://www.gnu.org/licenses/>.
//
//--------------------------------------------------------------------------
//
// GRINS_EquationSystems.h: Declarations for the GRINS_EquationSystems class.
//
// $Id$
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

#ifndef GRINS_EQUATIONSYSTEMS_H
#define GRINS_EQUATIONSYSTEMS_H

#include <string>

#include "libmesh.h"
#include "fem_system.h"

// FEMSystem, TimeSolver and  NewtonSolver will handle most tasks,
// but we must specify element residuals
class GRINS_EquationSystems : public FEMSystem
{

public:
  // Constructor
  GRINS_EquationSystems(EquationSystems& es,
                        const std::string& name,
                        const unsigned int number)
  : FEMSystem(es, name, number)
    {}

  // Destructor
  ~GRINS_EquationSystems() {}

  void set_application( const std::string application_options );

private:
  std::string _application_options;
};

#endif