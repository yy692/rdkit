// $Id$
//
//  Copyright (C) 2004-2008 Greg Landrum and Rational Discovery LLC
//
//   @@ All Rights Reserved @@
//  This file is part of the RDKit.
//  The contents are covered by the terms of the BSD license
//  which is included in the file license.txt, found at the root
//  of the RDKit source tree.
//
#include <RDBoost/python.h>
#include <GraphMol/GraphMol.h>
#include <RDBoost/Wrap.h>

#include <ForceField/ForceField.h>
#include <ForceField/Wrap/PyForceField.h>
#include <ForceField/UFF/Params.h>
#include <GraphMol/ForceFieldHelpers/FFConvenience.h>
#include <GraphMol/ForceFieldHelpers/UFF/AtomTyper.h>
#include <GraphMol/ForceFieldHelpers/UFF/Builder.h>
#include <GraphMol/ForceFieldHelpers/UFF/UFF.h>
#include <GraphMol/ForceFieldHelpers/MMFF/AtomTyper.h>
#include <GraphMol/ForceFieldHelpers/MMFF/Builder.h>
#include <GraphMol/ForceFieldHelpers/MMFF/MMFF.h>

namespace python = boost::python;

namespace RDKit {
int UFFHelper(ROMol &mol, int maxIters, double vdwThresh, int confId,
              bool ignoreInterfragInteractions) {
  NOGIL gil;
  return UFF::UFFOptimizeMolecule(mol, maxIters, vdwThresh, confId,
                                  ignoreInterfragInteractions)
      .first;
}
python::object UFFConfsHelper(ROMol &mol, int numThreads, int maxIters,
                              double vdwThresh, int confId,
                              bool ignoreInterfragInteractions) {
  RDUNUSED_PARAM(confId);  // XXX FIX ME?
  std::vector<std::pair<int, double>> res;
  {
    NOGIL gil;
    UFF::UFFOptimizeMoleculeConfs(mol, res, numThreads, maxIters, vdwThresh,
                                  ignoreInterfragInteractions);
  }
  python::list pyres;
  for (auto &itm : res) {
    pyres.append(python::make_tuple(itm.first, itm.second));
  }
  return pyres;
}

python::object MMFFConfsHelper(ROMol &mol, int numThreads, int maxIters,
                               std::string mmffVariant, double nonBondedThresh,
                               int confId, bool ignoreInterfragInteractions) {
  RDUNUSED_PARAM(confId);  // Fix me?
  std::vector<std::pair<int, double>> res;
  {
    NOGIL gil;
    MMFF::MMFFOptimizeMoleculeConfs(mol, res, numThreads, maxIters, mmffVariant,
                                    nonBondedThresh,
                                    ignoreInterfragInteractions);
  }
  python::list pyres;
  for (auto &itm : res) {
    pyres.append(python::make_tuple(itm.first, itm.second));
  }
  return pyres;
}

int FFHelper(ForceFields::ForceField &ff, int maxIters) {
  NOGIL gil;
  return ForceFieldsHelper::OptimizeMolecule(ff, maxIters).first;
}

python::object FFConfsHelper(ROMol &mol, ForceFields::ForceField &ff, int numThreads, int maxIters) {
  std::vector<std::pair<int, double>> res;
  {
    NOGIL gil;
    ForceFieldsHelper::OptimizeMoleculeConfs(mol, ff, res, numThreads, maxIters);
  }
  python::list pyres;
  for (auto &itm : res) {
    pyres.append(python::make_tuple(itm.first, itm.second));
  }
  return pyres;
}

ForceFields::PyForceField *UFFGetMoleculeForceField(
    ROMol &mol, double vdwThresh = 10.0, int confId = -1,
    bool ignoreInterfragInteractions = true) {
  ForceFields::ForceField *ff = UFF::constructForceField(
      mol, vdwThresh, confId, ignoreInterfragInteractions);
  auto *res = new ForceFields::PyForceField(ff);
  res->initialize();
  return res;
}

python::object UFFGetForceFieldTerms(
    ROMol &mol, double vdwThresh = 10.0, int confId = -1,
    bool ignoreInterfragInteractions = true) {
   
  python::list ETerms;
  std::vector<std::vector<double>> terms;
  UFF::getForceFieldTerms(
        mol, terms, vdwThresh, confId, ignoreInterfragInteractions);
  for (std::vector<double> term: terms) {
    ETerms.append(python::make_tuple(term[0],term[1],term[2],term[3],term[4],term[5]));
  }
  return ETerms;
}

bool UFFHasAllMoleculeParams(const ROMol &mol) {
  UFF::AtomicParamVect types;
  bool foundAll;
  boost::tie(types, foundAll) = UFF::getAtomTypes(mol);
  return foundAll;
}

int MMFFOptimizeMolecule(ROMol &mol, std::string mmffVariant = "MMFF94",
                         int maxIters = 200, double nonBondedThresh = 100.0,
                         int confId = -1,
                         bool ignoreInterfragInteractions = true) {
  int res = -1;

  MMFF::MMFFMolProperties mmffMolProperties(mol, mmffVariant);
  if (mmffMolProperties.isValid()) {
    NOGIL gil;
    ForceFields::ForceField *ff =
        MMFF::constructForceField(mol, &mmffMolProperties, nonBondedThresh,
                                  confId, ignoreInterfragInteractions);
    ff->initialize();
    res = ff->minimize(maxIters);
    delete ff;
  }

  return res;
}

unsigned int SanitizeMMFFMol(ROMol &mol) {
  return MMFF::sanitizeMMFFMol((RWMol &)mol);
};

ForceFields::PyMMFFMolProperties *GetMMFFMolProperties(
    ROMol &mol, std::string mmffVariant = "MMFF94",
    unsigned int mmffVerbosity = MMFF::MMFF_VERBOSITY_NONE) {
  auto *mmffMolProperties =
      new MMFF::MMFFMolProperties(mol, mmffVariant, mmffVerbosity);
  ForceFields::PyMMFFMolProperties *pyMP = nullptr;

  if (mmffMolProperties && mmffMolProperties->isValid()) {
    pyMP = new ForceFields::PyMMFFMolProperties(mmffMolProperties);
  }

  return pyMP;
}

ForceFields::PyForceField *MMFFGetMoleculeForceField(
    ROMol &mol, ForceFields::PyMMFFMolProperties *pyMMFFMolProperties,
    double nonBondedThresh = 100.0, int confId = -1,
    bool ignoreInterfragInteractions = true) {
  ForceFields::PyForceField *pyFF = nullptr;
  boost::python::list res;

  if (pyMMFFMolProperties) {
    MMFF::MMFFMolProperties *mmffMolProperties =
        &(*(pyMMFFMolProperties->mmffMolProperties));
    ForceFields::ForceField *ff =
        MMFF::constructForceField(mol, mmffMolProperties, nonBondedThresh,
                                  confId, ignoreInterfragInteractions);
    pyFF = new ForceFields::PyForceField(ff);
    if (pyFF) {
      pyFF->initialize();
    }
  }

  return pyFF;
}

bool MMFFHasAllMoleculeParams(const ROMol &mol) {
  ROMol molCopy(mol);
  MMFF::MMFFMolProperties mmffMolProperties(molCopy);

  return mmffMolProperties.isValid();
}
};

