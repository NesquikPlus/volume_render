#include <iostream> 
#include <math.h>
#include <algorithm>
#include <unistd.h>
#include <utility> 
#include <limits>     
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>

#include "shader_m.h"
#include "camera.h"

#define DATA_WIDTH 128
#define DATA_HEIGHT 128
#define DATA_DEPTH 128
#define DATA_FILE "brain.txt"

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
bool isPointBetween(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
float pseudoAngle(glm::vec3 p1, glm::vec3 p2);
float positiveAngle(glm::vec3 vec);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 1.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;

vector<Vertex> vertexBuffer;

glm::vec3 worldSpaceCubeVertices[] = 
{
    glm::vec3(-0.5f, -0.5f, -0.5f), //sol alt arka
    glm::vec3(-0.5f,  0.5f, -0.5f), //sol ust arka
    glm::vec3( 0.5f, -0.5f, -0.5f), //sag alt arka
    glm::vec3( 0.5f,  0.5f, -0.5f), //sag ust arka

    glm::vec3(-0.5f, -0.5f,  0.5f), //sol alt on
    glm::vec3(-0.5f,  0.5f,  0.5f), //sol ust on
    glm::vec3( 0.5f, -0.5f,  0.5f), //sag alt on
    glm::vec3( 0.5f,  0.5f,  0.5f)  //sag ust on
};

glm::vec3 verticesTexCoords[] = 
{
    glm::vec3(-1.0f, -1.0f, -1.0f), //sol alt arka
    glm::vec3(-1.0f,  1.0f, -1.0f), //sol ust arka
    glm::vec3( 1.0f, -1.0f, -1.0f), //sag alt arka
    glm::vec3( 1.0f,  1.0f, -1.0f), //sag ust arka

    glm::vec3(-1.0f, -1.0f,  1.0f), //sol alt on
    glm::vec3(-1.0f,  1.0f,  1.0f), //sol ust on
    glm::vec3( 1.0f, -1.0f,  1.0f), //sag alt on
    glm::vec3( 1.0f,  1.0f,  1.0f)  //sag ust on
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Volume Render", NULL, NULL);
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
    unsigned char data[DATA_WIDTH * DATA_HEIGHT * DATA_DEPTH];
    FILE* fp = fopen(DATA_FILE, "r");
    fread(data , sizeof(unsigned char), sizeof(data), fp);
    fclose (fp);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile the shader zprogram
    Shader theShader("shader.vs", "shader.fs");

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

    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);//TODO: Gerekli mi?

    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_3D, texture1);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);//bilinear filtering
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//bilinear filtering

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, DATA_WIDTH, DATA_HEIGHT, DATA_DEPTH, 0, GL_RED, GL_UNSIGNED_BYTE, data);

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

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
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

    // optional: de-allocate all resources once they've outlived their purpose:
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

    glm::vec3 vertices[vertexCount];
    glm::mat4 viewMatrix = camera.GetViewMatrix();
    
    for(int i=0; i < vertexCount; i++)
    {
        glm::vec4 tmp = viewMatrix * glm::vec4(worldSpaceCubeVertices[i],1.0f);
        vertices[i] = glm::vec3(tmp);
    }
    //Find min max z viewSpaceCubeVertex
    int minIndex = 0;
    int maxIndex = 0;

    for(int i=0; i< vertexCount; i++)
    {
        if(vertices[i].z < vertices[minIndex].z){
            minIndex = i;
        }

        if(vertices[i].z > vertices[maxIndex].z){
            maxIndex = i;
        }
    }

    vertexBuffer.clear();
    float zValue = vertices[minIndex].z;

    while(zValue < vertices[maxIndex].z)
    {
        vector<Vertex> planeVerticesInitial(0);
        //For each plane(zValue) find each edge's intersection point.
        for(int i=0; i < edgeCount; i++)
        {
            glm::vec3 p1 = vertices[edges[i].first];
            glm::vec3 p2 = vertices[edges[i].second];
            glm::vec3 p1TexCoord = verticesTexCoords[edges[i].first];
            glm::vec3 p2TexCoord = verticesTexCoords[edges[i].second];

            //r(t) = p1 + (p2-p1) * t
            //Solve parametric equation of line for zValue, p1.z, p2.z and find t.
            //zValue = p1.z + (p2.z - p1.z) * t

            if(p2.z - p1.z == 0)//No intersection with this edge.
                continue;

            float t = (zValue - p1.z)/(p2.z - p1.z);
            glm::vec3 vertexCoord(p1.x + (p2.x -p1.x) * t, p1.y + (p2.y -p1.y) * t, zValue);
            
            if(isPointBetween(vertexCoord, p1, p2) == false)//No intersection with this edge.
                continue;

            glm::vec3 texCoord = p1TexCoord + (p2TexCoord - p1TexCoord) * t;
            //std::cout<<glm::to_string(texCoord)<<std::endl;
            Vertex intersectionPoint(vertexCoord, texCoord);
            planeVerticesInitial.push_back(intersectionPoint);
        }

        //Sort vertices of the plane in planeVerticesInitial ccw.
        sort(planeVerticesInitial.begin(), planeVerticesInitial.end(), ccw_sort());

        //Avarage vertices in planeVerticesInitial find middle vertex for the plane.
        Vertex midPoint;
        for(int i=0; i < planeVerticesInitial.size(); i++)
        {
            midPoint.vertexCoord += planeVerticesInitial[i].vertexCoord;
            midPoint.texCoord += planeVerticesInitial[i].texCoord;
        }
        midPoint.vertexCoord /= planeVerticesInitial.size();
        midPoint.texCoord /= planeVerticesInitial.size();
        
        //Construct triangle fan for the plane, add vertices to float vertexBuffer vector.
        for(int i=0; i < planeVerticesInitial.size(); i++)
        {
            vertexBuffer.push_back(planeVerticesInitial[i]);

            if(i != planeVerticesInitial.size() - 1)
                vertexBuffer.push_back(planeVerticesInitial[i+1]);
            else
                vertexBuffer.push_back(planeVerticesInitial[0]);

            vertexBuffer.push_back(midPoint);
        }
        zValue += 0.005f;
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



bool isPointBetween(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)//TODO: Optimize
{
    float minX, maxX;
    float minY, maxY;

    if(p2.x > p3.x){
        minX = p3.x;
        maxX = p2.x;
    }
    else{
        maxX = p3.x;
        minX = p2.x;
    }

    if(minX > p1.x || p1.x > maxX)
        return false;

    if(p2.y > p3.y){
        minY = p3.y;
        maxY = p2.y;
    }
    else{
        maxY = p3.y;
        minY = p2.y;
    }

    if(minY > p1.y || p1.y > maxY)
        return false;

    return true;
}


void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(UPWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWNWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
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
    camera.ProcessMouseScroll(yoffset);
}