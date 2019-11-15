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

#ifndef __glutinc_h
#define __glutinc_h

#if defined(_WIN32)
#include <GL/freeglut.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#endif