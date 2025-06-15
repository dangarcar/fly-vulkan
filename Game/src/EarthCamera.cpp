#include "EarthCamera.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <GLFW/glfw3.h>

#include <imgui.h>

#include <Window.hpp>


void EarthCamera::setPos(glm::vec3 newPos) {
    auto lat = glm::degrees(glm::asin(newPos.y));
    
    while(glm::abs(lat) - MAX_LAT > 1e-5) {
        auto y = glm::sin(glm::radians(MAX_LAT)) * glm::sign(newPos.y);
        newPos.y = y;
        newPos = glm::normalize(newPos);

        lat = glm::degrees(glm::asin(newPos.y));
    }

    if(!glm::any(glm::isnan(newPos)) && !glm::any(glm::isinf(newPos)) && glm::abs(glm::length(newPos) - 1) < 1e-6)
        this->normPos = glm::normalize(newPos);
}

void EarthCamera::update(fly::Window& window, float dt) {
    /*ImGui::SliderFloat("FOV", &this->fov, 10, 170);
    
    ImGui::LabelText("Pos", "%f %f %f", getPos().x, getPos().y, getPos().z);
    ImGui::LabelText("Angular speed", "%f", this->angularVel);

    ImGui::SliderFloat("Speed", &this->speed, 0, 10);
    ImGui::SliderFloat("Max dragging time", &this->maxDraggingTime, 0, 1);
    ImGui::SliderFloat("Scroll acc", &this->scrollAcc, 0, 100);
    ImGui::SliderFloat("Angular damping", &this->angularDamping, 0, 1);*/

    this->mouseControlled = false;
    if(window.isMouseBtnPressed(fly::MouseButton::LEFT)) {
        auto p = intersectRayUnitSphere( mouseRay(window, window.getMousePos()));
        auto oldMouse = window.getMousePos() - window.getMouseDelta();
        if(window.mouseClicked(fly::MouseButton::LEFT)) { //If last state was not pressed
            this->incT = 0;
            this->firstMouse = oldMouse;
        }
        this->mouseControlled = true;
        this->incT += dt;

        if(glm::length(window.getMouseDelta()) > 0) {
            auto q = intersectRayUnitSphere( mouseRay(window, oldMouse));
            this->lastMouse = window.getMousePos();

            auto angle = glm::angle(p, q);
            if(!glm::isnan(angle) || angle != 0 || !glm::isinf(angle)) {
                this->rotAxis = glm::normalize(glm::cross(p, q));
                this->angularVel += angle;

                setPos(glm::angleAxis(angle, this->rotAxis) * this->normPos);
            }
        }        
    } else {
        auto right = glm::cross(UP, normPos);
        auto localUp = glm::cross(normPos, right);
        glm::vec3 newPos = this->normPos;
        
        if(window.isKeyPressed(GLFW_KEY_W))
            newPos += localUp * speed * dt;
        else if(window.isKeyPressed(GLFW_KEY_S))
            newPos -= localUp * speed * dt;
        
        if(window.isKeyPressed(GLFW_KEY_A))
            newPos -= right * speed * dt;
        else if(window.isKeyPressed(GLFW_KEY_D))
            newPos += right * speed * dt;

        setPos(newPos);
    }
    

    if(glm::abs(window.getScroll()) > 0 && !this->mouseControlled) {
        this->mouseControlled = true;
        auto scrollSpeed = this->height * this->height * this->scrollAcc * window.getScroll();

        auto p = intersectRayUnitSphere(mouseRay(window, window.getMousePos()));
        this->height = glm::clamp(this->height - scrollSpeed * dt, MIN_HEIGHT, MAX_HEIGHT);
        this->view = glm::lookAt(this->height * this->normPos, glm::vec3(0.0f), UP);
        auto q = intersectRayUnitSphere(mouseRay(window, window.getMousePos()));

        auto angle = -glm::angle(p, q);
        if(!glm::isnan(angle) || angle != 0 || !glm::isinf(angle)) {
            auto axis = glm::normalize(glm::cross(p, q));
            setPos(glm::angleAxis(angle, axis) * this->normPos);
        }
    }
    
    if(glm::abs(this->angularVel) > 0 && !this->mouseControlled) {
        if(this->incT != 0) {
            auto p = intersectRayUnitSphere(mouseRay(window, this->firstMouse));
            auto q = intersectRayUnitSphere(mouseRay(window, this->lastMouse));
            this->angularVel = -glm::angle(p, q) / this->incT;
            this->rotAxis = glm::normalize(glm::cross(p, q));
            
            if(this->incT > this->maxDraggingTime)
                this->angularVel = 0;
            this->incT = 0;
        }

        setPos(glm::angleAxis(this->angularVel * dt, this->rotAxis) * this->normPos);
        
        this->angularVel *= glm::pow(this->angularDamping, 2*dt);
    }

    auto lat = glm::degrees(glm::asin(normPos.y/glm::length(normPos)));
    auto lon = glm::degrees(glm::atan(normPos.x, normPos.z));
    ImGui::Text("LAT: %.2f LON: %.2f", lat, lon);

    this->view = glm::lookAt(height * this->normPos, glm::vec3(0.0f), UP);
    if(window.getHeight() != 0)  {
        this->proj = glm::perspective(glm::radians(this->fov), window.getWidth() / (float) window.getHeight(), 0.05f, 10.0f);
        this->proj[1][1] *= -1;
    }

}

Ray EarthCamera::mouseRay(fly::Window& window, glm::vec2 mousePos) const {    
    float xNdc = (float(mousePos.x)/window.getWidth()  - 0.5f) * 2.0f;
    float yNdc = (float(mousePos.y)/window.getHeight() - 0.5f) * 2.0f;

    glm::mat4 invVP = glm::inverse(this->proj * this->view);

    glm::vec4 rayStart = invVP * glm::vec4(xNdc, yNdc,-1.0f,1.0f); 
    rayStart /= rayStart.w;
    glm::vec4 rayEnd = invVP * glm::vec4(xNdc, yNdc,0.0f,1.0f);
    rayEnd /= rayEnd.w;
    
    glm::vec3 rayDir = glm::normalize(glm::vec3(rayEnd - rayStart));
    
    return Ray {rayStart,rayDir };
}

glm::vec3 EarthCamera::intersectRayUnitSphere(Ray r) {
    float b = glm::dot(r.origin, r.direction); 
    float c = glm::dot(r.origin, r.origin) - 1; 

    if (c > 0.0f && b > 0.0f) 
        return glm::vec3(0, 0, 0); 
    float discr = b*b - c; 

    if (discr < 0.0f) 
        return glm::vec3(0, 0, 0);

    float t = -b - glm::sqrt(discr); 

    if (t < 0.0f) t = 0.0f; 
    glm::vec3 q = r.origin + t * r.direction; 

    return q;
}

