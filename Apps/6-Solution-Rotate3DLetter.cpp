// Rotate3DLetter.cpp: the letter rotates in response to user input and be rendered in 3D

#include <glad.h>
#include <GLFW/glfw3.h>
#include "GLXtras.h"
#include <time.h>

// Application Data

GLuint vBuffer = 0;     // GPU vertex buffer ID
GLuint program = 0;     // GLSL program ID

// vertices
float  points[][2] =
{ {-.2f, -.2f}, {-.2f, .8f}, {-.6f, .8f}, {-.6f, -.6f}, {.5f, -.6f}, {.5f, -.2f} };
float  colors[][3] =
{ {1, 1, 1}, {0.85, 0.7, 0}, {0.4, 0.8, 0}, {0, 1, 1} , {1, 0, 1}, {.7,.4 ,.5} };

// triangles
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
        gl_Position = view*vec4(r, 0, 1);
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

void Display() {
    mat4 view = RotateY(rotNew.x) * RotateX(rotNew.y);
    SetUniform(program, "view", view);
    float dt = (float)(clock()-startTime)/CLOCKS_PER_SEC; // duration since start
    glClearColor(.5, .5, .5, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(program);
    SetUniform(program, "radAng", setAngle+(3.1415f/180.f)*dt*degPerSec);
    SetUniform(program, "scale", noScale? 1 : 1.5f+cos(dt));
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // set vertex feed for points and colors, then draw
    VertexAttribPointer(program, "point", 2, 0, (void *) 0);
    VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));
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
        Display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vBuffer);
    glfwDestroyWindow(window);
    glfwTerminate();
}
