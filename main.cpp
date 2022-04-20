#include <iostream> 
#include <math.h>
#include <algorithm>
#include <unistd.h>
#include <utility> 
#include <limits>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>

#include "Shader.h"
#include "Camera.h"

//TODO: Pseudo angles for sorting polygon vertices.
//TODO: Use constant number of slices instead of zValue += 0.005f
//TODO: Automatic texture coordinate generation.
//TODO: Use EBO.
//TODO: Camera process mouse movement, zoom, support arbitrary initial position.

#define DATA_WIDTH 256
#define DATA_HEIGHT 256
#define DATA_DEPTH 256
#define DATA_FILE "./resources/data/brain256.raw"

using namespace std;

struct Vertex
{
public:
    Vertex()
    {
        this->vertexCoord = glm::vec3(0);
        this->texCoord = glm::vec3(0);
    }

    Vertex(glm::vec3& vertexCoord, glm::vec3& texCoord)
    {
        this->vertexCoord = glm::vec3(vertexCoord);
        this->texCoord = glm::vec3(texCoord);
    }

    glm::vec3 vertexCoord;
    glm::vec3 texCoord;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void calculatePlanes();
float pseudoAngle(glm::vec3 p1, glm::vec3 p2);
float positiveAngle(glm::vec3 vec);

// window
const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;

// camera
glm::vec3 camPos =  glm::vec3(0.25f, 0.25f, 2.0f);
glm::vec3 center = glm::vec3(0.25f, 0.25f, 0.25f);
Camera camera(camPos, center);

float lastX = WINDOW_WIDTH / 2.0f;
float lastY = WINDOW_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;

vector<Vertex> vertexBuffer;

glm::vec3 worldSpaceCubeVertices[] = 
{
    glm::vec3(-0.5f, -0.5f, -0.5f), //left bottom back
    glm::vec3(-0.5f,  0.5f, -0.5f),  //left top back
    glm::vec3( 0.5f, -0.5f, -0.5f),  //right bottom back
    glm::vec3( 0.5f,  0.5f, -0.5f),  //right top back

    glm::vec3(-0.5f, -0.5f,  0.5f),  //left bottom front
    glm::vec3(-0.5f,  0.5f,  0.5f),  //left top front
    glm::vec3( 0.5f, -0.5f,  0.5f),  //right bottom front
    glm::vec3( 0.5f,  0.5f,  0.5f)  //right top front
};

glm::vec3 verticesTexCoords[] = 
{
    glm::vec3(-1.0f, -1.0f, -1.0f), //left bottom back
    glm::vec3(-1.0f,  1.0f, -1.0f), //left top back
    glm::vec3( 1.0f, -1.0f, -1.0f), //right bottom back
    glm::vec3( 1.0f,  1.0f, -1.0f), //right top back

    glm::vec3(-1.0f, -1.0f,  1.0f), //left bottom front
    glm::vec3(-1.0f,  1.0f,  1.0f), //left top front
    glm::vec3( 1.0f, -1.0f,  1.0f), //right bottom front
    glm::vec3( 1.0f,  1.0f,  1.0f)  //right top front
};

pair<int, int> edges[] =
{
    pair <int, int> (0,1),
    pair <int, int> (2,3),
    pair <int, int> (0,2),
    pair <int, int> (1,3),

    pair <int, int> (4,5),
    pair <int, int> (6,7),
    pair <int, int> (4,6),
    pair <int, int> (5,7),

    pair<int, int> (0,4),
    pair<int, int> (1,5),
    pair<int, int> (2,6),
    pair<int, int> (3,7)
};

int main(void)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Volume Render", NULL, NULL);
    if (window == NULL){
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //Read texture from file
    unsigned char* AMPLITUDES = new unsigned char[DATA_WIDTH * DATA_HEIGHT * DATA_DEPTH];
    FILE* fp = fopen(DATA_FILE, "r");
    fread(AMPLITUDES , sizeof(unsigned char), DATA_WIDTH * DATA_HEIGHT * DATA_DEPTH, fp);
    fclose (fp);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile the shader zprogram
    Shader theShader("./resources/shaders/shader.vs", "./resources/shaders/shader.fs");

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // texture coord attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // load and create a texture
    unsigned int texture1;

    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_3D, texture1);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);//trilinear filtering
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//trilinear filtering

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, DATA_WIDTH, DATA_HEIGHT, DATA_DEPTH, 0, GL_RED, GL_UNSIGNED_BYTE, AMPLITUDES);

    glEnable(GL_TEXTURE_3D);
    //glGenerateMipmap(GL_TEXTURE_3D);//TODO: mipmap gerekli mi?

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);
        calculatePlanes();

        // render
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
        theShader.use();

        glm::mat4 projection = glm::perspective(0.78f, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
        theShader.setMat4("projection", projection);

        // render boxes
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(Vertex), &vertexBuffer[0], GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, vertexBuffer.size());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //de-allocate all resources once they've outlived their purpose:
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

struct ccw_sort
{
    inline bool operator() (Vertex& v1, Vertex& v2)
    {
        return positiveAngle(v1.vertexCoord) < positiveAngle(v2.vertexCoord);
    }
};


