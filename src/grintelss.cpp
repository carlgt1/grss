/*
  The main file (well not with the actual main() - that's in ss_app.cpp) for the GridRepublic / Intel Screensaver for BOINC
 
  started by Carl Christensen  May 26th, 2009

 */

#include "grintelss.h"
#include "gui_rpc_client.h"

// for winsock closing on atexit()
#ifdef _WIN32
  #include "network.h"
#endif

//#include <locale.h>  // used for comma-thousands separator formatting if you really want

#include "util.h"  // boinc utilities
#include "graphics2.h" // for boinc_max_fps & cpu_frac

//vars from ss_app.cpp
extern CC_STATE  cc_state;
extern CC_STATUS cc_status;
extern HOST_INFO cc_host_info;
extern RPC_CLIENT rpc;
extern bool retry_connect; 
extern bool connected;
extern double next_connect_time;

static FTFont* g_font[NUM_FONT];

// note the use of the mask:  white (1) blocks out original image, black (0) let's through, so can draw fancier shapes than rectangles! :-)

// logo order: intel, gr, project, total -- these are all jpeg's
static const char g_cstrTexture[NUM_TEXTURE][16] = { "intel.rgb",      "gr.rgb",      "project.jpg",        "total.jpg" };
static const char g_cstrBackground[] = {"gradient.jpg"};

static const char g_cstrMask[NUM_TEXTURE][18]       = { "", "", "project_mask.jpg",   "total_mask.jpg"};

/*
static const char g_cstrFont[NUM_FONT][32]       = {
	"NeoSansIntelLight.ttf",
	"NeoSansIntel.ttf",
	"NeoSansIntelMedium.ttf",
	"ArialNarrow.ttf", 
	"ArialRoundedBold.ttf", 
	"InterstateRegular.ttf"
};
*/

static const char g_cstrFont[NUM_FONT][8]       = {
"simt",
};

static TEXTURE_DESC g_texture[NUM_TEXTURE];
static TEXTURE_DESC g_textureMask[NUM_TEXTURE];
static TEXTURE_DESC g_textureBackground;

static float g_sizeGradient[3] = { GRADIENT_SIZE, GRADIENT_SIZE, 0.0f };
static float g_posGradient[3];

static double g_dtime = dtime();  // render vars
static int g_xs;
static int g_ys;
static int g_iResetCircle = 0;

static const float g_posDefault[NUM_TEXTURE][3] = {
{0.50f, 0.50f, 0.00f},
{0.60f, 0.00f, 0.00f},
{.2f, .2f, .0f},
{.6f, .45f, .0f}
};

static float g_posLogo[NUM_TEXTURE][3] = { // logo order: intel, gr, project, total
{0.50f, 0.50f, 0.00f },
{0.60f, 0.00f, 0.00f },
{.2f, .4f, .0f },
{.8f, .2f, .0f}
};

static float g_move[NUM_TEXTURE][2] = {
{ .00f, .00f },
{ .00f, .00f },
{ .001f, .0008f },
{ -.0009f, -.001f }
}; // 'movement matrix'

static float g_sizeLogo[NUM_TEXTURE][3] = { // logo order: intel, gr, project, total
{ 0.08f, 0.08f, 0.0f }, 
{ 0.35f, 0.023333333f, 0.0f }, // gr logo is 1200x80
{ 0.21f, 0.21f, 0.0f },
{ 0.16f, 0.16f, 0.0f }
};

static float g_fRotate[NUM_TEXTURE];
static bool g_bUpdating = false; // flag when we're updating data

static const GLfloat g_cfcolor_background[3][4] =  
{
  { 0.0f/255.0f, 157.0f/255.0f, 222.0f/255.0f, 1.0f },
  { 40.0f/255.0f, 69.0f/255.0f, 151.0f/255.0f, 1.0f },
  { 22.0f/255.0f, 32.0f/255.0f, 103.0f/255.0f, 1.0f }
};

static GLfloat g_transColor[3] = { 255, 0, 255 };  // this should be magenta - red = 255, green = 0, blue = 255

static GLfloat g_fArcAlpha = .90f;
static GLfloat g_alpha = 1.0f;
static GLfloat g_alphaMax = 0.0f;
static const GLfloat ALPHA_ERROR = .30f;

static GLfloat g_cfcolor_lines[4]      =  { 91.0f/255.0f, 194.0f/255.0f, 242.0f/255.0f, .5f };

//static GLfloat g_cfcolor_extremelylightblue[4] = { 80.0f/255.0f, 164.0f/255.0f, 220.0f/255.0f, .20f };
static GLfloat g_cfcolor_verylightblue[4] =     { 91.0f/255.0f, 194.0f/255.0f, 242.0f/255.0f, .20f };
static GLfloat g_cfcolor_notsoverylightblue[4] = { 4.0f/255.0f, 159.0f/255.0f, 220.0f/255.0f, g_fArcAlpha };
static GLfloat g_cfcolor_lightblue[4] =         { 60.0f/255.0f, 138.0f/255.0f, 201.0f/255.0f, g_fArcAlpha };
static GLfloat g_cfcolor_darkblue[4] =           { 9.0f/255.0f, 43.0f/255.0f,  135.0f/255.0f, g_fArcAlpha };
static GLfloat g_cfcolor_verydarkblue[4] =       { 4.0f/255.0f, 21.0f/255.0f,   60.0f/255.0f, g_fArcAlpha };

static GLfloat g_cfcolor_totalinnerring[4] =     { 3.f/255.0f, 195.0f/255.0f, 251.0f/255.0f, g_fArcAlpha-.20f};
static GLfloat g_cfcolor_totalouterring[4] =     {0.0f/255.0f, 159.0f/255.0f, 223.0f/255.0f, g_fArcAlpha-.40f};

static GLfloat g_cfcolor_black[4] = { 0.0f,0.0f,0.0f,1.0f };
static GLfloat g_cfcolor_white[4] = { 1.0f,1.0f,1.0f,1.0f };
//static GLfloat g_cfcolor_red[4] = { 1.0f,0.0f,0.0f,1.0f };
//static GLfloat g_cfcolor_green[4] = { 0.0f,1.0f,0.0f,1.0f };
//static GLfloat g_cfcolor_blue[4] = { 0.0f,0.0f,1.0f,1.0f };
//static GLfloat g_cfcolor_grey[4] = { 160.0f/255.0f, 167.0f/255.0f, 170.0f/255.0f, .7f };

// order is intel, gr, project, total
static const float g_cfRadiusMax[NUM_TEXTURE]         = { 0.0f, 0.0f, 0.230f, 0.120f };  // note min size is halfway between radius max & radius min
static const float g_cfRadiusMin[NUM_TEXTURE]         = { 0.0f, 0.0f, 0.110f, 0.080f };
static const float g_cfRadiusOuterOffset[NUM_TEXTURE] = { 0.0f, 0.0f, 0.065f, 0.020f };    // if g_cfRadiusMax is updated above you need to add the difference to these three lines
static const float g_cfRadiusInnerOffset[NUM_TEXTURE] = { 0.0f, 0.0f, 0.100f, 0.030f };
static const float g_cfRadiusMsgOffset[NUM_TEXTURE]   = { 0.0f, 0.0f, 0.070f, 0.020f };
static const float g_cfRadiusTimeOffset[NUM_TEXTURE]  = { 0.0f, 0.0f, 0.100f, 0.073f };

static float g_fRadiusMaxCurrent[NUM_TEXTURE] = { 0.0f, 0.0f, g_cfRadiusMax[TEXTURE_PROJECT], (2.0f * g_cfRadiusMax[TEXTURE_TOTAL]) - g_cfRadiusMin[TEXTURE_TOTAL] };  // keep track of current max radius for collision detection

static long g_frame = 0L;

// 6 project colors - red, orange, blue, dark blue, very dark, light blue (for totals supposedly)
// these color values obtained by using Gimp color picker ("eyedropper" button) on the docs/grss_final.png image
static GLfloat g_cfcolor_proj[MAX_PROJECTS][4] = 
{
{98.0f/255.0f, 4.0f/255.0f, 137.0f/255.0f, g_fArcAlpha},
{255.0f/255.0f, 93.0f/255.0f, 1.0f/255.0f, g_fArcAlpha},
{229.0f/255.0f, 2.0f/255.0f, 43.0f/255.0f, g_fArcAlpha},
{0.0f/255.0f, 21.0f/255.0f, 54.0f/255.0f, g_fArcAlpha},
{0.0f/255.0f, 32.0f/255.0f, 126.0f/255.0f, g_fArcAlpha},
{30.0f/255.0f, 62.0f/255.0f, 154.0f/255.0f, g_fArcAlpha},
};

static GLfloat g_cfcolor_text[4] = {1.0f, 1.0f, 1.0f, 1.0f};


struct structProject {
  public:
	char strProject[MAX_DISPLAY_STRLEN+1];
	float fCredit;
	structProject() { memset(this, 0x00, sizeof(structProject)); };
};

static vector<structProject> vProject;
static float fCreditTotal;
static float fCreditMax;
static float g_gigaflops = 0.0f;
const unsigned int cmsgsize=128;
char g_strTimeUpdated[cmsgsize];
char g_strErrorMsg[2][cmsgsize];

