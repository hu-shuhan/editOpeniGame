#pragma once
#ifndef _DEFINETYPE_H_
#define _DEFINETYPE_H_

enum CURVETYPE
{
	LINE = 1,			  
	BSPLINECURVE,         
	BEZIERCURVE,           
	CIRCLE,               
	ARC                  
	
};

enum SURFACETYPE
{
	PLANE = 1,            
	BSPLINESURFACE,       
	BEZIERSURFACE,        
	CYLINDRICALSURFACE,    
	LINESURFACE,          
	REVOSURFACE,          
	SPHERE                
};


#endif  
