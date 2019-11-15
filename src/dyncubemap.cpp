/*
-----------------------------------------------------------------------
This source file is part of "cgcourse-examples"
(Examples for Computer Graphics Course of CCCE GUCAS.)
Copyright (C) 2011 Xue Jian (jian.xue.cn@gmail.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

#include <stdlib.h>
#include <iostream>
#include <vector>

#include "glinc.h"
#include "trackball.h"
#include "quaternion.h"
#include "camera.h"
#include "cpucounter.h"

#include "FreeImage.h"

#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;

PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;

PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmap;

struct TriangleMesh3D
{
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;
};

char const *g_cm_tex_files[] = {
    "data/dyncubemap/cm_right.bmp",
    "data/dyncubemap/cm_left.bmp",
    "data/dyncubemap/cm_top.bmp",
    "data/dyncubemap/cm_bottom.bmp",
    "data/dyncubemap/cm_back.bmp",
    "data/dyncubemap/cm_front.bmp",
};

GLenum g_face_target[6] = {
  GL_TEXTURE_CUBE_MAP_POSITIVE_X,
  GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
  GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
  GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
  GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
  GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

GLuint g_fbo;					// Our handle to the FBO
GLuint g_depth_buffer;			// Our handle to the depth render buffer
GLsizei g_cm_width = 128;
GLsizei g_cm_height = 128;

GLuint g_cm_texture_id[3];
GLuint g_texture_id[6];
GLuint g_cm_bg_texture_id[18];

float g_light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
float g_light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
float g_light_position[] = { 0.0f, 0.0f, 10.0f, 1.0f };
float g_light_direction[] = { 0.0f, 0.0f, -1.0f, 0.0f };

float g_light_rot = 0.0f;
float g_light_rot_speed = 20.0f; // degrees per second

float g_light_rot2 = 0.0f;
float g_light_rot_speed2 = 60.0f;  // degrees per second

float g_material_ambient[]  = { 1.0f, 0.57f, 0.04f, 1.0f };
float g_material_diffuse[]  = { 1.0f, 0.57f, 0.04f, 1.0f };
float g_material_specular[]  = { 0.33f, 0.33f, 0.52f, 1.0f };
float g_material_emission[]  = { 0.0f, 0.0f, 0.0f, 0.0f };
float g_material_specular_power = 80.0f;

float g_xrot = 0.0f;
float g_yrot = 0.0f;
bool g_left_button_down = false;
bool g_middle_button_down = false;
bool g_right_button_down = false;
int g_last_x = 0;
int g_last_y = 0;

int g_texgen_mode = GL_REFLECTION_MAP;
int g_tex_wrap = GL_CLAMP_TO_EDGE;

unsigned int g_filter = 2;				// Which Filter To Use

unsigned int const MODEL_NUM = 7;
unsigned int g_model = 4;

bool g_wireframe = false;

bool g_cubemap = true;

bool g_obj_cubemap = false;

GLint g_viewport[4];
TrackBallf g_trackball;
Matrixf g_rotm;
Quaternionf g_rotv;
Cameraf g_cam;
CPUCounter g_timer;

Vectorf g_objpos(0.0f, 0.0f, 4.0f);
Vectorf g_campos(0.0f, 0.0f, 5.0f);

aiScene const *g_ai_scene = nullptr;
TriangleMesh3D g_triangle_mesh;

void set_material()
{
    GLenum face = GL_FRONT_AND_BACK;
    //glMaterialfv(face, GL_AMBIENT, g_material_ambient);
    glMaterialfv(face, GL_AMBIENT, g_material_diffuse);
    glMaterialfv(face, GL_DIFFUSE, g_material_diffuse);
    glMaterialfv(face, GL_SPECULAR, g_material_specular);
    glMaterialfv(face, GL_EMISSION, g_material_emission);
    glMaterialf(face, GL_SHININESS, g_material_specular_power);
}

bool is_extension_supported(const char *extension)
{
    const GLubyte *extensions = NULL;
    const GLubyte *start;
    GLubyte *where, *terminator;

    /* Extension names should not have spaces. */
    where = (GLubyte *) strchr(extension, ' ');
    if (where || *extension == '\0') return false;

    extensions = glGetString(GL_EXTENSIONS);

    /* It takes a bit of care to be fool-proof about parsing the
    OpenGL extensions string.  Don't be fooled by sub-strings,
    etc. */
    start = extensions;
    for (;;) 
    {
        where = (GLubyte *) strstr((const char *) start, extension);
        if (!where)
            break;
        terminator = where + strlen(extension);
        if (where == start || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0')
                return true;
        start = terminator;
    }
    return false;
}

