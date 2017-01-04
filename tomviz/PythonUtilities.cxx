/******************************************************************************

  This source file is part of the tomviz project.

  Copyright Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/
#include "PythonUtilities.h"
#include "vtkPython.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"

#include <QDebug>

#include <pybind11/pybind11.h>

namespace tomviz {

Python::Capsule::Capsule(const void* ptr)
  : m_capsule(new pybind11::capsule(ptr))
{
}

Python::Capsule::operator PyObject*() const
{
  return m_capsule->ptr();
}

void Python::Capsule::incrementRefCount()
{
  m_capsule->inc_ref();
}

Python::Capsule::~Capsule()
{
  delete m_capsule;
}

Python::Object::Object() : m_smartPyObject(new vtkSmartPyObject())
{
}

Python::Object::Object(const Python::Object& other)
: m_smartPyObject(new vtkSmartPyObject(*other.m_smartPyObject))
{
}

Python::Object::Object(const QString& str)
{
  m_smartPyObject = new vtkSmartPyObject(toPyObject(str));
}

Python::Object::Object(const QVariant& value)
{
  m_smartPyObject = new vtkSmartPyObject(toPyObject(value));
}

Python::Object::Object(const QVariantList& list)
{
  m_smartPyObject = new vtkSmartPyObject(toPyObject(list));
}

Python::Object::Object(PyObject *obj)
{
  m_smartPyObject = new vtkSmartPyObject(obj);
}

Python::Object& Python::Object::operator=(const Python::Object& other)
{
  *m_smartPyObject = *other.m_smartPyObject;
  return *this;
}

Python::Object::operator PyObject*() const
{
  return *m_smartPyObject;
}

Python::Object::operator bool() const
{
  return *m_smartPyObject;
}

bool Python::Object::toBool() const
{
  return m_smartPyObject->GetPointer() == Py_True;
}

bool Python::Object::isDict() const
{

  return PyDict_Check(m_smartPyObject->GetPointer());
}

bool Python::Object::isValid() const
{
  return m_smartPyObject->GetPointer() != nullptr;
}


Python::Dict Python::Object::toDict()
{

  return m_smartPyObject->GetPointer();
}

Python::Object::~Object()
{
  delete m_smartPyObject;
}

void Python::Object::incrementRefCount()
{
  m_smartPyObject->GetAndIncreaseReferenceCount();
}

Python::Tuple::Tuple() : Object()
{
}

Python::Tuple::Tuple(const Python::Tuple& other) : Object(other)
{
}

Python::Tuple::Tuple(int size)
{
  m_smartPyObject->TakeReference(PyTuple_New(size));
}

void Python::Tuple::set(int index, Python::Object& obj)
{
  // Note we increment ref count
  // as PyTuple_SET_ITEM will "steal" the reference.
  obj.incrementRefCount();
  PyTuple_SET_ITEM(m_smartPyObject->GetPointer(), index, obj);
}

void Python::Tuple::set(int index, Python::Module& module)
{
  // Note we increment ref count
  // as PyTuple_SET_ITEM will "steal" the reference.
  module.incrementRefCount();
  PyTuple_SET_ITEM(m_smartPyObject->GetPointer(), index, module);
}

void Python::Tuple::set(int index, Python::Capsule& capsule)
{
  // Increment ref count as the pybind11::capsule destructor will decrement
  // and we need the capsule to stay around. The PyTuple_SET_ITEM will
  // steal the other reference.
  capsule.incrementRefCount();
  PyTuple_SET_ITEM(m_smartPyObject->GetPointer(), index, capsule);
}

Python::Dict::Dict() : Object()
{
  m_smartPyObject->TakeReference(PyDict_New());
}

Python::Dict::Dict(PyObject *obj) : Object(obj)
{

}


Python::Dict::Dict(const Python::Dict& other) : Object(other)
{
}

Python::Object Python::Dict::operator[](const QString& key)
{
  return PyDict_GetItemString(m_smartPyObject->GetPointer(), key.toLatin1().data());;
}

void Python::Dict::set(const QString& key, const Object& value)
{
  Python::Object pyKey(key);
  PyDict_SetItem(m_smartPyObject->GetPointer(), pyKey, value);
}

void Python::Dict::set(const QString& key, const QVariant& value)
{
  Python::Object obj(value);
  set(key, obj);
}

void Python::Dict::set(const QString& key, const QString& str)
{
  Python::Object obj(str);
  set(key, obj);
}

void Python::Dict::set(const QString& key, const QVariantList& list)
{
  Python::Object obj(list);
  set(key, obj);
}

Python::Function::Function() : Object()
{
}

Python::Function::Function(PyObject *obj)
  : Object(obj)
{

}

Python::Function::Function(const Python::Function& other) : Object(other)

{
}

Python::Function& Python::Function::operator=(const Python::Object& other)
{
  Object::operator =(other);

  return *this;
}

Python::Object Python::Function::call(Tuple& args)
{
  Python::Dict empty;
  return call(args, empty);
}

Python::Object Python::Function::call(Tuple& args, Dict& kwargs)
{
  Python::Object result = PyObject_Call(m_smartPyObject->GetPointer(), args, kwargs);

  if (!result.isValid()) {
    checkForPythonError();
  }

  return result;
}

Python::Object Python::VTK::GetObjectFromPointer(vtkObjectBase* ptr)
{
  return vtkPythonUtil::GetObjectFromPointer(ptr);
}

vtkObjectBase* Python::VTK::GetPointerFromObject(Python::Object obj,
                                                 const char* classname)
{
  return vtkPythonUtil::GetPointerFromObject(obj, classname);
}

void Python::initialize()
{
  vtkPythonInterpreter::Initialize();
}

Python::Python() : m_ensurer(new vtkPythonScopeGilEnsurer(true))
{
}

Python::~Python()
{
  delete m_ensurer;
}

Python::Module Python::import(const QString& name)
{
  Python::Module module = PyImport_ImportModule(name.toLatin1().data());
  if (!module.isValid()) {
    checkForPythonError();
  }

  return module;
}

Python::Module::Module() : Object()
{
}

Python::Module::Module(PyObject *obj)
  : Object(obj)
{

}

Python::Module::Module(const Python::Module& other) : Object(other)
{
}

Python::Module& Python::Module::operator=(const Python::Module& other)
{
  Python::Object::operator=(other);
  return *this;
}

Python::Function Python::Module::findFunction(const QString& name)
{

  Python::Function func = PyObject_GetAttrString(m_smartPyObject->GetPointer(),
                                name.toLatin1().data());
  if (!func.isValid()) {
    checkForPythonError();
  }

  return func;
}

Python::Module Python::import(const QString& str, const QString& filename)
{

  Python::Module module;

  vtkSmartPyObject code =
    Py_CompileString(str.toLatin1().data(), filename.toLatin1().data(),
                     Py_file_input /*Py_eval_input*/);
  if (!code) {
    checkForPythonError();
    qCritical("Invalid script. Please check the traceback message for details");
    return module;
  }

  module = PyImport_ExecCodeModule(
    QString("tomviz_%1").arg(filename).toLatin1().data(), code);
  if (!module.isValid()) {
    checkForPythonError();
    qCritical("Failed to create module.");
    return module;
  }

  return module;
}

