#pragma once

#include "EarthRenderer.hpp"
#include "EarthCamera.hpp"
#include "CitySpawner.hpp"

#include <Engine.hpp>
#include <renderer/DefaultPipeline.hpp>
#include <renderer/Skybox.hpp>
#include <unordered_map>

inline static const std::filesystem::path COUNTRIES_DATA_FILE = "resources/countries.json";

class Game: public fly::Scene {
public:
    Game() = default;
    ~Game() = default;
    
    void init(fly::Engine& engine) override;

    void run(double dt, uint32_t currentFrame, fly::Engine& engine) override;
    
    void loadMap();

private:
    fly::DefaultPipeline* defaultPipeline = nullptr;

    unsigned planeIdx;

    std::vector<fly::Vertex> vertices;
    std::vector<uint32_t> indices;

    std::unique_ptr<fly::TextureSampler> planeSampler;
    std::unique_ptr<fly::Texture> planeTexture;

    std::unique_ptr<fly::TUniformBuffer<fly::DefaultUBO>> uniformBuffer;
    std::unique_ptr<fly::Skybox> skybox;

    std::unique_ptr<EarthRenderer> earth;

    CitySpawner spawner;
    std::unordered_map<std::string, Country> countries;

    float gamma = 1.0f;
    glm::vec4 myColor;
    double totalTime = 0;
    std::string str;
    EarthCamera cam;

};