bool init_extfuncs()
{
    if (!is_extension_supported("GL_EXT_framebuffer_object")) return false;

    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSEXTPROC) wglGetProcAddress("glGenFramebuffersEXT");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFEREXTPROC) wglGetProcAddress("glBindFramebufferEXT");
    glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC) wglGetProcAddress("glDeleteFramebuffersEXT");
    glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC) wglGetProcAddress("glFramebufferRenderbufferEXT");
    glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC) wglGetProcAddress("glFramebufferTexture2DEXT");
    glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) wglGetProcAddress("glCheckFramebufferStatusEXT");

    glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC) wglGetProcAddress("glGenRenderbuffersEXT");
    glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC) wglGetProcAddress("glBindRenderbufferEXT");
    glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC) wglGetProcAddress("glDeleteRenderbuffersEXT");
    glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC) wglGetProcAddress("glRenderbufferStorageEXT");

    glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC) wglGetProcAddress("glGenerateMipmapEXT");


    if (   glGenFramebuffers==0
        || glBindFramebuffer==0
        || glDeleteFramebuffers==0
        || glFramebufferRenderbuffer==0
        || glFramebufferTexture2D==0
        || glCheckFramebufferStatus==0
        || glGenRenderbuffers==0
        || glBindRenderbuffer==0
        || glDeleteRenderbuffers==0
        || glRenderbufferStorage==0
        || glGenerateMipmap==0)
    {
        std::cout << "glGenFramebuffers:        " << glGenFramebuffers << std::endl
                  << "glBindFramebuffer:        " << glBindFramebuffer << std::endl
                  << "glDeleteFramebuffers      " << glDeleteFramebuffers << std::endl
                  << "glFramebufferRenderbuffer " << glFramebufferRenderbuffer << std::endl
                  << "glFramebufferTexture2D:   " << glFramebufferTexture2D << std::endl
                  << "glCheckFramebufferStatus: " << glCheckFramebufferStatus << std::endl
                  << "glGenRenderbuffers:       " << glGenRenderbuffers << std::endl
                  << "glBindRenderbuffer:       " << glBindRenderbuffer << std::endl
                  << "glDeleteRenderbuffers:    " << glDeleteRenderbuffers << std::endl
                  << "glRenderbufferStorage:    " << glRenderbufferStorage << std::endl
                  << "glGenerateMipmap:         " << glGenerateMipmap << std::endl;
        return false;
    }

    return true;
}

bool init_fbo()
{
    // Setup our FBO
    glGenFramebuffers(1, &g_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);

    // Create the render buffer for depth	
    glGenRenderbuffers(1, &g_depth_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, g_depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, g_cm_height, g_cm_height);

    // Attach the depth render buffer to the FBO as it's depth attachment
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_depth_buffer);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);  // Unbind the FBO for now

    return true;
}

void update_texgen()
{
	if (g_cubemap)
	{
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, g_texgen_mode);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, g_texgen_mode);
		glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, g_texgen_mode);
	}
	else
	{
		GLint genmode = GL_SPHERE_MAP;
		//GLenum pname = GL_OBJECT_PLANE;
		//GLfloat planes[4] = {0.05f, 0.0f, 0.0f, 0.0f};
		//GLfloat planet[4] = {0.0f, 0.05f, 0.0f, 0.0f};
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, genmode); // Set The Texture Generation Mode For S To Sphere Mapping (NEW)
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, genmode); // Set The Texture Generation Mode For T To Sphere Mapping (NEW)
		//glTexGenfv(GL_S, pname, planes);
		//glTexGenfv(GL_T, pname, planet);
	}
}

