#include "Game.hpp"
#include "CitySpawner.hpp"

#include <memory>
#include <random>

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <nlohmann/json.hpp>

static const char* const SKYBOX_SRC = "assets/skybox/skybox.ktx2";
static const char* const PLANE_MODEL_PATH = "assets/plane.obj";

static const char* const PLANE_TEXTURE_PATH = "assets/plane.jpg";


void Game::init(fly::Engine& engine) {
    std::mt19937_64 rng(std::random_device{}());
    std::normal_distribution norm(78.0, 5.0);
    for(int i=0; i<50; ++i) {
        for(int j=0; j<200; ++j)
            this->str += (char) std::lround(norm(rng));
        this->str += '\n';
    }
    
    engine.getTextRenderer().loadFont(
        "DS_DIGITAL", 
        std::filesystem::path("assets/font.png"), 
        std::filesystem::path("assets/font.json")
    );
    
    this->defaultPipeline = engine.addPipeline<fly::DefaultPipeline>(0);
    this->planeTexture = std::make_unique<fly::Texture>(
        engine.getVulkanInstance(), engine.getCommandPool(), std::filesystem::path(PLANE_TEXTURE_PATH), 
        fly::STB_Format::STBI_rgb_alpha, VK_FORMAT_R8G8B8A8_SRGB
    );
    this->planeSampler = std::make_unique<fly::TextureSampler>(engine.getVulkanInstance(), planeTexture->getMipLevels());

    auto cubemap = std::make_unique<fly::Texture>(engine.getVulkanInstance(), engine.getCommandPool(), std::filesystem::path(SKYBOX_SRC));
    auto cubemapSampler = std::make_unique<fly::TextureSampler>(engine.getVulkanInstance(), cubemap->getMipLevels());
    this->skybox = std::make_unique<fly::Skybox>(engine, std::move(cubemap), std::move(cubemapSampler));
    this->uniformBuffer = std::make_unique<fly::TUniformBuffer<fly::DefaultUBO>>(engine.getVulkanInstance());
    
    auto planeVAO = loadModel(engine.getVulkanInstance(), engine.getCommandPool(), std::filesystem::path(PLANE_MODEL_PATH));
    this->vertices = planeVAO->getVertices();
    this->indices = planeVAO->getIndices();

    this->earth = std::make_unique<EarthRenderer>(engine, 100);

    this->loadMap();
}

void Game::run(double dt, uint32_t currentFrame, fly::Engine& engine) {
    auto& window = engine.getWindow();
    auto& vk = engine.getVulkanInstance();
    static int sphereDivs = 100; // FIXME:

    {
        ImGui::ColorEdit4("Color", &myColor[0]);
        ImGui::SliderFloat("Gamma", &gamma, 0, 3);
        ImGui::SliderInt("Divs", &sphereDivs, 2, 1000);
    }

    this->cam.update(engine.getWindow(), dt);
    if(window.keyJustPressed(GLFW_KEY_F)) {
        auto vertices = this->vertices;
        auto indices = this->indices;
        this->planeIdx = defaultPipeline->attachModel(
            std::make_unique<fly::VertexArray>(vk, engine.getCommandPool(), std::move(vertices), std::move(indices))
        );
        this->defaultPipeline->updateDescriptorSet(
            planeIdx, 
            *uniformBuffer, 
            *planeTexture, 
            *planeSampler
        );
    }
    if(window.keyJustReleased(GLFW_KEY_F)) {
        this->defaultPipeline->detachModel(this->planeIdx, currentFrame);
    }
    if(window.isKeyPressed(GLFW_KEY_G)) {
        fly::ScopeTimer t("TEXT CPU RENDERING");
        engine.getTextRenderer().renderText(
            "DS_DIGITAL", 
            str, 
            {0, 0}, fly::Align::LEFT, 14.0, {1, 1, 1, 1}
        );
    }
    totalTime += dt;
    fly::DefaultUBO ubo, skyboxUbo;
    ubo.model = glm::mat4(1.0f);
    ubo.view = this->cam.getView();
    ubo.proj = this->cam.getProjection();

    skyboxUbo.model = glm::mat4(1.0f);
    skyboxUbo.view = glm::mat4(glm::mat3(this->cam.getView()));
    skyboxUbo.proj = this->cam.getProjection();
    ubo.gamma = skyboxUbo.gamma = gamma;

    this->uniformBuffer->updateUBO(ubo, currentFrame);
    
    //RENDER TEXTURE
    this->skybox->render(currentFrame, cam.getProjection(), cam.getView());

    this->earth->render(engine, currentFrame, sphereDivs, cam.getProjection(), cam.getView());


    if(window.keyJustPressed(GLFW_KEY_C)) {
        auto isoIt = this->countries.begin();
        std::advance( isoIt, rand() % this->countries.size() );

        if(this->countries[isoIt->first].state == CountryState::LOCKED) {
            spawner.addCountry(isoIt->first);
            this->countries[isoIt->first].state = CountryState::UNLOCKED;
            std::cout << isoIt->second.name << std::endl;
        }
    }

    if(window.keyJustPressed(GLFW_KEY_V)) {
        std::optional<City> city;
        while(city = spawner.getRandomCity(), !city.has_value());
        std::cout << std::format("{} -> {}", city->name, countries[city->country].name) << std::endl;
    }
}

void Game::loadMap() {
    spawner.load();

    //COUNTRY MESH
    using json = nlohmann::json;
    std::ifstream countryFile(COUNTRIES_DATA_FILE);
    json countryData = json::parse(countryFile);

    for(auto& [k, v]: countryData.items()) {
        Country c;
        c.name = v["name"].template get<std::string>();
        auto banned = v["banned"].template get<bool>();
        c.state = banned? CountryState::BANNED : CountryState::LOCKED;

        this->countries[k] = std::move(c);
    }
}
