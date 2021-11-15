// Original: FacetedGouraud.cpp (c) Jules Bloomenthal 2020, all rights reserved
// Modified: SmoothShadingFace.cpp by Edward Lam

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <vector>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "VecMat.h"
#include "Widgets.h"
#include <float.h>
using namespace std;


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

const char* vertexShader = R"(
    #version 130
    in vec3 point;
    in vec3 normal;
    in vec3 color;
    out vec3 vPoint;
    out vec3  vNormal;
    uniform mat4 persp;
    uniform mat4 modelview;
    void main() {
        vPoint = (modelview*vec4(point,1)).xyz;
        vNormal = (modelview*vec4(normal,0)).xyz;
        gl_Position = persp*vec4(vPoint, 1);
    }
)";

const char* pixelShader = R"(
    #version 130
    uniform float a = .2;
    in vec3 vPoint;
    in vec3 vNormal;
    out vec4 pColor;

    uniform vec3 lightPos = vec3(1,0,0);

    uniform vec3 color = vec3(1,1,1);

    vec3 N = normalize(vNormal);

    vec3 L = normalize(lightPos-vPoint);

    float d = abs(dot(N,L));

    vec3 R = reflect(L,N);

    vec3 E = normalize(vPoint);

    void main() {
        float h = max(0, dot(R,E));
        float s = pow(h, 100);
        float intensity = clamp(a+d+s, 0, 1);
        pColor = vec4(intensity*color, 1);
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

int triangles[][3] = { 
    //triangles -- index starts from 0
    {0,1,11},{1,11,12},{1,2,12},{2,12,13},{12,13,14},{13,14,16},{13,15,16},{2,3,13},{3,13,15},
    {3,4,15},{4,15,17},{15,17,18},{15,18,19},{15,16,19},{14,16,24},{16,19,24},{18,19,23},{19,23,24},
    {18,21,23},{20,21,22},{4,17,20},{4,20,22},{4,22,32},{4,32,5},{21,22,23},{23,24,28},{22,23,28},
    {22,27,28},{32,26,22},{22,26,27},{5,32,25},{5,25,6},{6,25,26},{6,26,27},{6,27,7},{7,27,8},
    {8,27,29},{27,29,30},{27,28,30},{30,28,31},{30,10,31},{9,30,10}
};

vec3 points[] = {
    vec3(0,.85f,.19f),vec3(0,.7f,.25f),vec3(0,.45f,.3f),vec3(0,.35f,.3f),                           //1-4
    vec3(0,-.2f,.45f),vec3(0,-.1f,.52f),vec3(0,-.2f,.35f),vec3(0,-.3f,.4f),                         //5-8
    vec3(0,-.4f,.4f), vec3(0, -.5f, .35f), vec3(0,-.85f, .22f), vec3(.4f,.8f,.05f),                 //9-12
    vec3(.55f,.65f,-.1f),vec3(.51f,.5f,-.1f),vec3(.68f,.49f,-.2f),vec3(.35f,-.2f,.2f),              //13-16
    vec3(.59f,.37f,-.1f),vec3(.24f,.26f,.28f),vec3(.49f,.21f,-.01f),vec3(.55f,.21f,-.01f),          //17-20
    vec3(.18f,.2f,.3f),vec3(.36f,.15f,.2f),vec3(.3f,.5f,.2f),vec3(.48f,.01f,.08f),                  //21-24
    vec3(.68f,-.01f,-.33f),vec3(.1f,-.1f,.25f),vec3(.18f,-.13f,-.25f),vec3(.2f,-.2f,.29f),          //25-28
    vec3(.55f,-.35f,-.49f),vec3(.1f,-.41f,.29f),vec3(.28f,-.47f,.23f),vec3(.35f,-.78f,-.2f),        //29-32
    vec3(.12f,-.01f,.33f),                                                                          //33 Missed in middle

};

// declare array of normals, one for each point
const int npoints = sizeof(points) / sizeof(int);
vec3 normals[npoints];
int sizePts;

void InitVertexBuffer() {
    // create GPU buffer to hold positions and colors, and make it the active buffer
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // allocate memory for vertex positions and colors
    int sizePts = sizeof(points), sizeNms = sizeof(normals);
    glBufferData(GL_ARRAY_BUFFER, sizePts + sizeNms, NULL, GL_STATIC_DRAW);
    // load data to sub-buffers
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizePts, &points[0]);
    glBufferSubData(GL_ARRAY_BUFFER, sizePts, sizeNms, &normals[0]);
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

    VertexAttribPointer(progFaceted, "point", 3, 0, (void*)0);
    VertexAttribPointer(progFaceted, "normal", 3, 0, (void*)sizePts);


    SetUniform(progFaceted, "modelview", camera.modelview);
    SetUniform(progFaceted, "persp", camera.persp);
    SetUniform(progFaceted, "lightPos", light);
    glDrawElements(GL_TRIANGLES, sizeof(triangles) / sizeof(int), GL_UNSIGNED_INT, triangles);
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

void Normalize() {
    int npoints = sizeof(points) / sizeof(vec3);
    // scale and offset so that points all within +/-1 in x, y , and z
    vec3 mn(FLT_MAX), mx(-FLT_MAX);
    for (int i = 0; i < npoints; i++) {
        vec3 p = points[i];
        for (int k = 0; k < 3; k++) {
            if (p[k] < mn[k]) mn[k] = p[k];
            if (p[k] < mx[k]) mx[k] = p[k];
        }
    }
    vec3 center = .5f * (mn + mx), range = mx - mn;
    float maxrange = max(range.x, max(range.y, range.z));
    float s = 2 / maxrange;
    for (int i = 0; i < npoints; i++)
        points[i] = s * (points[i] - center);
}


int main(int ac, char **av) {
    // zero array
    for (int i = 0; i < npoints; i++) {
        normals[i] = vec3(0, 0, 0);
    }
    //compute normal for each triangle, accululate for each vertex
    int ntriangles = sizeof(triangles) / (3 * sizeof(int));
    for (int i = 0; i < ntriangles; i++) {
        int* t = triangles[i];
        vec3 p1(points[t[0]]), p2(points[t[1]]), p3(points[t[2]]);
        vec3 n = normalize(cross(p3 - p2, p2 - p1));
        for (int k = 0; k < 3; k++)
            normals[t[k]] += n;
    }
    for (int i = 0; i < npoints; i++)
        normals[i] = normalize(normals[i]);

    // init app window and GL context
    glfwInit();
    GLFWwindow *w = glfwCreateWindow(screenWidth, screenHeight, "Smooth Shading Face", NULL, NULL);
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