void update_wrap()
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, g_cm_texture_id[g_filter]);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, g_tex_wrap);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, g_tex_wrap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, g_tex_wrap);
    for (int i=0; i<18; i++)
    {
        glBindTexture(GL_TEXTURE_2D, g_cm_bg_texture_id[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, g_tex_wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, g_tex_wrap);
    }
}

FIBITMAP* load_image(char const *filename, int flag = 0)
{
    FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(filename, 0);

    if (fif == FIF_UNKNOWN) fif = FreeImage_GetFIFFromFilename(filename);
    if (fif == FIF_UNKNOWN) return false;

	FIBITMAP *dib = FreeImage_Load(fif, filename, flag);
	if (!dib) return nullptr;

	//if (fif == FIF_BMP) FreeImage_FlipVertical(dib);

    return dib;
}

bool load_texture_from_file(GLsizei &texWidth, GLsizei &texHeight, 
                            char const *filename, GLuint texIds[3],
                            GLenum texTarget = GL_TEXTURE_2D, GLenum faceTarget = GL_TEXTURE_2D,
                            bool flipV = false)
{
    FIBITMAP *tdib = load_image(filename);
    if (!tdib) return false;

    bool status(false);
    unsigned int bpp = FreeImage_GetBPP(tdib);
    if (flipV) {
        if (!FreeImage_FlipVertical(tdib)) {
            FreeImage_Unload(tdib);
            return false;
        }
    }

    FIBITMAP *dib = tdib;
    if (bpp != 24) dib = FreeImage_ConvertTo24Bits(tdib);


    BYTE *bits = FreeImage_GetBits(dib);
    unsigned int width = FreeImage_GetWidth(dib);
    unsigned int height = FreeImage_GetHeight(dib);

    GLenum format = FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR ? GL_BGR : GL_RGB;

    //RGBQUAD *pal = FreeImage_GetPalette(dib);

    if (bits != 0 && width > 0 && height > 0) {
        status = true;									// Set The Status To TRUE

        // Create Nearest Filtered Texture
        glBindTexture(texTarget, texIds[0]);
        glTexParameteri(texTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(texTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(faceTarget, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, bits);

        // Create Linear Filtered Texture
        glBindTexture(texTarget, texIds[1]);
        glTexParameteri(texTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(texTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(faceTarget, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, bits);

        // Create MipMapped Texture
        glBindTexture(texTarget, texIds[2]);
        glTexParameteri(texTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(texTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        gluBuild2DMipmaps(faceTarget, GL_RGB, width, height, format, GL_UNSIGNED_BYTE, bits);
    }

    if (dib != tdib) FreeImage_Unload(dib);

    FreeImage_Unload(tdib);

    texWidth = width;
    texHeight = height;

    return status;
}

bool load_cube_map_bg_textures()
{
    glGenTextures(18, g_cm_bg_texture_id);

    for (int loop = 0; loop < 6; ++loop)
    {
        GLuint texIds[3];
        texIds[0] = g_cm_bg_texture_id[loop];
        texIds[1] = g_cm_bg_texture_id[loop+6];
        texIds[2] = g_cm_bg_texture_id[loop+12];
        GLsizei w, h;
        if (!load_texture_from_file(w, h, g_cm_tex_files[loop], texIds, GL_TEXTURE_2D, GL_TEXTURE_2D, true)) return false;
    }

    return true;
}

bool load_cube_map_textures()
{
    glGenTextures(3, g_cm_texture_id);

    for (int loop = 0; loop < 6; ++loop)
    {
        GLsizei w, h;
        if (!load_texture_from_file(w, h, g_cm_tex_files[loop], g_cm_texture_id, GL_TEXTURE_CUBE_MAP, g_face_target[loop], true)) return false;
    }
    //g_cm_width = w;
    //g_cm_height = h;
    return true;
}

bool load_textures()
{
    glGenTextures(6, g_texture_id);

    GLuint texIds[3];
    GLsizei w, h;
    texIds[0] = g_texture_id[0];
    texIds[1] = g_texture_id[2];
    texIds[2] = g_texture_id[4];
    if (!load_texture_from_file(w, h, "data/dyncubemap/BG.bmp", texIds, GL_TEXTURE_2D, GL_TEXTURE_2D, true)) return false;

    texIds[0] = g_texture_id[1];
    texIds[1] = g_texture_id[3];
    texIds[2] = g_texture_id[5];
    if (!load_texture_from_file(w, h, "data/dyncubemap/Reflect.bmp", texIds, GL_TEXTURE_2D, GL_TEXTURE_2D, true)) return false;

    return true;
}

// load the first mesh from scene
aiMesh const * get_first_mesh(aiNode *node, aiScene const *scene)
{
    if (node) {
        if (node->mMeshes) {
            return scene->mMeshes[node->mMeshes[0]];
        } else {
            for (unsigned int i = 0; i < node->mNumChildren; ++i) {
                aiMesh const *mesh = get_first_mesh(node->mChildren[i], scene);
                if (mesh) return mesh;
            }
        }
    }

    return nullptr;
}

bool load_model(char const *filename)
{
    g_ai_scene = aiImportFile(filename, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals);
    if (!g_ai_scene) {
        std::cout << "Failed to load model from file " << filename << " (" << aiGetErrorString() << ")." << std::endl;
        return false;
    }

    aiMesh const *mesh = get_first_mesh(g_ai_scene->mRootNode, g_ai_scene);
    if (!mesh) {
        std::cout << "No mesh found in the scene of " << filename << "." << std::endl;
        return false;
    }

    assert(mesh->mNormals != 0);
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        const aiVector3D &pos = mesh->mVertices[i];
        g_triangle_mesh.vertices.push_back(pos.x);
        g_triangle_mesh.vertices.push_back(pos.y);
        g_triangle_mesh.vertices.push_back(pos.z);

        const aiVector3D &nrm = mesh->mNormals[i];
        g_triangle_mesh.vertices.push_back(nrm.x);
        g_triangle_mesh.vertices.push_back(nrm.y);
        g_triangle_mesh.vertices.push_back(nrm.z);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        if (mesh->mFaces[i].mNumIndices != 3) continue; // ignore non-triangle face
        g_triangle_mesh.indices.push_back(mesh->mFaces[i].mIndices[0]);
        g_triangle_mesh.indices.push_back(mesh->mFaces[i].mIndices[1]);
        g_triangle_mesh.indices.push_back(mesh->mFaces[i].mIndices[2]);
    }
	
	return true;
}

bool init()
{

    if (!load_model("data/dyncubemap/bunny.ply")) {
        std::cout << "init: failed to load model file!" << std::endl;
        return false;
    }

    if (!load_textures()) {
        std::cout << "init: failed to load textures!" << std::endl;
        return false;
    }

    if (!load_cube_map_bg_textures()) {
        std::cout << "init: failed to load cube map bg textures!" << std::endl;
        return false;
    }

    if (!load_cube_map_textures()) {
        std::cout << "init: failed to load cube map textures!" << std::endl;
        return false;
    }

    if (!init_extfuncs()) {
        std::cout << "init: failed to initialize ext functions!" << std::endl;
        return false;
    }
    if (!init_fbo()) {
        std::cout << "init: failed to initialize frame buffer object!" << std::endl;
        return false;
    }

	update_texgen();
	update_wrap();

	g_rotm.IdentityMatrix();

	glEnable(GL_NORMALIZE);
	//glEnable(GL_AUTO_NORMAL);
	glEnable(GL_TEXTURE_2D);							// Enable Texture Mapping
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations

	glLightfv(GL_LIGHT0, GL_AMBIENT, g_light_ambient);		// Setup The Ambient Light
	glLightfv(GL_LIGHT0, GL_DIFFUSE, g_light_diffuse);		// Setup The Diffuse Light
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 1.0f);
    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 60.0f);
	glEnable(GL_LIGHT0);								// Enable Light One

	//g_cam.LookAt(0.0f, 0.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    g_timer.CounterStart();

	return true;
}

void draw_surface(TriangleMesh3D const &mesh)
{
    GLuint const *facedata = mesh.indices.data();
    GLfloat const *vertdata = mesh.vertices.data();
	glBegin(GL_TRIANGLES);
		for (size_t i=0; i<mesh.indices.size()/3; ++i)
		{
			glNormal3fv(vertdata + 6*facedata[3*i] + 3);
			glVertex3fv(vertdata + 6*facedata[3*i]);

			glNormal3fv(vertdata + 6*facedata[3*i+1] + 3);
			glVertex3fv(vertdata + 6*facedata[3*i+1]);

			glNormal3fv(vertdata + 6*facedata[3*i+2] + 3);
			glVertex3fv(vertdata + 6*facedata[3*i+2]);
		}
	glEnd();
}

void draw_wireframe(TriangleMesh3D const &mesh)
{
    GLuint const *facedata = mesh.indices.data();
    GLfloat const *vertdata = mesh.vertices.data();
    glBegin(GL_LINES);
		for (unsigned int i=0; i<mesh.indices.size()/3; ++i)
		{
			glVertex3fv(&vertdata[6*facedata[3*i]]);
			glVertex3fv(&vertdata[6*facedata[3*i+1]]);
			glVertex3fv(&vertdata[6*facedata[3*i+1]]);
			glVertex3fv(&vertdata[6*facedata[3*i+2]]);
			glVertex3fv(&vertdata[6*facedata[3*i+2]]);
			glVertex3fv(&vertdata[6*facedata[3*i]]);
		}
	glEnd();
}

void draw_something()
{
	switch (g_model)
	{
	case 0:
		if (g_wireframe) glutWireTeapot(1.0);
		else glutSolidTeapot(1.0);
		break;

	case 1:
		if (g_wireframe) glutWireCube(1.6);
		else glutSolidCube(1.6);
		break;

	case 2:
		if (g_wireframe) glutWireTorus(0.4, 1.0, 50, 50);
		else glutSolidTorus(0.4, 1.0, 50, 50);
		break;

	case 3:
		if (g_wireframe) glutWireCone(1.0, 1.5, 60, 20);
		else glutSolidCone(1.0, 1.5, 60, 20);
		break;

	case 4:
		if (g_wireframe) glutWireSphere(1.0, 50, 50);
		else glutSolidSphere(1.0, 50, 50);
		break;

	case 5:
		//glScalef(20.0f, 20.0f, 20.0f);
		if (g_wireframe) glutWireIcosahedron();
		else glutSolidIcosahedron();
		break;

	case 6:
		//glScalef(10.0f, 10.0f, 10.0f);
		if (g_wireframe) draw_wireframe(g_triangle_mesh);
		else draw_surface(g_triangle_mesh);
		break;

	default:
		break;
	}
}

void draw_background()
{
    glEnable(GL_TEXTURE_2D);
    float pos = -5.0f;
    float sw = 16.0f;//180.0f * tan(22.5f);
    if (g_cubemap) 
    {
        // right side
        glBindTexture(GL_TEXTURE_2D, g_cm_bg_texture_id[g_filter*6]);
        glBegin(GL_QUADS);
        glNormal3f( -1.0f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( sw,  sw,  sw);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( sw,  sw, -sw);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( sw, -sw, -sw);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( sw, -sw,  sw);
        glEnd();

        // left side
        glBindTexture(GL_TEXTURE_2D, g_cm_bg_texture_id[g_filter*6+1]);
        glBegin(GL_QUADS);
        glNormal3f( 1.0f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-sw,  sw, -sw);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-sw,  sw,  sw);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-sw, -sw,  sw);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-sw, -sw, -sw);
        glEnd();

        // top side
        glBindTexture(GL_TEXTURE_2D, g_cm_bg_texture_id[g_filter*6+2]);
        glBegin(GL_QUADS);
        glNormal3f( 0.0f, -1.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-sw,  sw, -sw);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( sw,  sw, -sw);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( sw,  sw,  sw);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-sw,  sw,  sw);
        glEnd();

        // bottom side
        glBindTexture(GL_TEXTURE_2D, g_cm_bg_texture_id[g_filter*6+3]);
        glBegin(GL_QUADS);
        glNormal3f( 0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-sw, -sw,  sw);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( sw, -sw,  sw);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( sw, -sw, -sw);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-sw, -sw, -sw);
        glEnd();

        // back side
        glBindTexture(GL_TEXTURE_2D, g_cm_bg_texture_id[g_filter*6+4]);
        glBegin(GL_QUADS);
        glNormal3f( 0.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-sw, -sw,  sw);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-sw,  sw,  sw);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( sw,  sw,  sw);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( sw, -sw,  sw);
        glEnd();

        // front side
        glBindTexture(GL_TEXTURE_2D, g_cm_bg_texture_id[g_filter*6+5]);
        glBegin(GL_QUADS);
        glNormal3f( 0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-sw,  sw, -sw);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-sw, -sw, -sw);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( sw, -sw, -sw);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( sw,  sw, -sw);
        glEnd();
    }
    else 
    {
        glBindTexture(GL_TEXTURE_2D, g_texture_id[g_filter<<1]);	// This Will Select The BG Maps...
        //glTranslatef(0.0f, 0.0f, -130.0f);
        glBegin(GL_QUADS);
            glNormal3f( 0.0f, 0.0f, 1.0f);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-sw, -sw,  pos);
            glTexCoord2f(1.0f, 0.0f); glVertex3f( sw, -sw,  pos);
            glTexCoord2f(1.0f, 1.0f); glVertex3f( sw,  sw,  pos);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-sw,  sw,  pos);
        glEnd();
    }
}