namespace grint {   // "grint" is short for Grid Republic/Intel and also in honor of Rupert Grint from the Harry Potter stories! :-)

// current number of projects displayed (from gui rpc calls)
unsigned int g_iProjectDisplayCount = 0;
unsigned int g_iProjectTotalCount = 0;
unsigned int g_iProjectOffset = 0;
float g_fArcLength = 0.0f;
float g_fTextWidth = 1500.0f;
float g_fFontScale = 1.0f;
int g_iFont = -1;
float g_fSweepOffset = 0.0f;
float g_fSweepRadians = M_PI;
	
int BigFudge = 0;
int LittleFudge = 0;
		
float g_fAspect = 1.33333f;
float g_fWidth = 800.0f;
float g_fHeight = 600.0f;
static GLfloat g_xGL[2], g_yGL[2];  // opengl screen boundaries
static bool g_bViewportChanged = true; // flag when viewport changed, i.e. recompute g_xGL coords

//bool bDataUpdated = false;
bool bBOINCRunning = false;

void drawMaskedImage(const int Img, const float* pos, const float* size, const double& alpha, const bool& bMaskOnly = false);
void backgroundGradient();
void backgroundLines();
void drawProjectCircle();
void drawTotalCircle();
void drawText();	
void drawLogoIntel();
void drawLogoGR();
void drawLogoTotal();
void drawLogoProject();
void GetOpenGLScreenCoord(GLfloat xIn, GLfloat yIn, GLfloat& xOut, GLfloat& yOut); //, GLfloat& zOut);
void calcRotationAndMovement();
void makeTimestampString(); // from QCN; const double dtime, const char cType, char* strTime)
char *commaPrint(float fTest);
GLuint CreateRGBTransparentTexture(const char* strFileName, float* transColor = NULL);
GLuint CreateRGBAlpha(const char* strFileName);
	
inline void calcNewPositions(const int& iTexture);
inline int hitTest(const int& iTexture, const int& iOffset, const int& iLastHit);
inline void otherDirection(float* pfVal, const float& fRand, const int& iOffset);
inline float my_frand(long lSeed = 1L); // like the boinc drand but uses random()
bool setBOINCStatus();
	
#ifdef _WIN32
    TCHAR g_strInstallPath[_MAX_PATH]; // important -- the path with the resource files (jpg, rgb, truetype fonts etc), usually the BOINC Data path
	void GetRunTimeDirectory(TCHAR* strPath);
#else
    char g_strInstallPath[_MAX_PATH]; // important -- the path with the resource files (jpg, rgb, truetype fonts etc), usually the BOINC Data path
	void GetRunTimeDirectory(char* strPath);
#endif
	
void cleanup()
{
#ifdef _WIN32
		WinsockCleanup();
#endif		
    g_iFont = -1;
	g_iProjectDisplayCount = 0;

#if !defined(_DEBUG) && !defined(_WIN32)
	// the fonts seemed to get cleaned up on their own, at least in Windows debugger, which causes a crash here as they're already freed
	for (int i = 0; i < NUM_FONT; i++) {
		if (g_font[i]) delete g_font[i];
		g_font[i] = NULL;
	}
#endif
}
	
void resetCircles(bool bError = false)
{   
	memcpy(g_posLogo[TEXTURE_PROJECT], g_posDefault[TEXTURE_PROJECT], sizeof(float) * 6); // copies proj & total values
	if (bError) g_iResetCircle++;
	if (g_iResetCircle > 10) {
		// things are really screwed up, just use safe numbers from g_posDefault
		g_iResetCircle = 0;
		return;
	}
	
	// reset circles as it can get in a weird state i.e. offscreen due to the need for hitTest()
//	{.2f, .2f, .0f},   proj
//	{.6f, .45f, .0f}   total

	if (my_frand() > .50f) { // flipped
		g_posLogo[TEXTURE_PROJECT][0] = g_posDefault[TEXTURE_TOTAL][0]; // + (my_frand() * .10f);
		g_posLogo[TEXTURE_PROJECT][1] = g_posDefault[TEXTURE_TOTAL][1] - .10f; // + (my_frand() * .10f);
		g_posLogo[TEXTURE_TOTAL][0] = g_posDefault[TEXTURE_PROJECT][0]; // + (my_frand() * .05f);
		g_posLogo[TEXTURE_TOTAL][1] = g_posDefault[TEXTURE_PROJECT][1]; // + (my_frand() * .10f);
	}
	
/*  // this gets too crazy as the circles are big
    g_posLogo[TEXTURE_PROJECT][0] = g_posDefault[TEXTURE_PROJECT][0] + (my_frand() * .10f);
	g_posLogo[TEXTURE_PROJECT][1] = g_posDefault[TEXTURE_PROJECT][1] + (my_frand() * .10f);
	g_posLogo[TEXTURE_TOTAL][0] = g_posDefault[TEXTURE_TOTAL][0] + (my_frand() * .05f);
	g_posLogo[TEXTURE_TOTAL][1] = g_posDefault[TEXTURE_TOTAL][1] + (my_frand() * .10f);
*/	
	// setup random vectors?
    if (my_frand() > .75f) {
		g_move[TEXTURE_PROJECT][0] = -g_move[TEXTURE_PROJECT][0];
		g_move[TEXTURE_TOTAL][1] = -g_move[TEXTURE_TOTAL][1];
	}
    else if (my_frand() > .5f) {
		g_move[TEXTURE_PROJECT][0] = -g_move[TEXTURE_PROJECT][0];
		g_move[TEXTURE_TOTAL][0] = -g_move[TEXTURE_TOTAL][0];
		g_move[TEXTURE_PROJECT][1] = -g_move[TEXTURE_PROJECT][1];
		g_move[TEXTURE_TOTAL][1] = -g_move[TEXTURE_TOTAL][1];
	}
    else if (my_frand() > .25f) {
		g_move[TEXTURE_PROJECT][1] = -g_move[TEXTURE_PROJECT][1];
		g_move[TEXTURE_TOTAL][0] = -g_move[TEXTURE_TOTAL][0];
	}
}

void graphics_init()
{
/*
  no lighting in ortho mode
	// setup lighting
	GLfloat ambient[] = {1., 1., 1., 1.0};
	GLfloat position[] = {-0.0, 3.0, 20.0, 1.0};
	GLfloat dir[] = {-1, -.5, -3, 1.0};
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_POSITION, position);
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, dir);
*/
	
#ifdef _DEMO_BUILD
	grint::loadTextures();
#else
	grint::loadTextures(g_strInstallPath);  // load the textures -- this also takes an optional parameter for the path to the textures
#endif

    glClearColor(g_cfcolor_black[0],
				 g_cfcolor_black[1],
				 g_cfcolor_black[2],
				 g_cfcolor_black[3]);

	/*		
	glClearColor(g_cfcolor_extremelylightblue[0],
				 g_cfcolor_extremelylightblue[1],
				 g_cfcolor_extremelylightblue[2],
				 g_cfcolor_extremelylightblue[3]);
	*/	
	
	glClearDepth(1.0f);                     // Depth buffer setup
	glShadeModel(GL_FLAT);                // Enable smooth shading
}

#ifdef _WIN32
void GetRunTimeDirectory(TCHAR* strPath) 
#else
void GetRunTimeDirectory(char* strPath) 
#endif
{ // courtesy of Rom Walton, make sure strPath is _MAX_PATH i.e. 255 TCHARS
#ifdef _WIN32
		// change the current directory to the boinc install directory
	    ::GetModuleFileName(NULL, strPath, _MAX_PATH);
        
		CHAR *pszProg = strrchr(strPath, '\\');
		if (pszProg) {
			strPath[pszProg - strPath] = 0;
		}
#else // we can assume Apple, which has a fixed BOINC intall directory
	strcpy(strPath, "/Library/Application Support/BOINC Data");  // maybe need single quote ' around this string?
#endif
}
	
void initVars()
{ // this is called before app_graphics_init (graphics_init() here) - so don't do any OpenGL stuff yet
        // initialize the BOINC structs
      //  memset(&cc_state, 0x00, sizeof(CC_STATE));
      //  memset(&cc_status, 0x00, sizeof(CC_STATUS));
      //  memset(&cc_host_info, 0x00, sizeof(HOST_INFO));

	// set the frame rate & max cpu to give a good experience with the screensaver (no choppy animation):
	boinc_max_fps = BOINC_GRAPHICS_MAX_FPS;
	boinc_max_gfx_cpu_frac = BOINC_GRAPHICS_MAX_CPU;

	my_frand((long)g_dtime); // seed the random generator
	vProject.clear();
	fCreditTotal = 0.0f;
	g_iProjectDisplayCount = 0;
	g_iProjectOffset = 0;
	memset(g_strTimeUpdated, 0x00, sizeof(char) * cmsgsize);		
	memset(g_strInstallPath, 0x00, sizeof(char) * _MAX_PATH);
	GetRunTimeDirectory(g_strInstallPath);		
}
	
	
int graphics_resize(const int& w, const int& h)
{
	g_fWidth = (float) w;
	g_fHeight = (float) h;
	if (h > 0) g_fAspect = g_fWidth / g_fHeight;
	glViewport(0, 0, w, h);
	g_bViewportChanged = true;
	return 0;
}
	
// note the logo_fader for alpha values is in ss_app:render which is why I pass in alpha here
int graphics_render(const int& xs, const int& ys, const double& t, const double& alpha)
{
	g_dtime = t; // save params for random seeds etc
	g_xs = xs;
	g_ys = ys;

	glDisable( GL_DEPTH_TEST );
	
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// the whole thing is in 2D ortho mode!
	mode_unshaded();
	mode_ortho();
	
	///////////////////
	// status msg text -- also sets max alpha if required	
#ifndef _DEMO_BUILD	
	setBOINCStatus();
#endif
	
	// set appropriate alpha levels
	if (g_alphaMax > 0.0f) { //error condition, dim the lights!
		g_alpha = g_alphaMax >= alpha ? alpha : g_alphaMax;  // range should be from 0 to g_alphaMax
		g_cfcolor_text[3] = g_alphaMax;
		g_cfcolor_lines[3] = g_alphaMax - .27f;
		g_cfcolor_verylightblue[3] = .10f;
		g_cfcolor_notsoverylightblue[3] = g_alphaMax-.2f;
		g_cfcolor_lightblue[3] = g_alphaMax-.2f;
		g_cfcolor_darkblue[3] = g_alphaMax-.2f;
		g_cfcolor_verydarkblue[3] = g_alphaMax-.2f;
		g_cfcolor_totalinnerring[3] = g_alphaMax-.05f;
		g_cfcolor_totalouterring[3] = g_alphaMax-.1f;
		for (int i = 0; i < MAX_PROJECTS; i++)
			g_cfcolor_proj[i][3] = g_alphaMax;
	}
	else { // normal
		g_alpha = alpha;
		g_cfcolor_text[3] = 1.0f;
		g_cfcolor_lines[3] = .50f;
		g_cfcolor_verylightblue[3] = .20f;
		g_cfcolor_notsoverylightblue[3] = g_fArcAlpha;
		g_cfcolor_lightblue[3] = g_fArcAlpha;
		g_cfcolor_darkblue[3] = g_fArcAlpha;
		g_cfcolor_verydarkblue[3] = g_fArcAlpha;
		g_cfcolor_totalinnerring[3] = g_fArcAlpha-.20f;
		g_cfcolor_totalouterring[3] = g_fArcAlpha-.40f;
		for (int i = 0; i < MAX_PROJECTS; i++)
			g_cfcolor_proj[i][3] = g_fArcAlpha;
   }
	
	// compute screen boundaries if viewport changed
	if (g_bViewportChanged) {
	   GetOpenGLScreenCoord(0, 0, g_xGL[0], g_yGL[0]);  // left, bottom
	   GetOpenGLScreenCoord(g_fWidth, g_fHeight, g_xGL[1], g_yGL[1]);  // right, top
	   resetCircles(); // reset circles because screen may be "unfriendly" to our big circles
	   g_bViewportChanged = false;
	}
	
	// note: the order matters which objects get drawn & blended with the others - I give priority to text & logos (i.e. they should be "solid")
    backgroundLines();	
	backgroundGradient();
	if (!g_bUpdating) {
	  drawTotalCircle();
	  drawProjectCircle();	
	  drawText();
	  calcRotationAndMovement(); // rotate for next view
	}
	drawLogoIntel();
	drawLogoGR();

#ifndef _DEMO_BUILD	
	if (g_strErrorMsg[0][0] && g_iFont>-1) { // if string isn't null, we have an error msg!
		float fScale = 1200.f;
		//float x = ((g_xGL[1] - g_xGL[0])/(2.0f))
		float x = .5f - (g_font[g_iFont]->Advance(g_strErrorMsg[0], -1, FTPoint())/(fScale * 2.0f));
		float y = (g_yGL[0] - g_yGL[1]) / 2.0f;
		ttf_render_string(2.0f * g_alpha, x, y, 0, fScale, g_cfcolor_white, 0, g_strErrorMsg[0]);
                x = .5f - (g_font[g_iFont]->Advance(g_strErrorMsg[1], -1, FTPoint())/(fScale * 2.0f));
		ttf_render_string(2.0f * g_alpha, x, y-.1f, 0, fScale, g_cfcolor_white, 0, g_strErrorMsg[1]);
	}
#endif
	
	glDisable(GL_BLEND);

	glFlush();
	
	ortho_done();

	return 0;
}
	
void procPath(const char* strPath, char* strDir, const char* strInput = NULL, char* strFullPath = NULL)
{
#ifdef _WIN32
	char cSep = '\\';
#else
	char cSep = '/';
#endif
	
		if (strPath) { 
			strcpy(strDir, strPath); // path was given, just copy over
		}
		else {  // development -- texfonts in res (resource) directory
#ifdef __APPLE_CC__  
#ifndef _DEBUG
			strcpy(strDir, "../Resources");
#else
			strcpy(strDir, "../../res");
#endif
#else
			strcpy(strDir, "res");
#endif
		}
	
	if (strInput && strFullPath) {
		sprintf(strFullPath, "%s%c%s", strDir, cSep, strInput);
	}
		
}
	
int loadTextures(const char* strPath)
{
	char strDir[_MAX_PATH];
	char strFullPath[_MAX_PATH];
	int i;
	
	procPath(strPath, strDir);
	ttf_load_fonts(strDir);

	// the background jpg
	procPath(strPath, strDir, g_cstrBackground, strFullPath);
	if (boinc_file_exists(strFullPath) && !g_textureBackground.CreateTextureJPG(strFullPath)) g_textureBackground.present = true;

	// the texture rgb's - should be magenta (255/0/255) and flipped vertically
 	for (i = 0; i < NUM_TEXTURE; i++) {
		procPath(strPath, strDir, g_cstrTexture[i], strFullPath);
		if (i == TEXTURE_TOTAL || i == TEXTURE_PROJECT) { // use jpeg's
			if (boinc_file_exists(strFullPath) && !g_texture[i].CreateTextureJPG(strFullPath)) g_texture[i].present = true;
		}
		else  {
			if (boinc_file_exists(strFullPath) && (g_texture[i].id = CreateRGBTransparentTexture(strFullPath))) g_texture[i].present = true;
		}
	}

	// masks -- just for project & texture circles
	for (i = TEXTURE_PROJECT; i < NUM_TEXTURE; i++) { // use rgb's for mask
		procPath(strPath, strDir, g_cstrMask[i], strFullPath);
		//if (boinc_file_exists(strFullPath) && (g_textureMask[i].id = CreateRGBAlpha(strFullPath))) g_textureMask[i].present = true;
		if (boinc_file_exists(strFullPath) && !g_textureMask[i].CreateTextureJPG(strFullPath)) g_textureMask[i].present = true;
	}
	
	return 0;
}

/*
void drawCircle(float x, float y, float radius)
	{return;
		float y1=y+radius;
		float x1=x;
	    glShadeModel(GL_SMOOTH);                // Enable smooth shading

		glBegin(GL_TRIANGLE_FAN);
		for(double angle=0.0f;angle<=(2.0f*M_PI);angle+=0.1f)
		{
			double x2=x+(radius*sin(angle));
			double y2=y+(radius*cos(angle));
			glVertex2d(x1,y1);
			y1=y2;
			x1=x2;
		}
		glEnd();
}
*/
	
void backgroundGradient()
{ // should be a circular halo emanating from the desk/projects	
	if (!g_textureBackground.present) {
		return;
	}
	g_posGradient[0] = g_posLogo[TEXTURE_PROJECT][0] + g_sizeLogo[TEXTURE_PROJECT][0]/2.0f;  // current center of project circle
    g_posGradient[1] = g_posLogo[TEXTURE_PROJECT][1] + g_sizeLogo[TEXTURE_PROJECT][1]/2.0f;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	float pos[3] = { g_posGradient[0] - (g_sizeGradient[0]/2.0f), g_posGradient[1] - (g_sizeGradient[1]/2.0f), 0.0f };
	g_textureBackground.draw(pos, g_sizeGradient, ALIGN_BOTTOM, ALIGN_BOTTOM, g_alphaMax > 0.0f ? g_alphaMax + .2f :.93f);
}	
	
void backgroundLines()
{
	if (g_fWidth < 200.f || g_fHeight < 200.f) return;  // avoid ridiculous sizes for lines
	glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
	glColor4fv(g_cfcolor_lines);
			
	// have to take the aspect size into consideration to draw square grid
	const float fHeight = .006667f;  
	const float fWidth =  fHeight; 

	const float fFudge = g_sizeGradient[0] / 2.4f; // "radius" to draw lines
	
	float yval[2] = {g_posGradient[1] - fFudge, g_posGradient[1] + fFudge};
	if (yval[0] < g_yGL[1]) {
		yval[0] = g_yGL[1];
	}
	if (yval[1] > g_yGL[0]) {
		yval[1] = g_yGL[0];
	}

	float xval[2] = {g_posGradient[0] - fFudge, g_posGradient[0] + fFudge};
	if (xval[0] < g_xGL[0]) {
		xval[0] = g_xGL[0];
	}
	if (xval[1] > g_xGL[1]) {
		xval[1] = g_xGL[1];
	}

	float fExtent[2][2] = { { g_xGL[0], g_xGL[1] }, {g_yGL[1], g_yGL[0]} };	

	float yMin = 999.f, yMax = -999.f, xMin = 999.f, xMax = -999.f;
	int iCtrYm = 0, iCtrYM = 0, iCtrXm = 0, iCtrXM = 0;
	int xCtr = 1, yCtr = 1; // start here it lines up with intel logo

    // get bounding box of static points (i.e. not based on the moving g_posGradient but close to the points)
	for (float fv = fExtent[1][0]; fv <= fExtent[1][1]; fv+=fHeight) {  // horizontal moves by height/dy  -- don't forget the y's are flipped i.e. -1 is top
		if (++yCtr == 6) yCtr = 0; // thick line counter
		if (fv < yval[0] || fv > yval[1]) continue; // skip lines out of our range of interest		
		for (float fu = fExtent[0][0]; fu <= fExtent[0][1]; fu+=fWidth) { // vertical moves by width/dx
			if (++xCtr == 6) xCtr = 0; // thick line counter
			if (fu < xval[0] || fu > xval[1]) continue; // skip lines out of our range of interest
			// if we made it here then we have good points
			if (fv < yMin) {
				yMin = fv;
				iCtrYm = yCtr;
			}
			else if (fv > yMax) {
				yMax = fv;
				iCtrYM = yCtr;
			}
			if (fu < xMin) {
				xMin = fu;
				iCtrXm = xCtr;
			}
			else if (fu > xMax) {
				xMax = fu;
				iCtrXM = xCtr;
			}
		}
	}
	
	// OK, now we can draw...
	fExtent[0][0] = xMin; fExtent[0][1] = xMax; fExtent[1][0] = yMin; fExtent[1][1] = yMax;
	int iCtr = iCtrYm; // get the offset for the heavy line ctr
	bool bChange = false;
	glLineWidth(2);
	
	//float fCWidth = (fExtent[0][1] - fExtent[0][0])/2.0f;
	//float fCenter = (fExtent[0][1] + fExtent[0][0])/2.0f;
	//float fIter = (fExtent[1][1] - fExtent[1][0]) / fHeight;
	//float iTotal = 0.0f;
	for (float fv = fExtent[1][0]; fv <= fExtent[1][1]; fv+=fHeight) {  // horizontal moves by height/dy  -- don't forget the y's are flipped i.e. -1 is top
		if (++iCtr == 6) {
			iCtr = 0;
			bChange = true;
			glLineWidth(2);
		}

		//float fCalc = M_PI  * (fv - fExtent[1][0]) / (fExtent[1][1] - fExtent[1][0]);
		glBegin(GL_LINES);
		//glVertex2f(fCenter - (fCWidth * cos((-M_PI/2.0f) + M_PI * iTotal / fIter)), fv);
		//glVertex2f(fCenter + (fCWidth * cos((-M_PI/2.0f) + M_PI * (iTotal+1) / fIter)), fv);
		glVertex2f(fExtent[0][0], fv);
		glVertex2f(fExtent[0][1], fv);
		glEnd();
		if (bChange) { 
			glLineWidth(1);
			bChange = false;
		}
		//iTotal += 1.0f;
	}

	iCtr = iCtrXm;
	glLineWidth(1);
	//fCWidth = (fExtent[1][1] - fExtent[1][0])/2.0f;
	//fCenter = (fExtent[1][1] + fExtent[1][0])/2.0f;
	for (float fv = fExtent[0][0]; fv <= fExtent[0][1]; fv+=fWidth) {  // horizontal moves by height/dy  -- don't forget the y's are flipped i.e. -1 is top
		if (++iCtr == 6) {
			iCtr = 0;
			bChange = true;
			glLineWidth(2);
		}

  		//float fCalc = M_PI * .5f * (fv - fExtent[0][0]) / (fExtent[0][1] - fExtent[0][0]);
        glBegin(GL_LINES);
		//glVertex2f(fv, fCenter - (fCWidth * cos(fCalc)));
		//glVertex2f(fv, fCenter + (fCWidth * cos(fCalc)));
		glVertex2f(fv, fExtent[1][0]);
		glVertex2f(fv, fExtent[1][1]);
		glEnd();		
		if (bChange) { 
			glLineWidth(1);
			bChange = false;
		}
	}

}
	
void drawMaskedImage(const int Img, const float* pos, const float* size, const double& alpha, const bool& bMaskOnly)
{ // this will draw a "transparent bitmap" given a mask to block out (white (1) blocks out, black (0) let's through)
	// note the rgb mask (.rgb file) seems to be flipped vertically, so save it "upside-down" from the jpg original

	float npos[3], nsize[3];
	memcpy(npos, pos, sizeof(float) * 3);
	memcpy(nsize, size, sizeof(float) * 3);

	if (Img < NUM_TEXTURE && g_textureMask[Img].present) {
		// we need to draw a mask
		glBlendFunc(GL_DST_COLOR, GL_ZERO);
		g_textureMask[Img].draw(npos, nsize, ALIGN_BOTTOM, ALIGN_BOTTOM, alpha); 
		if (bMaskOnly) 
			glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE);  // just show a really faint image
		else
			glBlendFunc(GL_ONE, GL_ONE);
		memcpy(npos, pos, sizeof(float) * 3);   // copy back our positions
	}
	
