#ifndef _GRINTELSS_H_
#define _GRINTELSS_H_
#ifdef _WIN32
#include "boinc_win.h"
#else
#include <math.h>
#endif
#include <string>
#include <vector>

// usual boinc includes
#include "app_ipc.h"
#include "filesys.h"
#include "boinc_gl.h"
#include "gutil.h"
#include "graphics2.h"

// FTGL OpenGL TrueType/FreeType font rendering
#include <FTGL/ftgl.h>

using std::vector;

#ifndef M_PI
#define M_PI 3.1415926536f
#endif

#ifndef _MAX_PATH
#define _MAX_PATH 255
#endif

#define NUM_FONT          1
#define NUM_TEXTURE       4
#define NUM_MASK          0

/* g_iFont array offsets
#define TTF_INTEL_LT   0 
#define TTF_INTEL_NRM  1 
#define TTF_INTEL_MED  2 
#define TTF_ARIAL_N    3 
#define TTF_ARIAL_R    4 
#define TTF_INTERSTATE 5 
*/

#define GRADIENT_SIZE  1.7f

#define TTF_INTEL_MED  0

#define TEXTURE_INTEL     0
#define TEXTURE_GR        1
#define TEXTURE_PROJECT   2
#define TEXTURE_TOTAL     3

//gluDisk settings
#define DISK_SLICE 50
#define DISK_LOOP  1

// show up to 6 project "arcs"
#define MAX_PROJECTS      6

#define MAX_DISPLAY_STRLEN 30

#define ARC_OFFSET  2.f
#define GHR_CREDIT_FACTOR 0.24f

// hit test constants
#define HIT_NONE   0
#define HIT_TOP    1
#define HIT_BOTTOM 2
#define HIT_LEFT   3
#define HIT_RIGHT  4
#define HIT_CIRCLE 5
#define HIT_INTEL  6
#define HIT_GR     7
#define HIT_PROGRESS 8

#define HIT_RETRY 10

#define BOINC_GRAPHICS_MAX_FPS 20.0f
#define BOINC_GRAPHICS_MAX_CPU .30f

#define MIN_SPEED     (.0050f / BOINC_GRAPHICS_MAX_FPS)
#define NORMAL_SPEED  (.0100f / BOINC_GRAPHICS_MAX_FPS)
#define MAX_SPEED     (.0200f / BOINC_GRAPHICS_MAX_FPS)

namespace grint {
    extern unsigned int g_iProjectDisplayCount;
    extern unsigned int g_iProjectTotalCount;
	extern float g_fArcLength;
	extern float g_fTextWidth;
	extern float g_fFontScale;
	extern float g_fSweepOffset;
	extern int g_iFont;
	extern int BigFudge, LittleFudge;
	//extern bool g_bDataUpdated;
	extern bool bBOINCRunning;
	
	extern float g_fAspect;
	extern float g_fWidth;
	extern float g_fHeight;
	
	extern void graphics_init();
	extern int graphics_resize(const int& w, const int& h);
	extern int graphics_render(const int& xs, const int& ys, const double& t, const double& alpha);

	extern int loadTextures(const char* strPath = NULL);	
	extern int updateData(bool bDemo = false);
	extern void initVars();
	extern void cleanup();
	
	extern void ttf_load_fonts(const char* dir = NULL);
				
	extern void ttf_render_string(
						   const double& alpha_value,
						   // reference value to which incoming alpha values are compared.
						   // 0 through to 1
						   const double& x, 
						   const double& y, 
						   const double& z, // text position
						   const float& fscale,                 // scale factor
						   const GLfloat * col,                // colour 
						   const int& iFont,                        // font index 
						   const char* s,				  	  // string ptr
						   const float& fRotAngle = 0.0f,        // optional rotation angle
						   const float& fRotX = 0.0f,            // optional rotation vector for X
						   const float& fRotY = 0.0f,            // optional rotation vector for Y
						   const float& fRotZ = 1.0f,            // optional rotation vector for Z
						   const float& fRadius = 0.0f           // circular radius to draw along
						   );

}

#endif  // _GRINTELSS
