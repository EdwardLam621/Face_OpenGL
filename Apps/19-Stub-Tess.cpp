// Stub-Tess.cpp - various shapes using tessellation shader
// Topic - a Bezier patch with interactive control points
// Editor: Edward Lam

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "Misc.h"
#include "Widgets.h"
#include "VecMat.h"

// display parameters
int         winWidth = 800, winHeight = 600;
Camera      camera(winWidth, winHeight, vec3(0, 0, 0), vec3(0, 0, -7));

// shading
GLuint      program = 0;
int			textureName = 0, textureUnit = 0;
const char *textureFilename = "C:/Users/edward/Desktop/CPSC5700/linedface/image2.tga";

// Bezier patch
vec3 ctrlPts[4][4];

// interaction
vec3        light(1.5f, 1.5f, 1);
void*       picked = NULL;
Mover       mover;

// point controller
bool			viewMesh = true;
int				xCursorOffset = -7, yCursorOffset = -3;

// vertex shader (no op)
const char *vShaderCode = "void main() { gl_Position = vec4(0); } // no-op";

// tessellation evaluation
const char* teShaderCode = R"(
	#version 400 core
	layout (quads, equal_spacing, ccw) in;
	uniform vec3 ctrlPts[16];
    uniform mat4 modelview;
	uniform mat4 persp;
	out vec3 teNormal;
	out vec3 tePoint;
	out vec2 teUv;
	vec3 BezTangent(float t, vec3 b1, vec3 b2, vec3 b3, vec3 b4) {
		float t2 = t*t;
		return (-3*t2+6*t-3)*b1+(9*t2-12*t+3)*b2+(6*t-9*t2)*b3+3*t2*b4;
	}
	vec3 BezPoint(float t, vec3 b1, vec3 b2, vec3 b3, vec3 b4) {
		float t2 = t*t, t3 = t*t2;
		return (-t3+3*t2-3*t+1)*b1+(3*t3-6*t2+3*t)*b2+(3*t2-3*t3)*b3+t3*b4;
	}
	void main() {
		vec3 spts[4], tpts[4];
        float s = gl_TessCoord.st.s, t = gl_TessCoord.st.t;
		teUv = vec2(s,t);
		for (int i = 0; i < 4; i++) {
			spts[i] = BezPoint(s, ctrlPts[4*i], ctrlPts[4*i+1], ctrlPts[4*i+2], ctrlPts[4*i+3]);
			tpts[i] = BezPoint(t, ctrlPts[i], ctrlPts[i+4], ctrlPts[i+8], ctrlPts[i+12]);
		}
		vec3 p = BezPoint(t, spts[0], spts[1], spts[2], spts[3]);
		vec3 tTan = BezTangent(t, spts[0], spts[1], spts[2], spts[3]);
		vec3 sTan = BezTangent(s, tpts[0], tpts[1], tpts[2], tpts[3]);
		vec3 n = normalize(cross(sTan, tTan));
		teNormal = (modelview*vec4(n, 0)).xyz;
		tePoint = (modelview*vec4(p, 1)).xyz;
		gl_Position = persp*vec4(tePoint, 1);
	}
)";

// pixel shader
const char* pShaderCode = R"(
    #version 400
    in vec3 tePoint, teNormal;
    in vec2 teUv;
    uniform sampler2D textureMap;
    uniform vec3 light;
    void main() {
        vec3 N = normalize(teNormal);             // surface normal
        vec3 L = normalize(light-tePoint);        // light vector
        vec3 E = normalize(tePoint);                // eye vertex
        vec3 R = reflect(L, N);                    // highlight vector
        float dif = max(0, dot(N, L));            // one-sided diffuse
        float spec = pow(max(0, dot(E, R)), 50);
        float ad = clamp(.15+dif, 0, 1);
        vec3 texColor = texture(textureMap, teUv).rgb;
        gl_FragColor = vec4(ad*texColor+vec3(spec), 1);
    }
)";

// display