	// image
	if (Img < NUM_TEXTURE && g_texture[Img].present) {
		   g_texture[Img].draw(npos, nsize, ALIGN_BOTTOM, ALIGN_BOTTOM, alpha); 
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

bool setBOINCStatus()   // based on boinc/clientscr/ss_app.cpp's show_jobs
{
	bool bOK = false;
		unsigned int nfound = 0;
		unsigned int i;
		cc_status.task_suspend_reason &= ~SUSPEND_REASON_CPU_USAGE_LIMIT;
		
		if (!cc_status.task_suspend_reason) {
			for (i=0; i<cc_state.results.size(); i++) {
				RESULT* r = cc_state.results[i];
				if (!r->active_task) {
					continue;
				}
				if (r->scheduler_state != CPU_SCHED_SCHEDULED) continue;
				//if (nfound == index) {
				//	txf_render_string(.1, x, y, 0, 1200., white, 0, "Running tasks:");
				//	y -= .05;
				//}
				//if (nfound >= index && nfound < index+4) {
				//	show_result(r, x, y, alpha);
				//	y -= .05;
				//}
				nfound++;
			}
		}
	
	    memset(g_strErrorMsg, 0x00, sizeof(char) * cmsgsize * 2);  // two lines!
		if (nfound) {
			bOK = true;
			g_alphaMax = 0.0f;
			//sprintf(strMsg, "%d task%srunning", nfound, nfound==1 ? " " : "s ");
		}
		else {
			g_alphaMax = ALPHA_ERROR;
			bOK = false;
			strncpy(g_strErrorMsg[0], "Progress Thru Processors is ", cmsgsize);
			if (!bBOINCRunning) {
				strncat(g_strErrorMsg[0],"not running.", cmsgsize);
				strncpy(g_strErrorMsg[1], "Please check your settings.", cmsgsize);
			}
			else {
				strncat(g_strErrorMsg[0],"suspended.", cmsgsize);
			  switch (cc_status.task_suspend_reason) {
				case SUSPEND_REASON_BATTERIES:
					strncpy(g_strErrorMsg[1],"Computer is running on batteries.", cmsgsize); break;
				case SUSPEND_REASON_USER_ACTIVE:
					strncpy(g_strErrorMsg[1], "Computer is in use.", cmsgsize); break;
				case SUSPEND_REASON_USER_REQ:
					strncpy(g_strErrorMsg[1], "Computing suspended by user.", cmsgsize); break;
				case SUSPEND_REASON_TIME_OF_DAY:
					strncpy(g_strErrorMsg[1], "Computing suspended during this time of day.", cmsgsize); break;
				case SUSPEND_REASON_BENCHMARKS:
					strncpy(g_strErrorMsg[1], "Computing suspended while running benchmarks.", cmsgsize); break;
				case SUSPEND_REASON_DISK_SIZE:
					strncpy(g_strErrorMsg[1], "Computing suspended due to no disk space.", cmsgsize); break;
				case SUSPEND_REASON_NO_RECENT_INPUT:
					strncpy(g_strErrorMsg[1], "Computing suspended while computer not in use.", cmsgsize); break;
				case SUSPEND_REASON_INITIAL_DELAY:
					strncpy(g_strErrorMsg[1], "Computing suspended while BOINC is starting up.", cmsgsize); break;
				case SUSPEND_REASON_EXCLUSIVE_APP_RUNNING:
					strncpy(g_strErrorMsg[1], "Computing suspended while exclusive app running.", cmsgsize); break;
		       }
			}
		}
	return bOK;
}			
	
void drawText()
{	
	//text_fader.value(t, alpha); // not needed here, use the value from the logo fader calls in drawTextures();

#ifdef _DEBUG
    char strTemp[_MAX_PATH];
/*
	 if (vProject.size() >= 6) {
       sprintf(strTemp, "fc0=%.2f  fc1=%.2f  fc2=%.2f  fc3=%.2f  fc4=%.2f  fc5=%.2f  fmax=%.2f  ftotal=%.2f",
			   vProject[0].fCredit, vProject[1].fCredit, vProject[2].fCredit, vProject[3].fCredit, vProject[4].fCredit, vProject[5].fCredit, fCreditMax, fCreditTotal);
	}
	else {
		strcpy(strTemp, "Click on screen to cycle demo projects");
	}
*/
    sprintf(strTemp, "alpha=%f  txtWid=%f  ArcLen=%f  FontScale=%f  SweepOff=%f  SweepRad=%f  bigFudge=%d  lilFudge=%d", 
			g_alpha, g_fTextWidth, g_fArcLength, g_fFontScale, g_fSweepOffset, g_fSweepRadians, BigFudge, LittleFudge);
	//ttf_render_string(g_cfcolor_text[3], 0.01, .01, 0.0, 2000.0f, (GLfloat*) g_cfcolor_white, g_iFont, (char*) strTemp);
#endif
	
	
	// PROGRESS THRU PROCESSORS
	float fOff[2] = { -.025f, -.050f };
	float pos[2]; 
	pos[0] = g_xGL[0] + .025f;
	pos[1] = g_yGL[0] - .039f;

	ttf_render_string( g_alpha, pos[0],           pos[1], 0.0f, 1200.0f, (GLfloat*) g_cfcolor_white, g_iFont, "PROGRESS");
	ttf_render_string( g_alpha, pos[0], pos[1] + fOff[0], 0.0f, 1200.0f, (GLfloat*) g_cfcolor_white, g_iFont, "THRU");
	ttf_render_string( g_alpha, pos[0], pos[1] + fOff[1], 0.0f, 1200.0f, (GLfloat*) g_cfcolor_white, g_iFont, "PROCESSORS");

	//////////////////	
	// Sponsors of tomorrow.  (Intel slogan)
	ttf_render_string( g_alpha, g_posLogo[TEXTURE_INTEL][0] - .185f, g_posLogo[TEXTURE_INTEL][1] + .0318f, 0.0f, 
					  1800.0f, (GLfloat*) g_cfcolor_white, 	g_iFont, "Sponsors of Tomorrow.");
	
	// Trademark TM .006 above
	ttf_render_string( g_alpha, g_posLogo[TEXTURE_INTEL][0] - .012f, g_posLogo[TEXTURE_INTEL][1] + .0378f, 0.0f, 
					  5000.0f, (GLfloat*) g_cfcolor_white, 	g_iFont, "TM");
//////////////////	
	// GridRepublic stuff
	// in association with
    pos[0] = g_posLogo[TEXTURE_GR][0] - .128f;
	pos[1] = g_posLogo[TEXTURE_GR][1] + .005f;
	ttf_render_string( g_alpha, pos[0],           pos[1], 0.0f, 2000.0f, (GLfloat*) g_cfcolor_white, g_iFont, "in association with");

	/*
	// gridrepublic
    pos[0] = g_posLogo[TEXTURE_GR][0] + .052f;
	pos[1] = g_posLogo[TEXTURE_GR][1] + .022f;
	ttf_render_string( g_alpha, pos[0],           pos[1], 0.0f, 2000.0f, (GLfloat*) g_cfcolor_white, g_iFont, "gridrepublic");

	// gridrepublic
    pos[0] = g_posLogo[TEXTURE_GR][0] + .136f;
	pos[1] = g_posLogo[TEXTURE_GR][1] + .022f;
	ttf_render_string( g_alpha, pos[0],           pos[1], 0.0f, 2000.0f, (GLfloat*) g_cfcolor_grey, g_iFont, "volunteer computing");	
    */	
	
}
	
void GetOpenGLScreenCoord(GLfloat xIn, GLfloat yIn, GLfloat& xOut, GLfloat& yOut) //, GLfloat& zOut)
{
    GLint viewport[4];
	GLdouble modelview[16];
	GLdouble projection[16];
	GLfloat winX, winY, winZ;
	GLdouble posX, posY, posZ;
	posX = posY = posZ = 0.0f;
	
	glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewport );
	
	winX = xIn;
	winY = (float) viewport[3] - yIn;
	winZ = 0.0f;
	glReadPixels((GLint) winX, (GLint) winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );	
	gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);

