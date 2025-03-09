// Copyright 2023 Dassault Systemes Simulia Corp.

#ifndef SMABasSrmResourceEnums_h                       
#define SMABasSrmResourceEnums_h                     

/////////////////////////////////////
//  This is generated source code  //
//     DO NOT MODIFY DIRECTLY      //
//     ----------------------      //
// Generator: srm_ResourceSetup.pl //
/////////////////////////////////////
// Begin local includes              
// End local includes              

enum srm_ResourceEnums
{                           
  e_res_NULL = -1, // reserved keyword
  e_res_simmeta = 0, // sim
  e_res_simindexsets = 1, // sim
  e_res_simdist = 2, // sim
  e_res_adbmeta = 3, // adb meta data
  e_res_adbbulkndb = 4, // adb node database
  e_res_adbmodelndb = 5, // adb node model database          
  e_res_adbbulkeldb = 6, // adb element database
  e_res_adbmodeleldb = 7, // adb element model database
  e_res_adbcustpkg = 8, // adb custom package database
  e_res_adbreusable = 9, // adb reusable pool
  e_res_adbmdbedbreusable = 10, // adb reusable pool for modelDb in ElementDb
  e_res_adbmdbndbreusable = 11, // adb reusable pool for modelDb in NodeDb
  e_res_dbslnew = 12, // New Short list database
  e_res_dblegacy = 13, // Other legacy databases
  e_res_sint = 14, // sint 
  e_res_ebeoperator = 15, // ebe operator
  e_res_ebeoperatorglobal = 16, // ebe operator global
  e_res_elemworkspace = 17, // element loop workspaces
  e_res_globworkspace = 18, // workspaces outside of element loop
  e_res_cachworkspace = 19, // several workspaces for one Id
  e_res_masssparsedata = 20, // mass sparsedata used in lnz
  e_res_asmoperatorbs = 21, // assembled operator before scatter
  e_res_asmoperatoras = 22, // assembled operator after scatter
  e_res_smpfactor = 23, // factorization pool in smp solver
  e_res_dmplocalfactor = 24, // local  factorization in dmp solver
  e_res_dmpremotefactor = 25, // remote factorization in dmp solver
  e_res_smpstack = 26, // local stack in smp solver
  e_res_dmplocalstack = 27, // local stack in dmp solver
  e_res_dmpremotestack = 28, // remote stack in dmp solver
  e_res_dmprhs = 29, // rhs in dmp solver
  e_res_nickname = 30, // ebe nickname
  e_res_ellag = 31, // element lagrange used in dst code
  e_res_nodegraph = 32, // node graph
  e_res_solvermisc = 33, // misc in-core, out-of-core in dmp solver
  e_res_contactdb = 34, // contact database
  e_res_metis = 35, // metis allocation // don't remove this pool!!!
  e_res_rdmmisc = 36, // rdmMemory->AllocationTemporary
  e_res_regularization = 37, // regularization
  e_res_bfgs = 38, // bfgs solver for quasi-newton solution technique
  e_res_tinytmp = 39, // only intended for temporary resources with small memory
  e_res_testResource1 = 40, // testResource1 // used in test program
  e_res_testResource2 = 41, // testResource2 // can be used for debugging
  e_res_testResource3 = 42, // testResource3 // purpose, avoid using in real code
  e_res_lnzmtxvectors = 43, // mtx-type vectors used in Lanczos
  e_res_lnzeigvectors = 44, // mtx-type eigenvectors
  e_res_lnzdvectors = 45, // double*-type vectors used in Lanczos
  e_res_mdeigenvectors = 46, // memory used for eigenvectors
  e_res_xsiPetsc = 47, // memory used by xsi PETSc
  e_res_xsiSolver = 48, // memory used by xsi solver implementation (e.g., Uzawa)
  e_res_xsiIface = 49, // memory used by xsi operators, partitioning etc.
  e_res_amsBwdStack = 50, // memory used for bwd red+update in ams eigensolver
  e_res_householder = 51, // householder eigensolver
  e_res_amsRedShort = 52, // short-lived ams phase 3 data (Ma)
  e_res_amsRedMedium = 53, // short-lived ams phase 3 data (psi,phi)
  e_res_amsRedLong = 54, // long-lived ams phase 3 data (Da)
  e_res_amsDistilOpers = 55, // ams distilled operators
  e_res_amsModesA = 56, // eigenmodes after distillation
  e_res_amsMassA = 57, // mass after distillation
  e_res_amsTransA = 58, // transformation matrix
  e_res_amsModesG = 59, // global eigen modes
  e_res_amsModalOperators = 60, // projected operators
  e_res_output = 61, // pool used for processing bulk output
  e_res_preprocessor = 62, // preprocessor memory exclusive to Elaboration and Package
  e_res_dmpcomm = 63, // pool used for dmp data communication
  e_res_user = 64, // pool for user allocations
  e_res_smallblock = 65, // pool for small block allocations
  e_res_sharedOperator = 66, // solver operator in shared memory
  e_res_sharedSolution = 67, // solver solution in shared memory
  e_res_default = 68, // default
  e_res_numResources = 69,
  e_res_numPurgeables = 47     
};                                        
                                          
