// 8-Camera&Scene.cpp: Add camera class 

#include <glad.h>
#include <GLFW/glfw3.h>
#include "GLXtras.h"
#include <time.h>
#include <Camera.h>

//declare camera
int winWidth = 500, winHeight = 500;
Camera camera(winWidth / 2, winHeight, vec3(0, 0, 0), vec3(0, 0, -1), 30);

// Application Data

GLuint vBuffer = 0;     // GPU vertex buffer ID
GLuint program = 0;     // GLSL program ID

float l = -1, r = 1, b = -1, t = 1, n = -1, f = 1; //left, right,bottom,top, near, far
float points[][3] = { {l,b,n}, {l,b,f}, {l,t,n}, {l,t,f}, {r,b,n}, {r,b,f}, {r,t,n}, {r,t,f} };     //8 points
float colors[][3] = { {0,0,1}, {0,1,0}, {0,1,1}, {1,0,0}, {1,0,1}, {1,1,0}, {0,0,0}, {1,1,1} };     //8 colors
int faces[][4] = { {1,3,2,0}, {6,7,5,4},{4,5,1,0},{3,7,6,2},{2,6,4,0},{5,7,3,1} };                  //6 faces


float fieldOfView = 30, cubeSize = .05f, cubeStretch = cubeSize;


// Vertex and Pixel Shaders

const char *vertexShader = R"(
    #version 130
    in vec3 point;
    in vec3 color;
    out vec4 vColor;
    uniform float radAng = 0;
    uniform float scale = 1;
    uniform mat4 view;
    void main() {
        gl_Position = view*vec4(point, 1);
        vColor = vec4(color, 1);
    }
)";

const char *pixelShader = R"(
    #version 130
    in vec4 vColor;
    out vec4 pColor;
    void main() {
        pColor = vColor;
    }
)";

vec2 mouseDown(0, 0);           //location of last mouse down
vec2 rotOld(0, 0), rotNew(0, 0);   //.x is rotation about Y-axis, in degrees; .y about X-aix
float rotSpeed = .3f;

vec3 tranOld, tranNew(0, 0, -1);
float tranSpeed = 0.01f;

// Initialization

void InitVertexBuffer() {
    // create GPU buffer to hold positions and colors, and make it the active buffer
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // allocate memory for vertex positions and colors
    glBufferData(GL_ARRAY_BUFFER, sizeof(points)+sizeof(colors), NULL, GL_STATIC_DRAW);
    // load data to sub-buffers
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);
}

// Animation

time_t startTime = clock();
float degPerSec = 30, setAngle = 0;
bool noScale = false;

void Display(GLFWwindow *w) {
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    int screenWidth, screenHeight;
    glfwGetWindowSize(w, &screenWidth, &screenHeight);

    int halfWidth = screenWidth / 2;
    float aspectRatio = (float)halfWidth / (float)screenHeight;

    float nearDistance = .001f, farDistance = 500;
    mat4 persp = Perspective(fieldOfView, aspectRatio, nearDistance, farDistance);

    mat4 scale = Scale(cubeSize, cubeSize, cubeStretch);
    mat4 rot = RotateY(rotNew.x) * RotateX(rotNew.y);
    mat4 tran = Translate(tranNew);
    mat4 modelview = tran * rot * scale;
    mat4 view = persp * modelview;
    SetUniform(program, "view", view);

    mat4 m = camera.fullview * Scale(cubeSize, cubeSize, cubeStretch);
    SetUniform(program, "view", m);

    float dt = (float)(clock()-startTime)/CLOCKS_PER_SEC; // duration since start
    glClearColor(.5, .5, .5, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    VertexAttribPointer(program, "point", 3, 0, (void *) 0);
    VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));
    glViewport(0, 0, halfWidth, screenHeight); //left half of app; solid cube
    glDrawElements(GL_QUADS, sizeof(faces)/sizeof(int), GL_UNSIGNED_INT, faces);
    glViewport(halfWidth, 0, halfWidth, screenHeight);
    glLineWidth(5);
    for (int i = 0; i < 6; i++)
        glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_INT, &faces[i]);

    glFlush();
}

// Application

void Keyboard(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        // prevent angle jump
        time_t now = clock();
        float dt = (float)(now-startTime)/CLOCKS_PER_SEC; // duration since start
        setAngle = setAngle+(3.1415f/180.f)*dt*degPerSec;
        startTime = now;
        // adjust speed/direction
        switch (key) {
            case 'A': degPerSec *= 1.3f; break;
            case 'S': degPerSec = degPerSec < 0 ? -30.f : 30.f; break;
            case 'D': degPerSec *= .7f; break;
            case 'R': degPerSec *= -1; break;
			case 'N': noScale = !noScale; break;
        }
    }
    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}


void MoseWheel(GLFWwindow* w, double xoffset, double yoffset) {
    //registered with glfwSetScrollCallback
    //yoffset is amount of wheel rotation

}

// respond to mouse clicks
void MouseButton(GLFWwindow* w, int butn, int action, int mods) {
    // called when mouse button pressed or released
    if (action == GLFW_PRESS) {
        // save reference for MouseDrage
        double x, y;
        glfwGetCursorPos(w, &x, &y);
        camera.MouseDown(x, y);
        //mouseDown = vec2((float)x, (float) y);
    }
    if (action == GLFW_RELEASE)
        // save reference rotation
        camera.MouseUp();
}

// respond to mouse movement
void MouseMove(GLFWwindow* w, double x, double y) {
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        // compute mouse drage difference, update rotation
        vec2 mouse((float)x, (float)y), dif = mouse - mouseDown;
        bool shift = glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        camera.MouseDrag(x, y, shift);
    }
}

const char *usage = R"(
	Usage
        Mouse: Hold Left Click and Drag to control X and Y axis
)";

int main() {

    // init window
    if (!glfwInit())
        return 1;
    GLFWwindow *window = glfwCreateWindow(600, 600, "Camera Class added by Edward Lam", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    // build and use shader program
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    PrintGLErrors();
    if (!(program = LinkProgramViaCode(&vertexShader, &pixelShader)))
        return 0;

    // allocate vertex memory in the GPU and link it to the vertex shader
    InitVertexBuffer();
	printf(usage);

    // callbacks and event loop
    glfwSetKeyCallback(window, Keyboard);
    glfwSetMouseButtonCallback(window, MouseButton);
    glfwSetCursorPosCallback(window, MouseMove);
    glfwSwapInterval(1);
    while (!glfwWindowShouldClose(window)) {
        Display(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vBuffer);
    glfwDestroyWindow(window);
    glfwTerminate();
}
