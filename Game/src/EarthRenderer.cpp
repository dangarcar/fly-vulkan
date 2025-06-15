#include "EarthRenderer.hpp"

#include <Utils.hpp>
#include <filesystem>


//EARTH RENDERER IMPLEMENTATION
EarthRenderer::EarthRenderer(fly::Engine& engine, unsigned divs) {
	this->pipeline = engine.addPipeline<EarthPipepine>(0);

    this->earthCubemap = std::make_unique<fly::Texture>(engine.getVulkanInstance(), engine.getCommandPool(), std::filesystem::path(EARTH_CUBEMAP_SRC));
    this->earthCubemapSampler = std::make_unique<fly::TextureSampler>(engine.getVulkanInstance(), this->earthCubemap->getMipLevels());

    this->uniformBuffer = std::make_unique<fly::TUniformBuffer<UBOEarth>>(engine.getVulkanInstance());

	this->divs = divs;
    this->meshIdx = this->pipeline->attachModel(EarthRenderer::generateCubesphere(engine.getVulkanInstance(), engine.getCommandPool(), divs));
	this->pipeline->updateDescriptorSet(this->meshIdx, *this->uniformBuffer, *this->earthCubemap, *this->earthCubemapSampler);
}

void EarthRenderer::render(fly::Engine& engine, uint32_t currentFrame, unsigned divs, glm::mat4 projection, glm::mat4 view) {
	if(this->divs != divs) {
		this->divs = divs;
        this->pipeline->detachModel(this->meshIdx, currentFrame);
   		this->meshIdx = this->pipeline->attachModel(EarthRenderer::generateCubesphere(engine.getVulkanInstance(), engine.getCommandPool(), divs));
		this->pipeline->updateDescriptorSet(this->meshIdx, *this->uniformBuffer, *this->earthCubemap, *this->earthCubemapSampler);
	}

	UBOEarth ubo;
	ubo.projection = projection;
	ubo.view = view;
	this->uniformBuffer->updateUBO(ubo, currentFrame);
}


