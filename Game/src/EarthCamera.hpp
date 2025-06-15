#pragma once

#include <glm/glm.hpp>

namespace fly {
    class Window;
}

struct Ray {
    glm::vec3 origin, direction;
};

class EarthCamera {
public:
    static constexpr glm::vec3 UP = {0, 1, 0};
    static constexpr float MAX_LAT = 65.0f, MIN_HEIGHT = 1.15f, MAX_HEIGHT = 5.0f;

public:
    EarthCamera() = default;
    ~EarthCamera() = default;

    void update(fly::Window& window, float dt);

    glm::mat4 getProjection() const { return this->proj; }
    glm::mat4 getView() const { return this->view; } 

    glm::vec3 getPos() const { return this->normPos * height; }

    Ray mouseRay(fly::Window& window, glm::vec2 mousePos) const;
    static glm::vec3 intersectRayUnitSphere(Ray ray);

private:
    glm::mat4 proj, view;
    glm::vec3 normPos = {0, 0, 1};
    float height = 1.5f;

    float fov = 45.0f, speed = 1.0f, maxDraggingTime = 0.2;

    float scrollAcc = 52.0f;
    float angularVel = 0.0f, angularDamping = 0.01f;
    bool mouseControlled = false;

    float incT;
    glm::vec3 rotAxis;
    glm::vec2 firstMouse, lastMouse;

private:
    void setPos(glm::vec3 newPos);

};