void Display() {
    // background, blending, zbuffer
    glClearColor(.6f, .6f, .6f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);
    // ctrl points
    GLint id = glGetUniformLocation(program, "ctrlPts");
    glUniform3fv(id, 16, (float*)&ctrlPts[0][0]);
    // update matrices
    SetUniform(program, "modelview", camera.modelview);
    SetUniform(program, "persp", camera.persp);
	// set texture
	SetUniform(program, "textureMap", textureUnit);
    glActiveTexture(GL_TEXTURE0+textureUnit);       // active texture corresponds with textureUnit
	glBindTexture(GL_TEXTURE_2D, textureName);      // bind active texture to textureName
	// transform light and send to pixel shader
    vec4 hLight = camera.modelview*vec4(light, 1);
    glUniform3fv(glGetUniformLocation(program, "light"), 1, (float *) &hLight);
    // tessellate patch
    glPatchParameteri(GL_PATCH_VERTICES, 4);
    float res = 25, outerLevels[] = {res, res, res, res}, innerLevels[] = {res, res};
    glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, outerLevels);
    glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, innerLevels);
    glDrawArrays(GL_PATCHES, 0, 4);
    // light
    glDisable(GL_DEPTH_TEST);
    UseDrawShader(camera.fullview);


    if (viewMesh) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 3; j++) {
                Line(ctrlPts[i][j], ctrlPts[i][j + 1], 1.25f, vec3(1, 1, 0));
                Line(ctrlPts[j][i], ctrlPts[j + 1][i], 1.25f, vec3(1, 1, 0));
            }
        for (int i = 0; i < 16; i++)
            Disk(ctrlPts[i / 4][i % 4], 7, vec3(1, 1, 0));
    }

    Disk(light, 12, vec3(1, 0, 0));
    glFlush();
}

// mouse

int WindowHeight(GLFWwindow *w) {
    int width, height;
    glfwGetWindowSize(w, &width, &height);
    return height;
}

bool Shift(GLFWwindow *w) {
    return glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
           glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
}

vec3* PickControlPoint(int x, int y) {
    for (int k = 0; k < 16; k++)
        if (MouseOver(x, y, ctrlPts[k / 4][k % 4], camera.fullview, xCursorOffset, yCursorOffset))
            return &ctrlPts[k / 4][k % 4];
    return NULL;
}

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
    double x, y;
    glfwGetCursorPos(w, &x, &y);
    y = WindowHeight(w)-y; // invert y for upward-increasing screen space
    picked = NULL;
    if (action == GLFW_RELEASE)
        camera.MouseUp();
    if (action == GLFW_PRESS) {
        vec3* pp = viewMesh ? PickControlPoint(x, y) : NULL;
        if (pp) {
            if (butn == GLFW_MOUSE_BUTTON_LEFT) {
                mover.Down(pp, x, y, camera.modelview, camera.persp);
                picked = &mover;
            }
        }
        else if (MouseOver(x, y, light, camera.fullview, xCursorOffset, yCursorOffset)) {
            mover.Down(&light, x, y, camera.modelview, camera.persp);
            picked = &mover;
        }
		else {
			picked = &camera;
			camera.MouseDown(x, y);
		}
	}
}



void MouseMove(GLFWwindow *w, double x, double y) {
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		y = WindowHeight(w)-y;
		if (picked == &mover)
			mover.Drag((int) x, (int) y, camera.modelview, camera.persp);
		if (picked == &camera)
			camera.MouseDrag((int) x, (int) y, Shift(w));
	}
}

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
    camera.MouseWheel(spin > 0, Shift(w));
}



// application

void Resize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

// configure default control points
void DefaultControlPoints() {
    vec3 p0 = vec3(-.8f, -.8f, 0), p1 = vec3(.8f, -.8f, 0), p2 = vec3(-.8f, .8f, 0), p3 = vec3(.8f, .8f, 0);
    float vals[] = { 0, 1 / 3.f, 2 / 3.f, 1. };
    for (int i = 0; i < 16; i++) {
        float ax = vals[i % 4], ay = vals[i / 4];
        vec3 p10 = p0 + ax * (p1 - p0), p32 = p2 + ax * (p3 - p2);
        ctrlPts[i / 4][i % 4] = p10 + ay * (p32 - p10);
    }
}

int main(int ac, char **av) {
    // init app window
    if (!glfwInit())
        return 1;
    GLFWwindow *w = glfwCreateWindow(winWidth, winHeight, "Bezier Patch w Interactive Points by Edwrd Lam", NULL, NULL);
    glfwSetWindowPos(w, 100, 100);
    glfwMakeContextCurrent(w);
    // init OpenGL, shader program, texture
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    program = LinkProgramViaCode(&vShaderCode, NULL, &teShaderCode, NULL, &pShaderCode);
    DefaultControlPoints();
    textureName = LoadTexture(textureFilename, textureUnit);
    // callbacks
    glfwSetCursorPosCallback(w, MouseMove);
    glfwSetMouseButtonCallback(w, MouseButton);
    glfwSetScrollCallback(w, MouseWheel);
    glfwSetWindowSizeCallback(w, Resize);
    // event loop
    glfwSwapInterval(1);
    while (!glfwWindowShouldClose(w)) {
        Display();
        glfwPollEvents();
        glfwSwapBuffers(w);
    }
    glfwDestroyWindow(w);
    glfwTerminate();
}
