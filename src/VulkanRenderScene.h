#ifndef _VULKAN_RENDER_SCENE_H_
#define _VULKAN_RENDER_SCENE_H_

#include "Utility.h"
#include "VulkanCamera.h"
#include <string>
#include <unordered_map>
#include "VulkanPipeline.h"
#include "SkyBoxPipeline.h"
#include "IrradianceMapPipeline.h"
#include "PrefilterEnvirPipeline.h"
#include "PbrLightPipeline.h"
#include "GBufferPipeline.h"
#include "TBDRLightPipeline.h"
#include "ShadowDepthPipeline.h"

class VulkanRenderScene
{
public:
	VulkanRenderScene(
		VulkanDevice * device, 
		VkQueue queue , 
		VulkanSwapChain * swapChain , 
		VkImageView depthStencilImage , 
		int renderWidth ,
		int renderHeight ,
		int renderX ,
		int renderY , 
		int screenWdith ,
		int screenHeight  ,
		const RenderGlobalState & renderGlobalState )
	{
		device_ = device;
		queue_ = queue;
		render_width_ = renderWidth;
		render_height_ = renderHeight;
		render_x_ = renderX;
		render_y_ = renderY;
		screen_width_ = screenWdith;
		screen_height_ = screenHeight;
		swapChain_ = swapChain;
		depth_stencil_image_ = depthStencilImage;
		InitResources( renderGlobalState );

	}

	~VulkanRenderScene()
	{
		for (auto obj : objects_) delete obj;
		for (auto mesh : global_mesh_) delete mesh.second;
	}

public:
	void Update( float deltaTime )
	{
		DistributeObjectToPipeline();
		camera_->Update(deltaTime);

		lightCullComputePipeline->UpdateData();
		preDepthPipeline->UpdateData();
		forwardPlusLightPipeline->UpdateData();
		pbrLightPipeline->UpdateData();
		shadowDepthPipeline->UpdateData();
	}

	void MouseEvent(const MouseStateType & mouseState)
	{
		camera_->MouseEvent(mouseState);
	}

	void KeyDownEvent(WPARAM wParam)
	{
		camera_->KeyDownEvent(wParam);
	}

	void KeyUpEvent(WPARAM wParam)
	{
		camera_->KeyUpEvent(wParam);
	}