QDebug operator<<(QDebug dbg, const Python::Dict& dict)
{
  PyObject* objectRepr = PyObject_Repr(dict);
  dbg.nospace() << PyString_AsString(objectRepr);

  return dbg.maybeSpace();
}

bool Python::checkForPythonError()
{
  PyObject* exception = PyErr_Occurred();
  if (exception) {
    // We use PyErr_PrintEx(0) to prevent sys.last_traceback being set
    // which holds a reference to any parameters passed to PyObject_Call.
    // This can cause a temporary "leak" until sys.last_traceback is reset.
    // This can be a problem if the object in question is a VTK object that
    // holds a reference to a large memory allocation.
    PyErr_PrintEx(0);
    return true;
  }
  return false;
}

PyObject* toPyObject(const QString& str)
{
  return PyUnicode_DecodeUTF16((const char*)str.utf16(), str.length() * 2, NULL,
                               NULL);
}

PyObject* toPyObject(const QVariant& value)
{

  switch (value.type()) {
    case QVariant::Int:
      return PyInt_FromLong(value.toInt());
    case QVariant::Double:
      return PyFloat_FromDouble(value.toDouble());
    case QVariant::Bool:
      return value.toBool() ? Py_True : Py_False;
    case QVariant::String: {
      QString str = value.toString();
      return toPyObject(str);
    }
    case QVariant::List: {
      QVariantList list = value.toList();
      return toPyObject(list);
    }
    default:
      qCritical() << "Unsupported type";
  }

  return nullptr;
}

PyObject* toPyObject(const QVariantList& list)
{
  PyObject* pyList = PyTuple_New(list.count());
  int i = 0;

  foreach (QVariant value, list) {
    PyTuple_SET_ITEM(pyList, i, toPyObject(value));
    i++;
  }

  return pyList;
}
}