namespace ForceFields {
typedef std::vector<const ForceFields::UFF::AtomicParams *> AtomicParamVect;

PyObject *GetUFFAtomTypes(const RDKit::ROMol &mol) {
  PyObject *res = nullptr;
  PyObject *resitem = nullptr;
  AtomicParamVect params;
  bool foundAll;
  boost::tie(params, foundAll) = RDKit::UFF::getAtomTypes(mol);

  
  res = PyList_New(mol.getNumAtoms());
  for (unsigned int i = 0; i < mol.getNumAtoms(); i++) {
    resitem = PyTuple_New(11);
    PyTuple_SetItem(resitem, 0, PyFloat_FromDouble(params[i]->r1));
    PyTuple_SetItem(resitem, 1, PyFloat_FromDouble(params[i]->theta0));
    PyTuple_SetItem(resitem, 2, PyFloat_FromDouble(params[i]->x1));
    PyTuple_SetItem(resitem, 3, PyFloat_FromDouble(params[i]->D1));
    PyTuple_SetItem(resitem, 4, PyFloat_FromDouble(params[i]->zeta));
    PyTuple_SetItem(resitem, 5, PyFloat_FromDouble(params[i]->Z1));
    PyTuple_SetItem(resitem, 6, PyFloat_FromDouble(params[i]->V1));
    PyTuple_SetItem(resitem, 7, PyFloat_FromDouble(params[i]->U1));
    PyTuple_SetItem(resitem, 8, PyFloat_FromDouble(params[i]->GMP_Xi));
    PyTuple_SetItem(resitem, 9, PyFloat_FromDouble(params[i]->GMP_Hardness));
    PyTuple_SetItem(resitem, 10, PyFloat_FromDouble(params[i]->GMP_Radius));
    PyList_SetItem(res, i, resitem);
  }
    
  return res;
};

PyObject *getUFFBondStretchParams(const RDKit::ROMol &mol,
                                  const unsigned int idx1,
                                  const unsigned int idx2) {
  PyObject *res = nullptr;
  ForceFields::UFF::UFFBond uffBondStretchParams;
  if (RDKit::UFF::getUFFBondStretchParams(mol, idx1, idx2,
                                          uffBondStretchParams)) {
    res = PyTuple_New(2);
    PyTuple_SetItem(res, 0, PyFloat_FromDouble(uffBondStretchParams.kb));
    PyTuple_SetItem(res, 1, PyFloat_FromDouble(uffBondStretchParams.r0));
  }
  return res;
};

PyObject *getUFFAngleBendParams(const RDKit::ROMol &mol,
                                const unsigned int idx1,
                                const unsigned int idx2,
                                const unsigned int idx3) {
  PyObject *res = nullptr;

  ForceFields::UFF::UFFAngle uffAngleBendParams;
  if (RDKit::UFF::getUFFAngleBendParams(mol, idx1, idx2, idx3,
                                        uffAngleBendParams)) {
    res = PyTuple_New(2);
    PyTuple_SetItem(res, 0, PyFloat_FromDouble(uffAngleBendParams.ka));
    PyTuple_SetItem(res, 1, PyFloat_FromDouble(uffAngleBendParams.theta0));
  }
    
  return res;
};

PyObject *getUFFTorsionParams(const RDKit::ROMol &mol, const unsigned int idx1,
                              const unsigned int idx2, const unsigned int idx3,
                              const unsigned int idx4) {
  PyObject *res = nullptr;
  ForceFields::UFF::UFFTor uffTorsionParams;
  if (RDKit::UFF::getUFFTorsionParams(mol, idx1, idx2, idx3, idx4,
                                      uffTorsionParams)) {
    res = PyFloat_FromDouble(uffTorsionParams.V);
  }
  return res;
};

PyObject *getUFFInversionParams(const RDKit::ROMol &mol,
                                const unsigned int idx1,
                                const unsigned int idx2,
                                const unsigned int idx3,
                                const unsigned int idx4) {
  PyObject *res = nullptr;
  ForceFields::UFF::UFFInv uffInversionParams;
  if (RDKit::UFF::getUFFInversionParams(mol, idx1, idx2, idx3, idx4,
                                        uffInversionParams)) {
    res = PyFloat_FromDouble(uffInversionParams.K);
  }
  return res;
};

PyObject *getUFFVdWParams(const RDKit::ROMol &mol, const unsigned int idx1,
                          const unsigned int idx2) {
  PyObject *res = nullptr;
  ForceFields::UFF::UFFVdW uffVdWParams;
  if (RDKit::UFF::getUFFVdWParams(mol, idx1, idx2, uffVdWParams)) {
    res = PyTuple_New(2);
    PyTuple_SetItem(res, 0, PyFloat_FromDouble(uffVdWParams.x_ij));
    PyTuple_SetItem(res, 1, PyFloat_FromDouble(uffVdWParams.D_ij));
  }
  return res;
};
}