// Lookup poolID by enum                  
static int srm_PoolId (srm_ResourceEnums e)       
{                                         
    switch(e) {                           
	    case e_res_simmeta:            return  0; // sim
	    case e_res_simindexsets:       return  1; // sim
	    case e_res_simdist:            return  2; // sim
	    case e_res_adbmeta:            return  3; // adb meta data
	    case e_res_adbbulkndb:         return  4; // adb node database
	    case e_res_adbmodelndb:        return  5; // adb node model database          
	    case e_res_adbbulkeldb:        return  6; // adb element database
	    case e_res_adbmodeleldb:       return  7; // adb element model database
	    case e_res_adbcustpkg:         return  8; // adb custom package database
	    case e_res_adbreusable:        return  9; // adb reusable pool
	    case e_res_adbmdbedbreusable:  return 10; // adb reusable pool for modelDb in ElementDb
	    case e_res_adbmdbndbreusable:  return 11; // adb reusable pool for modelDb in NodeDb
	    case e_res_dbslnew:            return 12; // New Short list database
	    case e_res_dblegacy:           return 13; // Other legacy databases
	    case e_res_sint:               return 14; // sint 
	    case e_res_ebeoperator:        return 15; // ebe operator
	    case e_res_ebeoperatorglobal:  return 16; // ebe operator global
	    case e_res_elemworkspace:      return 17; // element loop workspaces
	    case e_res_globworkspace:      return 18; // workspaces outside of element loop
	    case e_res_cachworkspace:      return 19; // several workspaces for one Id
	    case e_res_masssparsedata:     return 20; // mass sparsedata used in lnz
	    case e_res_asmoperatorbs:      return 21; // assembled operator before scatter
	    case e_res_asmoperatoras:      return 22; // assembled operator after scatter
	    case e_res_smpfactor:          return 23; // factorization pool in smp solver
	    case e_res_dmplocalfactor:     return 24; // local  factorization in dmp solver
	    case e_res_dmpremotefactor:    return 25; // remote factorization in dmp solver
	    case e_res_smpstack:           return 26; // local stack in smp solver
	    case e_res_dmplocalstack:      return 27; // local stack in dmp solver
	    case e_res_dmpremotestack:     return 28; // remote stack in dmp solver
	    case e_res_dmprhs:             return 29; // rhs in dmp solver
	    case e_res_nickname:           return 30; // ebe nickname
	    case e_res_ellag:              return 31; // element lagrange used in dst code
	    case e_res_nodegraph:          return 32; // node graph
	    case e_res_solvermisc:         return 33; // misc in-core, out-of-core in dmp solver
	    case e_res_contactdb:          return 34; // contact database
	    case e_res_metis:              return 35; // metis allocation // don't remove this pool!!!
	    case e_res_rdmmisc:            return 36; // rdmMemory->AllocationTemporary
	    case e_res_regularization:     return 37; // regularization
	    case e_res_bfgs:               return 38; // bfgs solver for quasi-newton solution technique
	    case e_res_tinytmp:            return 39; // only intended for temporary resources with small memory
	    case e_res_testResource1:      return 40; // testResource1 // used in test program
	    case e_res_testResource2:      return 41; // testResource2 // can be used for debugging
	    case e_res_testResource3:      return 42; // testResource3 // purpose, avoid using in real code
	    case e_res_lnzmtxvectors:      return 43; // mtx-type vectors used in Lanczos
	    case e_res_lnzeigvectors:      return 44; // mtx-type eigenvectors
	    case e_res_lnzdvectors:        return 45; // double*-type vectors used in Lanczos
	    case e_res_mdeigenvectors:     return 46; // memory used for eigenvectors
	    case e_res_xsiPetsc:           return 47; // memory used by xsi PETSc
	    case e_res_xsiSolver:          return 48; // memory used by xsi solver implementation (e.g., Uzawa)
	    case e_res_xsiIface:           return 49; // memory used by xsi operators, partitioning etc.
	    case e_res_amsBwdStack:        return 50; // memory used for bwd red+update in ams eigensolver
	    case e_res_householder:        return 51; // householder eigensolver
	    case e_res_amsRedShort:        return 52; // short-lived ams phase 3 data (Ma)
	    case e_res_amsRedMedium:       return 53; // short-lived ams phase 3 data (psi,phi)
	    case e_res_amsRedLong:         return 54; // long-lived ams phase 3 data (Da)
	    case e_res_amsDistilOpers:     return 55; // ams distilled operators
	    case e_res_amsModesA:          return 56; // eigenmodes after distillation
	    case e_res_amsMassA:           return 57; // mass after distillation
	    case e_res_amsTransA:          return 58; // transformation matrix
	    case e_res_amsModesG:          return 59; // global eigen modes
	    case e_res_amsModalOperators:  return 60; // projected operators
	    case e_res_output:             return 61; // pool used for processing bulk output
	    case e_res_preprocessor:       return 62; // preprocessor memory exclusive to Elaboration and Package
	    case e_res_dmpcomm:            return 63; // pool used for dmp data communication
	    case e_res_user:               return 64; // pool for user allocations
	    case e_res_smallblock:         return 65; // pool for small block allocations
	    case e_res_sharedOperator:     return 66; // solver operator in shared memory
	    case e_res_sharedSolution:     return 67; // solver solution in shared memory
	    case e_res_default:            return 68; // default
	    return 0; // default case
    }           
    return 0;   
}             

#endif // SMABasSrmResourceEnums_h
