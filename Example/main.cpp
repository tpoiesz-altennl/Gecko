#include "Defines.h"

#include "Rendering/Frontend/ApplicationContext.h"

#include "Rendering/Frontend/Scene/GLTFSceneLoader.h"

#include "Rendering/Frontend/Renderer/RenderPasses/ShadowPass.h"
#include "Rendering/Frontend/Renderer/RenderPasses/ShadowRaytracePass.h"
#include "Rendering/Frontend/Renderer/RenderPasses/GeometryPass.h"
#include "Rendering/Frontend/Renderer/RenderPasses/DeferredPBRPass.h"
#include "Rendering/Frontend/Renderer/RenderPasses/FXAAPass.h"
#include "Rendering/Frontend/Renderer/RenderPasses/BloomPass.h"
#include "Rendering/Frontend/Renderer/RenderPasses/ToneMappingGammaCorrectionPass.h"
#include "UI/DebugUIRenderer.h"

#include "Core/Input.h"

#include "CustomPass.h"

int main()
{

	Gecko::Platform::AppInfo info;
	info.Width = 900;
	info.Height = 600;

	info.FullScreenWidth = 1920;
	info.FullScreenHeight = 1080;
	info.X = 200;
	info.Y = 200;
	info.NumBackBuffers = 2;
	info.Name = "Gecko App";
	info.WorkingDir = WORKING_DIR_PATH;

	Gecko::Event::Init();
	Gecko::Platform::Init(info);
	Gecko::Input::Init();
	Gecko::Logger::Init();


	// Create the context
	Gecko::ApplicationContext ctx;
	ctx.Init(info);

	Gecko::Renderer* renderer = ctx.GetRenderer();
	Gecko::ResourceManager* resourceManager = ctx.GetResourceManager();
	Gecko::SceneManager* sceneManager = ctx.GetSceneManager();

	Gecko::Scene* scene = sceneManager->CreateScene("Main Scene");

	Gecko::Ref<CustomPass> customPass = renderer->CreateRenderPass<CustomPass>();
	// Configure renderpasses
	renderer->ConfigureRenderPasses({
		customPass
		});

	
	// Create the render passess
	Gecko::Ref<Gecko::ShadowPass> shadowPass = renderer->CreateRenderPass<Gecko::ShadowPass>();
	Gecko::Ref<Gecko::GeometryPass> geometryPass = renderer->CreateRenderPass<Gecko::GeometryPass>();
	Gecko::Ref<Gecko::DeferredPBRPass> deferredPBRPass = renderer->CreateRenderPass<Gecko::DeferredPBRPass>();
	Gecko::Ref<Gecko::FXAAPass> FXAAPass = renderer->CreateRenderPass<Gecko::FXAAPass>();
	Gecko::Ref<Gecko::BloomPass> bloomPass = renderer->CreateRenderPass<Gecko::BloomPass>();
	Gecko::Ref<Gecko::ToneMappingGammaCorrectionPass> toneMappingGammaCorrectionPass = renderer->CreateRenderPass<Gecko::ToneMappingGammaCorrectionPass>();
	// Configure renderpasses
	renderer->ConfigureRenderPasses({
		shadowPass,
		geometryPass,
		deferredPBRPass,
		FXAAPass,
		bloomPass,
		toneMappingGammaCorrectionPass
		});

	// Add an environment map
	scene->SetEnvironmentMapHandle(resourceManager->CreateEnvironmentMap("Assets/scythian_tombs_2_4k.hdr"));

	// Load the Sponza gltf scene
	Gecko::Scene* sponzaScene = Gecko::GLTFSceneLoader::LoadScene("Assets/sponza/glb/Sponza.glb", ctx);
	Gecko::SceneNode* sponzaNode = scene->GetRootNode()->AddNode("Sponza node");
	sponzaNode->AppendScene(sponzaScene);
	sponzaNode->Transform.Rotation.y = 90.f;

	// Load Helmet gltf Scene
	Gecko::Scene* helmetScene = Gecko::GLTFSceneLoader::LoadScene("Assets/gltfHelmet/glTF-Binary/DamagedHelmet.glb", ctx);
	Gecko::SceneNode* helmetRootNode = scene->GetRootNode()->AddNode("Helmet node");
	helmetRootNode->AppendScene(helmetScene);
	helmetRootNode->Transform.Position.y = 3.f;

	// Create a camera in the scene
	Gecko::SceneNode* cameraNode = scene->GetRootNode()->AddNode("Camera node");
	Gecko::SceneCamera* camera = scene->CreateCamera();
	camera->SetIsMain(true);
	camera->SetAutoAspectRatio(true);
	cameraNode->AttachCamera(camera);
	cameraNode->Transform.Position.z = 4.f;
	cameraNode->Transform.Position.y = 2.f;

	// Create directional light
	Gecko::SceneDirectionalLight* directionalLight = scene->CreateDirectionalLight();
	Gecko::SceneNode* lightNode = scene->GetRootNode()->AddNode("Light node");
	directionalLight->SetColor({ 1., 1., 1. });
	directionalLight->SetIntenstiy(1.f);
	lightNode->AppendLight(directionalLight);
	lightNode->Transform.Rotation.x = -90.f;

	Gecko::f32 lastTime = Gecko::Platform::GetTime();
	

	while (Gecko::Platform::IsRunning()) {
		Gecko::Platform::PumpMessage();

		Gecko::f32 currentTime = Gecko::Platform::GetTime();
		Gecko::f32 deltaTime = (currentTime - lastTime);
		lastTime = currentTime;

		helmetRootNode->Transform.Rotation.y += .73f * deltaTime * 50.f;
		helmetRootNode->Transform.Rotation.x += 1.6f * deltaTime * 50.f;
		helmetRootNode->Transform.Rotation.z += 1.0f * deltaTime * 50.f;
	
		glm::vec3 rot{ 0. };
		
		if (Gecko::Input::IsKeyDown(Gecko::Input::Key::UP))		rot.x += 1.f;
		if (Gecko::Input::IsKeyDown(Gecko::Input::Key::DOWN))	rot.x -= 1.f;
		if (Gecko::Input::IsKeyDown(Gecko::Input::Key::LEFT))	rot.y += 1.f;
		if (Gecko::Input::IsKeyDown(Gecko::Input::Key::RIGHT))	rot.y -= 1.f;

		cameraNode->Transform.Rotation += rot * deltaTime * 40.f;
		
		glm::vec3 pos{ 0. };

		if (Gecko::Input::IsKeyDown(Gecko::Input::Key::W))		pos.z -= 1.f;
		if (Gecko::Input::IsKeyDown(Gecko::Input::Key::S))		pos.z += 1.f;
		if (Gecko::Input::IsKeyDown(Gecko::Input::Key::A))		pos.x -= 1.f;
		if (Gecko::Input::IsKeyDown(Gecko::Input::Key::D))		pos.x += 1.f;
		if (Gecko::Input::IsKeyDown(Gecko::Input::Key::LSHIFT)) pos.y -= 1.f;
		if (Gecko::Input::IsKeyDown(Gecko::Input::Key::SPACE))	pos.y += 1.f;
		if (glm::length2(pos) > 0.) pos = glm::normalize(pos);

		glm::vec3 movement = glm::mat3(cameraNode->Transform.GetMat4()) * pos;
		
		cameraNode->Transform.Position += movement * deltaTime;

		// Do the imgui things
		Gecko::DebugUIRenderer::RenderDebugUI(ctx);
		
		renderer->RenderScene(scene->GetSceneRenderInfo());


		Gecko::Input::Update();
	}

	ctx.Shutdown();

	Gecko::Logger::Shutdown();
	Gecko::Input::Shutdown();
	Gecko::Platform::Shutdown();
	Gecko::Event::Shutdown();

	return 0;
}