	xOut = (float) posX;
	yOut = (float) posY;
	//zOut = (float) posZ;
}
	
void drawLogoProject()
{		
	float pos[3] = {g_posLogo[TEXTURE_PROJECT][0], g_posLogo[TEXTURE_PROJECT][1], 0.0f};
	drawMaskedImage(TEXTURE_PROJECT, pos, g_sizeLogo[TEXTURE_PROJECT], g_alphaMax > 0.0 ? g_alphaMax : 1.0f	, g_alphaMax > 0.0f);		
}	

void drawLogoTotal()
{		
	// draw solid black behind so doesn't blend
	float pos[3] = {g_posLogo[TEXTURE_TOTAL][0], g_posLogo[TEXTURE_TOTAL][1], 0.0f};
	drawMaskedImage(TEXTURE_TOTAL, pos, g_sizeLogo[TEXTURE_TOTAL],  g_alphaMax > 0.0 ? g_alphaMax : 1.0f, g_alphaMax > 0.0f);		
}	
	
void drawLogoIntel()
{	
	g_posLogo[TEXTURE_INTEL][0] = g_xGL[1] - g_sizeLogo[TEXTURE_INTEL][0] - .025f;
	g_posLogo[TEXTURE_INTEL][1] = g_yGL[0] - g_sizeLogo[TEXTURE_INTEL][1] - .004f;
	drawMaskedImage(TEXTURE_INTEL, g_posLogo[TEXTURE_INTEL], g_sizeLogo[TEXTURE_INTEL], g_alpha);	
}
	
void drawLogoGR()
{
	g_posLogo[TEXTURE_GR][0] = g_xGL[1] - g_sizeLogo[TEXTURE_GR][0] - .02f;
	g_posLogo[TEXTURE_GR][1] = g_yGL[1] + g_sizeLogo[TEXTURE_GR][1] - .01f;
	drawMaskedImage(TEXTURE_GR, g_posLogo[TEXTURE_GR], g_sizeLogo[TEXTURE_GR], g_alpha);	
}
		
