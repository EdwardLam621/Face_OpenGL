// Assignment 8 - Bezier patch with interactive control points 
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
const char *textureFilename = "C:/Users/Jules/Code/Exe/Lily.tga";

// patch
vec3 ctrlPts[4][4];			    // 16 Bezier control points, indexed [s][t]
vec3 coeffs[4][4];              // 16 polybomial coefficients in x,y,z
GLuint vBufferId = -1;
int res = 1;

int nQuadrilaterals = 0;

vec3 BezierTangent(float t, vec3 b1, vec3 b2, vec3 b3, vec3 b4) {
    return b1 * (6 * t - 3 * pow(t, 2) - 3) + b2 * (9 * pow(t, 2) - 12 * t + 3) +
        b3 * (6 * t - 9 * pow(t, 2)) + 3 * pow(t, 2) * b4;

}

vec3 BezierPoint(float t, vec3 b1, vec3 b2, vec3 b3, vec3 b4) {
    // return the position of the curve given parameter t
    return (0 - pow(t, 3) + 3 * pow(t, 2) - 3 * t + 1) * b1 + (3 * pow(t, 3) - 6 * pow(t, 2) + 3 * t) * b2 +
        (-3 * pow(t, 3) + 3 * pow(t, 2)) * b3 + pow(t, 3) * b4;
}

void BezierPatch(float s, float t, vec3* point, vec3* normal) {
    vec3 spts[4], tpts[4];
    for (int i = 0; i < 4; i++) {
        spts[i] = BezierPoint(s, ctrlPts[i][0], ctrlPts[i][1],
            ctrlPts[i][2], ctrlPts[i][3]);
        tpts[i] = BezierPoint(t, ctrlPts[0][i], ctrlPts[1][i],
            ctrlPts[2][i], ctrlPts[3][i]);
    }
    *point = BezierPoint(t, spts[0], spts[1], spts[2], spts[3]);
    vec3 tTan = BezierTangent(t, spts[0], spts[1], spts[2], spts[3]);
    vec3 sTan = BezierTangent(s, tpts[0], tpts[1], tpts[2], tpts[3]);
    *normal = normalize(cross(sTan, tTan));
};

void DefaultControlPoints() {
    float vals[] = { -.75, -.25, .25, .75 };
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            ctrlPts[i][j] = vec3(vals[i], vals[j], i % 3 == 0 || j % 3 == 0 ? .5 : 0);
}

void SetCoeffs() {
    // set Bezier coefficient matrix
    mat4 m(vec4(-1, 3, -3, 1), vec4(3, -6, 3, 0), vec4(-3, 3, 0, 0), vec4(1, 0, 0, 0));
    mat4 g;
    for (int k = 0; k < 3; k++) {
        for (int i = 0; i < 16; i++)
            g[i / 4][i % 4] = ctrlPts[i / 4][i % 4][k];
        mat4 c = m * g * m;
        for (int i = 0; i < 16; i++)
            coeffs[i / 4][i % 4][k] = c[i / 4][i % 4];
    }
}

vec3 PointFromCoeffs(float s, float t) {
    vec3 p;
    float s2 = s * s, s3 = s * s2, t2 = t * t, ta[] = { t * t2, t2, t, 1 };
    for (int i = 0; i < 4; i++)
        p += ta[i] * (s3 * coeffs[i][0] + s2 * coeffs[i][1] + s * coeffs[i][2] + coeffs[i][3]);
    return p;
}

