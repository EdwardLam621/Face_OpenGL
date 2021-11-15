// 2-Example-ClearScreen-v2.cpp - use OpenGL shader architecture

#include <glad.h>                           // GL header file
#include <glfw3.h>                          // GL toolkit
#include <stdio.h>                          // printf, etc.
#include "GLXtras.h"                        // convenience routines

GLuint vBuffer = 0;                         // GPU buf ID, valid > 0
GLuint program = 0;                         // shader ID, valid if > 0

// vertex shader: operations before the rasterizer
const char *vertexShader = R"(
    #version 130
    in vec2 point;                          // 2D point from GPU memory
    void main() {
        // REQUIREMENT 1A) transform vertex:
        gl_Position = vec4(point, 0, 1);    // 'built-in' variable
    }
)";

// pixel shader: operations after the rasterizer
const char *pixelShader = R"(
    #version 130
    out vec4 pColor;

    //subroutine method to determine if a space if odd or even
    //for instance, c1 = 2 + 1 = 3, so it is a black space
    bool odd(float row, float col){
        highp int x = int(row);
        highp int y = int(col);
        if((((x / 50 % 2 + 1) % 2) + ((y / 50 % 2 + 1) % 2)) %2 != 0) return true; 
        else return false;
    }

    void main() {
        // REQUIREMENT 1B) shade pixel:
        if(odd(gl_FragCoord.x, gl_FragCoord.y)){
            pColor = vec4(0, 0, 0, 1);          // r, g, b, alpha
        }else{
            pColor = vec4(1, 1, 1, 1);
        }
    }
)";

void InitVertexBuffer() {
    // REQUIREMENT 3A) create GPU buffer, copy 4 vertices
#ifdef GL_QUADS
    float pts[][2] = {{-1,-1},{-1,1},{1,1},{1,-1}};               // �object?
#else
    float pts[][2] = {{-1,-1},{-1,1},{1,1},{-1,-1},{1,1},{1,-1}}; // �object?
#endif
    glGenBuffers(1, &vBuffer);                      // ID for GPU buffer
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);         // make it active
    glBufferData(GL_ARRAY_BUFFER, sizeof(pts), pts, GL_STATIC_DRAW);
}

void Display() {
    glUseProgram(program);                          // ensure correct program
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);         // activate vertex buffer
    // REQUIREMENT 3B) set vertex feeder
    GLint id = glGetAttribLocation(program, "point");
    glEnableVertexAttribArray(id);
    glVertexAttribPointer(id, 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
//  in subsequent code we will replace the above three lines with
//  VertexAttribPointer(program, "point", 2, 0, (void *) 0);
#ifdef GL_QUADS
    glDrawArrays(GL_QUADS, 0, 4);                   // display entire window
#else
	glDrawArrays(GL_TRIANGLES, 0, 6);
#endif
    glFlush();                                      // flush GL ops
}

void Keyboard(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)     // test for program exit
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void GlfwError(int id, const char *reason) {
    printf("GFLW error %i: %s\n", id, reason);
    getchar();
}

void APIENTRY GlslError(GLenum source, GLenum type, GLuint id, GLenum severity,
                        GLsizei len, const GLchar *msg, const void *data) {
    printf("GLSL Error: %s\n", msg);
    getchar();
}

int AppError(const char *msg) {
    glfwTerminate();
    printf("Error: %s\n", msg);
    getchar();
    return 1;
}

int main() {                                                // application entry
    glfwSetErrorCallback(GlfwError);                        // init GL toolkit
    if (!glfwInit())
        return 1;
    // create named window of given size
    GLFWwindow *w = glfwCreateWindow(400, 400, "Chess Board by Edward Lam", NULL, NULL);
    if (!w)
        return AppError("can't open window");
    glfwMakeContextCurrent(w);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);    // set OpenGL extensions
    // following line will not compile unless glad.h >= OpenGLv4.3
    glDebugMessageCallback(GlslError, NULL);
    // REQUIREMENT 2) build shader program
    if (!(program = LinkProgramViaCode(&vertexShader, &pixelShader)))
        return AppError("can't link shader program");
    InitVertexBuffer();                                     // set GPU vertex memory
    glfwSetKeyCallback(w, Keyboard);
    while (!glfwWindowShouldClose(w)) {                     // event loop
        Display();
        if (PrintGLErrors())                                // test for runtime GL error
            getchar();                                      // if so, pause
        glfwSwapBuffers(w);                                 // double-buffer is default
        glfwPollEvents();
    }
    glfwDestroyWindow(w);
    glfwTerminate();
}
