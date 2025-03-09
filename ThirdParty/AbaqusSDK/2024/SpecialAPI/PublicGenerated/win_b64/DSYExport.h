#ifndef DSYExport_h
#define DSYExport_h

#ifdef DSYLocal
# undef DSYLocal
#endif /* DSYLocal */

#ifdef DSYExport
# undef DSYExport
#endif /* DSYExport */

#ifdef DSYImport
# undef DSYImport
#endif /* DSYImport */

#define DSYLocal
#define DSYExport __declspec(dllexport)
#define DSYImport __declspec(dllimport)

#endif /* DSYExport_h */
