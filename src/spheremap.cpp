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
#include "glutinc.h"
#include "glextinc.h"

#include "trackball.h"
#include "quaternion.h"
#include "camera.h"

#include "FreeImage.h"

GLuint g_texture_id[6];

float g_light_ambient[]  = { 0.5f, 0.5f, 0.5f, 1.0f };
float g_light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
float g_light_position[] = { 0.0f, 0.0f, 4.0f, 1.0f };
float g_light_direction[] = { 0.0f, 0.0f, -1.0f, 1.0f };

bool g_left_button_down = false;
bool g_middle_button_down = false;
bool g_right_button_down = false;
int g_last_x = 0;
int g_last_y = 0;

int g_tex_wrap = GL_CLAMP;

unsigned int g_filter = 2;				// Which Filter To Use

unsigned int const MODEL_NUM = 6;
unsigned int g_model = 0;

bool g_wireframe = false;

bool g_cubemap = true;

bool g_obj_cubemap = false;

GLint g_viewport[4];
TrackBallf g_trackball;
Matrixf g_rotm;
Quaternionf g_rotv;
Cameraf g_cam;


void update_wrap()
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, g_tex_wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, g_tex_wrap);
}

FIBITMAP* load_image(char const *filename, int flag = 0)
{
    FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(filename, 0);

    if (fif == FIF_UNKNOWN) fif = FreeImage_GetFIFFromFilename(filename);
    if (fif == FIF_UNKNOWN) return false;

    return FreeImage_Load(fif, filename, flag);
}

bool load_texture_from_file(char const *filename, GLuint texIds[3])
{
    FIBITMAP *tdib = load_image(filename);
    if (!tdib) return false;

    bool status(false);
    unsigned int bpp = FreeImage_GetBPP(tdib);

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
        glBindTexture(GL_TEXTURE_2D, texIds[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, bits);

        // Create Linear Filtered Texture
        glBindTexture(GL_TEXTURE_2D, texIds[1]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, bits);

        // Create MipMapped Texture
        glBindTexture(GL_TEXTURE_2D, texIds[2]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, width, height, format, GL_UNSIGNED_BYTE, bits);
    }

    if (dib != tdib) FreeImage_Unload(dib);

    FreeImage_Unload(tdib);

    return status;
}

bool load_textures()
{
    glGenTextures(6, g_texture_id);

    GLuint texIds[3];
    texIds[0] = g_texture_id[0];
    texIds[1] = g_texture_id[2];
    texIds[2] = g_texture_id[4];
    if (!load_texture_from_file("data/spheremap/bg2.jpg", texIds)) return false;

    texIds[0] = g_texture_id[1];
    texIds[1] = g_texture_id[3];
    texIds[2] = g_texture_id[5];
    if (!load_texture_from_file("data/spheremap/reflect2.jpg", texIds)) return false;

	return true;
}

bool init()
{
	if (!load_textures()) return false;

	GLint genmode = GL_SPHERE_MAP;
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, genmode); // Set The Texture Generation Mode For S To Sphere Mapping (NEW)
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, genmode); // Set The Texture Generation Mode For T To Sphere Mapping (NEW)

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
	glLightfv(GL_LIGHT0, GL_POSITION, g_light_position);	// Position The Light
	glEnable(GL_LIGHT0);								// Enable Light One

	//g_cam.LookAt(0.0f, 0.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	return true;
}

void motion_function(int x, int y)
{
	if (g_left_button_down)
	{
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

	default:
		break;
	}
}

void draw_background()
{
	glBindTexture(GL_TEXTURE_2D, g_texture_id[g_filter<<1]);	// This Will Select The BG Maps...
	float pos = -5.0f;
	float sw = 8.0f * tan(22.5f);
	//glTranslatef(0.0f, 0.0f, -130.0f);
	glBegin(GL_QUADS);
	glNormal3f( 0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-sw, -sw,  pos);
	glTexCoord2f(1.0f, 0.0f); glVertex3f( sw, -sw,  pos);
	glTexCoord2f(1.0f, 1.0f); glVertex3f( sw,  sw,  pos);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-sw,  sw,  pos);
	glEnd();
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	g_cam.LookAt(0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	glEnable(GL_TEXTURE_GEN_S);							// Enable Texture Coord Generation For S (NEW)
	glEnable(GL_TEXTURE_GEN_T);							// Enable Texture Coord Generation For T (NEW)

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glScalef(1.0f,-1.0f,1.0f);
    glTranslatef(0, -1.0f, 0);
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();

		if (!g_left_button_down) 
		{
			g_trackball.Update(g_trackball.GetUpdateQuat());
			g_trackball.BuildRotMatrix(g_rotm);
		}

		glMultMatrixf(g_rotm);
		glBindTexture(GL_TEXTURE_2D, g_texture_id[(g_filter<<1)+1]);
		draw_something();

	glPopMatrix();

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);

	glPushMatrix();
		draw_background();
    glPopMatrix();

	glutSwapBuffers();
}

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glGetIntegerv(GL_VIEWPORT, g_viewport);

	//g_cam.Frustum(-10.0f, 10.0f, -10.0f, 10.0f, 10.0f, 150.0f);
	g_cam.Perspective(45.0f, float(w) / float(h), 2.0f, 500.0f);
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

	default:
		break;
	}
}


int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(512,512);
	glutCreateWindow("Sphere Map");

	if (!init()) return 1;

	glutMouseFunc(mouse_function);
	glutMotionFunc(motion_function);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutMainLoop();

	return 0;
}