// if iProjCount < 0 it's the TEXTURE_TOTAL for totals etc
void drawTextArc(const int iProjCount, const char* strText, const float fSweep, const int iOffset, const float fRadius, const int iBigFont, const float fRotate, const GLfloat* pcolorFont)
{		
	if (g_iFont < 0) return;

	float fReclaim = -ARC_OFFSET * M_PI / 180.0f; // angle to reclaim from the 2 degree offset	
	//char strTemp[MAX_DISPLAY_STRLEN+1];
	//memset(strTemp, 0x00, sizeof(char) * (MAX_DISPLAY_STRLEN+1));
	//strncpy(strTemp, strText, MAX_DISPLAY_STRLEN); // truncate at 30 chars
	
	// fake centered text by going down from the center point (g_fSweepRadians) 
	g_fSweepOffset = 0.0f;
	
	g_fFontScale = 1500.0f;

	glPushMatrix();

	// measure the width of the string so we can center it below
	g_fTextWidth = g_font[g_iFont]->Advance(strText, -1, FTPoint());
		
	// inner arc max text width is about 210 on the 6 section disk
	// outer arc max text width is about 360
	if (iProjCount < 4) {
		if (g_fTextWidth < 400.0f) {
			g_fFontScale = iBigFont ? 1400.0f : 1600.0f;
		}
		else { // >=30
			g_fFontScale = iBigFont ? 2000.0f : 2200.0f;
		}
	}
	else if (iProjCount == 4) {
		if (g_fTextWidth < 200.0f) {
			g_fFontScale = iBigFont ? 1400.0f : 1600.0f;
		}
		else if (g_fTextWidth < 400.0f) {
			g_fFontScale = iBigFont ? 1800.0f : 2000.0f;
		}
		else { // >=30
			g_fFontScale = iBigFont ? 2200.0f : 2400.0f;
		}
	}
	else if (iProjCount == 5) {
		if (g_fTextWidth < 200.0f) {
			g_fFontScale = iBigFont ? 1800.0f : 2000.0f;
		}
		else if (g_fTextWidth < 300.0f) {
			g_fFontScale = iBigFont ? 2400.0f : 2600.0f;
		}
		else if (g_fTextWidth < 400.0f) {
			g_fFontScale = iBigFont ? 2800.0f : 3000.0f;
		}
		else { // >=30
			g_fFontScale = iBigFont ? 3400.0f : 3600.0f;
		}
	}
	else {
		if (g_fTextWidth < 200.0f) {
			g_fFontScale = iBigFont ? 1800.0f : 2000.0f;
		}
		else if (g_fTextWidth < 300.0f) {
			g_fFontScale = iBigFont ? 2400.0f : 2800.0f;
		}
		else if (g_fTextWidth < 400.0f) {
			g_fFontScale = iBigFont ? 3000.0f : 3400.0f;
        }
		else { // >=30
			g_fFontScale = iBigFont ? 3400.0f : 3800.0f;
		}
	}
	
	if (iProjCount == -1) g_fFontScale = 2800.0f;   // for inner text msg

	// well this is a bit of a fudge but long strings seem to "drift" on the usual 840 setting 
	float fFactor = 840.0f;
	if (g_fTextWidth > 400) 
		fFactor = 790.0f;
	else if (g_fTextWidth < 200) 
		fFactor = 890.0f;
	else if (g_fTextWidth < 100) 
		fFactor = 940.0f;
	
	fFactor += 360.0f;  // needs this adjustment "fudge factor"
	
	float fTextRadius = fFactor / (fRadius * g_fFontScale);   // 840 = 30 (max string length) * 14 (ascent base of mono font) * 2 (?)
	
	if (iProjCount <= 1) { 
		g_fSweepRadians = -M_PI/2.0f;
		g_fArcLength = fRadius * M_PI/2.0f;
	    g_fSweepOffset =  (-fRotate * M_PI / 180.0f) + g_fSweepRadians/2.0f * ((g_fTextWidth / (g_fArcLength * g_fFontScale)));  // note no fReclaim needed on 1 disk!
		glTranslatef(fRadius * cos(g_fSweepRadians + g_fSweepOffset),  fRadius * sin(g_fSweepRadians + g_fSweepOffset), 0.0f);
		ttf_render_string(g_cfcolor_text[3], 0.0, 0.0, 0.0, g_fFontScale, (GLfloat*) pcolorFont, g_iFont, (char*) strText, 
				g_fSweepOffset * 180.0f / M_PI, 0.0f, 0.0f, 1.0f, fTextRadius); 
	}
	else if (iProjCount == 2) {
		g_fSweepRadians = ((iOffset == 0 ? -.5f : .5f) * M_PI);
		g_fArcLength = fRadius * M_PI/2.0f;
		g_fSweepOffset = fReclaim + (-fRotate * M_PI / 180.0f) + (iOffset == 0 ? 1.0f : -1.0f) * g_fSweepRadians/2.0f * ((g_fTextWidth / (g_fArcLength * g_fFontScale)));
		glTranslatef(fRadius * cos(g_fSweepRadians + g_fSweepOffset),  fRadius * sin(g_fSweepRadians + g_fSweepOffset), 0.0f);
		ttf_render_string(g_cfcolor_text[3], 0.0, 0.0, 0.0, g_fFontScale, (GLfloat*) pcolorFont, g_iFont, (char*) strText, 
            (iOffset == 0 ? 0.0f : 180.0f) + g_fSweepOffset * 180.0f / M_PI, 0.0f, 0.0f, 1.0f, fTextRadius); 
	}
	else {
		g_fSweepRadians = (M_PI / 2.0f) + ((float) iOffset) * 2 * M_PI / (float) iProjCount;
		g_fArcLength = fRadius * g_fSweepRadians; // M_PI/2.0f;
		g_fSweepOffset = fReclaim + (-fRotate * M_PI / 180.0f) - g_fSweepRadians/2.0f * ((g_fTextWidth / (g_fArcLength * g_fFontScale)));
		glTranslatef(fRadius * cos(g_fSweepRadians + g_fSweepOffset),  fRadius * sin(g_fSweepRadians + g_fSweepOffset), 0.0f);
		ttf_render_string(g_cfcolor_text[3], 0.0, 0.0, 0.0, g_fFontScale, (GLfloat*) pcolorFont, g_iFont, (char*) strText, 
			  (-180.0f + ((float) (iOffset * fSweep))) + g_fSweepOffset * 180.0f / M_PI, 0.0f, 0.0f, 1.0f, fTextRadius); 
	 }	

/*
 // draw little squares to see if they line up to "pie chart"
 glColor4fv(invcolor);
 glBegin(GL_QUADS);
 glVertex2f(-0.005,-.005);
 glVertex2f(-0.005,.005);
 glVertex2f(0.005,.005);
 glVertex2f(0.005,-.005);
 glEnd();
*/
	
	glPopMatrix();
}

inline void calcNewPositions(const int& iTexture)
{ // calcs new movement coords given current settings, use hitTest to see if we'll hit anything and need to readjust
	//for (int j = TEXTURE_PROJECT; j <= TEXTURE_TOTAL; j++)  {  // circle		
		for (int i = 0; i < 2; i++) {  // x & y position
			g_posLogo[iTexture][i] += g_move[iTexture][i]; // total moves at diff speed
		}
		g_posLogo[iTexture][2] = 0.0f; // z always 0 in ortho 2d mode
	//}
}

inline float my_frand(long lSeed) // like the boinc drand but uses random()
{
	if (lSeed != 1L) 
		srand(lSeed);
	return (float) rand() / (float) RAND_MAX;
}

inline void otherDirection(float* pfVal, const float& fRand, const int& iOffset)
{
	switch (iOffset) {
		case 0:
			pfVal[0] = (pfVal[0] < 0.0f ? -1.0f : 1.0f) * fRand;
			break;
		case 1:
			pfVal[0] = (pfVal[0] < 0.0f ? 1.0f : -1.0f) * fRand;
			break;			
		case 2:
			pfVal[1] = (pfVal[1] < 0.0f ? -1.0f : 1.0f) * fRand;
			break;
		case 3:
			pfVal[1] = (pfVal[1] < 0.0f ? 1.0f : -1.0f) * fRand;
			break;
		case 4:
			pfVal[0] = (pfVal[0] < 0.0f ? 1.0f : -1.0f) * fRand;
			pfVal[1] = (pfVal[1] < 0.0f ? 1.0f : -1.0f) * fRand;
			break;
		case 5:
			pfVal[0] = (pfVal[0] < 0.0f ? -1.0f : 1.0f) * fRand;
			pfVal[1] = (pfVal[1] < 0.0f ? 1.0f : -1.0f) * fRand;
			break;
		case 6:
			pfVal[0] = (pfVal[0] < 0.0f ? -1.0f : 1.0f) * fRand;
			pfVal[1] = (pfVal[1] < 0.0f ? -1.0f : 1.0f) * fRand;
			break;
		case 7:
			pfVal[0] = (pfVal[0] < 0.0f ? 1.0f : -1.0f) * fRand;
			pfVal[1] = (pfVal[1] < 0.0f ? -1.0f : 1.0f) * fRand;
			break;
		case 8:
			pfVal[0] = (pfVal[0] < 0.0f ? 1.0f : -1.0f) * fRand;
			pfVal[1] = (pfVal[1] < 0.0f ? -1.0f : 1.0f) * fRand;
			break;
		case 9:
			pfVal[0] = (pfVal[0] < 0.0f ? 1.0f : -1.0f) * fRand * 1.5f;
			pfVal[1] = (pfVal[1] < 0.0f ? -1.0f : 1.0f) * fRand * .80f;
			break;
	}
}
	
