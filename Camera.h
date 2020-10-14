#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
    private:

        // Camera attributes
        glm::vec3 camPos,rotPoint;
        glm::mat4 viewMatrix;

        //Spherical coordinates
        float azimuthalAngle;
        float polarAngle;

        //Mouse settings
        float MovementSpeed;
        float MouseSensitivity;

    public:

        Camera(glm::vec3 camPos, glm::vec3 rotPoint)
        {
            polarAngle = 0.0f;
            azimuthalAngle =  0.0f;

            this->camPos = camPos;
            this->rotPoint = rotPoint;

            viewMatrix = glm::translate(glm::mat4(1.0f), -camPos);

            viewMatrix = glm::translate(viewMatrix, -rotPoint);
            viewMatrix = glm::rotate(viewMatrix, azimuthalAngle, glm::vec3(0,1,0));
            viewMatrix = glm::translate(viewMatrix, rotPoint);
        }

        void rotateRight()
        {
            viewMatrix = glm::translate(viewMatrix, rotPoint);
            viewMatrix = glm::rotate(viewMatrix, 0.005f, glm::vec3(0,1,0));
            viewMatrix = glm::translate(viewMatrix, -rotPoint);
            azimuthalAngle -= 0.005f;
        }


        void rotateLeft()
        {
            viewMatrix = glm::translate(viewMatrix, rotPoint);
            viewMatrix = glm::rotate(viewMatrix, -0.005f, glm::vec3(0,1,0));
            viewMatrix = glm::translate(viewMatrix, -rotPoint);
            azimuthalAngle += 0.005f;
        }

        void rotateUp()
        {
            viewMatrix = glm::translate(viewMatrix, rotPoint);
            viewMatrix = glm::rotate(viewMatrix, -0.005f, glm::vec3(cos(azimuthalAngle),0, -sin(azimuthalAngle)));
            viewMatrix = glm::translate(viewMatrix, -rotPoint);
            polarAngle -= 0.005f;
        }

        void rotateDown()
        {
            viewMatrix = glm::translate(viewMatrix, rotPoint);
            viewMatrix = glm::rotate(viewMatrix, 0.005f, glm::vec3(cos(azimuthalAngle),0, -sin(azimuthalAngle)));
            viewMatrix = glm::translate(viewMatrix, -rotPoint);
            polarAngle += 0.005f;
        }



        void move(glm::vec3 deltaPos)
        {
            camPos += deltaPos;
            viewMatrix = glm::translate(viewMatrix, -deltaPos);
        }

        // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
        glm::mat4 GetViewMatrix()
        {
            return viewMatrix;
        }

        // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
        void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
        {
            xoffset *= MouseSensitivity;
            yoffset *= MouseSensitivity;
        }
};
#endif