void SetVertices(int res, bool init = false) {
    // activate GPU vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, vBufferId);
    // get pointers to GPU memory for vertices, normals
    nQuadrilaterals = res * res;
    int sizeBuffer = 2 * 4 * nQuadrilaterals * sizeof(vec3);
    if (init)
        glBufferData(GL_ARRAY_BUFFER, sizeBuffer, NULL, GL_STATIC_DRAW);
    vec3* vPtr = (vec3*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    for (int i = 0; i < res; i++) {
        float s0 = (float)i / res, s1 = (float)(i + 1) / res;
        for (int j = 0; i < res; i++) {
            float t0 = (float)j / res, t1 = (float)(j + 1) / res;
            BezierPatch(s0, t0, vPtr, vPtr + 1); vPtr += 2;
            BezierPatch(s1, t0, vPtr, vPtr + 1); vPtr += 2;
            BezierPatch(s1, t1, vPtr, vPtr + 1); vPtr += 2;
            BezierPatch(s0, t1, vPtr, vPtr + 1); vPtr += 2;
        }
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
}

vec3 PointFromCtrlPts(float s, float t) {
    vec3 spt[4];
    for (int i = 0; i < 4; i++)
        spt[i] = BezierPoint(s, ctrlPts[i][0], ctrlPts[i][1], ctrlPts[i][2], ctrlPts[i][3]);
    return BezierPoint(t, spt[0], spt[1], spt[2], spt[3]);
}


// interaction
vec3        light(1.5f, 1.5f, 1);
void       *picked = NULL;
Mover       mover;

// vertex shader (no op)
const char* vShaderCode = R"(
    #version 130 core									
    in vec3 point, normal;														
    out vec3 vPoint, vNormal;													
    uniform mat4 modelview;												
    uniform mat4 persp;													
    void main() {														
	    vNormal = (modelview*vec4(normal, 0)).xyz;						
	    vPoint = (modelview*vec4(point, 1)).xyz;						
	    gl_Position = persp*vec4(vPoint, 1);							
    }
)";

//// tessellation evaluation
//const char *teShaderCode = R"(
//    #version 400 core
//    layout (quads, equal_spacing, ccw) in;
//    uniform mat4 modelview, persp;
//    out vec3 point, normal;
//    out vec2 uv;
//    void main() {
//        uv = gl_TessCoord.st;
//		vec3 p, n;
//		point = (modelview*vec4(p, 1)).xyz;
//		normal = (modelview*vec4(n, 0)).xyz;
//        gl_Position = persp*vec4(point, 1);
//    }
//)";

// pixel shader
const char *pShaderCode = R"(
    #version 130 core
    in vec3 point, normal;
    out vec4 pColor;
    uniform vec3 light;
    void main() {
        vec3 N = normalize(normal);             // surface normal
        vec3 L = normalize(light-point);        // light vector
        vec3 E = normalize(point);              // eye vertex
        vec3 R = reflect(L, N);                 // highlight vector
        float dif = abs(dot(N,L));              // two-sided diffuse
        float spec = pow(h, 100);
        float ad = clamp(.15+dif, 0, 1);
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
    // update matrices
    SetUniform(program, "modelview", camera.modelview);
    SetUniform(program, "persp", camera.persp);
	// set texture
	//SetUniform(program, "textureMap", textureUnit);
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
    //shade quadrilaterals
    glDrawArrays(GL_QUADS, 0, 4 * nQuadrilaterals);
    UseDrawShader(camera.fullview);
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

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
    double x, y;
    glfwGetCursorPos(w, &x, &y);
    y = WindowHeight(w)-y; // invert y for upward-increasing screen space
    picked = NULL;
    if (action == GLFW_RELEASE)
        camera.MouseUp();
    if (action == GLFW_PRESS) {
		if (MouseOver(x, y, light, camera.fullview)) {
			mover.Down(&light, (int) x, (int) y, camera.modelview, camera.persp);
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

int main(int ac, char **av) {
    // init app window
    if (!glfwInit())
        return 1;
    GLFWwindow *w = glfwCreateWindow(winWidth, winHeight, "Bezier Patch W/ Interactive Control Points", NULL, NULL);
    glfwSetWindowPos(w, 100, 100);
    glfwMakeContextCurrent(w);
    // init OpenGL, shader program, texture
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    program = LinkProgramViaCode(&vShaderCode, &pShaderCode);
    textureName = LoadTexture(textureFilename, textureUnit);
    // make shader program
    // init patch
    DefaultControlPoints();
    // make vertex buffer
    glGenBuffers(1, &vBufferId);
    SetVertices(res, true);
    SetCoeffs();
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