void motion_function(int x, int y)
{
	if (g_left_button_down)
	{
		g_yrot += x - g_last_x;
		g_xrot += y - g_last_y;
		g_last_x = x;
		g_last_y = y;
		g_trackball.Update((float)(2.0*(x-g_viewport[0])/g_viewport[2]-1), (float)(2.0*(g_viewport[3]-y-1-g_viewport[1])/g_viewport[3]-1));
		g_trackball.BuildRotMatrix(g_rotm);

		glutPostRedisplay();
	}
}

void mouse_function(int button, int state, int x, int y)
{
	bool down = (state == GLUT_DOWN);

	switch (button) 
	{
	case GLUT_LEFT_BUTTON: g_left_button_down = down; break;
	case GLUT_MIDDLE_BUTTON: g_middle_button_down = down; break;
	case GLUT_RIGHT_BUTTON: g_right_button_down = down; break;
	}

	if (g_left_button_down)
	{
		g_trackball.Start((float)(2.0*(x-g_viewport[0])/g_viewport[2]-1), (float)(2.0*(g_viewport[3]-y-1-g_viewport[1])/g_viewport[3]-1));
	}

	g_last_x = x;
	g_last_y = y;	
}

void draw_objects()
{
    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    g_material_diffuse[0] = 1.0f;
    g_material_diffuse[1] = 0.57f;
    g_material_diffuse[2] = 0.04f;
    set_material();
    glPushMatrix();
        glRotatef(30.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(15.0f, 0.0f, 0.0f, 1.0f);

        glPushMatrix();
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            glutSolidTorus(0.01, 2.0, 10, 300);
        glPopMatrix();

        glRotatef(g_light_rot, 0.0f, 1.0f, 0.0f);
        glTranslatef(2.0f, 0.0f, 0.0f);
        glutSolidSphere(0.3, 50, 50);
    glPopMatrix();

    g_material_diffuse[0] = 0.9f;
    g_material_diffuse[1] = 0.2f;
    g_material_diffuse[2] = 0.1f;
    set_material();
    glPushMatrix();
        glRotatef(-60.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(35.0f, 0.0f, 0.0f, 1.0f);

        glPushMatrix();
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            glutSolidTorus(0.01, 2.0, 10, 300);
        glPopMatrix();

        glRotatef(g_light_rot2, 0.0f, 1.0f, 0.0f);
        glTranslatef(2.0f, 0.0f, 0.0f);
        glutSolidSphere(0.1, 50, 50);
    glPopMatrix();
}

void create_dynamic_cube_map()
{
    // First we bind the FBO so we can render to it
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    // Save the view port and set it to the size of the texture
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, g_cm_width, g_cm_height);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    //MUST USE 90 degree FOV!
    g_cam.Perspective(90.0f, 1.0f, 0.1f, 5000.0f);

    glMatrixMode(GL_MODELVIEW);

    float facepos[6][6] = {
        {  5.0f,  0.0f,  0.0f,  0.0f, -1.0f,  0.0f },
        { -5.0f,  0.0f,  0.0f,  0.0f, -1.0f,  0.0f },
        {  0.0f,  5.0f,  0.0f,  0.0f,  0.0f,  1.0f },
        {  0.0f, -5.0f,  0.0f,  0.0f,  0.0f, -1.0f },
        {  0.0f,  0.0f,  5.0f,  0.0f, -1.0f,  0.0f },
        {  0.0f,  0.0f, -5.0f,  0.0f, -1.0f,  0.0f }
    };

    glBindTexture(GL_TEXTURE_CUBE_MAP, g_cm_texture_id[g_filter]);

    for (int face=0; face<6; ++face) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, g_face_target[face], g_cm_texture_id[g_filter], 0);

        Vectorf up(facepos[face][3], facepos[face][4], facepos[face][5]);

        g_campos.ele[0] = facepos[face][0];
        g_campos.ele[1] = facepos[face][1];
        g_campos.ele[2] = facepos[face][2];
        g_campos.ele[3] = 1.0f;

        g_campos *= g_rotm;
        up *= g_rotm;

        g_campos += g_objpos;
        g_campos.ele[3] = 1.0f;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glPushMatrix();

        g_cam.LookAt(g_objpos.ele[0], g_objpos.ele[1], g_objpos.ele[2], g_campos.ele[0], g_campos.ele[1], g_campos.ele[2], up.ele[0], up.ele[1], up.ele[2]);
        glLightfv(GL_LIGHT0, GL_POSITION, g_light_position);	// Position The Light
        glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, g_light_direction);

        glPushMatrix();
        glTranslatef(g_objpos.ele[0], g_objpos.ele[1], g_objpos.ele[2]);
        draw_objects();
        glPopMatrix();

        glDisable(GL_LIGHTING);
        glPushMatrix();
        draw_background();
        glPopMatrix();

        glPopMatrix();

        glFlush();
    }

    if (g_filter==2) glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // Restore old view port and set rendering back to default frame buffer
    glPopAttrib();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!g_left_button_down) 
    {
        g_trackball.Update(g_trackball.GetUpdateQuat());
        g_trackball.BuildRotMatrix(g_rotm);
    }

    if (g_cubemap && !g_obj_cubemap) create_dynamic_cube_map();

    g_campos.ele[0] = 0.0f;
    g_campos.ele[1] = 0.0f;
    g_campos.ele[2] = 5.0f;
    g_campos.ele[3] = 1.0f;

    g_campos *= g_rotm;

    Vectorf up(0.0f, 1.0f, 0.0f);
    up *= g_rotm;

    g_campos += g_objpos;
    g_campos.ele[3] = 1.0f;
    g_cam.LookAt(g_campos.ele[0], g_campos.ele[1], g_campos.ele[2], g_objpos.ele[0], g_objpos.ele[1], g_objpos.ele[2], up.ele[0], up.ele[1], up.ele[2]);
    glLightfv(GL_LIGHT0, GL_POSITION, g_light_position);	// Position The Light
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, g_light_direction);

	glPushMatrix();

	if (g_obj_cubemap)
	{
		Matrixf mat = g_cam.GetInverseOfViewMatrix();
		mat.ele[12] = mat.ele[13] = mat.ele[14] = 0.0f;
		mat.ele[3] = mat.ele[7] = mat.ele[11] = 0.0f;
		mat.ele[15] = 1.0f;
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glLoadMatrixf(mat);
		glMatrixMode(GL_MODELVIEW);
	}

    glTranslatef(g_objpos.ele[0], g_objpos.ele[1], g_objpos.ele[2]);
    draw_objects();

    glDisable(GL_LIGHTING);

    g_material_diffuse[0] = 0.9f;
    g_material_diffuse[1] = 0.9f;
    g_material_diffuse[2] = 0.9f;
    set_material();
    glEnable(GL_TEXTURE_GEN_S);							// Enable Texture Coord Generation For S (NEW)
	glEnable(GL_TEXTURE_GEN_T);							// Enable Texture Coord Generation For T (NEW)
	if (g_cubemap) glEnable(GL_TEXTURE_GEN_R);
    if (g_cubemap) glEnable(GL_TEXTURE_CUBE_MAP);
    else {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_texture_id[(g_filter<<1)+1]); // This Will Select The Sphere Map
    }

    draw_something();

	glPopMatrix();

    if (g_obj_cubemap) {
        glMatrixMode(GL_TEXTURE);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	if (g_cubemap) glDisable(GL_TEXTURE_GEN_R);
	glDisable(GL_TEXTURE_CUBE_MAP);

    glDisable(GL_LIGHTING);
	glPushMatrix();
	draw_background();
	glPopMatrix();

	glutSwapBuffers();

    double dt = g_timer.GetCounts() / g_timer.GetFrequence();
    g_timer.CounterStart();
    g_light_rot += g_light_rot_speed * dt;
    if (g_light_rot > 360.0f) g_light_rot = 0.0f;

    g_light_rot2 += g_light_rot_speed2 * dt;
    if (g_light_rot2 > 360.0f) g_light_rot2 = 0.0f;
}

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glGetIntegerv(GL_VIEWPORT, g_viewport);

	//g_cam.Frustum(-10.0f, 10.0f, -10.0f, 10.0f, 10.0f, 150.0f);
	g_cam.Perspective(60.0f, float(w) / float(h), 2.0f, 5000.0f);
	//g_cam.Ortho(-50.0f, 50.0f, -50.0f, 50.0f, 10.0f, 150.0f);
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:
		exit(0);
		break;

	case 'f':
		g_filter = (g_filter+1) % 3;
		break;

	case 'c':
        if (g_tex_wrap == GL_REPEAT) g_tex_wrap = GL_CLAMP;
        else if (g_tex_wrap == GL_CLAMP) g_tex_wrap = GL_CLAMP_TO_EDGE;
        else g_tex_wrap = GL_REPEAT;
		update_wrap();
		break;

	case ',':
		g_model = (g_model+MODEL_NUM-1) % MODEL_NUM;
		break;

	case '.':
		g_model = (g_model+1) % MODEL_NUM;
		break;

	case 'w':
		g_wireframe = !g_wireframe;
		break;

	case 'b':
		g_cubemap = !g_cubemap;
		update_texgen();
		break;

	case 'm':
		if (g_cubemap)
		{
			if (g_texgen_mode == GL_REFLECTION_MAP) g_texgen_mode = GL_NORMAL_MAP;
			else g_texgen_mode = GL_REFLECTION_MAP;
			update_texgen();
		}
		break;

	case 's':
		g_obj_cubemap = !g_obj_cubemap;
		break;

	default:
		break;
	}
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(512,512);
	glutCreateWindow("Dynamic Cube Map");
	if (!init()) return 1;
	glutMouseFunc(mouse_function);
	glutMotionFunc(motion_function);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutMainLoop();

	if (g_ai_scene) aiReleaseImport(g_ai_scene);

	return 0;
}
