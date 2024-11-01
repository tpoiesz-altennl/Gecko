#pragma once
#include "Defines.h"

#include "Rendering/Backend/Objects.h"
#include "Rendering/Backend/Device.h"
#include "Rendering/Frontend/ResourceManager/ResourceManager.h"
#include "Rendering/Frontend/Renderer/RenderPasses/RenderPass.h"

#include "Core/Platform.h"


namespace Gecko {


struct SceneDescriptor;

class Renderer
{
public:
	Renderer() = default;
	~Renderer();
	
	void Init(Platform::AppInfo& info, ResourceManager* resourceManager, Device* _device);
	void Shutdown();

	template<typename T>
	Ref<T> CreateRenderPass()
	{
		Ref<T> renderPass = CreateRef<T>();
		renderPass->Init(m_Info, m_ResourceManager);
		m_RenderPasses.push_back(renderPass);
		return renderPass;
	}

	void ConfigureRenderPasses(std::vector<Ref<RenderPass>> renderPassStack)
	{
		m_RenderPassStack = renderPassStack;
	}

	void RenderScene(const SceneRenderInfo& sceneRenderInfo);
	void Present();

private:
	Device* device;

	ResourceManager* m_ResourceManager;

	std::vector<Ref<RenderPass>> m_RenderPasses;
	std::vector<Ref<RenderPass>> m_RenderPassStack;

	GraphicsPipelineHandle FullScreenTexturePipelineHandle;

	MeshHandle quadMeshHandle{ 0 };

	Platform::AppInfo m_Info;
};

}
