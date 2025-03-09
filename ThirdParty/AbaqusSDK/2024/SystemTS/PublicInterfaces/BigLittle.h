

// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/



#ifndef BigLittle
	//-----------------------------------------------------------------
	// Macro specifique Windows
	//-----------------------------------------------------------------

   // Traduction Big Endian Little Endian

#define BigLittle(buffer,length,basictype)      \
if (length != 0 ) {                             \
  int _idx1_ = 0;                               \
  for ( ; _idx1_ < length ; _idx1_ += sizeof(basictype)) { \
    char * _begin_ = (char*)(((char*)buffer)+_idx1_);      \
    char * _end_ =  _begin_ + sizeof(basictype) - 1 ;      \
    int  _idx2_ = 0;                                       \
    for ( ; _idx2_ < sizeof(basictype)/2 ; _idx2_++) {     \
      char _tmp_   = *_begin_;     \
      *(_begin_++) = *_end_;       \
      *(_end_--)   = _tmp_;        \
    }                          \
  }                            \
}


/* API sur big/little endian
   ************************* */

/* Explications
   ============ */

/* Problematique
   ------------- */

// Les divers possibilites de ranger les octets composant un mot machine amenent a distinguer
// les processeurs selon le mode rangement.

// Les 2 manieres de ranger les octets composant un mot machine de 16 bits sont :

//    Octet-addresse-0 | Octet-addresse-1
//   ------------------|------------------
// 1) Poid fort        | Poid faible
// 2) Poid faible      | Poid fort

// Les 2 manieres les plus repandues de ranger les octets composant un mot machine de 32 bits sont :

//    Octet-addresse-0 | Octet-addresse-1 | Octet-addresse-2 | Octet-addresse-3
//   ------------------|------------------|------------------|------------------
// 1) Poid tres fort   | Poid fort        | Poid faible      | Poid tres faible
// 2) Poid tres faible | Poid faible      | Poid fort        | Poid tres fort

// Cette dichotomie des processeurs a inspire certain a parler de sexe d'un processeur, sans preciser
// qui est male de qui est femelle. Une explication peu convaincante prouve qu'il existe d'autres possibilites
// de ranger les octets des mots de 32 bits ... je propose ce sujet sans fondement aux specialistes du genre.

// Le vocable precedant est plus croustillant mais moins epique que les petits et les grands indiens.
// Le rangement 1 est appele big    endian et caracterise les processeurs SPARC de SUN, 68000 de Motorola, etc.
// Le rangement 2 est appele little endian et caracterise les processeurs Pentium d'Intel, Alpha de Compaq, etc.

// L'adjectif "big" est a relier au poid "fort" en tete de mot, notion male par excellence,
// et complementairement, l'adjectif "little" est a relier au poid "faible" en tete de mot ...

// Quand aux indiens ici present, ils font parti d'un episode des aventures de Gulliver de Swift ou
// divers personnages cherchent a savoir par quel bout on ouvre un oeuf cuit dur.
// Cette question crutiale a servi de comparaison, sous forme de boutade, a Danny Cohen dans un article
// du 1 Avril 1980, a l'autre question existentielle a propos du sexe des processeurs.
// Comme cette question a ete qualifiee d'"endian", ce qualificatif est reste parmi les informaticiens
// lorsqu'ils cherchent a savoir ce qu'il faut faire quand on manipule des donnees membres entre processeurs.

// Notons le cas d'espece "bi-endian" ou certains processeurs peuvent fonctionner en little ou en big endian.
// Les processeurs hermaphrodites existent, par exemple le PA-RISC d'HP, MIPS de SGI, le PowerPC, etc.

// Est-ce que l'humour gaulois, des Jacky de base, peut compenser le fait de ne pas etre des anges ? Bof !

/* Specifications
   -------------- */

// Existence de 2 flags de compilation :
// _ENDIAN_LITTLE : pour les processeurs en little endian
// _ENDIAN_BIG    : pour les processeurs en big    endian

// Il est possible d'utiliser ces flags uniquement dans les cas non adresses par l'API.

