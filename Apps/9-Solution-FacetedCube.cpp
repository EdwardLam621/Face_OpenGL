// FacetedGouraud.cpp (c) Jules Bloomenthal 2020, all rights reserved

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <vector>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "VecMat.h"
#include "Widgets.h"

// vertex buffer, shader program ids
GLuint vBuffer = 0, progFaceted = 0;

// display parameters
int screenWidth = 900, screenHeight = 900;
Camera camera(screenWidth, screenHeight, vec3(0, 0, 0), vec3(0, 0, -20), 30);
    // respond to mouse events, half screen size, set back on z-axis, with fov of 30

// interaction
vec3 light(1.1f, 1.7f, -1.2f);
Mover lightMover;
void *picked = NULL;

// Shaders

const char *vertexShader = R"(
    #version 130
    in vec3 point;
    in vec3 color;
    in vec3 normal;
    out vec3 vColor;
    uniform vec3 light;
    uniform mat4 modelview;
    uniform mat4 persp;
    uniform float a = .2;                       // ambient
    void main() {
        gl_Position = persp*modelview*vec4(point, 1);
        vec3 vlight = normalize(light-point);
        float d = max(0, dot(normal, vlight)); // diffuse
        float s = pow(d, 50);                  // specular
        float intensity = clamp(a+d+s, 0, 1);
        vColor = intensity*color;
    }
)";

const char *pixelShader = R"(
    #version 130
    in vec3 vColor;
    out vec4 pColor;
    void main() {
        pColor = vec4(vColor, 1);
    }
)";

// Mouse

bool Shift(GLFWwindow *w) {
    return glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
           glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
}

int WindowHeight(GLFWwindow *w) {
    int width, height;
    glfwGetWindowSize(w, &width, &height);
    return height;
}

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
    double x, y;
    glfwGetCursorPos(w, &x, &y);
	y = WindowHeight(w)-y;
    picked = NULL;
    if (action == GLFW_PRESS) {
        if (MouseOver(x, y, light, camera.fullview, 20)) {
			picked = &lightMover;
            lightMover.Down(&light, (int) x, (int) y, camera.modelview, camera.persp);
        }
        if (picked == NULL) {
            picked = &camera;
            camera.MouseDown((int) x, (int) y);
        }
    }
    if (action == GLFW_RELEASE)
        camera.MouseUp();
}

void MouseMove(GLFWwindow *w, double x, double y) {
	y = WindowHeight(w)-y;
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (picked == &lightMover)
            lightMover.Drag((int) x, (int) y, camera.modelview, camera.persp);
        else
            camera.MouseDrag(x, y, Shift(w));
    }
}

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
    camera.MouseWheel(spin > 0, Shift(w));
}

// Vertex Buffer

struct Vertex {
    vec3 point, color, normal;
    Vertex() { }
    Vertex(vec3 p, vec3 c, vec3 n) : point(p), color(c), normal(n) { }
};

// 8 points and 8 colors:
float l = -1, r = 1, b = -1, t = 1, n = -1, f = 1;
float points[][3] = {{l, b, n}, {l, b, f}, {l, t, n}, {l, t, f}, {r, b, n}, {r, b, f}, {r, t, n}, {r, t, f}};
float colors[][3] = {{0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 1, 1}, {1, 0, 0}, {1, 0, 1}, {1, 1, 0}, {1, 1, 1}};

// 6 faces (quads and normals)
int quads[][4] = {{1, 3, 2, 0}, {6, 7, 5, 4}, {4, 5, 1, 0}, {3, 7, 6, 2}, {2, 6, 4, 0}, {5, 7, 3, 1}};
vec3 normals[] = {vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, -1, 0), vec3(0, 1, 0), vec3(0, 0, -1), vec3(0, 0, 1)};
const int nvertices = sizeof(quads)/sizeof(int), nquads = nvertices/4;

void InitVertexBuffer() {
    // create interleaved vertices (see ex. 4.5), non-indexed (ie, for use with glDrawArrays)
    Vertex vertices[nvertices];
    for (int i = 0; i < nquads; i++) {
        vec3 n = normals[i];        // use same normal for all 4 face vertices
        int *f = quads[i];
        for (int k = 0; k < 4; k++) {
            int vid = f[k];
            vertices[4*i+k] = Vertex(vec3(points[vid]), vec3(colors[vid]), n);
        }
    }
    // create and bind GPU vertex buffer, copy vertex data
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    glBufferData(GL_ARRAY_BUFFER, nvertices*sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
}

// Display

void Display(GLFWwindow *w) {
    // clear screen, enable z-buffer
    glClearColor(.5, .5, .5, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(progFaceted);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    VertexAttribPointer(progFaceted, "point", 3, sizeof(Vertex), (void *) 0);
    VertexAttribPointer(progFaceted, "color", 3, sizeof(Vertex), (void *) sizeof(vec3));
    VertexAttribPointer(progFaceted, "normal", 3, sizeof(Vertex), (void *) (2*sizeof(vec3)));
    SetUniform(progFaceted, "modelview", camera.modelview);
    SetUniform(progFaceted, "persp", camera.persp);
    SetUniform(progFaceted, "light", light);
    glDrawArrays(GL_QUADS, 0, nvertices);
    // draw light
    UseDrawShader(camera.fullview);
    glDisable(GL_DEPTH_TEST);
    bool visible = IsVisible(light, camera.fullview);
    bool incube = fabs(light.x) < 1 && fabs(light.y) < 1 && fabs(light.z) < 1;
    Disk(light, 12, incube? vec3(0,0,1) : vec3(1,0,0), visible? 1 : .25f);
    glFlush();
}

// Application

const char *usage = "Usage\n\
    mouse-drag:\t\trotate x,y\n\
    with shift:\t\ttranslate x,y\n\
    mouse-wheel:\trotate/translate z\n";

void Resize(GLFWwindow *w, int width, int height) {
    camera.Resize(width, height);
    glViewport(0, 0, screenWidth = width, screenHeight = height);
}

int main(int ac, char **av) {
    // init app window and GL context
    glfwInit();
    GLFWwindow *w = glfwCreateWindow(screenWidth, screenHeight, "Shaded, Faceted Cube", NULL, NULL);
    glfwSetWindowPos(w, 100, 100);
    glfwMakeContextCurrent(w);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    // init shader and GPU data
    progFaceted = LinkProgramViaCode(&vertexShader, &pixelShader);
    InitVertexBuffer();
    printf(usage);
    // callbacks
    glfwSetCursorPosCallback(w, MouseMove);
    glfwSetMouseButtonCallback(w, MouseButton);
    glfwSetScrollCallback(w, MouseWheel);
    glfwSetWindowSizeCallback(w, Resize);
    // event loop
    glfwSwapInterval(1);
    while (!glfwWindowShouldClose(w)) {
        glfwPollEvents();
        Display(w);
        glfwSwapBuffers(w);
    }
    glDeleteBuffers(1, &vBuffer);
    glfwDestroyWindow(w);
    glfwTerminate();
}