//EARTH PIPEPELINE IMPLEMENTATION
void EarthPipepine::updateDescriptorSet(
    unsigned meshIndex,
    const fly::TUniformBuffer<UBOEarth>& uniformBuffer,
    
    const fly::Texture& earthCubemap,
    const fly::TextureSampler& earthCubemapSampler
) {
	for(int i=0; i<fly::MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffer.getBuffer(i);
        bufferInfo.offset = 0;
        bufferInfo.range = uniformBuffer.getSize();

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = earthCubemap.getImageView();
        imageInfo.sampler = earthCubemapSampler.getSampler();

        std::vector<VkWriteDescriptorSet> descriptorWrites(2);

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = this->meshes[meshIndex].descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = this->meshes[meshIndex].descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

VkDescriptorSetLayout EarthPipepine::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
    
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
	VkDescriptorSetLayout descriptorSetLayout;
	if (vkCreateDescriptorSetLayout(vk.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
    
	return descriptorSetLayout;
}

VkDescriptorPool EarthPipepine::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(fly::MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(fly::MAX_FRAMES_IN_FLIGHT);
    
	VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(fly::MAX_FRAMES_IN_FLIGHT);
    
	VkDescriptorPool descriptorPool;
    if(vkCreateDescriptorPool(vk.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
    
	return descriptorPool;
}


//CUBESPHERE CREATION
int setQuad(std::vector<uint32_t>& triangles, int i, int v00, int v10, int v01, int v11) {
	triangles[i] = v00;
	triangles[i + 1] = triangles[i + 4] = v01;
	triangles[i + 2] = triangles[i + 3] = v10;
	triangles[i + 5] = v11;
	return i + 6;
}

int createTopFace(std::vector<uint32_t>& triangles, int divs, int t, int ring) {
    int v = ring * divs;
	for (int x = 0; x < divs - 1; x++, v++) {
		t = setQuad(triangles, t, v, v + 1, v + ring - 1, v + ring);
	}
	t = setQuad(triangles, t, v, v + 1, v + ring - 1, v + 2);
	
    int vMin = ring * (divs + 1) - 1;
	int vMid = vMin + 1;
    int vMax = v + 2;
    for (int z = 1; z < divs - 1; z++, vMin--, vMid++, vMax++) {
		t = setQuad(triangles, t, vMin, vMid, vMin - 1, vMid + divs - 1);
		for (int x = 1; x < divs - 1; x++, vMid++) {
			t = setQuad(triangles, t,vMid, vMid + 1, vMid + divs - 1, vMid + divs);
		}
		t = setQuad(triangles, t, vMid, vMax, vMid + divs - 1, vMax + 1);
	}
    int vTop = vMin - 2;
	t = setQuad(triangles, t, vMin, vMid, vTop + 1, vTop);
    for (int x = 1; x < divs - 1; x++, vTop--, vMid++) {
		t = setQuad(triangles, t, vMid, vMid + 1, vTop, vTop - 1);
	}
    t = setQuad(triangles, t, vMid, vTop - 2, vTop, vTop - 1);
	return t;
}

int createBottomFace(size_t len, std::vector<uint32_t>& triangles, int divs, int t, int ring) {
	int v = 1;
	int vMid = len - (divs - 1) * (divs - 1);
	t = setQuad(triangles, t, ring - 1, vMid, 0, 1);
	for(int x = 1; x < divs - 1; x++, v++, vMid++) {
		t = setQuad(triangles, t, vMid, vMid + 1, v, v + 1);
	}
	t = setQuad(triangles, t, vMid, v + 2, v, v + 1);
	int vMin = ring - 2;
	vMid -= divs - 2;
	int vMax = v + 2;
	for(int z = 1; z < divs - 1; z++, vMin--, vMid++, vMax++) {
		t = setQuad(triangles, t, vMin, vMid + divs - 1, vMin + 1, vMid);
		for (int x = 1; x < divs - 1; x++, vMid++) {
			t = setQuad(
				triangles, t,
				vMid + divs - 1, vMid + divs, vMid, vMid + 1);
		}
		t = setQuad(triangles, t, vMid + divs - 1, vMax + 1, vMid, vMax);
	}
	int vTop = vMin - 1;
	t = setQuad(triangles, t, vTop + 1, vTop, vTop + 2, vMid);
	for (int x = 1; x < divs - 1; x++, vTop--, vMid++) {
		t = setQuad(triangles, t, vTop, vTop - 1, vMid, vMid + 1);
	}
	t = setQuad(triangles, t, vTop, vTop - 1, vMid, vTop - 2);
	
	return t;
}

std::unique_ptr<fly::SimpleVertexArray> EarthRenderer::generateCubesphere(const fly::VulkanInstance& vk, const VkCommandPool commandPool, int divs) {
    int cornerVertices = 8;
	int edgeVertices = (divs + divs + divs - 3) * 4;
	int faceVertices = (
		(divs - 1) * (divs - 1) +
		(divs - 1) * (divs - 1) +
		(divs - 1) * (divs - 1)) * 2;
	
	std::vector<fly::SimpleVertex> vertices(cornerVertices + edgeVertices + faceVertices);
    {
        int v = 0;
        for (int y = 0; y <= divs; y++) {
            for(int x = 0; x <= divs; x++)
                vertices[v++].pos = glm::vec3(x, y, 0);
            for(int z = 1; z <= divs; z++)
                vertices[v++].pos = glm::vec3(divs, y, z);
            for(int x = divs - 1; x >= 0; x--)
                vertices[v++].pos = glm::vec3(x, y, divs);
            for(int z = divs - 1; z > 0; z--)
                vertices[v++].pos = glm::vec3(0, y, z);
        }
        for(int z = 1; z < divs; z++)
            for(int x = 1; x < divs; x++)
                vertices[v++].pos = glm::vec3(x, divs, z);
        for(int z = 1; z < divs; z++)
            for(int x = 1; x < divs; x++)
                vertices[v++].pos = glm::vec3(x, 0, z);
    }
    
	int quads = (divs * divs + divs * divs + divs * divs) * 2;
    std::vector<uint32_t> indices(quads * 6);
    {
        int ring = (divs + divs) * 2;
	    int t = 0, v = 0;
	    for(int y = 0; y < divs; y++, v++) {
	    	for(int q = 0; q < ring - 1; q++, v++) 
	    		t = setQuad(indices, t, v, v + 1, v + ring, v + ring + 1);
	    	t = setQuad(indices, t, v, v - ring + 1, v + ring, v + 1);
	    }
        t = createTopFace(indices, divs, t, ring);
        t = createBottomFace(vertices.size(), indices, divs, t, ring);
    }
	
	for(auto& vtx: vertices) {
        glm::vec3 v = vtx.pos * (2.0f/divs) - glm::vec3(1,1,1);
        float x2 = v.x * v.x;
	    float y2 = v.y * v.y;
	    float z2 = v.z * v.z;

	    vtx.pos.x = v.x * glm::sqrt(1 - y2 / 2 - z2 / 2 + y2 * z2 / 3);
	    vtx.pos.y = v.y * glm::sqrt(1 - x2 / 2 - z2 / 2 + x2 * z2 / 3);
	    vtx.pos.z = v.z * glm::sqrt(1 - x2 / 2 - y2 / 2 + x2 * y2 / 3);
    }

    return std::make_unique<fly::SimpleVertexArray>(vk, commandPool, std::move(vertices), std::move(indices));
}
