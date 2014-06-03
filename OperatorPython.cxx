/******************************************************************************

  This source file is part of the TEM tomography project.

  Copyright Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/
#include "OperatorPython.h"

#include "vtkPythonInterpreter.h"
#include <sstream>

namespace TEM
{

class OperatorPython::OPInternals
{
public:
};

//-----------------------------------------------------------------------------
OperatorPython::OperatorPython(QObject* parentObject) :
  Superclass(parentObject),
  Internals(new OperatorPython::OPInternals())
{
  vtkPythonInterpreter::PrependPythonPath("/home/utkarsh/Kitware/TEMTomography/Python");
  vtkPythonInterpreter::PrependPythonPath("/home/utkarsh/Kitware/ParaView3/ParaViewBin/lib");
  vtkPythonInterpreter::PrependPythonPath(
    "/home/utkarsh/Kitware/ParaView3/ParaViewBin/lib/site-packages");

  this->Script = "import numpy as np\n"
                 "np.cos(scalars, scalars)\n";
}

//-----------------------------------------------------------------------------
OperatorPython::~OperatorPython()
{
}

//-----------------------------------------------------------------------------
QIcon OperatorPython::icon() const
{
  return QIcon(":/pqWidgets/Icons/pqProgrammableFilter24.png");
}

//-----------------------------------------------------------------------------
bool OperatorPython::transform(vtkDataObject* data)
{
  if (this->Script.isEmpty()) { return true; }

  Q_ASSERT(data);

  // ensure Python is initialized.
  vtkPythonInterpreter::Initialize();

  // Set self to point to this
  char addrofthis[1024];
  sprintf(addrofthis, "%p", data);
  char *aplus = addrofthis;
  if ((addrofthis[0] == '0') &&
      ((addrofthis[1] == 'x') || addrofthis[1] == 'X'))
    {
    aplus += 2; //skip over "0x"
    }

  std::ostringstream stream;
  stream <<
    "from paraview.vtk import vtkDataObject\n"
    "from matviz import operator as moperator\n"
    "dataObj = vtkDataObject('" << aplus <<"')\n"
    "scalars = moperator.getscalars(dataObj)\n"
    << this->Script.toLatin1().data()  << "\n"
    "del scalars\n"
    "del dataObj\n"
    "import gc\n"
    "gc.collect()\n";
  vtkPythonInterpreter::RunSimpleString(stream.str().c_str());
  return true;
}

}