void calculatePlanes()
{
    unsigned int vertexCount = 8;
    unsigned int edgeCount = 12;

    //Transform world space vertex coords to view space vertex coords.
    glm::vec3 vertices_viewspace[vertexCount];
    glm::mat4 viewmatrix = camera.GetViewMatrix();
    
    for(int i=0; i < vertexCount; i++)
    {
        glm::vec4 vertex_viewspace = viewmatrix * glm::vec4(worldSpaceCubeVertices[i],1.0f);
        vertices_viewspace[i] = glm::vec3(vertex_viewspace);
    }

    //Find min and max z valued vertices in view space.
    int minz_idx = 0;
    int maxZ_idx = 0;

    for(int i=0; i< vertexCount; i++)
    {
        if(vertices_viewspace[i].z < vertices_viewspace[minz_idx].z){
            minz_idx = i;
        }

        if(vertices_viewspace[i].z > vertices_viewspace[maxZ_idx].z){
            maxZ_idx = i;
        }
    }

    vertexBuffer.clear();
    
    //Slice the bounding box with view-aligned planes.
    //For each plane(zValue) find the intersection points of bounding box's edges.
    for(float zValue = vertices_viewspace[minz_idx].z; zValue < vertices_viewspace[maxZ_idx].z; zValue += 0.005f)
    {
        vector<Vertex> planeVertices(0);//max 6 intersections possible for a plane
        for(int i=0; i < edgeCount; i++)
        {
            glm::vec3 p1 = vertices_viewspace[edges[i].first];
            glm::vec3 p2 = vertices_viewspace[edges[i].second];
            glm::vec3 p1TexCoord = verticesTexCoords[edges[i].first];
            glm::vec3 p2TexCoord = verticesTexCoords[edges[i].second];

            //r(t) = p1 + (p2-p1) * t
            //Solve parametric equation of line for zValue, p1.z, p2.z and find t.
            //zValue = p1.z + (p2.z - p1.z) * t

            float t = (zValue - p1.z)/(p2.z - p1.z);
            if(t < 0 || t > 1)//No intersection with this edge.
            	continue;

            glm::vec3 vertexCoord(p1.x + (p2.x -p1.x) * t, p1.y + (p2.y -p1.y) * t, zValue);
            glm::vec3 texCoord = p1TexCoord + (p2TexCoord - p1TexCoord) * t;
            Vertex intersectionPoint(vertexCoord, texCoord);
            planeVertices.push_back(intersectionPoint);
        }

        //Sort vertices of the plane in ccw order.
        sort(planeVertices.begin(), planeVertices.end(), ccw_sort());

        //Avarage vertices in planeVertices, find middle vertex for the plane.
        Vertex midPoint;
        for(int i=0; i < planeVertices.size(); i++)
        {
            midPoint.vertexCoord += planeVertices[i].vertexCoord;
            midPoint.texCoord += planeVertices[i].texCoord;
        }
        midPoint.vertexCoord /= planeVertices.size();
        midPoint.texCoord /= planeVertices.size();
        
        //Construct triangle fan for the plane then add to a final integrated vector that will be sent to shader.
        for(int i=0; i < planeVertices.size(); i++)
        {
            vertexBuffer.push_back(planeVertices[i]);

            if(i != planeVertices.size() - 1)
                vertexBuffer.push_back(planeVertices[i+1]);
            else
                vertexBuffer.push_back(planeVertices[0]);

            vertexBuffer.push_back(midPoint);
        }
    }
}



inline float positiveAngle(glm::vec3 vec)
{
    return atan2 (vec.y, vec.x);
}

/*! Returns the pseudoangle between the line p1 to (infinity, p1.y) and the 
line from p1 to p2. The pseudoangle has the property that the ordering of 
points by true angle anround p1 and ordering of points by pseudoangle are the 
same The result is in the range [0, 4) (or error -1). */

float pseudoAngle(glm::vec3 p1, glm::vec3 p2) 
{
   glm::vec2 p1Proj = glm::normalize(glm::vec2(p1.x,p1.y));
   glm::vec2 p2Proj = glm::normalize(glm::vec2(p2.x,p2.y));
   glm::vec2 delta = p2Proj - p1Proj;
   float result;
   
   if ((delta.x == 0) && (delta.y == 0)) {
      return -1;
   } 
   else 
   {
      result = delta.y / (abs(delta.x) + abs(delta.y));
      
      if (delta.x < 0.0) {
         result = 2.0f - result;
      } else {
         result = 4.0f + result;
      }   
   }
   
   return result;
}


void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.move(glm::vec3(0.0f, -0.01f, 0.0f));
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.move(glm::vec3(-0.01f, 0.0f, 0.0f));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.move(glm::vec3(0.01f, 0.0f, 0.0f));
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.move(glm::vec3(0.0f, 0.01f, 0.0f));


    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        camera.move(glm::vec3(0.0f, 0.00f, 0.01f));

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        camera.move(glm::vec3(0.0f, 0.00f, -0.01f));


    if (glfwGetKey(window, GLFW_KEY_KP_4) == GLFW_PRESS)
        camera.rotateLeft();

    if (glfwGetKey(window, GLFW_KEY_KP_6) == GLFW_PRESS)
        camera.rotateRight();

    if (glfwGetKey(window, GLFW_KEY_KP_2) == GLFW_PRESS)
        camera.rotateDown();

    if (glfwGetKey(window, GLFW_KEY_KP_8) == GLFW_PRESS)
        camera.rotateUp();
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    //camera.ProcessMouseScroll(yoffset);
}
