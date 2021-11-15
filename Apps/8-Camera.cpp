// Rotate3DLetter.cpp: the letter rotates in response to user input and be rendered in 3D

#include <glad.h>
#include <GLFW/glfw3.h>
#include "GLXtras.h"
#include <time.h>
#include <Lib\Camera.cpp>

// Application Data

int winWidth = 500, winHeight = 500;
Camera camera(winWidth / 2, winHeight, vec3(0, 0, 0), vec3(0, 0, -1), 30);

GLuint vBuffer = 0;     // GPU vertex buffer ID
GLuint program = 0;     // GLSL program ID

// vertices
float l = -1, r = 1, b = -1, t = 1, n = -1, f = 1; //left, right,bottom,top, near, far
float points[][3] = { {l,b,n}, {l,b,f},{l,t,n},{l,t,f},{r,b,n},{r,b,f},{r,t,n},{r,t,f} };   //8 points
float colors[][3] = { {0,0,1},{0,1,0},{0,1,1},{1,0,0},{1,0,1},{1,1,0},{0,0,0},{1,1,1} };    //8 colors
int faces[][4] = { {1,3,2,0}, {6,7,5,4},{4,5,1,0},{3,7,6,2},{2,6,4,0},{5,7,3,1} };          //6 faces

float fieldOfView = 30, cubeSize = .05f, cubeStretch = cubeSize;

int triangles[][3] = { {0,1,2},{0,2,3},{0,3,4}, {0,4,5} };

// Vertex and Pixel Shaders

const char *vertexShader = R"(
    #version 130
    in vec2 point;
    in vec3 color;
    out vec4 vColor;
    uniform float radAng = 0;
    uniform float scale = 1;
    uniform mat4 view;
    vec2 Rotate2D(vec2 v) {
        float c = cos(radAng), s = sin(radAng);
        return scale*vec2(c*v.x-s*v.y, s*v.x+c*v.y);
    }
    void main() {
        vec2 r = Rotate2D(point);
        gl_Position = view*vec4(poiny, 1);
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

void Display(GLFWwindow* w) {
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    mat4 m = camera.fullview * Scale(cubeSize, cubeSize, cubeStretch);
    SetUniform(program, "view", m);

    

    float dt = (float)(clock()-startTime)/CLOCKS_PER_SEC; // duration since start
    glClearColor(.5, .5, .5, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(program);
    SetUniform(program, "radAng", setAngle+(3.1415f/180.f)*dt*degPerSec);
    SetUniform(program, "scale", noScale? 1 : 1.5f+cos(dt));
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // set vertex feed for points and colors, then draw
    VertexAttribPointer(program, "point", 3, 0, (void *) 0);
    VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));

    int screenWidth, screenHeight;
    glfwGetWindowSize(w, &screenWidth, &screenHeight);

    glDrawElements(GL_TRIANGLES, sizeof(triangles)/sizeof(int), GL_UNSIGNED_INT, triangles);
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


// respond to mouse clicks
void MouseButton(GLFWwindow* w, int butn, int action, int mods) {
    // called when mouse button pressed or released
    if (action == GLFW_PRESS) {
        // save reference for MouseDrage
        double x, y;
            glfwGetCursorPos(w, &x, &y);
            mouseDown = vec2((float)x, (float) y);
    }
    if (action == GLFW_RELEASE)
        // save reference rotation
        rotOld = rotNew;
}

// respond to mouse movement
void MouseMove(GLFWwindow* w, double x, double y) {
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        // compute mouse drage difference, update rotation
        vec2 dif((float)x - mouseDown.x, (float)y - mouseDown.y);
        rotNew = rotOld + rotSpeed * dif;
    }
}

const char *usage = R"(
	Usage
        A: increase rotation speed
        D: decrease rotation speed
        S: reset rotation speed
        R: reverse rotation
        N: toggle scaling
        Mouse: Hold Left Click and Drag to control X and Y axis
)";

int main() {
    // init window
    if (!glfwInit())
        return 1;
    GLFWwindow *window = glfwCreateWindow(600, 600, "Rotate 3D Letter by Edward Lam", NULL, NULL);
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