// Caracteristique de l'API :
// - traiter les donnees sur 16, 32 et 64 bits
// - traiter une donnee elementaire, traiter un tableau de donnee elementaire
// - conversion statique de little vers big endian et vis versa
// - conversion dynamique en fonction d'un flag et du processeur
// - soin apporter aux performances (en particulier, toute l'API est "inline")
// - la donnee passee en argument pour chaque fonction de l'API est modifiee si necessaire

// Type des donnees pour les processeurs 32 et 64 bits et quelque soit le systeme d'exploitation :
// - 16 bits : short
// - 32 bits : int
// - 64 bits : long long
// - le type long a une implementation variable, a plutot procrire

// Pour la conversion statique de big vers little endian,
// la donnee source est en big endian et
// si le processeur est big    endian, rien n'est fait, la donnee n'est donc pas modifiee
// si le processeur est little endian, la conversion du big vers little endian est effectuee
// ce cas correspond par exemple a des donnees purement SUN a relire sur n'importe quelle machine dont Windows

// Pour la conversion statique de little vers big endian,
// la donnee source est en little endian et
// si le processeur est little endian, rien n'est fait, la donnee n'est pas modifiee
// si le processeur est big    endian, la conversion du little vers big endian est effectuee
// ce cas correspond par exemple a des donnees purement Windows a relire sur n'importe quelle machine dont AIX

// Pour la conversion dynamique,
// la donnee source est en big endian si le flag vaut 1 ou en little endian si le flag vaut 0, et
// si le processeur   a     le meme sexe que la donnee, rien n'est fait, la donnee n'est pas modifiee
// si le processeur n'a pas le meme sexe, la conversion est effectuee, la donnee est donc modifiee
// la conversion dynamique ne doit etre utilisee que si l'on connait uniquement au run-time le sexe de la donnee
// sinon la conversion statique suffit et est plus efficace.

/* Fonctions de conversion big <--> little endian
   ============================================== */

/* Fonction de swap de 2 octets dont on a l'adresse
   ------------------------------------------------ */


#define Endian__SwapAt(adr, o1, o2) \
    do {                            \
      char endian__c=adr[o1];       \
      adr[o1]=adr[o2];              \
      adr[o2]=endian__c;            \
    } while(0)

/* Fonction de swap de 2 octets
   ---------------------------- */

#define Endian__Swap(datum, o1, o2) Endian__SwapAt(((char *)(&(datum))), o1, o2)

/* Fonction de conversion d'un mot de 16 bits
   ------------------------------------------ */

#define Endian_16(f, datum) \
    f(datum, 0, 1)

/* Fonction de conversion d'un mot de 32 bits
   ------------------------------------------ */

#define Endian_32(f, datum) \
    f(datum, 0, 3);         \
    f(datum, 1, 2)

/* Fonction de conversion d'un mot de 64 bits
   ------------------------------------------ */

#define Endian_64(f, datum) \
    f(datum, 0, 7);         \
    f(datum, 1, 6);         \
    f(datum, 2, 5);         \
    f(datum, 3, 4)

/* Fonction de conversion d'un tableau
   ----------------------------------- */

#define Endian__Array(adr, n, b, f)     \
    do {                                \
        char *endian__p=(char *)(adr);  \
        for (int endian__i=0; endian__i<n; endian__i++, endian__p+=b) { \
            f(Endian__SwapAt, endian__p);                               \
        }                                                               \
    }\
    while(0);

/* Fonction de conversion d'un tableau de mot de 16 bits
   ----------------------------------------------------- */

#define Endian_Array16(adr, n) Endian__Array(adr, n, 2, Endian_16)

/* Fonction de conversion d'un tableau de mot de 32 bits
   ----------------------------------------------------- */

#define Endian_Array32(adr, n) Endian__Array(adr, n, 4, Endian_32)

/* Fonction de conversion d'un tableau de mot de 64 bits
   ----------------------------------------------------- */

#define Endian_Array64(adr, n) Endian__Array(adr, n, 8, Endian_64)

/* Definition des flags statiques
   ============================== */