	void InitResources(const RenderGlobalState & renderGlobalState)
	{
		auto InitCamera = [&]() -> void
		{
			camera_ = new VulkanCamera();
			camera_->setPosition(renderGlobalState.cameraPosition);
			camera_->setViewParameters(renderGlobalState.cameraYaw, renderGlobalState.cameraPitch);
			camera_->setPerspective(60.0f, (float)render_width_ / render_height_, 0.1f, 100.0f);
			camera_->UpdateViewDirection();
		};
		auto LoadDefaultResources = [&]() -> void
		{
			global_mesh_file_string_vec_ = {
					GetAssetPath() + "models/sphere.obj" ,
					GetAssetPath() + "models/chalet.obj" , 
					GetAssetPath() + "models/cube.obj" ,
					GetAssetPath() + "models/cerberus/cerberus.fbx"
			};
			float scale = 0.01f;
			std::ifstream texFile;
			texFile.open(GetAssetPath() + "texture2DFile.txt");
			texFile.seekg(0, std::ios::end);
			size_t fileSize = texFile.tellg();
			char * buffer = new char[fileSize];
			texFile.seekg(0, std::ios::beg);
			texFile.read(buffer , fileSize);
			std::string tmp(buffer);

			size_t prePos = 0, latePos = 0;
			std::string texFiles, texNames;
			while (latePos < fileSize && buffer[latePos] != '\0')
			{
				if (buffer[latePos] == ' ')
				{
					texFiles = tmp.substr(prePos, latePos - prePos);
					prePos = latePos + 1;
				}
				if (buffer[latePos] == '\n' || buffer[latePos] == '\0')
				{
					texNames = tmp.substr(prePos, latePos - prePos);
					prePos = latePos + 1;
					latePos = prePos + 1;
					bool ktxTex = texFiles.find(".ktx") != texFiles.npos;
					Texture2D * newTex;
					if (texFiles.find("ao.ktx") != texFiles.npos || texFiles.find("metallic.ktx") != texFiles.npos || texFiles.find("roughness.ktx") != texFiles.npos)
					{
						newTex = new Texture2D(texFiles, VK_FORMAT_R8_UNORM , device_, queue_, 1, false, ktxTex);
					}
					else {
						newTex = new Texture2D(texFiles, VK_FORMAT_R8G8B8A8_UNORM, device_, queue_, 4, false, ktxTex);
					}
					texture_.insert(std::pair<std::string , Texture2D*>( texNames , newTex ) );
				}
				latePos++;
			}

			texFile.close();
			delete buffer;
		};
		auto InitForwardPlusPipeline = [&]() -> void
		{
			const int tileSize = 16;
			size_t tileCountX = render_width_ / tileSize;
			size_t tileCountY = render_height_ / tileSize;
			size_t lightCount = 1000;
			glm::vec3 lightMin = glm::vec3(-15.0f, -5.0f, -5.0f);
			glm::vec3 lightMax = glm::vec3(15.0f, 20.0f, 5.0f);

			preDepthPipeline = new PreDepthRenderingPipeline(render_width_, render_height_, device_ );
			lightCullComputePipeline = new CullLightComputePipeline(tileSize, tileSize, tileCountX, tileCountY, lightCount, lightMin, lightMax, 3.0f, preDepthPipeline->GetDepthImage(), device_);
			forwardPlusLightPipeline = new ForwardPlusLightPassPipeline(device_, swapChain_, lightCullComputePipeline->GetTileLightVisibleBuffer(), lightCullComputePipeline->GetLightUniformBuffer(), screen_width_, screen_height_, tileCountX, tileCountY, camera_->position, depth_stencil_image_ , camera_);
			forwardPlusNewCommandBufferVec.resize(2);
			device_->CreateCommandBuffer(forwardPlusNewCommandBufferVec.size(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, forwardPlusNewCommandBufferVec.data());
		};
		auto InitSponzaScene = [&]() -> void
		{
			std::string file = "sponza.obj";
			std::string folder = GetAssetPath() + "models/sponza_full/";
			sceneObjects = new VulkanSceneObjectsGroup(file, folder, device_, queue_);
			std::vector<VulkanObject*> objs = sceneObjects->GetObjectsVecFromMaterial(forwardPlusLightPipeline, dynamic_cast<Texture2D*>((*texture_.find("dummy")).second), device_);
			for (auto obj : objs)
			{
				obj->SetPipelineType(renderGlobalState.sponzaPipelineType);
				glm::vec3 scaleV(0.01f, 0.01f, 0.01f);
				obj->SetScale(scaleV);
				objects_.push_back(obj);
			}
		};
		auto InitSkyBoxPipeline = [&]() -> void 
		{
			TextureCube * skyBoxCube = new TextureCube(GetAssetPath() + "textures/skybox/iceflats", ".tga", device_, queue_, VK_FORMAT_R8G8B8A8_UNORM);
			texture_.insert(std::pair<std::string , Texture*>("iceflats" , skyBoxCube));
			skyboxPipeline = new SkyBoxPipeline(skyBoxCube , device_ , swapChain_ , screen_width_, screen_height_ );
			std::string meshFile = global_mesh_file_string_vec_[2];
			std::string layoutName;
			VertexLayout vertLayout = skyboxPipeline->GetVertexLayout(layoutName);
			skyboxPipeline->SetMesh(GetMesh(meshFile, vertLayout, layoutName));
			device_->CreateCommandBuffer(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &skyboxCommandBuffer);
		};
		auto InitIrradiancePipeline = [&]()->void
		{
			device_->CreateCommandBuffer(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &irradianceMapCommandBuffer);
			TextureCube * skyBoxCube = dynamic_cast<TextureCube*>( (*(texture_.find("iceflats"))).second );
			std::string meshFile = global_mesh_file_string_vec_[2];
			std::string layoutName;
			VertexLayout vertLayout = skyboxPipeline->GetVertexLayout(layoutName);
			irradianceMapPipeline = new IrradianceMapPipeline(device_, queue_, GetMesh(global_mesh_file_string_vec_[2], vertLayout, layoutName), skyBoxCube);
			
			VkCommandBufferBeginInfo commandBufferBeginInfo = VulkanInitializer::InitCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			vkBeginCommandBuffer(irradianceMapCommandBuffer, &commandBufferBeginInfo);
			irradianceMapPipeline->SetupCommandBuffer(irradianceMapCommandBuffer, false, false);
			vkEndCommandBuffer(irradianceMapCommandBuffer);
			SubmitPrecomputeCommand(irradianceMapCommandBuffer);
		};
		auto InitPrefilterEnvirPipeline = [&]()->void
		{
			device_->CreateCommandBuffer(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &prefilterEnvirCommandBuffer);
			TextureCube * skyBoxCube = dynamic_cast<TextureCube*>((*(texture_.find("iceflats"))).second);
			std::string meshFile = global_mesh_file_string_vec_[2];
			std::string layoutName;
			VertexLayout vertLayout = skyboxPipeline->GetVertexLayout(layoutName);
			prefilterEnvirPipeline = new PrefilterEnvironmentPipeline(device_, queue_, GetMesh(global_mesh_file_string_vec_[2], vertLayout, layoutName), skyBoxCube);

			VkCommandBufferBeginInfo commandBufferBeginInfo = VulkanInitializer::InitCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			vkBeginCommandBuffer(prefilterEnvirCommandBuffer, &commandBufferBeginInfo);
			prefilterEnvirPipeline->SetupCommandBuffer(prefilterEnvirCommandBuffer, false, false);
			vkEndCommandBuffer(prefilterEnvirCommandBuffer);
			SubmitPrecomputeCommand(prefilterEnvirCommandBuffer);

		};
		auto InitPBRLightPipeline = [&]()->void
		{
			shadowDepthPipeline = new ShadowDepthPipeline(device_, camera_, glm::vec3(1.0f, 1.0f, 1.0f));
			device_->CreateCommandBuffer(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &shadowDepthCommandBuffer);
			device_->CreateCommandBuffer(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &pbrLightCommandBuffer);
			pbrLightPipeline = new PBRLightPipeline(device_, swapChain_, camera_, screen_width_, screen_height_, depth_stencil_image_,shadowDepthPipeline->GetCascadeTransform(), shadowDepthPipeline->GetShadowMapImage());
			std::string meshFile = global_mesh_file_string_vec_[0];
			std::string layoutName;
			VertexLayout vertLayout = pbrLightPipeline->GetVertexLayout(layoutName);
			pbrLightPipeline->SetMesh(GetMesh(meshFile, vertLayout, layoutName));
		};
		auto InitTBDRPipeline = [&]()->void
		{
			device_->CreateCommandBuffer(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &gbufferCommandBuffer);
			device_->CreateCommandBuffer(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &tbdrlightCommandBuffer);
			gbufferPipeline = new GBufferPipeline( render_width_ , render_height_ , device_  );

			tbdrPipeline = new TBDRLightPipeline(
				lightCullComputePipeline->GetTileLightVisibleBuffer(),
				lightCullComputePipeline->GetLightUniformBuffer(),
				depth_stencil_image_,
				gbufferPipeline->GetAlbedoImage(),
				gbufferPipeline->GetPositionImage(),
				gbufferPipeline->GetNormalImage(),
				gbufferPipeline->GetPBRImage(),
				irradianceMapPipeline->GetIrradianceMapImage(),
				prefilterEnvirPipeline->GetPrefilterEnvirMapImage(),
				dynamic_cast<Texture2D*>((*(texture_.find("LUT"))).second),
				device_,
				swapChain_,
				screen_width_,
				screen_height_
				);
		};

		InitCamera();
		LoadDefaultResources();
		InitSkyBoxPipeline();
		InitForwardPlusPipeline();
		InitIrradiancePipeline();
		InitPrefilterEnvirPipeline();
		InitPBRLightPipeline();
		InitTBDRPipeline();
		if (renderGlobalState.usingSponzaScene)
		{
			InitSponzaScene();
		}
		IMaterial *forwardPBRMat = new PbrLightPassMaterial(
			dynamic_cast<Texture2D*>((*texture_.find("treeAlbedo")).second),
			dynamic_cast<Texture2D*>((*texture_.find("treeNormal")).second),
			dynamic_cast<Texture2D*>((*texture_.find("treeAo")).second),
			dynamic_cast<Texture2D*>((*texture_.find("treeMetallic")).second),
			dynamic_cast<Texture2D*>((*texture_.find("treeRoughness")).second),
			irradianceMapPipeline->GetIrradianceMapImage(),
			dynamic_cast<Texture2D*>((*texture_.find("LUT")).second),
			prefilterEnvirPipeline->GetPrefilterEnvirMapImage(),
			device_,
			pbrLightPipeline,
			texture_
		);
		IMaterial *forwardPBRMat2 = new PbrLightPassMaterial(
			dynamic_cast<Texture2D*>((*texture_.find("treeAlbedo")).second),
			dynamic_cast<Texture2D*>((*texture_.find("treeNormal")).second),
			dynamic_cast<Texture2D*>((*texture_.find("treeAo")).second),
			dynamic_cast<Texture2D*>((*texture_.find("treeMetallic")).second),
			dynamic_cast<Texture2D*>((*texture_.find("treeRoughness")).second),
			irradianceMapPipeline->GetIrradianceMapImage(),
			dynamic_cast<Texture2D*>((*texture_.find("LUT")).second),
			prefilterEnvirPipeline->GetPrefilterEnvirMapImage(),
			device_,
			pbrLightPipeline,
			texture_
		);
		AddObject({ 0.0f , 0.0f , 0.0f }, { 0.0f , 0.0f ,0.0f }, { 0.1f , 0.1f , 0.1f }, 0, PIPELINE_FORWARD_PBR, forwardPBRMat);
		AddObject({ -0.98f , -2.83f , -0.18f }, { 0.1f , 0.0f ,1.2f }, { 0.1f , 0.66f , 0.84f }, 2, PIPELINE_FORWARD_PBR, forwardPBRMat2);

		DistributeObjectToPipeline();
	}

	void SetupCommandBuffers( std::vector<VkCommandBuffer> & commandBuffer , int imageIndex  )
	{
		SetupSkyboxPass(commandBuffer, imageIndex);
		if( forward_plus_objects_.size() != 0 ) 	SetupForwardPlusPass( commandBuffer, imageIndex );
		if( forward_pbr_light_objects_.size() != 0 ) SetupForwardPBRLightPass( commandBuffer , imageIndex );
		if (tbdr_objects_.size() != 0) SetupTBDRPass(commandBuffer, imageIndex);
	}

	void SetupForwardPlusPass(std::vector<VkCommandBuffer> & commandBuffer , int imageIndex )
	{
		// preDepthPass
		VkCommandBuffer newCommandBuffer = forwardPlusNewCommandBufferVec[0];

		VkCommandBufferBeginInfo commandBufferBeginInfo = VulkanInitializer::InitCommandBufferBeginInfo(0);
		vkBeginCommandBuffer(newCommandBuffer, &commandBufferBeginInfo);
		VkViewport viewport = VulkanInitializer::InitViewport( 0, 0 , render_width_, render_height_, 0.0f, 1.0f);
		VkRect2D scissor = { 0 , 0 , screen_width_ , screen_height_ }; 
		vkCmdSetViewport(newCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(newCommandBuffer, 0, 1, &scissor);
		glm::mat4 projView = camera_->matrices.perspective * camera_->matrices.view;
		for ( int i = 0 ; i < forward_plus_objects_.size() ; i ++ )
		{
			VulkanObject * obj = forward_plus_objects_[i];
			SetObjectMeshToPipeline(preDepthPipeline, obj);
			glm::mat4 model =  obj->GetWorldMatrix();
			glm::mat4 mvp = projView * model;
			preDepthPipeline->SetMVP(mvp);
			if( i == 0 ) preDepthPipeline->SetupCommandBuffer(newCommandBuffer , true , false );
			else if( i == forward_plus_objects_.size() - 1 ) preDepthPipeline->SetupCommandBuffer(newCommandBuffer, false, true);
			else preDepthPipeline->SetupCommandBuffer(newCommandBuffer, false, false);
		}
		
		VkImageMemoryBarrier imageBarrier = VulkanInitializer::InitImageMemoryBarrier(
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 
			VK_ACCESS_SHADER_READ_BIT, 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT , 0, 1, 0, 1, 
			preDepthPipeline->GetDepthImage()->image_
		);
		vkCmdPipelineBarrier(newCommandBuffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0 , NULL  , 1 , &imageBarrier);
		vkEndCommandBuffer(newCommandBuffer);
		commandBuffer.push_back(newCommandBuffer);

		// lightCullPass
		lightCullComputePipeline->SetPushConstantData(render_width_, render_height_, camera_->getNearClip() , camera_->getFarClip() , camera_->matrices.perspective, camera_->matrices.view);
		commandBuffer.push_back(lightCullComputePipeline->SetupCommandBuffer());

		// lightPass
		forwardPlusLightPipeline->SetFramebufferIndex(imageIndex);

		VkCommandBuffer lightPassCommandBuffer = forwardPlusNewCommandBufferVec[1];
		VkCommandBufferBeginInfo ncommandBufferBeginInfo = VulkanInitializer::InitCommandBufferBeginInfo(0);
		vkBeginCommandBuffer(lightPassCommandBuffer, &ncommandBufferBeginInfo);

		VkViewport nviewport = VulkanInitializer::InitViewport( render_x_ , render_y_ , render_width_ , render_height_ , 0.0f , 1.0f );
		VkRect2D nscissor = { 0 , 0 , screen_width_ , screen_height_ };
		vkCmdSetViewport(lightPassCommandBuffer, 0, 1, &nviewport);
		vkCmdSetScissor(lightPassCommandBuffer, 0, 1, &nscissor);

		for (int i = 0; i < forward_plus_objects_.size(); i++)
		{
			VulkanObject * obj = forward_plus_objects_[i];
			SetObjectMeshToPipeline(forwardPlusLightPipeline, obj);
			obj->UpdatePipeline();
			obj->SetupCommandBuffer(lightPassCommandBuffer);
			glm::mat4 model = obj->GetWorldMatrix();
			glm::mat4 mvp = projView * model;
			forwardPlusLightPipeline->SetPushConstantValue(obj->GetWorldMatrix(), mvp , render_x_ , render_y_ );
			if (i == 0) forwardPlusLightPipeline->SetupCommandBuffer(lightPassCommandBuffer, true, false);
			else if (i == forward_plus_objects_.size() - 1) forwardPlusLightPipeline->SetupCommandBuffer(lightPassCommandBuffer, false, true);
			else forwardPlusLightPipeline->SetupCommandBuffer(lightPassCommandBuffer, false, false);
		}

		vkEndCommandBuffer(lightPassCommandBuffer);
		commandBuffer.push_back(lightPassCommandBuffer);

	}

	void SetupSkyboxPass(std::vector<VkCommandBuffer> & commandBuffer, int imageIndex)
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo = VulkanInitializer::InitCommandBufferBeginInfo(0);
		vkBeginCommandBuffer(skyboxCommandBuffer, &commandBufferBeginInfo);
		VkViewport viewport = VulkanInitializer::InitViewport(render_x_, render_y_, render_width_, render_height_, 0.0f, 1.0f);
		VkRect2D scissor = { 0 , 0 , screen_width_ , screen_height_ };
		vkCmdSetViewport(skyboxCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(skyboxCommandBuffer, 0, 1, &scissor);
		glm::mat4 mvp = camera_->matrices.perspective * camera_->matrices.viewRotation ;
		skyboxPipeline->SetPushConstantData(mvp);
		skyboxPipeline->SetFrameBufferIndex(imageIndex);
		skyboxPipeline->SetupCommandBuffer(skyboxCommandBuffer, true, true);
		vkEndCommandBuffer(skyboxCommandBuffer);
		commandBuffer.push_back(skyboxCommandBuffer);
	}

	void SetupForwardPBRLightPass(std::vector<VkCommandBuffer> & commandBuffer , int imageIndex )
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo = VulkanInitializer::InitCommandBufferBeginInfo(0);
		vkBeginCommandBuffer(shadowDepthCommandBuffer, &commandBufferBeginInfo);
		VkViewport viewport = VulkanInitializer::InitViewport(0, 0, 4096, 4096, 0.0f, 1.0f);
		VkRect2D scissor = { 0 , 0 , 4096 , 4096 };
		vkCmdSetViewport(shadowDepthCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(shadowDepthCommandBuffer, 0, 1, &scissor);

		for (int j = 0; j < 4; j++)
		{
			for (int i = 0; i < forward_pbr_light_objects_.size(); i++)
			{
				VulkanObject * obj = forward_pbr_light_objects_[i];
				SetObjectMeshToPipeline(shadowDepthPipeline, obj);			
				glm::mat4 model = obj->GetWorldMatrix();
				shadowDepthPipeline->SetPushConstantData(model, j);
				shadowDepthPipeline->SetupCommandBuffer(shadowDepthCommandBuffer, true, true);
			}
		}
		vkEndCommandBuffer(shadowDepthCommandBuffer);
		commandBuffer.push_back(shadowDepthCommandBuffer);

		vkBeginCommandBuffer(pbrLightCommandBuffer, &commandBufferBeginInfo);
		viewport = VulkanInitializer::InitViewport(render_x_, render_y_, render_width_, render_height_, 0.0f, 1.0f);
		scissor = { 0 , 0 , (uint32_t)screen_width_ , (uint32_t)screen_height_ };
		vkCmdSetViewport(pbrLightCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(pbrLightCommandBuffer, 0, 1, &scissor);
		pbrLightPipeline->SetFrameBufferIndex(imageIndex);
		for (int i = 0; i < forward_pbr_light_objects_.size(); i++)
		{
			VulkanObject * obj = forward_pbr_light_objects_[i];
			SetObjectMeshToPipeline(pbrLightPipeline, obj);
			obj->UpdatePipeline();
			obj->SetupCommandBuffer(pbrLightCommandBuffer);
			glm::mat4 model = obj->GetWorldMatrix();
			pbrLightPipeline->SetPushConstantData(model, camera_->matrices.view, camera_->matrices.perspective, camera_->position , glm::vec3(1.0f , 1.0f ,1.0f));
			if (i == 0) pbrLightPipeline->SetupCommandBuffer(pbrLightCommandBuffer, true, forward_pbr_light_objects_.size() == 1 ? true : false);
			else if (i == forward_plus_objects_.size() - 1) pbrLightPipeline->SetupCommandBuffer(pbrLightCommandBuffer, false, true);
			else pbrLightPipeline->SetupCommandBuffer(pbrLightCommandBuffer, false, false);
		}
		vkEndCommandBuffer(pbrLightCommandBuffer);
		commandBuffer.push_back(pbrLightCommandBuffer);

	}

	void SetupTBDRPass(std::vector<VkCommandBuffer> & commandBuffer, int imageIndex)
	{
		// GBuffer Pass 
		VkCommandBufferBeginInfo commandBufferBeginInfo = VulkanInitializer::InitCommandBufferBeginInfo(0);
		vkBeginCommandBuffer(gbufferCommandBuffer, &commandBufferBeginInfo);
		VkViewport viewport = VulkanInitializer::InitViewport(0, 0, render_width_, render_height_, 0.0f, 1.0f);
		VkRect2D scissor = { 0 , 0 , screen_width_ , screen_height_ };
		vkCmdSetViewport(gbufferCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(gbufferCommandBuffer, 0, 1, &scissor);
		for (int i = 0; i < tbdr_objects_.size(); i++)
		{
			VulkanObject * obj = tbdr_objects_[i];
			SetObjectMeshToPipeline(gbufferPipeline, obj);
			obj->UpdatePipeline();
			obj->SetupCommandBuffer(gbufferCommandBuffer);
			glm::mat4 model = obj->GetWorldMatrix();
			gbufferPipeline->SetPushConstantData(model ,camera_->matrices.perspective * camera_->matrices.view * model );

			if (i == 0) gbufferPipeline->SetupCommandBuffer(gbufferCommandBuffer, true, tbdr_objects_.size() == 1 ? true : false);
			else if (i == tbdr_objects_.size() - 1) gbufferPipeline->SetupCommandBuffer(gbufferCommandBuffer, false, true);
			else gbufferPipeline->SetupCommandBuffer(gbufferCommandBuffer, false, false);
		}

		VkImageMemoryBarrier imageBarrier = VulkanInitializer::InitImageMemoryBarrier(
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1,
			gbufferPipeline->GetPredepthImage()->image_
		);
		vkCmdPipelineBarrier(gbufferCommandBuffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageBarrier);
		vkEndCommandBuffer(gbufferCommandBuffer);
		commandBuffer.push_back(gbufferCommandBuffer);

		// lightCullPass
		lightCullComputePipeline->SetPreDepth(gbufferPipeline->GetDepthImage());
		lightCullComputePipeline->SetPushConstantData(render_width_, render_height_, camera_->getNearClip(), camera_->getFarClip(), camera_->matrices.perspective, camera_->matrices.view);
		commandBuffer.push_back(lightCullComputePipeline->SetupCommandBuffer());

		// lightPass 
		vkBeginCommandBuffer(tbdrlightCommandBuffer, &commandBufferBeginInfo);
		vkCmdPipelineBarrier(tbdrlightCommandBuffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageBarrier);
		viewport = VulkanInitializer::InitViewport(render_x_, render_y_, render_width_, render_height_, 0.0f, 1.0f);
		scissor = { 0 , 0 , (uint32_t)screen_width_ , (uint32_t)screen_height_ };
		vkCmdSetViewport(tbdrlightCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(tbdrlightCommandBuffer, 0, 1, &scissor);
		tbdrPipeline->SetFrameIndex(imageIndex);
		tbdrPipeline->SetPushConstantData(glm::mat4(1.0f), camera_->matrices.view, camera_->matrices.perspective, camera_->position, render_x_, render_y_, render_width_ / 16, render_height_ / 16);
		tbdrPipeline->SetupCommandBuffer(tbdrlightCommandBuffer, true , true );
		vkEndCommandBuffer(tbdrlightCommandBuffer);
		commandBuffer.push_back(tbdrlightCommandBuffer);
	}

public:
	void AddEmptyObject()
	{
		int ind = objects_.size();
		std::string name = std::to_string(ind) + " object ";
		VulkanObject * obj = new VulkanObject( ind , name );
		IMaterial * emptyMaterial = new EmptyMaterial;
		obj->ResetMaterial(emptyMaterial, PIPELINE_EMPTY);
		objects_.push_back(obj);
	}

	void AddObject( glm::vec3 position , glm::vec3 rotation , glm::vec3 scale , int meshIndex , PipelineType pipelineType , IMaterial * material )
	{
		int ind = objects_.size();
		std::string name = std::to_string(ind) + " object ";
		VulkanObject * obj = new VulkanObject(ind, name);
		obj->SetPosition(position);
		obj->SetRotation(rotation);
		obj->SetScale(scale);
		obj->SetMeshIndex(meshIndex);
		obj->SetPipelineType(pipelineType);
		obj->ResetMaterial(material, pipelineType);
		objects_.push_back(obj);
		DistributeObjectToPipeline();
	}

	VulkanMesh* GetMesh(std::string & meshFileName , VertexLayout vertLayout, std::string & layoutName )
	{
		size_t position = meshFileName.find('/');
		std::string meshName = meshFileName.substr(position + 1, meshFileName.length() - position - 1) + layoutName ;
		auto resIter = global_mesh_.find(meshName);
		if (resIter != global_mesh_.end())
		{
			return (*resIter).second;
		}
		return AddMesh(meshFileName, vertLayout, meshName);
	}

	VulkanMesh* AddMesh(std::string & meshFileName , VertexLayout vertLayout, std::string & name  )
	{
		VulkanMesh * mesh = new VulkanMesh(meshFileName, vertLayout, 1.0f, device_, queue_ , name );
		global_mesh_.insert( std::pair<std::string , VulkanMesh*>(name, mesh));
		return mesh;
	}

	void SetObjectMeshToPipeline(IRenderingPipeline * pipeline , VulkanObject * obj  )
	{
		VulkanMesh * staticMesh; 
		if (obj->GetStaticMesh(staticMesh))
		{
			pipeline->SetMesh(staticMesh);
			return;
		}
		std::string layoutName;
		VertexLayout layout = pipeline->GetVertexLayout(layoutName);
		int meshInd = obj->GetMeshIndex();
		if (meshInd == -1) return;
		pipeline->SetMesh(GetMesh(global_mesh_file_string_vec_[meshInd], layout, layoutName));
	}

	void DistributeObjectToPipeline()
	{
		forward_plus_objects_.clear();
		forward_pbr_light_objects_.clear();
		tbdr_objects_.clear();
		for (auto obj : objects_)
		{
			if (obj->GetPipelineType() == PIPELINE_EMPTY)
			{
				if (obj->GetPrevPipelineType() != PIPELINE_EMPTY)
				{
					IMaterial * emptyMaterial = new EmptyMaterial();
					obj->ResetMaterial(emptyMaterial, PIPELINE_EMPTY);
				}
			}
			if (obj->GetPipelineType() == PIPELINE_FORWARD_PLUS)
			{
				forward_plus_objects_.push_back(obj);
				if (obj->GetPrevPipelineType() != PIPELINE_FORWARD_PLUS)
				{
					IMaterial * newMaterial = new ForwardLightPassMaterial(dynamic_cast<Texture2D*>((*texture_.find("dummy")).second), dynamic_cast<Texture2D*>((*texture_.find("dummy")).second), forwardPlusLightPipeline , device_);
					obj->ResetMaterial(newMaterial, PIPELINE_FORWARD_PLUS);
				}
			}
			if (obj->GetPipelineType() == PIPELINE_TBDR)
			{
				tbdr_objects_.push_back(obj);
				if (obj->GetPrevPipelineType() != PIPELINE_TBDR)
				{
					Texture2D* dummyTexture = dynamic_cast<Texture2D*>((*texture_.find("dummy")).second);
					IMaterial * tbdrMaterial = new TBDRMaterial(dummyTexture, dummyTexture, dummyTexture, dummyTexture, dummyTexture, device_ , gbufferPipeline);
				}
			}

			if (obj->GetPipelineType() == PIPELINE_FORWARD_PBR)
			{
				forward_pbr_light_objects_.push_back(obj);
				Texture2D* dummyTex = dynamic_cast<Texture2D*>((*texture_.find("dummy")).second);
				Texture2D* lutTex = dynamic_cast<Texture2D*>((*texture_.find("LUT")).second);
				if (obj->GetPrevPipelineType() != PIPELINE_FORWARD_PBR)
				{
					IMaterial * newMaterial = new PbrLightPassMaterial(
						dummyTex,
						dummyTex,
						dummyTex,
						dummyTex,
						dummyTex,
						irradianceMapPipeline->GetIrradianceMapImage(),
						lutTex,
						prefilterEnvirPipeline->GetPrefilterEnvirMapImage(),
						device_,
						pbrLightPipeline , texture_);
					obj->ResetMaterial(newMaterial, PIPELINE_FORWARD_PBR);
				}
			}
		}
	}

private:
	void SubmitPrecomputeCommand(VkCommandBuffer commandBuffer )
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue_);
	}

private:
	VulkanCamera * camera_;
	std::vector<VulkanObject*> objects_;
	std::unordered_map<std::string , VulkanMesh*> global_mesh_;
	std::vector<std::string> global_mesh_file_string_vec_;
	std::unordered_map<std::string, Texture*> texture_;
	VulkanSceneObjectsGroup * sceneObjects;

private:
	VulkanDevice * device_;
	VkQueue queue_;
	VulkanSwapChain * swapChain_;

private:
	//Forward Plus Pipeline 
	std::vector<VulkanObject*> forward_plus_objects_;
	PreDepthRenderingPipeline * preDepthPipeline;
	CullLightComputePipeline * lightCullComputePipeline;
	ForwardPlusLightPassPipeline* forwardPlusLightPipeline;
	std::vector<VkCommandBuffer> forwardPlusNewCommandBufferVec;

private:
	//SkyBox Pipeline 
	SkyBoxPipeline * skyboxPipeline;
	VkCommandBuffer skyboxCommandBuffer;

	//Irradiance map Pipeline 
	IrradianceMapPipeline * irradianceMapPipeline;
	VkCommandBuffer irradianceMapCommandBuffer;

	//PrefilterEnvironmentMap Pipeline 
	PrefilterEnvironmentPipeline * prefilterEnvirPipeline;
	VkCommandBuffer prefilterEnvirCommandBuffer;

	//PBR Light Pipeline 
	std::vector<VulkanObject*> forward_pbr_light_objects_;
	PBRLightPipeline *pbrLightPipeline;
	ShadowDepthPipeline *shadowDepthPipeline;
	VkCommandBuffer shadowDepthCommandBuffer;
	VkCommandBuffer pbrLightCommandBuffer;

	//TBDR Pipeline
	std::vector<VulkanObject*> tbdr_objects_;
	std::vector<VulkanObject*> tbdr_transparent_objects_;
	GBufferPipeline *gbufferPipeline;
	TBDRLightPipeline * tbdrPipeline;
	VkCommandBuffer gbufferCommandBuffer;
	VkCommandBuffer tbdrlightCommandBuffer;
private:
	int render_width_;
	int render_height_;
	int render_x_;
	int render_y_;
	int screen_width_;
	int screen_height_;

	VkImageView depth_stencil_image_ ;

	friend class VulkanBase;
};

#endif