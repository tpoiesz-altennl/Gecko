#include "Rendering/Frontend/Renderer/Renderer.h"

#include "Core/Logger.h"
#include "Core/Asserts.h"
#include "Rendering/Backend/Device.h"
#include "Rendering/Backend/CommandList.h"
#include "Rendering/Frontend/Scene/Scene.h"

#include "stb_image.h"
#include <imgui.h>
#include "Core/Platform.h"

#include "glm/matrix.hpp"
#include <random>


namespace Gecko {

const VertexLayout fullScreenQuadVertexLayout({
	{Format::R32G32_FLOAT, "POSITION"},
});


void Renderer::Init(Platform::AppInfo& info, ResourceManager* resourceManager, Device* _device)
{
	m_Info = info;

	device = _device;

	m_ResourceManager = resourceManager;

	//{
	//	Gecko::TextureDesc RaytraceTargetDesc;

	//	RaytraceTargetDesc.Width = info.FullScreenWidth;
	//	RaytraceTargetDesc.Height = info.FullScreenHeight;
	//	RaytraceTargetDesc.Type = Gecko::TextureType::Tex2D;
	//	RaytraceTargetDesc.Format = Gecko::Format::R32_FLOAT;
	//	RaytraceTargetDesc.NumMips = 1;
	//	RaytraceTargetDesc.NumArraySlices = 1;
	//	RaytraceTarget = device->CreateTexture(RaytraceTargetDesc);

	//}

	// FullScreenTexture Graphics Pipeline
	{
		std::vector<ShaderVisibility> textureShaderVisibilities =
		{
			ShaderVisibility::Pixel,
		};

		std::vector<SamplerDesc> samplerShaderDescs =
		{
			{
				ShaderVisibility::Pixel,
				SamplerFilter::Point,
			},
		};

		GraphicsPipelineDesc pipelineDesc;
		pipelineDesc.VertexShaderPath = "Shaders/FullScreenTexture.gsh";
		pipelineDesc.PixelShaderPath = "Shaders/FullScreenTexture.gsh";
		pipelineDesc.ShaderVersion = "5_1";
		pipelineDesc.VertexLayout = fullScreenQuadVertexLayout;
		pipelineDesc.RenderTargetFormats[0] = Format::R8G8B8A8_UNORM; // Albedo
		pipelineDesc.CullMode = CullMode::Back;

		pipelineDesc.TextureShaderVisibilities = textureShaderVisibilities;

		pipelineDesc.SamplerDescs = samplerShaderDescs;

		FullScreenTexturePipelineHandle = m_ResourceManager->CreateGraphicsPipeline(pipelineDesc);
	}


	// Make The quad
	{

		glm::vec2 vertices[] = {
			{-1.f, -1.f},
			{-1.f,  3.f},
			{ 3.f, -1.f},
		};

		Gecko::VertexBufferDesc vertexDesc;
		vertexDesc.Layout = fullScreenQuadVertexLayout;
		vertexDesc.VertexData = &vertices;
		vertexDesc.NumVertices = static_cast<u32>(sizeof(vertices) / sizeof(glm::vec2));

		u16 indices[] = {
			0, 1, 2
		};

		Gecko::IndexBufferDesc indexDesc;
		indexDesc.IndexFormat = Gecko::Format::R16_UINT;
		indexDesc.NumIndices = 3;
		indexDesc.IndexData = indices;

		quadMeshHandle = m_ResourceManager->CreateMesh(vertexDesc, indexDesc, false);
	}
}

Renderer::~Renderer()
{
}

void Renderer::Shutdown()
{

	m_RenderPassStack.clear();
	m_RenderPasses.clear();
	device = nullptr;
	m_ResourceManager = nullptr;
}

void Renderer::RenderScene(const SceneRenderInfo& sceneRenderInfo)
{
	/*TLASRefitDesc refitDesc;

	for (u32 i = 0; i < sceneDescriptor.Meshes.size(); i++)
	{
		const MeshInstanceDescriptor& meshInstanceDescriptor = sceneDescriptor.Meshes[i];

		glm::mat4 transform = sceneDescriptor.NodeTransformMatrices[meshInstanceDescriptor.NodeTransformMatrixIndex];
		BLAS blas = m_ResourceManager->GetMesh(meshInstanceDescriptor.MeshInstance.MeshHandle).BLAS;

		refitDesc.BLASInstances.push_back({blas, transform});
	}*/

	u32 currentBackBufferIndex = device->GetCurrentBackBufferIndex();

	// Upload Scene Data for this frame
	m_ResourceManager->SceneData[currentBackBufferIndex]->useRaytracing = false;
	m_ResourceManager->SceneData[currentBackBufferIndex]->ViewMatrix = sceneRenderInfo.Camera.View;
	m_ResourceManager->SceneData[currentBackBufferIndex]->CameraPosition = glm::vec3(m_ResourceManager->SceneData[currentBackBufferIndex]->ViewMatrix * glm::vec4(0.f, 0.f, 0.f, 1.f));
	m_ResourceManager->SceneData[currentBackBufferIndex]->ViewOrientation = glm::mat4(glm::mat3(m_ResourceManager->SceneData[currentBackBufferIndex]->ViewMatrix));
	m_ResourceManager->SceneData[currentBackBufferIndex]->InvViewOrientation = glm::inverse(m_ResourceManager->SceneData[currentBackBufferIndex]->ViewOrientation);
	m_ResourceManager->SceneData[currentBackBufferIndex]->ProjectionMatrix = sceneRenderInfo.Camera.Projection;
	m_ResourceManager->SceneData[currentBackBufferIndex]->InvViewMatrix = glm::inverse(m_ResourceManager->SceneData[currentBackBufferIndex]->ViewMatrix);
	m_ResourceManager->SceneData[currentBackBufferIndex]->invProjectionMatrix = glm::inverse(m_ResourceManager->SceneData[currentBackBufferIndex]->ProjectionMatrix);

	if (sceneRenderInfo.DirectionalLights.size() > 0)
	{
		glm::mat4 LighDirectionMatrix = glm::mat3(sceneRenderInfo.DirectionalLights[0].Transform);
		LighDirectionMatrix = glm::translate(glm::mat4(1.), m_ResourceManager->SceneData[currentBackBufferIndex]->CameraPosition - m_ResourceManager->SceneData[currentBackBufferIndex]->LightDirection * 50.f) * LighDirectionMatrix;
		m_ResourceManager->SceneData[currentBackBufferIndex]->ShadowMapProjection = glm::ortho(-30.f, 30.f, -30.f, 30.f, -100.f, 100.f) * glm::inverse(LighDirectionMatrix);
		m_ResourceManager->SceneData[currentBackBufferIndex]->ShadowMapProjectionInv = glm::inverse(m_ResourceManager->SceneData[currentBackBufferIndex]->ShadowMapProjection);
		m_ResourceManager->SceneData[currentBackBufferIndex]->LightDirection = glm::normalize(glm::vec3(LighDirectionMatrix * glm::vec4(0., 0., -1., 0.)));
	}


	// Create a command list for this frame
	Gecko::Ref<Gecko::CommandList> commandList = device->CreateGraphicsCommandList();

	// Render the render passes
	for (const Ref<RenderPass>& renderPass : m_RenderPassStack)
	{
		renderPass->Render(sceneRenderInfo, m_ResourceManager, commandList);
	}

	// Render to the back buffer
	// TODO: move this to the present function
	RenderTarget inputTarget = m_ResourceManager->GetRenderTarget(m_ResourceManager->GetRenderTargetHandle("ToneMappingGammaCorrection"));
	RenderTarget renderTarget = device->GetCurrentBackBuffer();
	
	GraphicsPipeline FullScreenTexturePipeline = m_ResourceManager->GetGraphicsPipeline(FullScreenTexturePipelineHandle);
	commandList->BindGraphicsPipeline(FullScreenTexturePipeline);
	Mesh quadMesh = m_ResourceManager->GetMesh(quadMeshHandle);
	commandList->BindVertexBuffer(quadMesh.VertexBuffer);
	commandList->BindIndexBuffer(quadMesh.IndexBuffer);
	commandList->BindTexture(0, inputTarget, Gecko::RenderTargetType::Target0);
	commandList->BindRenderTarget(renderTarget);

	commandList->Draw(3);

	// TODO: move this to its own function
	// Render ImGui
	commandList->BindRenderTarget(renderTarget);
	device->ImGuiRender(commandList);

	// Execute command list and display it
	device->ExecuteGraphicsCommandListAndFlip(commandList);
}

void Renderer::Present() 
{

}

}