inline int hitTest(const int& iTexture, const int& iOffset, const int& iLastHit)
{
    float fRand = -1.00f * (  MIN_SPEED + ( my_frand() * (MAX_SPEED - MIN_SPEED) )  );  // get a random new speed within range
	int iHit = HIT_NONE;

    float posSave[2];  // save last position in case it gets screwed up
	memcpy(posSave, g_posLogo[iTexture], sizeof(float) * 2);
	
	calcNewPositions(iTexture);
	
	// if hit bounce off, i.e. don't allow "overlap"
	// they want the logos to count as a "hit" but for now just do the corners & make sure don't hit each other
	int iTextureOther;
	// now get the position of the other floating object
	if (iTexture == TEXTURE_TOTAL) iTextureOther = TEXTURE_PROJECT;
	else iTextureOther = TEXTURE_TOTAL; // it's one or the other!	

	// note index 0 is x, 1 is y per convention
	float fMyCenter[2] = { g_posLogo[iTexture][0] + g_sizeLogo[iTexture][0]/2.0f, g_posLogo[iTexture][1] + g_sizeLogo[iTexture][1]/2.0f };
	float fOtherCenter[2] = { g_posLogo[iTextureOther][0] + g_sizeLogo[iTextureOther][0]/2.0f, g_posLogo[iTextureOther][1] + g_sizeLogo[iTextureOther][1]/2.0f };
	
	// OK, first see if we hit an edge, this will simply be the max tangents of the circle touching one of the first four 4Hit's
	// test if the right side exceeds right screen
	if ((fMyCenter[0] + g_fRadiusMaxCurrent[iTexture]) >= g_xGL[1]) { // try to introduce some randomness in the direction changing
        // flip the sign of the x movement vecture, note don't flip over & over again if just failed hit test
		if (iLastHit == HIT_RIGHT) { // already tried, do another direction?
			otherDirection(g_move[iTexture], fRand, iOffset);
		}
		else { // try a basic change of this direction
		   g_move[iTexture][0] = (g_move[iTexture][0] < 0.0f ? -1.0f : 1.0f) * fRand;
		}
		iHit = HIT_RIGHT;
	}
	// test if the left side left right screen
	else if (iHit == HIT_NONE && (fMyCenter[0] - g_fRadiusMaxCurrent[iTexture]) <= g_xGL[0]) {
		if (iLastHit == HIT_LEFT) { // already tried, do another direction?
			otherDirection(g_move[iTexture], fRand, iOffset);
		}
		else { // try a basic change of this direction
			g_move[iTexture][0] = (g_move[iTexture][0] < 0.0f ? -1.0f : 1.0f) * fRand;
		}
		iHit = HIT_LEFT;
	}

	// test if the top goes over
	if (iHit == HIT_NONE && (fMyCenter[1] + g_fRadiusMaxCurrent[iTexture]) >= g_yGL[0]) {
		if (iLastHit == HIT_TOP) { // already tried, do another direction?
			otherDirection(g_move[iTexture], fRand, iOffset);
		}
		else { // try a basic change of this direction
			g_move[iTexture][1] = (g_move[iTexture][1] < 0.0f ? -1.0f : 1.0f) * fRand; 
		}
		iHit = HIT_TOP;
	}  // test if the bottom goes over
	else if (iHit == HIT_NONE && (fMyCenter[1] - g_fRadiusMaxCurrent[iTexture]) <= g_yGL[1]) {
		if (iLastHit == HIT_BOTTOM) { // already tried, do another direction?
			otherDirection(g_move[iTexture], fRand, iOffset);
		}
		else { // try a basic change of this direction
			g_move[iTexture][1] = (g_move[iTexture][1] < 0.0f ? -1.0f : 1.0f) * fRand; 
		}
		iHit = HIT_BOTTOM;
	}
	
	// OK now the trickier bit - test for collision of the two circles & I guess send them in separate ways
	// I guess we'll want to see if there is an intersection of the two circumferi
    // since I'm lazy I'll just nick it from here:  http://local.wasp.uwa.edu.au/~pbourke/geometry/2circle/   http://local.wasp.uwa.edu.au/~pbourke/geometry/2circle/tvoght.c	

	if (iHit == HIT_NONE) { // don't bother checking if already failed hit test
	  // NB:  it could be bottom/top/left/right AND hitting a circle!
		float dx = fMyCenter[0] - fOtherCenter[0];  // distance between centers
		float dy = fMyCenter[1] - fOtherCenter[1];  // distance between centers
		// Determine the straight-line distance between the centers. */
		//d = sqrt((dy*dy) + (dx*dx));
#ifdef WIN32
		float d = _hypot(dx,dy); // windows complains & doesn't link hypot()
#else
		float d = hypot(dx,dy); 
#endif
		// see if circles intersect
		if ((d <= (g_fRadiusMaxCurrent[iTexture] + g_fRadiusMaxCurrent[iTextureOther])) )
			// || (d >= fabs(g_fRadiusMaxCurrent[iTexture] - g_fRadiusMaxCurrent[iTextureOther])) )  // this should show one circle is inside another
		{  // hmmm, what to do, well set their movements opposite
			if (iLastHit == HIT_CIRCLE) { // already tried, do another direction?
				otherDirection(g_move[iTexture], fRand, iOffset);
				float fRand2 = -1.00f * (  MIN_SPEED + ( my_frand() * (MAX_SPEED - MIN_SPEED) )  );  // get a random new speed within range
				g_move[TEXTURE_TOTAL][0] = (g_move[TEXTURE_TOTAL][0] < 0.0f ? -1.0f : 1.0f) * fRand2; 
				g_move[TEXTURE_TOTAL][1] = (g_move[TEXTURE_TOTAL][1] < 0.0f ? -1.0f : 1.0f) * fRand; 
			}
			else { // project cirle has precedence
				float fRand2 = -1.00f * (  MIN_SPEED + ( my_frand() * (MAX_SPEED - MIN_SPEED) )  );  // get a random new speed within range
				g_move[TEXTURE_TOTAL][0] = (g_move[TEXTURE_TOTAL][0] < 0.0f ? -1.0f : 1.0f) * fRand; 
				g_move[TEXTURE_TOTAL][1] = (g_move[TEXTURE_TOTAL][1] < 0.0f ? -1.0f : 1.0f) * fRand2; 
				//g_move[iTextureOther][0] = (g_move[iTextureOther][0] < 0.0f ? -.987643f : 1.234f) * fRand2; 
				//g_move[iTextureOther][1] = (g_move[iTextureOther][1] < 0.0f ? -.6789f : 1.4321f) * fRand; 
			}
			iHit = HIT_CIRCLE;
			//g_posLogo[iTexture][1] = g_yGL[1] + fabs(g_move[iTexture][1]);
		}
	}	

	if (iHit != HIT_NONE) {
		// go back to previous point
		memcpy(g_posLogo[iTexture], posSave, sizeof(float) * 2);
	}
	
	return iHit;
}
	
