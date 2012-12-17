//
//  coreRenderer.cpp
//
//  Created by Tristan Lorach on 10/17/11.
//  Copyright (c) 2011 NVIDIA. All rights reserved.
//
#define DOCHECKGLERRORS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef ANDROID
#pragma mark - Android defs
#   pragma MESSAGE("-----------------------------------------------")
#   pragma MESSAGE("---- ANDROID ----------------------------------")
#   pragma MESSAGE("-----------------------------------------------")
#   include <jni.h>
#   include <android/log.h>
#   include <GLES2/gl2.h>
#   include <GLES2/gl2ext.h>
#   define  LOG_TAG    "nvFXTest1"
#   define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#   define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#elif defined(IOS)
#pragma mark - IOS defs
#   import <OpenGLES/ES2/gl.h>
#   import <OpenGLES/ES2/glext.h>
#   define  LOGI(...)  { printf(__VA_ARGS__); printf("\n"); }
#   define  LOGE(...)  { printf(__VA_ARGS__); printf("\n"); }
#elif defined (ES2EMU)
#pragma mark - ES2Emu
#   pragma MESSAGE("-----------------------------------------------")
#   pragma MESSAGE("---- Windows ES2 Emulation --------------------")
#   pragma MESSAGE("-----------------------------------------------")
#   include <EGL/eglplatform.h>
#   include <EGL/egl.h>
#   include <EGL/eglext.h>
#   include <GLES2/gl2platform.h>
#   include <GLES2/gl2.h>
#   include <GLES2/gl2ext.h>
#   include "logging.h"
#   define  LOGI(...)  { PRINTF((__VA_ARGS__)); PRINTF(("\n")); }
#   define  LOGE(...)  { PRINTF((__VA_ARGS__)); PRINTF(("\n")); }
#else
#pragma mark - Windows defs
#   pragma MESSAGE("-----------------------------------------------")
#   pragma MESSAGE("---- Windows + Glew ---------------------------")
#   pragma MESSAGE("-----------------------------------------------")
#   include <windows.h>
#   include "gl\glew.h"
//#define GL_GLEXT_PROTOTYPES
//#include <gl\glext.h>
//#   include <gl\glut.h>
#   include "logging.h"
#   define  LOGI(...)  { PRINTF((__VA_ARGS__)); PRINTF(("\n")); }
#   define  LOGE(...)  { PRINTF((__VA_ARGS__)); PRINTF(("\n")); }
#endif

#pragma mark - Misc
#include "noise_ex.h"

#include "coreRenderer.h"
#include "oglText.h"
#include "tracedisplay.h"

#include <FXLibEx.h>
#include "FXParser.h"

#include "nv_dds.h"
//#include "noise.h"
#include "glError.h"

//#include "sockServer.h"
#include "sockClient.h"
#include "datagrams.h"

#define READ_VEC4_(var, dest) {\
nvFX::IUniform* p = fxSettings->findUniform(#var);\
    if(p) p->getValuefv(dest, 4); }
#define READ_VEC4(var) {\
nvFX::IUniform* p = fxSettings->findUniform(#var);\
    if(p) p->getValuefv(var.vec_array, 4); }
#define READ_VEC4_ARRAY(dest, n, var, sz) {\
nvFX::IUniform* p = fxSettings->findUniform(#var);\
    if(p) p->getValuefv(dest[n].vec_array, 4, sz); }
#define READ_VEC2(var) {\
nvFX::IUniform* p = fxSettings->findUniform(#var);\
    if(p) p->getValuefv(var.vec_array, 2); }
#define READ_FLOAT(var) {\
nvFX::IUniform* p = fxSettings->findUniform(#var);\
    if(p) p->getValuefv(&var, 1); }
#define READ_BOOL(var) {\
nvFX::IUniform* p = fxSettings->findUniform(#var);\
    if(p) p->getValuebv(&var, 1); }
#define READ_INT3(var) {\
nvFX::IUniform* p = fxSettings->findUniform(#var);\
    if(p) p->getValueiv(var, 3); }
#define READ_INT4(var) {\
nvFX::IUniform* p = fxSettings->findUniform(#var);\
    if(p) p->getValueiv(var, 4); }
#define READ_INT2(var) {\
nvFX::IUniform* p = fxSettings->findUniform(#var);\
    if(p) p->getValueiv(var, 2); }
#define READ_INT(var) {\
nvFX::IUniform* p = fxSettings->findUniform(#var);\
    if(p) p->getValueiv(&var, 1); }
#define READ_STRING(var) {\
    const char* s = fxSettings->annotations()->getAnnotationString(#var);\
    if(s) var = s; }\