BOOST_PYTHON_MODULE(rdForceFieldHelpers) {
  python::scope().attr("__doc__") =
      "Module containing functions to handle force fields";

  std::string docString =
      "uses UFF to optimize a molecule's structure\n\n\
 \n\
 ARGUMENTS:\n\n\
    - mol : the molecule of interest\n\
    - maxIters : the maximum number of iterations (defaults to 200)\n\
    - vdwThresh : used to exclude long-range van der Waals interactions\n\
                  (defaults to 10.0)\n\
    - confId : indicates which conformer to optimize\n\
    - ignoreInterfragInteractions : if true, nonbonded terms between\n\
                  fragments will not be added to the forcefield.\n\
\n\
 RETURNS: 0 if the optimization converged, 1 if more iterations are required.\n\
\n";
  python::def("UFFOptimizeMolecule", RDKit::UFFHelper,
              (python::arg("self"), python::arg("maxIters") = 200,
               python::arg("vdwThresh") = 10.0, python::arg("confId") = -1,
               python::arg("ignoreInterfragInteractions") = true),
              docString.c_str());

  docString =
      "uses UFF to optimize all of a molecule's conformations\n\n\
 \n\
 ARGUMENTS:\n\n\
    - mol : the molecule of interest\n\
    - numThreads : the number of threads to use, only has an effect if the RDKit\n\
                   was built with thread support (defaults to 1)\n\
                   If set to zero, the max supported by the system will be used.\n\
    - maxIters : the maximum number of iterations (defaults to 200)\n\
    - vdwThresh : used to exclude long-range van der Waals interactions\n\
                  (defaults to 10.0)\n\
    - confId : indicates which conformer to optimize\n\
    - ignoreInterfragInteractions : if true, nonbonded terms between\n\
                  fragments will not be added to the forcefield.\n\
\n\
 RETURNS: a list of (not_converged, energy) 2-tuples. \n\
     If not_converged is 0 the optimization converged for that conformer.\n\
\n";
  python::def("UFFOptimizeMoleculeConfs", RDKit::UFFConfsHelper,
              (python::arg("self"), python::arg("numThreads") = 1,
               python::arg("maxIters") = 200, python::arg("vdwThresh") = 10.0,
               python::arg("confId") = -1,
               python::arg("ignoreInterfragInteractions") = true),
              docString.c_str());

  docString =
      "returns a UFF force field for a molecule\n\n\
 \n\
 ARGUMENTS:\n\n\
    - mol : the molecule of interest\n\
    - vdwThresh : used to exclude long-range van der Waals interactions\n\
                  (defaults to 10.0)\n\
    - confId : indicates which conformer to optimize\n\
    - ignoreInterfragInteractions : if true, nonbonded terms between\n\
                  fragments will not be added to the forcefield.\n\
\n";
  python::def("UFFGetMoleculeForceField", RDKit::UFFGetMoleculeForceField,
              (python::arg("mol"), python::arg("vdwThresh") = 10.0,
               python::arg("confId") = -1,
               python::arg("ignoreInterfragInteractions") = true),
              python::return_value_policy<python::manage_new_object>(),
              docString.c_str());
  docString =
          "checks if UFF parameters are available for all of a molecule's atoms\n\n\
     \n\
     ARGUMENTS:\n\n\
        - mol : the molecule of interest.\n\
    \n";
  python::def("UFFGetForceFieldTerms", RDKit::UFFGetForceFieldTerms,
              (python::arg("mol"), python::arg("vdwThresh") = 10.0,
               python::arg("confId") = -1,
               python::arg("ignoreInterfragInteractions") = true),
              docString.c_str());
  docString =
      "checks if UFF parameters are available for all of a molecule's atoms\n\n\
 \n\
 ARGUMENTS:\n\n\
    - mol : the molecule of interest.\n\
\n";
  python::def("UFFHasAllMoleculeParams", RDKit::UFFHasAllMoleculeParams,
              (python::arg("mol")), docString.c_str());

  docString =
      "uses MMFF to optimize a molecule's structure\n\n\
 \n\
 ARGUMENTS:\n\n\
    - mol : the molecule of interest\n\
    - mmffVariant : \"MMFF94\" or \"MMFF94s\"\n\
    - maxIters : the maximum number of iterations (defaults to 200)\n\
    - nonBondedThresh : used to exclude long-range non-bonded\n\
                 interactions (defaults to 100.0)\n\
    - confId : indicates which conformer to optimize\n\
    - ignoreInterfragInteractions : if true, nonbonded terms between\n\
                 fragments will not be added to the forcefield\n\
\n\
 RETURNS: 0 if the optimization converged, -1 if the forcefield could\n\
          not be set up, 1 if more iterations are required.\n\
\n";
  python::def(
      "MMFFOptimizeMolecule", RDKit::MMFFOptimizeMolecule,
      (python::arg("mol"), python::arg("mmffVariant") = "MMFF94",
       python::arg("maxIters") = 200, python::arg("nonBondedThresh") = 100.0,
       python::arg("confId") = -1,
       python::arg("ignoreInterfragInteractions") = true),
      docString.c_str());

  docString =
      "sanitizes a molecule according to MMFF requirements.\n\n\
    - mol : the molecule of interest.\n\
\n";
  python::def("MMFFSanitizeMolecule", RDKit::SanitizeMMFFMol,
              (python::arg("mol")), docString.c_str());

  docString =
      "returns a PyMMFFMolProperties object for a\n\
  molecule, which is required by MMFFGetMoleculeForceField()\n\
  and can be used to get/set MMFF properties\n\n\
  \n\
  ARGUMENTS:\n\n\
    - mol : the molecule of interest\n\
    - mmffVariant : \"MMFF94\" or \"MMFF94s\"\n\
                  (defaults to \"MMFF94\")\n\
    - mmffVerbosity : 0: none; 1: low; 2: high (defaults to 0).\n\
\n";
  python::def("MMFFGetMoleculeProperties", RDKit::GetMMFFMolProperties,
              (python::arg("mol"), python::arg("mmffVariant") = "MMFF94",
               python::arg("mmffVerbosity") = 0),
              python::return_value_policy<python::manage_new_object>(),
              docString.c_str());

  docString =
      "returns a MMFF force field for a molecule\n\n\
 \n\
 ARGUMENTS:\n\n\
    - mol : the molecule of interest\n\
    - pyMMFFMolProperties : PyMMFFMolProperties object as returned\n\
                  by MMFFGetMoleculeProperties()\n\
    - nonBondedThresh : used to exclude long-range non-bonded\n\
                  interactions (defaults to 100.0)\n\
    - confId : indicates which conformer to optimize\n\
    - ignoreInterfragInteractions : if true, nonbonded terms between\n\
                  fragments will not be added to the forcefield\n\
\n";
  python::def(
      "MMFFGetMoleculeForceField", RDKit::MMFFGetMoleculeForceField,
      (python::arg("mol"), python::arg("pyMMFFMolProperties"),
       python::arg("nonBondedThresh") = 100.0, python::arg("confId") = -1,
       python::arg("ignoreInterfragInteractions") = true),
      python::return_value_policy<python::manage_new_object>(),
      docString.c_str());

  docString =
      "checks if MMFF parameters are available for all of a molecule's atoms\n\n\
 \n\
 ARGUMENTS:\n\n\
    - mol : the molecule of interest\n\
\n";
  python::def("MMFFHasAllMoleculeParams", RDKit::MMFFHasAllMoleculeParams,
              (python::arg("mol")), docString.c_str());

  docString =
      "uses MMFF to optimize all of a molecule's conformations\n\n\
 \n\
 ARGUMENTS:\n\n\
    - mol : the molecule of interest\n\
    - numThreads : the number of threads to use, only has an effect if the RDKit\n\
                   was built with thread support (defaults to 1)\n\
                   If set to zero, the max supported by the system will be used.\n\
    - maxIters : the maximum number of iterations (defaults to 200)\n\
    - mmffVariant : \"MMFF94\" or \"MMFF94s\"\n\
    - nonBondedThresh : used to exclude long-range non-bonded\n\
                  interactions (defaults to 100.0)\n\
    - confId : indicates which conformer to optimize\n\
    - ignoreInterfragInteractions : if true, nonbonded terms between\n\
                  fragments will not be added to the forcefield.\n\
\n\
RETURNS: a list of (not_converged, energy) 2-tuples. \n\
    If not_converged is 0 the optimization converged for that conformer.\n\
\n";
  python::def(
      "MMFFOptimizeMoleculeConfs", RDKit::MMFFConfsHelper,
      (python::arg("self"), python::arg("numThreads") = 1,
       python::arg("maxIters") = 200, python::arg("mmffVariant") = "MMFF94",
       python::arg("nonBondedThresh") = 100.0, python::arg("confId") = -1,
       python::arg("ignoreInterfragInteractions") = true),
      docString.c_str());

//  python::def(
//      "GetUFFAtomTypes", ForceFields::GetUFFAtomTypes,
//      (python::arg("mol")),
//      "Retrieves UFF atom types for atoms in the provided molecule "
//      "as a V float value, or None if no parameters could be found");
  python::def(
        "GetUFFAtomTypes", ForceFields::GetUFFAtomTypes,
        (python::arg("mol")),
        "Retrieves UFF atom types for atoms in the provided molecule "
        "as a V float value, or None if no parameters could be found");
  python::def(
      "GetUFFBondStretchParams", ForceFields::getUFFBondStretchParams,
      (python::arg("mol"), python::arg("idx1"), python::arg("idx2")),
      "Retrieves UFF bond stretch parameters for atoms with indexes idx1, idx2 "
      "as a (kb, r0) tuple, or None if no parameters could be found");
  python::def("GetUFFAngleBendParams", ForceFields::getUFFAngleBendParams,
              (python::arg("mol"), python::arg("idx1"), python::arg("idx2"),
               python::arg("idx3")),
              "Retrieves UFF angle bend parameters for atoms with indexes "
              "idx1, idx2, idx3 as a (ka, theta0) tuple, or None if no "
              "parameters could be found");
  python::def("GetUFFTorsionParams", ForceFields::getUFFTorsionParams,
              (python::arg("mol"), python::arg("idx1"), python::arg("idx2"),
               python::arg("idx3"), python::arg("idx4")),
              "Retrieves UFF torsion parameters for atoms "
              "with indexes idx1, idx2, idx3, idx4 as a V float value, or None "
              "if no parameters "
              "could be found");
  python::def("GetUFFInversionParams", ForceFields::getUFFInversionParams,
              (python::arg("mol"), python::arg("idx1"), python::arg("idx2"),
               python::arg("idx3"), python::arg("idx4")),
              "Retrieves UFF inversion parameters for atoms "
              "with indexes idx1, idx2, idx3, idx4 as a K float value, or None "
              "if no parameters "
              "could be found");
  python::def(
      "GetUFFVdWParams", ForceFields::getUFFVdWParams,
      (python::arg("mol"), python::arg("idx1"), python::arg("idx2")),
      "Retrieves UFF van der Waals parameters for atoms with indexes idx1, "
      "idx2 "
      "as a (x_ij, D_ij) tuple, or None if no parameters could be found");

  docString =
      "uses the supplied force field to optimize a molecule's structure\n\n\
 \n\
 ARGUMENTS:\n\n\
    - ff : the force field\n\
    - maxIters : the maximum number of iterations (defaults to 200)\n\
\n\
 RETURNS: 0 if the optimization converged, 1 if more iterations are required.\n\
\n";
  python::def("OptimizeMolecule", RDKit::FFHelper,
              (python::arg("ff"), python::arg("maxIters") = 200),
              docString.c_str());

  docString =
      "uses the supplied force field to optimize all of a molecule's conformations\n\n\
 \n\
 ARGUMENTS:\n\n\
    - mol : the molecule of interest\n\
    - ff : the force field\n\
    - numThreads : the number of threads to use, only has an effect if the RDKit\n\
                   was built with thread support (defaults to 1)\n\
                   If set to zero, the max supported by the system will be used.\n\
    - maxIters : the maximum number of iterations (defaults to 200)\n\
\n\
 RETURNS: a list of (not_converged, energy) 2-tuples. \n\
     If not_converged is 0 the optimization converged for that conformer.\n\
\n";
  python::def("OptimizeMoleculeConfs", RDKit::FFConfsHelper,
              (python::arg("mol"), python::arg("ff"),
              python::arg("numThreads") = 1, python::arg("maxIters") = 200),
              docString.c_str());
}