void calcRotationAndMovement()
{
	// they want circles to bounce off edges & even the logos & each other, i.e. no overlap

	// rotation is simple, just a degree every frame (clockwise for the project disk, counterclockwise for totals)
	g_fRotate[TEXTURE_TOTAL] = (float) -(g_frame % 720) / 2.0f;
	g_fRotate[TEXTURE_PROJECT]  = (float) (g_frame % 720) / 2.0f;
	
	// compute the new positions
	// try up to a 100 times to get a new position if needed
	int iCtr = 0, iLastHit = HIT_NONE;

	while ((iLastHit = hitTest(TEXTURE_PROJECT, iCtr % HIT_RETRY, iLastHit)) != HIT_NONE && iCtr++<100)
	{
#ifdef _DEBUG
		if (iLastHit != HIT_NONE) {
			if (iCtr > BigFudge) BigFudge = iCtr;
		}
#endif
	}
	if (iCtr == 101) { // after 100 do a panic reset
		resetCircles(true); // true means report error
	}
	
	iCtr = 0;
    while ((iLastHit = hitTest(TEXTURE_TOTAL, iCtr % HIT_RETRY, iLastHit)) != HIT_NONE && iCtr++<100)
	{
#ifdef _DEBUG
		if (iLastHit != HIT_NONE) {
			if (iCtr > LittleFudge) LittleFudge = iCtr;
		}
#endif
	}
	if (iCtr == 101) { // after 100 do a panic reset
		resetCircles(true);
	}
	
	g_frame++;
	if (g_frame > 1.44e9) g_frame = 1; // note that 1.44e9 is divisible by 360 so shouldn't "blink" on the switch

#ifdef _DEMO_BUILD
	if (!(g_frame % 200)) updateData(true); // demo data switch
#endif
}

	
void drawTotalCircle()
{
   
	const float fX = g_posLogo[TEXTURE_TOTAL][0] + (g_sizeLogo[TEXTURE_TOTAL][0] / 2.0f);
	const float fY = g_posLogo[TEXTURE_TOTAL][1] + (g_sizeLogo[TEXTURE_TOTAL][1] / 2.0f);
	
	glBlendFunc (GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
	drawLogoTotal();	
	
	glPushMatrix();
	
	glTranslatef(fX, fY, 0.0f);  // move to the center of the earth icon
	
	// first draw the "pie chart"
	// inner circle
	GLUquadric* quad = gluNewQuadric();
	if (!quad) return;
	gluQuadricDrawStyle(quad, GLU_FILL);  
	gluQuadricNormals(quad, GLU_SMOOTH); 
	gluQuadricOrientation(quad, GLU_INSIDE);

	glColor4fv(g_cfcolor_totalinnerring);
//	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	gluDisk(quad, g_cfRadiusMin[TEXTURE_TOTAL], g_cfRadiusMax[TEXTURE_TOTAL], DISK_SLICE, DISK_LOOP);
	
	// now draw text for totals
	char strTotal[64];
	//sprintf(strTotal, "%.2f G-Hrs Total", fCreditTotal);

	if (g_bUpdating) {
		sprintf(strTotal, "Updating project data, please wait!");
		drawTextArc(0, strTotal, 0.0f, 0, g_cfRadiusMax[TEXTURE_TOTAL] - g_cfRadiusMsgOffset[TEXTURE_TOTAL], 0, g_fRotate[TEXTURE_TOTAL], g_cfcolor_verydarkblue);
		gluDeleteQuadric(quad);
		glPopMatrix();
		return;
	}
	
	sprintf(strTotal, "Total Progress Thru Processors Contribution: %s G hrs", commaPrint(fCreditTotal));
	//strcpy(strTotal, "climateprediction.net");
	drawTextArc(0, strTotal, 0.0f, 0, g_cfRadiusMax[TEXTURE_TOTAL] - g_cfRadiusMsgOffset[TEXTURE_TOTAL], 0, g_fRotate[TEXTURE_TOTAL], g_cfcolor_verydarkblue);
	
	// draw text for timestamp
	drawTextArc(-1, g_strTimeUpdated, 0.0f, 0, g_cfRadiusTimeOffset[TEXTURE_TOTAL], 0, g_fRotate[TEXTURE_TOTAL], g_cfcolor_verylightblue); // g_cfcolor_verydarkblue
	
	// need a lighter, outer circle
	glColor4fv(g_cfcolor_totalouterring);
	gluDisk(quad, g_cfRadiusMax[TEXTURE_TOTAL], g_fRadiusMaxCurrent[TEXTURE_TOTAL], DISK_SLICE, DISK_LOOP);	
	
	// small badge disk down 45 degrees, non-rotating
	float fNewX = ((2.0f * g_cfRadiusMax[TEXTURE_TOTAL]) - g_cfRadiusMin[TEXTURE_TOTAL]) * cos(M_PI/4.0f);
	const float fBadgeSize = 0.018f;
	glTranslatef(fNewX, -fNewX, 0.0f);  // down to the edge of the outer rim to draw the badge

	glColor4fv(g_cfcolor_totalinnerring);
	gluDisk(quad, 0, fBadgeSize, DISK_SLICE, DISK_LOOP);	
	
	glColor4fv(g_cfcolor_totalouterring);
	gluDisk(quad, fBadgeSize, 2.0f * fBadgeSize, DISK_SLICE, DISK_LOOP);	
	
	// badge text
	long lBadge = 1e4L * (1L + (long) floor(fCreditTotal/1e4f));
    char* strBadge = commaPrint((float) lBadge);
	float fAdjust = ((float)strlen(strBadge)/400.0f);
    glTranslatef(-.005f - fAdjust, .001f, 0.0f);
	ttf_render_string(g_cfcolor_text[3], 0.0f, 0.0f, 0.0f, 2500.0f,  g_cfcolor_verydarkblue, g_iFont, strBadge);
    glTranslatef(fAdjust - .008f, -.011f, 0.0f);
	ttf_render_string(g_cfcolor_text[3], 0.0f, 0.0f, 0.0f, 2500.0f,  g_cfcolor_verydarkblue, g_iFont, "G Hrs");
	
	gluDeleteQuadric(quad);
	
	glPopMatrix();
	
}
	
void drawProjectCircle()
{ 
	// this will draw 1 - 6 arcs for the project
	
	char strCredit[64];
	
	const float fX = g_posLogo[TEXTURE_PROJECT][0] + g_sizeLogo[TEXTURE_PROJECT][0]/2.0f;
	const float fY = g_posLogo[TEXTURE_PROJECT][1] + g_sizeLogo[TEXTURE_PROJECT][1]/2.0f;
	
    drawLogoProject();

	float fSweep = 360.0f / (float) (g_iProjectDisplayCount ? g_iProjectDisplayCount : 1); // number of projects 6 max
	unsigned int iOffset = 0;
	
	if (g_bUpdating) return;
	
	glBlendFunc (GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    //glHint (GL_LINE_SMOOTH_HINT, GL_DONT_CARE);	

	glPushMatrix();

	glTranslatef(fX, fY, 0.0f);  // move to the center of the desk icon
	
	// first draw the "pie chart"
	unsigned int iObjCount = (g_iProjectDisplayCount ? g_iProjectDisplayCount : 1);
    //g_fRadiusMaxCurrent[TEXTURE_PROJECT] = g_cfRadiusMax[TEXTURE_PROJECT];  // todo: eventually set the max current radius for bouncing effects etc
	for (iOffset = 0; iOffset < iObjCount; iOffset++) {
		// the disk radius should change based on project credits -- from a minimum of fMin up to g_cfRadiusMax for largest credit project
		if (g_bUpdating) break;
		float fDiskMax = g_cfRadiusMin[TEXTURE_PROJECT] + ((g_cfRadiusMax[TEXTURE_PROJECT] - g_cfRadiusMin[TEXTURE_PROJECT]) / 2.0f);  // base size
		if (g_iProjectDisplayCount >= 1 && fCreditMax > 0.0f && vProject.size() > (iOffset + g_iProjectOffset)) {
			// compute credit fractions for disk size
			fDiskMax += (((g_cfRadiusMax[TEXTURE_PROJECT] - g_cfRadiusMin[TEXTURE_PROJECT]) / 2.0f) * (vProject[iOffset+g_iProjectOffset].fCredit / fCreditMax));  // as a project's credit approaches fCreditMax for this group, it's the max radius allowed
		}
		//if (fDiskMax > g_fRadiusMaxCurrent[TEXTURE_PROJECT]) g_fRadiusMaxCurrent[TEXTURE_PROJECT] = fDiskMax; // save this for later collision detection
		
		GLUquadric* quad = gluNewQuadric();
		if (!quad) return;
		gluQuadricDrawStyle(quad, GLU_FILL);  
		gluQuadricNormals(quad, GLU_SMOOTH); 	
		gluQuadricOrientation(quad, GLU_INSIDE);
		
		glColor4fv(g_cfcolor_proj[iOffset]);
		// glColor4f((float) iOffset * 0.1f, (float) iOffset * 0.1f, (float) iOffset * 0.1f, .8f);  // good shading for debugging - goes from black to lighter shades of grey (in order)
		if (g_iProjectDisplayCount <= 1) { // draw full disk for error message or 1 project
			gluDisk(quad, g_cfRadiusMin[TEXTURE_PROJECT], fDiskMax, DISK_SLICE, DISK_LOOP);
		}
       else if (g_iProjectDisplayCount == 2) { // draw two semi-circles but centered differently
			gluPartialDisk(quad, g_cfRadiusMin[TEXTURE_PROJECT], fDiskMax, DISK_SLICE, DISK_LOOP, (fSweep * .50f * (iOffset==0 ? 1.0f : -1.0f)) + ARC_OFFSET + g_fRotate[TEXTURE_PROJECT], fSweep - ARC_OFFSET);
		}
		else {
			gluPartialDisk(quad, g_cfRadiusMin[TEXTURE_PROJECT], fDiskMax, DISK_SLICE, DISK_LOOP, (fSweep * .50f) + ARC_OFFSET + g_fRotate[TEXTURE_PROJECT] + ((float) (iOffset+1) * -fSweep), fSweep - ARC_OFFSET);
		}
		gluDeleteQuadric(quad);
    }
	
	// now draw text for each wedge   (done separately for better alpha blending of text)
	/*if (g_iProjectDisplayCount == 0) { // error message
		if (bBOINCRunning)
			drawTextArc(0, "No Projects Attached", fSweep, 0, g_cfRadiusMax[TEXTURE_PROJECT] - g_cfRadiusMsgOffset[TEXTURE_PROJECT], 1, g_fRotate[TEXTURE_PROJECT], g_cfcolor_white);
		else
			drawTextArc(0, "BOINC Is Not Running", fSweep, 0, g_cfRadiusMax[TEXTURE_PROJECT] - g_cfRadiusMsgOffset[TEXTURE_PROJECT], 1, g_fRotate[TEXTURE_PROJECT], g_cfcolor_white);
	}
	else {*/
	iOffset = 0;
	if (g_iProjectDisplayCount > 0 && !g_bUpdating) {
		for (iOffset = 0; iOffset < g_iProjectDisplayCount; iOffset++) {
			if (g_bUpdating) break;  // && vProject.size() > (iOffset+g_iProjectOffset)
			if (vProject.size() > (iOffset+g_iProjectOffset)) {
				//glBlendFunc (GL_DST_COLOR, GL_ONE);
				//sprintf(strText, "Project %d", iOffset);		
				sprintf(strCredit, "( my G hrs: %s )", commaPrint( vProject[iOffset+g_iProjectOffset].fCredit ));
				drawTextArc(g_iProjectDisplayCount, strCredit,           fSweep, iOffset, g_cfRadiusMax[TEXTURE_PROJECT] - g_cfRadiusOuterOffset[TEXTURE_PROJECT], 0, g_fRotate[TEXTURE_PROJECT], g_cfcolor_white);
				drawTextArc(g_iProjectDisplayCount, vProject[iOffset+g_iProjectOffset].strProject, fSweep, iOffset, g_cfRadiusMax[TEXTURE_PROJECT] - g_cfRadiusInnerOffset[TEXTURE_PROJECT], 1, g_fRotate[TEXTURE_PROJECT], g_cfcolor_white);
			}
		}
	}
	
	// draw text for timestamp
	//drawTextArc(-1, g_strTimeUpdated, 0.0f, 0, g_cfRadiusTimeOffset[TEXTURE_PROJECT], 0, g_fRotate[TEXTURE_PROJECT], g_cfcolor_white); //g_cfcolor_verydarkblue);
	
	glPopMatrix();
}
		
	/*
	 // draw little squares to see if they line up to "pie chart"
	 glColor3f(1.0,0,0);
	 glBegin(GL_QUADS);
	 glVertex2f(-0.001,-.001);
	 glVertex2f(-0.001,.001);
	 glVertex2f(0.001,.001);
	 glVertex2f(0.001,-.001);
	 glEnd();
	 */
	
	
	/*
	 float alpha_value,
	 // reference value to which incoming alpha values are compared.
	 // 0 through to 1
	 double x, double y, double z, // text position
	 float fscale,                 // scale factor
	 GLfloat * col,                // colour 
	 int i,                        // font index see texfont.h 
	 char * s,				  	  // string ptr
	 float fRotAngle = 0.0f,        // optional rotation angle
	 float fRotX = 0.0f,            // optional rotation vector for X
	 float fRotY = 0.0f,            // optional rotation vector for Y
	 float fRotZ = 1.0f             // optional rotation vector for Z
	 */		

	
// get the GUI RPC data and put in our array for rendering
int updateData(bool bDemo)
{
    int retval = 0;
	
	g_bUpdating = true; // set the flag that we're updating, maybe prevent rendering while there's no data
	
	unsigned int i = 0;
	fCreditTotal = 0.0f;
	fCreditMax = 0.0f;
	vProject.clear();
	structProject sp;
	
	if (bDemo) {
		bBOINCRunning = true;
		unsigned int iMax = 40;
		   strcpy(sp.strProject, "climateprediction.net");
			vProject.push_back(sp);
			strcpy(sp.strProject, "Quake-Catcher Network");
			vProject.push_back(sp);
			strcpy(sp.strProject, "Einstein@home");
			vProject.push_back(sp);
			strcpy(sp.strProject, "SETI@home");
			vProject.push_back(sp);
			strcpy(sp.strProject, "World Community Grid");
			vProject.push_back(sp);
			strcpy(sp.strProject, "Rosetta@home Protein Folding");
			vProject.push_back(sp);

		for (i = 7; i <= iMax; i++) {
			sprintf(sp.strProject, "Project # %d", i);
			vProject.push_back(sp);
		}

		for (i = 0; i < iMax; i++) {
   		    vProject[i].fCredit = my_frand() * 126.0f;
			fCreditTotal += vProject[i].fCredit;
		    if (vProject[i].fCredit > fCreditMax) fCreditMax = vProject[i].fCredit;
		}
		g_gigaflops = 3.50f;
	}
	else {
        //memset(&cc_state, 0x00, sizeof(CC_STATE));
        //memset(&cc_status, 0x00, sizeof(CC_STATUS));
        //memset(&cc_host_info, 0x00, sizeof(HOST_INFO));

		retval = rpc.get_state(cc_state);   // note I'm just getting cc_state, seems to be all that's needed to get project info
		if (!retval) { 
			retval = rpc.get_host_info(cc_host_info);  
		}
		if (!retval) { 
			retval = rpc.get_cc_status(cc_status);  
		}
		
		for (i = 0; !retval && i < cc_state.projects.size(); i++) {
			PROJECT *p = cc_state.projects[i];
			memset(sp.strProject, 0x00, sizeof(char) * (MAX_DISPLAY_STRLEN+1));
			sp.fCredit = 0.0f;
			strncpy(sp.strProject, (char*)p->project_name.c_str(), MAX_DISPLAY_STRLEN);
			sp.fCredit = (float) p->user_total_credit * GHR_CREDIT_FACTOR;
			if (sp.fCredit > fCreditMax) fCreditMax = sp.fCredit; // just compute for this range of projects?
			fCreditTotal += sp.fCredit;
			vProject.push_back(sp);
		}
        // eventually I wanted to compare their machine gigaflops to a reference 1 gigaflop machine, maybe draw an arc comparing sizes etc
		// for now this gigaflops will be 0
		g_gigaflops = (float) cc_host_info.p_fpops / 1.0e9f;
	}

	makeTimestampString();	
	
	g_iProjectTotalCount = i;
	/*
	if (i > MAX_PROJECTS)  
		g_iProjectDisplayCount = MAX_PROJECTS;	
	else
		g_iProjectDisplayCount = i;	
	*/
	
	// this is a good spot to switch projects if needed, say every 100 frames?
	if (g_iProjectDisplayCount && g_iProjectDisplayCount < g_iProjectTotalCount) {
		g_iProjectOffset += MAX_PROJECTS;
		if (g_iProjectOffset > g_iProjectTotalCount) { // whoops too much
			g_iProjectOffset = 0;
		}
		g_iProjectDisplayCount = g_iProjectTotalCount - g_iProjectOffset;
		if (g_iProjectDisplayCount > MAX_PROJECTS) {
			g_iProjectDisplayCount = MAX_PROJECTS;
		}
	}
	else {
		g_iProjectDisplayCount = g_iProjectTotalCount > MAX_PROJECTS ? MAX_PROJECTS : g_iProjectTotalCount;
	}

	g_bUpdating = false; // reset the flag -- ok to render
    return retval;
}
	
// load fonts. call once.
//
void ttf_load_fonts(const char* dir) {
	memset(g_font, 0x00, sizeof(FTFont*) * NUM_FONT); // initialize to null's for error checking later]
	char vpath[_MAX_PATH];	
	g_iFont = -1;
	for (int i=0 ; i < NUM_FONT; i++){
	   sprintf(vpath, "%s/%s", dir, g_cstrFont[i]);
	   if (is_file(vpath)) {
		   //g_font[i] = new FTBitmapFont(vpath);
		   //g_font[i] = new FTPixmapFont(vpath);
		   //g_font[i] = new FTPolygonFont(vpath);
		   g_font[i] = new FTTextureFont(vpath);
		  if(!g_font[i]->Error()) {
#ifdef _DEBUG
	       fprintf(stderr, "Successfully loaded '%s'...\n", vpath);
#endif
			 
			 if(!g_font[i]->FaceSize(30))
			 {
				 fprintf(stderr, "Failed to set size");
			 }
			 
			 g_font[i]->Depth(3.);
			 g_font[i]->Outset(-.5f, 1.5f);
			 
			 g_font[i]->CharMap(ft_encoding_unicode);
			 g_iFont = i;
			 
		 } 
#ifdef _DEBUG
		 else {
			 fprintf(stderr, "Failed to load '%s'...\n", vpath);
		 }
#endif
	   }
    }	
}
	
void ttf_render_string(
				  const double& alpha_value,
				  // reference value to which incoming alpha values are compared.
				  // 0 through to 1
				  const double& x, 
				  const double& y, 
				  const double& z, // text position
				  const float& fscale,                 // scale factor
				  const GLfloat* col,                // colour 4vf
				  const int& iFont,                        // font index 
				  const char* s,				  	  // string ptr
				  const float& fRotAngle,        // optional rotation angle
				  const float& fRotX,            // optional rotation vector for X
				  const float& fRotY,            // optional rotation vector for Y
				  const float& fRotZ,            // optional rotation vector for Z
				  const float& fRadius           // circular radius to draw along
				  )
	{
 	// http://ftgl.sourceforge.net/docs/html/
	
		if(iFont < 0 || iFont > NUM_FONT || !g_font[iFont]) return;  //invalid font
		   
			int renderMode = FTGL::FTGL_RENDER_FRONT; // | FTGL::FTGL_RENDER_BACK;

		GLfloat color[4];
		memcpy(color, col, sizeof(GLfloat) * 4);
		color[3] = (GLfloat) alpha_value;  // force the alpha value passed in
			glColor4fv(color);
			
			glPushMatrix();

		    glTranslated(x, y, z);
		    glScaled(1.0f / fscale, 1.0f / fscale, 1.0f / fscale);
			glEnable(GL_TEXTURE_2D);
			
			if (fRotAngle != 0.0f) {
            	glRotatef(fRotAngle, fRotX, fRotY, fRotZ);
			}
					
			if (fRadius == 0.0f) {
				g_font[iFont]->Render(s, -1, FTPoint(), FTPoint(), renderMode);
		    } 
			else {
				int i = 0;
				float fAdvance = 1.0f;
			    while ( *(s+i) ) {
		 		  fAdvance = g_font[iFont]->Advance((s+i), 1, FTPoint());
                  g_font[iFont]->Render((s+i), 1, FTPoint(), FTPoint(), renderMode);
				  glTranslated(fAdvance, 0.0f, 0.0f);
            	  glRotatef(fRadius * fAdvance / (float) 20.0f, 0.0f, 0.0f, 1.0f);
				  i++;
			    }
			}
		
		glDisable(GL_TEXTURE_2D);	
		
		glPopMatrix();
	}

void makeTimestampString() // from QCN; const double dtime, const char cType, char* strTime)
{ // this sets strTime to a character string of UTC time from the given double, strTime at least 26 chars!
		
		
	/*
		struct tm tmp;
		time_t tt = (time_t) dtime;  // note that this truncates the microseconds i.e. decimal part of dtime   
#ifdef _WIN32 
        gmtime_s(&tmp, &tt);  // note the "secure" Microsoft version flips the args around!
#else
        gmtime_r(&tt, &tmp);
#endif
     */

		time_t rawtime;
		struct tm* tmp;
		
		time (&rawtime);
		tmp = localtime ( &rawtime );	
	
		int iHour = tmp->tm_hour;
		char ampm;
		if (iHour >= 12) { // pm
			iHour = (iHour==12) ? 12 : iHour-12;
			ampm = 'p';
		}
		else {
			ampm = 'a';
		}
		
		sprintf(g_strTimeUpdated, "updated as of %d.%d.%02d  %d.%02d%c",
				tmp->tm_mon+1, // month
				tmp->tm_mday,  // day
				tmp->tm_year-100, // year
				iHour,
				tmp->tm_min,
				ampm
			);
}
	
char *commaPrint(float fTest)
{
		static char retbuf[64];
		bool bNeg = fTest < 0.0f;
		float fRemain = (fTest > 0.0f) ? (float)((float) fTest - (long) fTest) : .0f;
		static int comma = ','; // '\0';
		char *p = &retbuf[sizeof(retbuf)-1];
		*p = '\0';
		
		int i = 0;
		long n = (long) (fabs(fTest));
		
		if (fRemain != .0f) {
			char strRemain[16];
			sprintf(strRemain, "%.2f", fRemain);
			*--p = strRemain[3];
			*--p = strRemain[2];
			*--p = strRemain[1];
		}
		/*
		 if(comma == '\0') {
		 struct lconv *lcp = localeconv();
		 if(lcp != NULL) {
		 if(lcp->thousands_sep != NULL &&
		 *lcp->thousands_sep != '\0')
		 comma = *lcp->thousands_sep;
		 else	comma = ',';
		 }
		 }
		 */
		
		do {
			if(i%3 == 0 && i != 0)
				*--p = comma;
			*--p = '0' +  n % 10;
			n /= 10;
			i++;
		} while(n != 0);
		
	    if (bNeg) *--p = '-';
		return p;
}

// based on boinc/api/gutil.cpp -- but makes an alpha map out of an RGB file for transparency
// basic pic editors such as Gimp are pretty easy to set a weird color not in the main image of course)
// or even easier -- use Gimp to set "Color to Alpha" and just embed it in the .rgb file as an RGBA (A = "alpha channel")
GLuint CreateRGBTransparentTexture(const char* strFileName, float* transColor)   // default in prototype to transColor = NULL i.e. no "filter color" required
{    
	// transcolor is an optional (but necessary for Z=3 i.e. RGB) which will flip the alpha of an RGB color (i.e. Magenta would be 255/0/255 passed in transColor
	
        GLuint uiTexture = 0;
        int sizeX, sizeY, sizeZ;	
        // Load the image and store the data - this is in rgba format but the a is 255 (max)
        unsigned int *pImage = read_rgb_texture(strFileName,&sizeX,&sizeY,&sizeZ);  // read_rgb_texgture makes a 4channel (1 byte per R/G/B/A) anyway, so may as well use the "A"!
        if(pImage == NULL) return 0;
     	if (sizeZ != 3 && sizeZ != 4) { // needs to be RGB i.e. z=3 or RGBA z=4
		  free(pImage);
		  return 0;
	    }
	
	// need to set transparency bytes/alpha value every place in pImage  using magenta 255/0/255!
	// also note image needs to be flipped vertically when you save it! (at least in gimp)
	if (sizeZ == 3 && transColor) { //rgb -> rgba via the RGB values in transColor[3] array, if transColor not set not much use to this so just default to the RGBA created above (A=255 for all pixels)
		for(int i = 0; i < (sizeX * sizeY); i++)
		{
			unsigned char* bb = (unsigned char*) (pImage+i);    // easy pointer to our image pixels
			// just take the avg of the RGB -- as it approaches 0, the alpha goes to 0 (so basically the blacker, the more transperent)
			if(*bb == g_transColor[0]
			  && *(bb+1) == g_transColor[1]
			  && *(bb+2) == g_transColor[2] )
			{
				*(bb+3) = 0x00;   // If so, set alpha to fully transparent.   (note this sets all 4 bytes to 0)
			} // alpha already set to 255 if not transparent
			
			//iTest = (*bb + *(bb+1) + *(bb+2)) / 3;
			//*(bb+3) = iTest > 255 ? 255 : iTest; 
		}
	}	
	// sizeZ == 4 is just "straight" RGBA where alpha is embedded in the file; which is already taken care of by the above load_file
		
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glGenTextures( 1, &uiTexture );
	glBindTexture( GL_TEXTURE_2D, uiTexture );
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);  // this isn't as fast supposedly, but looks pretty good
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);                // also not as fast as GL_NEAREST but looks better!
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, 
		sizeX, sizeY, GL_RGBA, GL_UNSIGNED_BYTE, pImage);	
	
	free(pImage);  // free the mem allocated by the rgb_texture function
	return uiTexture;
}
	