#define FIND_VEC4(var) {\
    fx_##var = pGLSLFx->findUniform(#var);\
    if(fx_##var) fx_##var->getValuefv(var.vec_array, 4); }
#define FIND_FLOAT(var) {\
    fx_##var = pGLSLFx->findUniform(#var);\
    if(fx_##var) fx_##var->getValuefv(&var, 1); }

//namespace nves2
//{
    //
    // GLSL-FX
    //
    nvFX::IContainer*    pGLSLFx        = NULL;
    nvFX::ITechnique*    fxTech         = NULL;
    nvFX::ITechnique*    fxTechHQBGND   = NULL;
    nvFX::ITechnique*    fxTechSMComputeWave = NULL;
    nvFX::ICstBuffer*    fxSettings     = NULL;
    static int           curPass = 0;
    nvFX::IPass*         fxPass_current = NULL;
    nvFX::IUniform*      fx_hq_box_color = NULL;

    //
    // Demo configuration data
    // most values come from the effect file
    //
    nvFX::IUniform*      fx_canvas       = NULL;
    vec2  canvas = vec2(800,600);
    nvFX::IUniform*      fx_screenRatio  = NULL;
    float screenRatio = 1.0;
    bool  fullscreen = false;

    std::string effectPathName = "effect.glslfx";
    std::string path_resources = "art_asset";
    std::string font_name = "Arial_48";
    std::string font_name_small = "Arial_16";
    std::string graph_prefix = "SM #";
    std::string graph_val_suffix = "%.2f GFlops";
    std::string general_perf = "%.2f TFLOPS";
    std::string hyperq_on = "HyperQ ON";
    std::string hyperq_off = "HyperQ OFF";

    float   lineThickness = 3.0;
    bool    smoothLine = true;
    vec4    clearColor = vec4(0.0,0.0,0.0,1.0);
    vec4    HQ_text = vec2(425, 80);
    vec4    HQ_box = vec4(300, 67, 257, 74);
    vec4    HQ_box_color_on  = vec4( 0.3, 0.8, 0.3, 0.8 );
    vec4    HQ_box_color_off = vec4( 0.8, 0.3, 0.3, 0.8 );
    vec4    chip_pos = vec4(100, 175, 752, -1.0); // -1 to let the app compute the size according to texture ratio
    vec4    graphs_area = vec4(846, 46, 1263-846, 966-46); // the location where we draw the graphs
    vec2    graphs_margin = vec2(12.0, 12.0);
    bool    graphs_static_disp = true;
    bool    graphs_fill_in = true;
    vec2    global_perf_pos = vec2(425, 934);
    int     graphs_distrib[4] = {2,8,16,16}; // # of colums, # of lines, and # of graphs to display
    #define NGRAPHS graphs_distrib[2]
    #define NSMS graphs_distrib[3]
    vec4    smboxes[15] = {vec4(133,210,190,94),vec4(327,210,190,94),vec4(523,210,190,94), vec4(133,544,190,94),
                           vec4(133,398,190,94),vec4(327,398,190,94),vec4(523,398,190,94), vec4(523,544,190,94),
                           vec4(133,492,190,94),vec4(327,492,190,94),vec4(523,492,190,94), vec4(133,638,190,94),
                           vec4(523,638,190,94),vec4(133,732,190,94),vec4(523,732,190,94)};
    int     cores_in_sm[2] = {4,8};
    int     graphs_font_color[2] = {3,0};
    int     refresh_cycle = 1.0;
    float   TAU = 0.0f;
    int     graphColorId = 0;

    int     port = 8080;
    std::string server_name = "localhost";

    vec2    maxDurationInSM = vec2(2.0, 1.0);

    ///////////////////////////////////////////////////////////////
    // internal objects
    //

    bool    hyperq_mode = false;
    float   tfPerf = 0.0f;
    float   heatTest = 1.0f;
    float   noiseSpeed = 0.1f;
    float   heatNoiseFact = 0.5f;
    float   currentTickSM = 0.0; // to only take into account the Datagrams that are > than the ones we already received
    float   currentTickGraph = 0.0; // to only take into account the Datagrams that are > than the ones we already received

    OGLText             oglText;
    OGLText             oglTextSmall;
    struct Attrib { Attrib() {} Attrib(vec2 _p, vec2 _tc) : p(_p), tc(_tc) {}
        vec2 p;
        vec2 tc;
    };
    static Attrib chipQuad[] = {
        Attrib(vec2(0.0, 0.0), vec2(0.0,0.0)),
        Attrib(vec2(0.5, 0.0), vec2(1.0,0.0)),
        Attrib(vec2(0.0, 0.5), vec2(0.0,1.0)),
        Attrib(vec2(0.5, 0.5), vec2(1.0,1.0)),
    };
    // very basic way of displaying a button ;-)
    static Attrib hqQuad[] = {
        Attrib(vec2(0.0, 0.0), vec2(0.0,0.0)),
        Attrib(vec2(0.5, 0.0), vec2(1.0,0.0)),
        Attrib(vec2(0.0, 0.5), vec2(0.0,1.0)),
        Attrib(vec2(0.5, 0.5), vec2(1.0,1.0)),
    };

    static int	ATTRIB_VERTEX = 0;
    static int	ATTRIB_TC = 1;

    static int	SMCWAVE_ATTRIB_VERTEX = 0;
    static int	SMCWAVE_ATTRIB_TC0 = 1;
    static int	SMCWAVE_ATTRIB_TC1 = 2;
    
    int width, height;
    float chipImageRatio = 1.0;

    float durationInSM = 0.0;
    float durationInSMTransition = 0.0;
    int SMShift = 0;

    void updateGraphPositions();

#pragma mark - Compute wave
    CCriticalSection* csTraceUpdate = NULL;

    struct SMComputeWave
    {
        // current value
        float               value;
        float               scale;
        float               bias;
        // status of the SM on-chip
        float noiseShift;
        float heat;
        float t, t2;
    };
    struct Graph
    {
        // current value
        float               targetValue;
        float               curValue;
        float               velocity;
        // Graph associated with this SM
        CTrace<float>       trace;
        COGLTraceDisplay    traceDisp;
    };
    SMComputeWave*      pSMComputeWave;
    Graph*              pGraph;
    // the table of vertices for all the possible SM boxes
    struct SMAttrib
    {
        vec2 p;
        vec4 uvCoordMask; // uv box for fetching the chip mask & noise; z for the depth of the noise volume
        vec4 uvCoordGradient; // uv for fetching the moving gradient; z for the intensity
    };
    SMAttrib *pSMvertices;

    void update_SMValues(SMUpdate* data)
    {
        if(data->tick <= currentTickSM)
        {
            //LOGE("Data older than previous ones... (%d <= %d) !", data->currentTick, currentTick);
            return;
        }
        csTraceUpdate->Enter();
        currentTickSM = data->tick;
        for(int i=0; i<data->numSMs; i++)
        {
            int smi = i+data->smOffset;
            if(smi >= 16)
            {
                LOGE("server trying to access SMs beyond the limit (%d >= %d) !", smi, 16);
                break;
            }
            SMComputeWave & sm = pSMComputeWave[smi];
            // only update the targetValue : the trace will read it at update() time
            sm.value = data->vals[i];
            sm.scale = data->scale;
            sm.bias = data->bias;
        }
        csTraceUpdate->Exit();
    }

    void update_GraphValues(GraphUpdate* data)
    {
        if(data->tick <= currentTickGraph)
        {
            //LOGE("Data older than previous ones... (%d <= %d) !", data->currentTick, currentTick);
            return;
        }
        csTraceUpdate->Enter();
        currentTickGraph = data->tick;
        for(int i=0; i<data->numGraphs; i++)
        {
            int g = i+data->graphOffset;
            if(g >= 16)
            {
                LOGE("server trying to access SMs beyond the limit (%d >= %d) !", g, 16);
                break;
            }
            Graph & sm = pGraph[g];
            // only update the targetValue : the trace will read it at update() time
            sm.targetValue = data->vals[i];
            sm.traceDisp._dataBias = data->bias;
            if(data->scale > 0.0f)
                sm.traceDisp._dataScale = data->scale;
        }
        csTraceUpdate->Exit();
    }

#pragma mark - Sockets
/////////////////////////////////////////////////////////////////////
//
//
//class CServer : public CBaseServer
//{
//public:
//    CServer() : CBaseServer(), CThread(/*startNow*/false, /*Critical*/false)
//    {}
//    virtual void CThreadProc()
//    {
//        char buffer[100];
//        int l;
//
//        while (1)
//        {
//            if ((l=recieve(buffer, MSG_MAX_SZ)) == 0)
//            {
//                LOGI("Bad error in listenForMessagesThread");
//            }
//            else
//            {
//                switch(buffer[0])
//                {
//                case DG_MSG:
//                    LOGI("Message: %s", buffer+1);
//                    break;
//                case DG_QUIT:
//                    {
//                        extern void quit();
//                        quit();
//                        break;
//                    }
//                case DG_SMUPDATE:
//                    update_SMValues((SMUpdate*)buffer);
//                    break;
//                case DG_HYPERQ:
//                    csTraceUpdate->Enter();
//                    hyperq_mode = ((SIntArg*)buffer)->ival != 0;
//                    csTraceUpdate->Exit();
//                    break;
//                case DG_REFRESHCYCLE:
//                    csTraceUpdate->Enter();
//                    refresh_cycle = ((SIntArg*)buffer)->ival;
//                    csTraceUpdate->Exit();
//                    break;
//                case DG_TERAFLOPS:
//                    csTraceUpdate->Enter();
//                    tfPerf = ((SFloatArg*)buffer)->fval;
//                    csTraceUpdate->Exit();
//                    break;
//                case DG_FULLSCREEN:
//                    {
//                        setFullScreen(((SIntArg*)buffer)->ival ? true : false, true);
//                    }
//                default:
//                    LOGI("Token %d...", buffer[0]);
//                    break;
//                }
//            }
//            Sleep(1);
//        }
//    }
//};
//CServer server; // this viewer is a server, too : to receive commands

class CClient : public CBaseClient
{
public:
    CClient() : CBaseClient(), CThread(/*startNow*/false, /*Critical*/false)
    {}
    virtual void CThreadProc()
    {
        char buffer[MSG_MAX_SZ];
        int l;

        while (1)
        {
            if((l=recieve(buffer, MSG_MAX_SZ)) == -1)
            {
                LOGI("server issue... trying to connect...");
                if(Init(server_name.c_str(), port))
                {
                    currentTickSM = 0.0;
                    currentTickGraph = 0.0;
                    Send(DG_REGISTER);
                }
                Sleep(1000);
            }
            else
            {
                switch(buffer[0])
                {
                case DG_MSG:
                    LOGI("Message: %s", buffer+1);
                    break;
                case DG_QUIT:
                    {
                        extern void quit();
                        quit();
                        break;
                    }
                case DG_SMUPDATE:
                    update_SMValues((SMUpdate*)buffer);
                    break;
                case DG_GRAPHUPDATE:
                    update_GraphValues((GraphUpdate*)buffer);
                    break;
                case DG_SETSMAMOUNT: // change of the # of SMs being active
                    if(((SIntArg*)buffer)->ival > 0)
                    {
                        csTraceUpdate->Enter();
                        NSMS = ((SIntArg*)buffer)->ival;
                        LOGI("Changing the # of active SMs to %d", NSMS);
                        csTraceUpdate->Exit();
                    }
                    break;
                case DG_SETGRAPHAMOUNT: // change of the # of graphs being active
                    if(((SIntArg*)buffer)->ival > 0)
                    {
                        csTraceUpdate->Enter();
                        NGRAPHS = ((SIntArg*)buffer)->ival;
                        LOGI("Changing the # of active Graphs to %d", NGRAPHS);
                        graphs_distrib[1] = (NGRAPHS>>1) + (NGRAPHS&1);
                        updateGraphPositions();
                        csTraceUpdate->Exit();
                    }
                    break;
                case DG_HYPERQ:
                    csTraceUpdate->Enter();
                    hyperq_mode = ((SIntArg*)buffer)->ival != 0;
                    csTraceUpdate->Exit();
                    break;
                case DG_REFRESHCYCLE:
                    csTraceUpdate->Enter();
                    refresh_cycle = ((SIntArg*)buffer)->ival;
                    csTraceUpdate->Exit();
                    break;
                case DG_TERAFLOPS:
                    csTraceUpdate->Enter();
                    tfPerf = ((SFloatArg*)buffer)->fval;
                    csTraceUpdate->Exit();
                    break;
                case DG_FULLSCREEN:
                    {
                        setFullScreen(((SIntArg*)buffer)->ival ? true : false, true);
                    }
                    break;
                default:
                    LOGI("Token %d...", buffer[0]);
                    break;
                }
            }
            Sleep(1);
        }
    }
};
CClient client; // the one client : where the experiment is happening

#pragma mark - Printing of Infos

    //-----------------------------------------------------------------------------
    // Effect error Cb
    //-----------------------------------------------------------------------------
    static void printGLString(const char *name, GLenum s) {
        const char *v = (const char *) glGetString(s);
        LOGI("GL %s = %s\n", name, v);
    }
    
    static void checkGlError(const char* op) {
        for (GLint error = glGetError(); error; error
             = glGetError()) {
            LOGI("after %s() glError (0x%x)\n", op, error);
        }
    }
    
    void myErrorCb(const char *errMsg)
    {
        LOGE("%s", errMsg);
    }
    /*----------------------------------------------
      --
      -- Include Callback : when #include in effect get invoked
      --
      ----------------------------------------------*/
    void myIncludeCb(const char *incName, FILE *&fp, const char *&buf)
    {
        char tmpName[200];
        fp = fopen(incName, "r");
        if(fp)
            return;
        // trying a specific folder
        sprintf(tmpName, "%s\\%s", path_resources.c_str(), incName);
        fp = fopen(tmpName, "r");
    }

#pragma mark - Init

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // 
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool initEffect(const char * fname)
    {
        LOGI("initEffect : Creating the Effect");
        //
        // Effect
        //
        effectPathName = path_resources + std::string("/") + std::string(fname);
        pGLSLFx = nvFX::IContainer::create(fname);
        if(!pGLSLFx)
        {
            LOGE("FAiled to create a Container");
            return false;
        } else LOGI("Created IContainer");
        nvFX::setErrorCallback(myErrorCb);
        if(!nvFX::loadEffectFromFile(pGLSLFx, effectPathName.c_str()))
        {
            LOGE("Failed to parse %s", fname);
            //pGLSLFx->destroy(); // destroy not ready... TODO
            pGLSLFx = NULL;
            return false;
        }
        LOGI("initEffect : Parsed the effect !");
        
        fxTech = pGLSLFx->findTechnique("DisplayChip");
        if(!fxTech)
        {
            LOGE("Error>> couldn't load the technique DisplayChip\n");
            return false;
        }
        bool bRes = fxTech->validate();
        if(!bRes)
        {
            LOGE("Error>> couldn't validate the technique DisplayChip\n");
            fxTech = NULL;
            return false;
        }
        checkGlError("fxTech->validate");
        fxTechSMComputeWave = pGLSLFx->findTechnique("SMComputeWave");
        if(!fxTechSMComputeWave)
        {
            LOGE("Error>> couldn't load the technique SMComputeWave\n");
            return false;
        } else {
            bRes = fxTechSMComputeWave->validate();
            if(!bRes)
            {
                LOGE("Error>> couldn't validate the technique SMComputeWave\n");
                fxTechSMComputeWave = NULL;
                return false;
            }
        }
        fxTechHQBGND = pGLSLFx->findTechnique("HQBGND");
        if(!fxTechHQBGND)
        {
            LOGE("Error>> couldn't load the technique HQBGND\n");
            return false;
        } else {
            bRes = fxTechHQBGND->validate();
            if(!bRes)
            {
                LOGE("Error>> couldn't validate the technique HQBGND\n");
                fxTechHQBGND = NULL;
                return false;
            }
        }
        //    fxnoiseTex  = pGLSLFx->getExInterface()->createUniform("noise3D");
        //    fxnoiseTex->setSamplerResource(nvFX::TEXTURE_3D_OES, TO_NOISE3D,6);
        //
        // Load textures on demande from the effect
        //
        nvFX::ITexture *pTex = NULL;
        for(int i=0; pTex = pGLSLFx->findTexture(i); i++)
        {
            if(pTex)
            {
                const char *fname = pTex->annotations()->getAnnotationString("defaultFile");
                //
                // Load a texture
                //
                if(fname)
                {
                    if(!strcmp(fname, "3D"))
                    {
                        FilteredNoiseEx*    noise;
                        CPerlinNoise*       pnoise;
                        noise = new FilteredNoiseEx(16,16,16),
                        pnoise = new CPerlinNoise();
                        GLuint texture;
                        glGenTextures(1, &texture);
                        glActiveTexture( GL_TEXTURE1 );
                        glBindTexture(GL_TEXTURE_3D, texture);
                        srand(250673);
                        noise->createNoiseTexture3D(64,64,64, 4, 1, FilteredNoise::ABSNOISE, texture);
                        pTex->setGLTexture(nvFX::TEXTURE_3D, texture);
                        glBindTexture(GL_TEXTURE_3D, 0);
                        continue;
                    }
                    if(!strcmp(fname, "3D2"))
                    {
                        FilteredNoiseEx*    noise;
                        CPerlinNoise*       pnoise;
                        noise = new FilteredNoiseEx(16,16,16),
                        pnoise = new CPerlinNoise();
                        GLuint texture;
                        glGenTextures(1, &texture);
                        glActiveTexture( GL_TEXTURE1 );
                        glBindTexture(GL_TEXTURE_3D, texture);
                        srand(250673);
                        noise->createNoiseTexture3D(64,64,64, 4, 1, FilteredNoise::VEINS, texture);
                        pTex->setGLTexture(nvFX::TEXTURE_3D, texture);
                        glBindTexture(GL_TEXTURE_3D, 0);
                        continue;
                    }
                    std::string s = path_resources + std::string("\\") + std::string(fname);
                    nv_dds::CDDSImage* ddsImage = new nv_dds::CDDSImage;
                    bool bRes = ddsImage->load(fname);
                    if(bRes == false)
                        bRes = ddsImage->load(s);
                    if(!bRes)
                    {
                        LOGE("Failed to load texture %s\n", fname);
                    } else {
                        if(pTex->annotations()->getAnnotationInt("computeRatio") == 1)
                            chipImageRatio = (float)ddsImage->get_width()/(float)ddsImage->get_height();
                        nvFX::TextureType texType = pTex->getType();
                        GLuint texture;
                        glGenTextures(1, &texture);
                        glActiveTexture(GL_TEXTURE1);// to remove...
                        switch(texType)
                        {
                        case nvFX::TEXTURE_1D:
                            glBindTexture(GL_TEXTURE_1D, texture);
                            glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
                            if(!ddsImage->upload_texture1D())
                                return false;
                            pTex->setGLTexture(nvFX::TEXTURE_1D, texture);
                            break;
                        case nvFX::TEXTURE_2D:
                            glBindTexture(GL_TEXTURE_2D, texture);
                            glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
                            if(!ddsImage->upload_texture2D())
                                return false;
                            pTex->setGLTexture(nvFX::TEXTURE_2D, texture);
                            break;
                        case nvFX::TEXTURE_3D:
                            glBindTexture(GL_TEXTURE_3D, texture);
                            glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
                            if(!ddsImage->upload_texture3D())
                                return false;
                            pTex->setGLTexture(nvFX::TEXTURE_3D, texture);
                            break;
                        case nvFX::TEXTURE_RECTANGLE:
                            glBindTexture(GL_TEXTURE_RECTANGLE, texture);
                            glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
                            if(!ddsImage->upload_textureRectangle())
                                return false;
                            pTex->setGLTexture(nvFX::TEXTURE_RECTANGLE, texture);
                            break;
                        }
                    }
                    delete ddsImage;
                } else {
                    LOGE("No 'defaultFile' in texture %s\n", pTex->getName());
                }
            }
        }
        //
        // Query the technique about the used attributes.(effect mapped them with "Attribute(...) = #;")
        // we could also query the passes...
        // This is an example : the idea here is to adapt to the effect we loaded
        // and setup the mapping of the geometry correctly for what the effect asked for
        // Note that we don't really know what names corresponds to what attribute in the geometry
        // but we can guess with casual keywords (pos, normal, binorm...)
        //
        if(fxTech->getNumBoundAttributes())
        {
            // Note: this action is optional if
            // - the app knows exactly which attributes locations will be used in any case
            // - the app does the mapping of attributes locations with IPass/ITechnique::bindAttribute()
            ATTRIB_VERTEX = -1;
            ATTRIB_TC = -1;
            for(int loc=0; loc<15; loc++)
            {
                LPCSTR name = fxTech->getAttributeNameForLocation(loc);
                if(!name)
                    continue;
                if( strstr(name, "pos") || strstr(name, "Pos") || strstr(name, "POS") )
                {
                    ATTRIB_VERTEX = loc;
                }
                else if( strstr(name, "tc") || strstr(name, "Tc") || strstr(name, "TC") || strstr(name, "texcoord") || strstr(name, "Texcoord") || strstr(name, "TEXCOORD") )
                {
                    ATTRIB_TC = loc;
                }
            }
        }
        if(fxTechSMComputeWave->getNumBoundAttributes())
        {
            SMCWAVE_ATTRIB_VERTEX = -1;
            SMCWAVE_ATTRIB_TC0 = -1;
            SMCWAVE_ATTRIB_TC1 = -1;
            for(int loc=0; loc<15; loc++)
            {
                LPCSTR name = fxTech->getAttributeNameForLocation(loc);
                if(!name)
                    continue;
                if(!strcmp(name, "position"))
                {
                    SMCWAVE_ATTRIB_VERTEX = loc;
                }
                else if(!strcmp(name, "texcoord0"))
                {
                    SMCWAVE_ATTRIB_TC0 = loc;
                }
                else if(!strcmp(name, "texcoord1"))
                {
                    SMCWAVE_ATTRIB_TC1 = loc;
                }
            }
        }
        LOGI("initEffect : assigned the attributes and buffers");

        return true;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // 
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void updateGraphPositions()
    {
        vec2 gstep = vec2(graphs_area.z / (float)graphs_distrib[0], graphs_area.w / (float)graphs_distrib[1]);
        int i = 0;
        for(int ix=0; ix<graphs_distrib[0]; ix++)
        for(int iy=0; iy<graphs_distrib[1]; iy++)
        {
            if(i >= NGRAPHS)
                break;
            Graph* pG = pGraph + i;
            // graph dimensions
            float x = graphs_area.x + (float)ix*gstep.x;
            float y = graphs_area.y + (float)iy*gstep.y;
            // setup params
            pG->traceDisp.Position(x,y, gstep.x-graphs_margin.x, gstep.y-graphs_margin.x);
            i++;
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // 
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool init(const char * pathresources, const char * effectName, const char * servername, const int *_port)
    {
        bool bRes;
        printGLString("Version", GL_VERSION);
        printGLString("Vendor", GL_VENDOR);
        printGLString("Renderer", GL_RENDERER);
        printGLString("Extensions", GL_EXTENSIONS);

        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_LESS );
        
        checkGlError("glViewport");
        //
        // basic path setup
        //
        if(pathresources)
        {
            path_resources = pathresources;
        } else {
            path_resources.clear();
        }

        //
        // Here we set the callback for #include : the way we load includes is very specific to Windows sample
        //
        nvFX::setIncludeCallback(myIncludeCb);

        //bRes = initTextures();

        if(!initEffect(effectName))
		{
			// TODO: destroy the effect
			pGLSLFx = NULL;
		}
        if(pGLSLFx)
        {
            // ---------------------------------------------------------------
            // Settings for the demo
            // these settings are in "settings" constant buffer. But this
            // buffer is never going to be bound to OpenGL : it is only
            // here to allow some configuration to be parsed by the demo
            // (here is a good example on how to extend the use of nvFX ;-)
            //
            fxSettings = pGLSLFx->findCstBuffer("settings");
            if(fxSettings)
            {
                READ_STRING(font_name)
                READ_STRING(font_name_small)
                READ_STRING(graph_prefix)
                READ_STRING(graph_val_suffix)
                READ_STRING(general_perf)
                READ_STRING(hyperq_on)
                READ_STRING(hyperq_off)

                READ_FLOAT(lineThickness)
                READ_BOOL(smoothLine)
                READ_VEC4(clearColor)
                READ_VEC4(HQ_box)
                READ_VEC2(HQ_text)
                READ_VEC4(HQ_box_color_on)
                READ_VEC4(HQ_box_color_off)
                READ_VEC4(chip_pos)
                READ_VEC4(graphs_area)
                READ_VEC2(graphs_margin)
                READ_BOOL(graphs_static_disp)
                READ_BOOL(graphs_fill_in)
                READ_VEC4_(graph_fill_color0, CTraceDisplay::fill_color0)
                READ_VEC4_(graph_fill_color1, CTraceDisplay::fill_color1)

                READ_VEC2(global_perf_pos)
                READ_INT4(graphs_distrib)
        // quads that cover the chip. Typically where the SMs are located
        // Note : a limitation in nvFX prevents to put them all in one... TODO : fix this asap
                READ_VEC4_ARRAY(smboxes, 0, smboxes0, 4)
                READ_VEC4_ARRAY(smboxes, 4, smboxes1, 4)
                READ_VEC4_ARRAY(smboxes, 8, smboxes2, 4)
                READ_VEC4_ARRAY(smboxes, 12, smboxes3, 3)
                READ_INT2(cores_in_sm)
                // params going directly into the static vars of the graphs
                nvFX::IUniform* fxColors = fxSettings->findUniform("graph_colors");
                if(fxColors)
                {
                    CTraceDisplay::cNbColors = fxColors->getArraySz();
                    fxColors->getValuefv(CTraceDisplay::colors[0], 4, CTraceDisplay::cNbColors);
                }
                READ_INT(graphColorId);
                READ_VEC4_(graph_edge_color, CTraceDisplay::edge_color)
                READ_VEC4_(graph_grid_color, CTraceDisplay::grid_color)
                READ_INT2(graphs_font_color);
                READ_INT(refresh_cycle);
                READ_FLOAT(TAU);
                READ_VEC2(maxDurationInSM)

				if(_port == NULL)
				{
					READ_INT(port);
				}
				if(servername == NULL)
				{
					READ_STRING(server_name);
				}

            } //if(fxSettings)
            //
            // Adjust the Chip quad
            //
            if(chip_pos.z < 0)
                chip_pos.z = chip_pos.w * chipImageRatio;
            else if(chip_pos.w < 0)
                chip_pos.w = chip_pos.z / chipImageRatio;
            chipQuad[0].p.x = chip_pos.x;
            chipQuad[2].p.x = chip_pos.x;
            chipQuad[1].p.x = chip_pos.z+chip_pos.x;
            chipQuad[3].p.x = chip_pos.z+chip_pos.x;
            chipQuad[0].p.y = chip_pos.y;
            chipQuad[1].p.y = chip_pos.y;
            chipQuad[2].p.y = chip_pos.w+chip_pos.y;
            chipQuad[3].p.y = chip_pos.w+chip_pos.y;
            //
            // Adjust the HyperQ 'button'
            //
            hqQuad[0].p.x = HQ_box.x;
            hqQuad[2].p.x = HQ_box.x;
            hqQuad[1].p.x = HQ_box.z+HQ_box.x;
            hqQuad[3].p.x = HQ_box.z+HQ_box.x;
            hqQuad[0].p.y = HQ_box.y;
            hqQuad[1].p.y = HQ_box.y;
            hqQuad[2].p.y = HQ_box.w+HQ_box.y;
            hqQuad[3].p.y = HQ_box.w+HQ_box.y;
            // -------------------------------------------------------------------
            // Other settings that are outside of the 'settings' buffer
            // Settings that are not only for the setup of the demo, but also used
            // in shaders
            //
            FIND_FLOAT(screenRatio)
            FIND_VEC4(canvas)
            setWindowSize(-1, -1, canvas.x, canvas.y);
            READ_BOOL(fullscreen);
            if(fullscreen)
                setFullScreen(true, true);

            fx_hq_box_color = pGLSLFx->findUniform("hq_box_color");
            // -------------------------------------------------------------------
            // Text rendering init : only 2 kind of fonts : small and big
            //
            if(!oglText.init(std::string(path_resources + "/" + font_name).c_str(), canvas.x, canvas.y))
                return false;
            if(!oglTextSmall.init(std::string(path_resources + "/" + font_name_small).c_str(), canvas.x, canvas.y))
                return false;
            // -------------------------------------------------------------------
            // trace init : depends on what was requested
            //
            COGLTraceDisplay::Init(canvas.x, canvas.y);
        }
        //
        // SM Compute wave effects : init visual overlays
        //
        int i = 0;
        pSMComputeWave = new SMComputeWave[16]; // those are for the visual effect on SMs. : same # than the graphs
        pSMvertices =  new SMAttrib[4*16]; // vertices for all possible boxes
        for(i=0; i<16; i++)
        {
            pSMComputeWave[i].t = 0.0f;
            pSMComputeWave[i].t2 = 0.0f;
            pSMComputeWave[i].heat = 0.0f;
            pSMComputeWave[i].noiseShift = 0.0f;
            pSMComputeWave[i].value = 0.0f;
            pSMvertices[i*4 + 0].p.x = smboxes[i].x;
            pSMvertices[i*4 + 2].p.x = smboxes[i].x;
            pSMvertices[i*4 + 1].p.x = smboxes[i].z+smboxes[i].x;
            pSMvertices[i*4 + 3].p.x = smboxes[i].z+smboxes[i].x;
            pSMvertices[i*4 + 0].p.y = smboxes[i].y;
            pSMvertices[i*4 + 1].p.y = smboxes[i].y;
            pSMvertices[i*4 + 2].p.y = smboxes[i].w+smboxes[i].y;
            pSMvertices[i*4 + 3].p.y = smboxes[i].w+smboxes[i].y;
            // uvs for the mask
            vec2 p = vec2(pSMvertices[i*4 + 0].p - chipQuad[0].p) * vec2(1.0f/chip_pos.z, 1.0f/chip_pos.w);
            pSMvertices[i*4 + 0].uvCoordMask.x = p.x;
            pSMvertices[i*4 + 0].uvCoordMask.y = p.y;
            p = vec2(pSMvertices[i*4 + 1].p - chipQuad[0].p) * vec2(1.0f/chip_pos.z, 1.0f/chip_pos.w);
            pSMvertices[i*4 + 1].uvCoordMask.x = p.x;
            pSMvertices[i*4 + 1].uvCoordMask.y = p.y;
            p = vec2(pSMvertices[i*4 + 2].p - chipQuad[0].p) * vec2(1.0f/chip_pos.z, 1.0f/chip_pos.w);
            pSMvertices[i*4 + 2].uvCoordMask.x = p.x;
            pSMvertices[i*4 + 2].uvCoordMask.y = p.y;
            p = vec2(pSMvertices[i*4 + 3].p - chipQuad[0].p) * vec2(1.0f/chip_pos.z, 1.0f/chip_pos.w);
            pSMvertices[i*4 + 3].uvCoordMask.x = p.x;
            pSMvertices[i*4 + 3].uvCoordMask.y = p.y;
            // uvs for the gradient
            pSMvertices[i*4 + 0].uvCoordGradient = vec4(0,0,0,0);
            pSMvertices[i*4 + 1].uvCoordGradient = vec4(1,0,1,0);
            pSMvertices[i*4 + 2].uvCoordGradient = vec4(0,1,0,1);
            pSMvertices[i*4 + 3].uvCoordGradient = vec4(1,1,1,1);
        }
        //
        // Graphs
        //
        i = 0;
        pGraph = new Graph[16]; // those are for the visual effect on SMs. : same # than the graphs
        for(i=0; i<16; i++)
        {
            pGraph[i].targetValue = 0.0f;
            pGraph[i].curValue = 0.0f;
            pGraph[i].velocity = 0.0f;
        }
        vec2 gstep = vec2(graphs_area.z / (float)graphs_distrib[0], graphs_area.w / (float)graphs_distrib[1]);
        i = 0;
        for(int ix=0; ix<graphs_distrib[0]; ix++)
        for(int iy=0; iy<graphs_distrib[1]; iy++)
        {
            if(i >= 16)
                break;
            Graph* pG = pGraph + i;
            // graph dimensions
            float x = graphs_area.x + (float)ix*gstep.x;
            float y = graphs_area.y + (float)iy*gstep.y;
            // setup params
            pG->traceDisp.Position(x,y, gstep.x-graphs_margin.x, gstep.y-graphs_margin.x);
            pG->traceDisp.Clear();
            pG->trace.init((int)(gstep.x-graphs_margin.x)/2, "v"); // let's populate the graph buffer according to the precision onscreen
            pG->traceDisp.Insert(&pG->trace, graphColorId);//&pG->trace, 0);
            pG->traceDisp.BackgroundColor(0.5f, 0.5f, 0.5f, 0.1f);
            pG->traceDisp.DrawText(true);
            pG->traceDisp.SetTextDrawColumn(true);
            pG->traceDisp.SetDrawDoubleColumn(false);
            pG->traceDisp._dataBias = 0.0f;
            pG->traceDisp._dataScale = 15.0f;
            pG->traceDisp._pTextHi = &oglText;
            pG->traceDisp._pTextLo = &oglTextSmall;
            pG->traceDisp._graphs_static_disp = graphs_static_disp;
            pG->traceDisp._graphs_fill_in = graphs_fill_in;
            char title[40];
            sprintf(title, graph_prefix.c_str(), i+1);
            pG->traceDisp._title = title;
            pG->traceDisp._valueString = graph_val_suffix;
            pG->traceDisp._line_thickness = lineThickness;
            pG->traceDisp._smooth_line = smoothLine;
            pG->traceDisp._txt_name_color = graphs_font_color[0];
            pG->traceDisp._txt_val_color = graphs_font_color[1];
            i++;
        }
		// Sockets
		if(_port)
			port = *_port;
		if(servername)
			server_name = servername;
        csTraceUpdate = new CCriticalSection;
        //if(!server.Init(port+1))
        //    return false;
        // Initialize the client : the port = port+1 !!
        client.Init(server_name.c_str(), port);
        client.ResumeThread();
        // Tell the server we connect
        client.Send(DG_REGISTER);
        return true;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // 
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void pingServer()
    {
        LOGI("telling the server we are alive");
        client.Send(DG_REGISTER);
    }
    void toggleHyperQ()
    {
        LOGI("asking to toggle the HyperQ mode");
        client.Send(DG_HYPERQ);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // 
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void finish()
    {
        client.Close();
        if(pGLSLFx)
            nvFX::IContainer::destroy(pGLSLFx);
        pGLSLFx = NULL;
    }
    
#pragma mark - Rendering
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // 
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void render_image()
    {
        bool bContinue = true;

        glViewport( 0, 0, width, height );
        if(pGLSLFx == NULL)
        {
            glClearColor( 0.7f, 0.3f, 0.1f, 0.0f );
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
            return;
        }

        glClearColor( clearColor.x, clearColor.y, clearColor.z, clearColor.w );
        glClear( GL_COLOR_BUFFER_BIT);
                
        if(fxTech)
        {
            //
            // Execute the pass
            //
            fxPass_current = fxTech->getPass(curPass);
            if(fxPass_current == NULL)
            {
                curPass = 0;
                fxPass_current = fxTech->getPass(curPass);
            }
            //fxPass_current = fxTech->getPass(PASS_FLOOR);
            fxPass_current->execute();

            glEnableVertexAttribArray(ATTRIB_VERTEX);
            glEnableVertexAttribArray(ATTRIB_TC);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(Attrib), chipQuad);
            glVertexAttribPointer(ATTRIB_TC, 2, GL_FLOAT, GL_FALSE, sizeof(Attrib), chipQuad->tc.vec_array);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            glDisableVertexAttribArray(ATTRIB_VERTEX);
            glDisableVertexAttribArray(ATTRIB_TC);
        }
        //
        // Draw rectangle for the HyperQ mode
        //
        if(fxTechHQBGND)
        {
            nvFX::IPass * fxPass = fxTechHQBGND->getPass(0);
            //fxPass_current = fxTech->getPass(PASS_FLOOR);
            fxPass->execute();
            if(fx_hq_box_color)
                fx_hq_box_color->updateValue4fv(hyperq_mode ? HQ_box_color_on.vec_array : HQ_box_color_off.vec_array, fxPass);
            // recycling the attributes from the Chip map... hopefully this is ok... but not clean
            glEnableVertexAttribArray(ATTRIB_VERTEX);
            glEnableVertexAttribArray(ATTRIB_TC);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(Attrib), hqQuad);
            glVertexAttribPointer(ATTRIB_TC, 2, GL_FLOAT, GL_FALSE, sizeof(Attrib), hqQuad->tc.vec_array);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            glDisableVertexAttribArray(ATTRIB_VERTEX);
            glDisableVertexAttribArray(ATTRIB_TC);
        }
        //
        // Initiate Text rendering on 2 slots : for big and small fonts
        //
        oglText.beginString();
        oglTextSmall.beginString();
        //
        // Text rendering for the main performance
        //
        static char tmpStr[100];
        float bbStr[2];
        //csTraceUpdate->Enter();
        if(!general_perf.empty())
        {
            sprintf(tmpStr, general_perf.c_str(), tfPerf);
            oglText.stringSize(tmpStr, bbStr);
            oglText.drawString(global_perf_pos.x - bbStr[0]*0.5, global_perf_pos.y, tmpStr, 0,0xF0F0F0F0);
        }
        //
        // Text rendering for the mode (HyperQ vs. Non-HyperQ)
        //
        const char *hqtxt = hyperq_mode ? hyperq_on.c_str() : hyperq_off.c_str();
        oglText.stringSize(hqtxt, bbStr);
        oglText.drawString(HQ_text.x - bbStr[0]*0.5, HQ_text.y, hqtxt, 0,0xF0F0F0F0);
        //csTraceUpdate->Exit();
        //
        // Graphs display
        //
        COGLTraceDisplay::Begin();
        for(int i=0; i<NGRAPHS; i++)
        {
            pGraph[i].traceDisp.Display(CTraceDisplay::LINE_STREAM);
        }
        COGLTraceDisplay::End();
        //
        // End the text rendering session : Draw all
        //
        oglTextSmall.endString();
        oglText.endString();
        //
        // Draw the Compute wave effects
        //
        if(fxTechSMComputeWave)
        {
            //
            // Execute the pass
            //
            nvFX::IPass * fxPass = fxTechSMComputeWave->getPass(0);
            //fxPass_current = fxTech->getPass(PASS_FLOOR);
            fxPass->execute();

//glActiveTexture(GL_TEXTURE0+2);
//glEnable(GL_TEXTURE_3D);
//glBindTexture(GL_TEXTURE_3D, 3);
//glProgramUniform1i(7,19, 2);
//GLenum eeee = glGetError();

            glEnableVertexAttribArray(SMCWAVE_ATTRIB_VERTEX);
            glEnableVertexAttribArray(SMCWAVE_ATTRIB_TC0);
            glEnableVertexAttribArray(SMCWAVE_ATTRIB_TC1);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            //
            // Draw Compute waves
            //
            for(int i=0; i<15; i++)
            {
                // heat:
                float h = 0.0;
                int i1 = (i + SMShift)%15;
                int i2 = (i + 1 + SMShift)%15;
                if(i1 < NSMS)
                    h += pSMComputeWave[i1].heat * (1.0f - durationInSMTransition);
                if(i2 < NSMS)
                    h += pSMComputeWave[i2].heat * (durationInSMTransition);
                pSMvertices[i*4+0].uvCoordMask.w = h;
                pSMvertices[i*4+1].uvCoordMask.w = h;
                pSMvertices[i*4+2].uvCoordMask.w = h;
                pSMvertices[i*4+3].uvCoordMask.w = h;
                // noise depth
                float s = 0.0;
                if(i1 < NSMS)
                    s += pSMComputeWave[i1].noiseShift * (1.0f - durationInSMTransition);
                if(i2 < NSMS)
                    s += pSMComputeWave[i2].noiseShift * (durationInSMTransition);
                pSMvertices[i*4+0].uvCoordMask.z = s;
                pSMvertices[i*4+1].uvCoordMask.z = s;
                pSMvertices[i*4+2].uvCoordMask.z = s;
                pSMvertices[i*4+3].uvCoordMask.z = s;

                glVertexAttribPointer(SMCWAVE_ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(SMAttrib), pSMvertices[i*4+0].p.vec_array);
                glVertexAttribPointer(SMCWAVE_ATTRIB_TC0, 4, GL_FLOAT, GL_FALSE, sizeof(SMAttrib), pSMvertices[i*4+0].uvCoordMask.vec_array);
                glVertexAttribPointer(SMCWAVE_ATTRIB_TC1, 4, GL_FLOAT, GL_FALSE, sizeof(SMAttrib), pSMvertices[i*4+0].uvCoordGradient.vec_array);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }

            glDisableVertexAttribArray(SMCWAVE_ATTRIB_VERTEX);
            glDisableVertexAttribArray(SMCWAVE_ATTRIB_TC0);
            glDisableVertexAttribArray(SMCWAVE_ATTRIB_TC1);
        }
        CHECKGLERRORS();
    }

#pragma mark - Various events

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // 
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void resize(int w, int h)
    {    
        width = w;
        height = h;
        // we compute the ratio for the given canvas
        screenRatio = ((float)h*canvas.x)/((float)w*canvas.y);
        if(fx_screenRatio)
            fx_screenRatio->setValue1f(screenRatio);
        glViewport(0, 0, width, height);

        oglText.changeSize(width, height);
        oglTextSmall.changeSize(width, height);
        COGLTraceDisplay::changeSize(width, height);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // 
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void push(int up)
    {    
        //LOGI("Screen push %d", up);
        if(!up)
        {
            //glDisableVertexAttribArray( ATTRIB_VERTEX[curPass]);    
            //glDisableVertexAttribArray( ATTRIB_NORMAL[curPass]);
            //glDisableVertexAttribArray( ATTRIB_TAN[curPass]);
            //glDisableVertexAttribArray( ATTRIB_BINORM[curPass]);
            //glDisableVertexAttribArray( ATTRIB_TC[curPass]);

            curPass++;
        }
        CHECKGLERRORS();
    }

    void update(float dt)
    {
        // simple trick to change the rate of graph refresh
        static int cnt = 0;
        cnt--;

        // FAKE graphs, for now
        static float t = 0.0;
        t += 0.05;
        //
        // Advance Compute waves
        //
        if(cnt <= 0)
        {
            cnt = refresh_cycle;
            if(csTraceUpdate->TryEnter())
            {
                for(int i=0; i<NGRAPHS; i++)
                {
                    Graph &graph = pGraph[i];
                    // compute some 2nd order motion...
                    float v = graph.curValue - graph.targetValue;
                    if(TAU<= 0.0f)
                    {
                        graph.curValue = graph.targetValue;
                    } else if(TAU >= 1.0f)
                    {
                        graph.velocity = -v * TAU;
                        graph.curValue += graph.velocity * dt;
                    } else {
                        float accel = (-2.0f/TAU)*graph.velocity - v/(TAU*TAU);
                        // integrate
                        graph.velocity += accel * dt;
                        graph.curValue += graph.velocity * dt;
                    }
                    // write the result in the trace
                    graph.trace.insert(graph.curValue);
                }
                csTraceUpdate->Exit();
            } else {
                //LOGI("Oops");
                for(int i=0; i<NGRAPHS; i++)
                {
                    Graph &graph = pGraph[i];
                    graph.trace.insert(graph.curValue);
                }
            }
        }
        for(int i=0; i<NSMS; i++)
        {
            float v;
            SMComputeWave &smComputeWave = pSMComputeWave[i];
            smComputeWave.t -= dt * 1.0;
            float t = smComputeWave.t;
            if(t < -1.0)
            {
                t = 1.0;
                smComputeWave.t = t;
            }
            float t2 = t+1.0;
            pSMvertices[i*4+0].uvCoordGradient.x = t2;
            pSMvertices[i*4+1].uvCoordGradient.x = t;
            pSMvertices[i*4+2].uvCoordGradient.x = t2;
            pSMvertices[i*4+3].uvCoordGradient.x = t;
            //smComputeWave.heat = heatTest;
            // Heat based on a normalized scale, using the scale of the graphs
            smComputeWave.heat = (smComputeWave.value - smComputeWave.bias) / smComputeWave.scale;
            smComputeWave.noiseShift += dt*(noiseSpeed + smComputeWave.heat*heatNoiseFact);
            if(smComputeWave.noiseShift > 1.0)
                smComputeWave.noiseShift -= 1.0;
        }
        //
        // update the cycle of shifting the SM target (fake behavior of the GPU)
        //
        durationInSM += dt;
        if(durationInSM >= maxDurationInSM.x)
        {
            durationInSMTransition += dt / maxDurationInSM.y;
            if(durationInSMTransition >= 1.0)
            {
                SMShift++;
                if(SMShift == 15)
                    SMShift = 0;
                durationInSMTransition = 0.0;
                durationInSM = 0.0;
            }
        }
    }
//} //nves2