/* Definition du flag _ENDIAN_LITTLE
   --------------------------------- */

#if defined(_LINUX_SOURCE) || defined(_WINDOWS_SOURCE) || defined(_DARWIN_SOURCE) || defined (_MACOSX_SOURCE) || defined(_ANDROID_SOURCE)
#ifndef _ENDIAN_LITTLE
#define _ENDIAN_LITTLE
#endif

/* Definition du flag _ENDIAN_BIG
   ------------------------------ */

#elif defined(_IRIX_SOURCE) || defined(_AIX) || defined(_HPUX_SOURCE)
#ifndef _ENDIAN_BIG
#define _ENDIAN_BIG
#endif

/* Sous Sun cela dépend du type du processeur 
   ------------------------------------------ */
#elif defined(_SUNOS_SOURCE)
#if defined(__i386) || defined(__amd64)
#ifndef _ENDIAN_LITTLE
#define _ENDIAN_LITTLE
#endif
#else
#ifndef _ENDIAN_BIG
#define _ENDIAN_BIG
#endif
#endif

/* Erreur de compilation si OS/processeur inconnu (aide au portage)
   ---------------------------------------------------------------- */

#else
#error "Unkown Operating System for Big or Little Endian Flag Definition."
#endif

/* Conversion statique de big endian vers little endian : cas d'une machine big endian
   =================================================================================== */

#ifdef _ENDIAN_BIG

#define Endian_BigToLittle16(datum)
#define Endian_BigToLittle32(datum)
#define Endian_BigToLittle64(datum)

#define Endian_BigToLittleArray16(datum, n)
#define Endian_BigToLittleArray32(datum, n)
#define Endian_BigToLittleArray64(datum, n)

#endif

/* Conversion statique de big endian vers little endian : cas d'une machine little endian
   ====================================================================================== */

#ifdef _ENDIAN_LITTLE

/* Conversion statique de big endian vers little endian d'une donnee elementaire sur 16 bits
   ----------------------------------------------------------------------------------------- */

#define Endian_BigToLittle16(datum) Endian_16(Endian__Swap, datum)

/* Conversion statique de big endian vers little endian d'une donnee elementaire sur 32 bits
   ----------------------------------------------------------------------------------------- */

#define Endian_BigToLittle32(datum) Endian_32(Endian__Swap, datum)

/* Conversion statique de big endian vers little endian d'une donnee elementaire sur 64 bits
   ----------------------------------------------------------------------------------------- */

#define Endian_BigToLittle64(datum) Endian_64(Endian__Swap, datum)

/* Conversion statique de big endian vers little endian d'un tableau de donnees elementaires sur 16 bits
   ----------------------------------------------------------------------------------------------------- */

#define Endian_BigToLittleArray16(datum, n) Endian_Array16(datum, n)

/* Conversion statique de big endian vers little endian d'un tableau de donnees elementaires sur 32 bits
   ----------------------------------------------------------------------------------------------------- */

#define Endian_BigToLittleArray32(datum, n) Endian_Array32(datum, n)

/* Conversion statique de big endian vers little endian d'un tableau de donnees elementaires sur 64 bits
   ----------------------------------------------------------------------------------------------------- */

#define Endian_BigToLittleArray64(datum, n) Endian_Array64(datum, n)

#endif

/* Conversion statique de little endian vers big endian : cas d'une machine little endian
   ====================================================================================== */

#ifdef _ENDIAN_LITTLE

#define Endian_LittleToBig16(datum)
#define Endian_LittleToBig32(datum)
#define Endian_LittleToBig64(datum)

#define Endian_LittleToBigArray16(datum, n)
#define Endian_LittleToBigArray32(datum, n)
#define Endian_LittleToBigArray64(datum, n)

#endif

/* Conversion statique de little endian vers big endian : cas d'une machine big endian
   =================================================================================== */

#ifdef _ENDIAN_BIG

/* Conversion statique de little endian vers big endian d'une donnee elementaire sur 16 bits
   ----------------------------------------------------------------------------------------- */

