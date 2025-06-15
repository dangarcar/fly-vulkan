#pragma once

#include <Engine.hpp>
#include <renderer/Skybox.hpp>

static const char* const EARTH_FRAG_SHADER_SRC = "Game/shaders/earthfrag.spv";
static const char* const EARTH_VERT_SHADER_SRC = "Game/shaders/earthvert.spv";

static const char* const EARTH_CUBEMAP_SRC = "assets/skybox/earth/cube.ktx2";

const std::array<const char*, 6> earthCubemapPaths = {
    "assets/skybox/earth/px.ktx2",
    "assets/skybox/earth/nx.ktx2",
    "assets/skybox/earth/py.ktx2",
    "assets/skybox/earth/ny.ktx2",
    "assets/skybox/earth/pz.ktx2",
    "assets/skybox/earth/nz.ktx2",
};

struct UBOEarth {
    glm::mat4 projection;
	glm::mat4 view;
};

class EarthPipepine;

class EarthRenderer {
public:
    EarthRenderer(fly::Engine& engine, unsigned divs);
    ~EarthRenderer() = default;

    void render(fly::Engine& engine, uint32_t currentFrame, unsigned divs, glm::mat4 projection, glm::mat4 view);

private:
    EarthPipepine* pipeline = nullptr;

    unsigned meshIdx, divs;

    std::unique_ptr<fly::TUniformBuffer<UBOEarth>> uniformBuffer;
    std::unique_ptr<fly::TextureSampler> earthCubemapSampler;
    std::unique_ptr<fly::Texture> earthCubemap;

private:
    static std::unique_ptr<fly::SimpleVertexArray> generateCubesphere(const fly::VulkanInstance& vk, const VkCommandPool commandPool, int divs);

};

class EarthPipepine: public fly::TGraphicsPipeline<fly::SimpleVertex> {
public:
    EarthPipepine(const fly::VulkanInstance& vk): TGraphicsPipeline{vk, true} {}
    ~EarthPipepine() = default;

    void updateDescriptorSet(
        unsigned meshIndex,
        const fly::TUniformBuffer<UBOEarth>& uniformBuffer,
        
        const fly::Texture& earthCubemap,
        const fly::TextureSampler& earthCubemapSampler
    );

private:
    std::vector<char> getVertShaderCode() override {
        return fly::readFile(EARTH_VERT_SHADER_SRC);
    }
    
    std::vector<char> getFragShaderCode() override {
        return fly::readFile(EARTH_FRAG_SHADER_SRC);
    }

    VkDescriptorSetLayout createDescriptorSetLayout() override;

    VkDescriptorPool createDescriptorPool() override;

};