// based on boinc/api/gutil.cpp -- but makes a GL_ALPHA out of an RGB file (sums values each point to make the alpha)
// the basic idea is you can make a simple image that can translate to a complex map -- white values (1) "pass" and black values (0) "block"
GLuint CreateRGBAlpha(const char* strFileName)
{
        //if(!strFileName && !boinc_file_exists(strFileName)) return 0;
        GLuint uiTexture = 0;
        int sizeX;
        int sizeY;
        int sizeZ;
        // Load the image and store the data
        unsigned int *pImage = read_rgb_texture(strFileName,&sizeX,&sizeY,&sizeZ);
        if(pImage == NULL) return 0;
        if (sizeZ > 1) { // error - just want a 1-D for alpha levels
            fprintf(stderr, "Improper RGB Image for Alpha: %s needs just 1 level, this file has %d\n", strFileName, sizeZ);
            free(pImage);
            return 0;
        }
        unsigned char* pByte = new unsigned char[sizeX*sizeY*sizeZ];
        memset(pByte, 0x00, sizeX*sizeY*sizeZ);
        for (int i = 0 ; i < (sizeX*sizeY*sizeZ) ; i++)  {
            pByte[i] = (unsigned char) *(pImage+i) & ~0xffffff00;   // get the final char from masking the higher bits
        }
        free(pImage); // don't need the image data, may as well free it
		
        glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        glGenTextures(1, &uiTexture);
        glBindTexture(GL_TEXTURE_2D, uiTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, sizeX, sizeY, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pByte);
		//glBindTexture(GL_TEXTURE_2D, 0);
        delete [] pByte;  // free the temporary byte array
        return uiTexture;
}
	
}   // namespace