#define Endian_LittleToBig16(datum) Endian_16(Endian__Swap, datum)

/* Conversion statique de little endian vers big endian d'une donnee elementaire sur 32 bits
   ----------------------------------------------------------------------------------------- */

#define Endian_LittleToBig32(datum) Endian_32(Endian__Swap, datum)

/* Conversion statique de little endian vers big endian d'une donnee elementaire sur 64 bits
   ----------------------------------------------------------------------------------------- */

#define Endian_LittleToBig64(datum) Endian_64(Endian__Swap, datum)

/* Conversion statique de little endian vers big endian d'un tableau de donnees elementaires sur 16 bits
   ----------------------------------------------------------------------------------------------------- */

#define Endian_LittleToBigArray16(datum, n) Endian_Array16(datum, n)

/* Conversion statique de little endian vers big endian d'un tableau de donnees elementaires sur 32 bits
   ----------------------------------------------------------------------------------------------------- */

#define Endian_LittleToBigArray32(datum, n) Endian_Array32(datum, n)

/* Conversion statique de little endian vers big endian d'un tableau de donnees elementaires sur 64 bits
   ----------------------------------------------------------------------------------------------------- */

#define Endian_LittleToBigArray64(datum, n) Endian_Array64(datum, n)

#endif

/* Conversion dynamique de donnee
   ============================== */

/* Conversion dynamique d'une donnee elementaire sur 16 bits
   --------------------------------------------------------- */

#ifdef _ENDIAN_BIG
#define Endian_Dynamic16(flag, datum) if (!(flag)) {Endian_16(Endian__Swap, datum);}
#endif

#ifdef _ENDIAN_LITTLE
#define Endian_Dynamic16(flag, datum) if (flag)    {Endian_16(Endian__Swap, datum);}
#endif

/* Conversion dynamique d'une donnee elementaire sur 32 bits
   --------------------------------------------------------- */

#ifdef _ENDIAN_BIG
#define Endian_Dynamic32(flag, datum) if (!(flag)) {Endian_32(Endian__Swap, datum);}
#endif

#ifdef _ENDIAN_LITTLE
#define Endian_Dynamic32(flag, datum) if (flag)    {Endian_32(Endian__Swap, datum);}
#endif

/* Conversion dynamique d'une donnee elementaire sur 64 bits
   --------------------------------------------------------- */

#ifdef _ENDIAN_BIG
#define Endian_Dynamic64(flag, datum) if (!(flag)) {Endian_64(Endian__Swap, datum);}
#endif

#ifdef _ENDIAN_LITTLE
#define Endian_Dynamic64(flag, datum) if (flag)    {Endian_64(Endian__Swap, datum);}
#endif

/* Conversion dynamique d'un tableau de donnees elementaires sur 16 bits
   --------------------------------------------------------------------- */

#ifdef _ENDIAN_BIG
#define Endian_DynamicArray16(flag, datum, n) if (!(flag)) {Endian_Array16(datum, n);}
#endif

#ifdef _ENDIAN_LITTLE
#define Endian_DynamicArray16(flag, datum, n) if (flag)    {Endian_Array16(datum, n);}
#endif

/* Conversion dynamique d'un tableau de donnees elementaires sur 32 bits
   --------------------------------------------------------------------- */

#ifdef _ENDIAN_BIG
#define Endian_DynamicArray32(flag, datum, n) if (!(flag)) {Endian_Array32(datum, n);}
#endif

#ifdef _ENDIAN_LITTLE
#define Endian_DynamicArray32(flag, datum, n) if (flag)    {Endian_Array32(datum, n);}
#endif

/* Conversion dynamique d'un tableau de donnees elementaires sur 64 bits
   --------------------------------------------------------------------- */

#ifdef _ENDIAN_BIG
#define Endian_DynamicArray64(flag, datum, n) if (!(flag)) {Endian_Array64(datum, n);}
#endif

#ifdef _ENDIAN_LITTLE
#define Endian_DynamicArray64(flag, datum, n) if (flag)    {Endian_Array64(datum, n);}
#endif

#endif
