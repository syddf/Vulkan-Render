#ifndef _VULKAN_INITIALIZER_H_
#define _VULKAN_INITIALIZER_H_

#include <vulkan/vulkan.h>
#include "VulkanDevice.hpp"
class VulkanInitializer
{
public:
	static VkCommandBufferBeginInfo InitCommandBufferBeginInfo(int flag , VkCommandBufferInheritanceInfo * inheritInfo )
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo = {};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags = flag;
		commandBufferBeginInfo.pInheritanceInfo = inheritInfo;
		return commandBufferBeginInfo;
	}

	static VkRenderPassBeginInfo InitRenderPassBeginInfo( 
		VkRenderPass renderPass , 
		VkFramebuffer frameBuffer , 
		std::vector<VkClearValue> & clearValues , 
		int width , 
		int height )
	{
		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.framebuffer = frameBuffer;
		renderPassBeginInfo.pClearValues = clearValues.data();
		renderPassBeginInfo.clearValueCount = clearValues.size();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		return renderPassBeginInfo;
	}

	static VkViewport InitViewport( int x , int y , int width , int height , float minDepth , float maxDepth )
	{
		VkViewport view;
		view.width = width;
		view.height = height;
		view.minDepth = minDepth;
		view.maxDepth = maxDepth;
		view.x = x;
		view.y = y;
		return view;
	}

	static VkPipelineLayoutCreateInfo InitPipelineLayoutCreateInfo( 
		int pushConstantCount , 
		VkPushConstantRange * pushConstantRange , 
		int descriptorSetLayoutCount , 
		VkDescriptorSetLayout * setLayout )
	{
		VkPipelineLayoutCreateInfo layoutCreateInfo = {} ;
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutCreateInfo.pushConstantRangeCount = pushConstantCount;
		layoutCreateInfo.pPushConstantRanges = pushConstantRange;
		layoutCreateInfo.setLayoutCount = descriptorSetLayoutCount;
		layoutCreateInfo.pSetLayouts = setLayout;

		return layoutCreateInfo;
	}

	static VkPipelineInputAssemblyStateCreateInfo GetNormalInputAssembly()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyState.primitiveRestartEnable = VK_FALSE;
		return inputAssemblyState;
	}

	static VkPipelineRasterizationStateCreateInfo InitRasterizationStateCreateInfo( VkCullModeFlags cullMode )
	{
		VkPipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.cullMode = cullMode;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizationState.lineWidth = 1.0f;
		return rasterizationState;
	}

	static VkPipelineColorBlendAttachmentState InitColorBlendAttachmentState( 
		VkBool32 blendEnable , 
		VkColorComponentFlags colorComponent = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	)
	{
		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		if (blendEnable == VK_FALSE)
		{
			blendAttachmentState.blendEnable = VK_FALSE;
			blendAttachmentState.colorWriteMask = 0xf;
			return blendAttachmentState;
		}
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorWriteMask = colorComponent;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		return blendAttachmentState;
	}

	static VkPipelineColorBlendStateCreateInfo InitPipelineColorBlendState( size_t count , VkPipelineColorBlendAttachmentState * blendAttachmentState )
	{
		VkPipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = count;
		colorBlendState.pAttachments = blendAttachmentState;
		return colorBlendState;
	}

	static VkPipelineDepthStencilStateCreateInfo InitDepthStencilState(
		VkBool32 depthWriteEnable ,
		VkBool32 depthTestEnable , 
		VkCompareOp depthCompareOp )
	{
		VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthWriteEnable = depthWriteEnable;
		depthStencilState.depthTestEnable = depthTestEnable;
		depthStencilState.depthCompareOp = depthCompareOp;
		depthStencilState.front = depthStencilState.back;
		depthStencilState.back.compareOp = depthCompareOp;
		return depthStencilState;
	}

	static VkPipelineViewportStateCreateInfo InitViewportState( int scissorCount , int viewportCount )
	{
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.scissorCount = scissorCount ;
		viewportState.viewportCount = viewportCount ;
		return viewportState;
	}

	static VkPipelineDynamicStateCreateInfo GetDefaultDynamicState()
	{
		static VkPipelineDynamicStateCreateInfo dynamicState = {};
		static std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStateEnables.data();

		return dynamicState;
	}

	static VkPipelineMultisampleStateCreateInfo InitMultiSampleState(VkSampleCountFlagBits sampleCountBit )
	{
		VkPipelineMultisampleStateCreateInfo multisampleState = {};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = sampleCountBit ;
		return multisampleState;
	}

	static VkPipelineVertexInputStateCreateInfo InitVertexInputState( 
		int vertexBindingDescriptorCount , 
		VkVertexInputBindingDescription * bindingDesc , 
		int vertexAttributeDescriptorCount ,
		VkVertexInputAttributeDescription * inputAttr
		)
	{
		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.vertexBindingDescriptionCount = vertexBindingDescriptorCount;
		vertexInputState.pVertexBindingDescriptions = bindingDesc ;
		vertexInputState.vertexAttributeDescriptionCount = vertexAttributeDescriptorCount;
		vertexInputState.pVertexAttributeDescriptions = inputAttr ;
		return vertexInputState;
	}

	static VkGraphicsPipelineCreateInfo InitGraphicsPipelineCreateInfo (
		VkPipelineLayout pipelineLayout , 
		VkRenderPass renderPass,
		VkPipelineVertexInputStateCreateInfo * vertexInputState,
		VkPipelineInputAssemblyStateCreateInfo * inputAssemblyState , 
		VkPipelineRasterizationStateCreateInfo * rasterizationState , 
		VkPipelineColorBlendStateCreateInfo * colorBlendState ,
		VkPipelineMultisampleStateCreateInfo * multisampleState ,
		VkPipelineViewportStateCreateInfo * viewportState ,
		VkPipelineDepthStencilStateCreateInfo * depthStencilState ,
		VkPipelineDynamicStateCreateInfo * dynamicState ,
		std::vector<VkPipelineShaderStageCreateInfo> & shaders,
		uint32_t subpass
	)
	{
		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.layout = pipelineLayout;
		pipelineCreateInfo.renderPass = renderPass;
		pipelineCreateInfo.pVertexInputState = vertexInputState;
		pipelineCreateInfo.pInputAssemblyState = inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = rasterizationState;
		pipelineCreateInfo.pColorBlendState = colorBlendState;
		pipelineCreateInfo.pMultisampleState = multisampleState;
		pipelineCreateInfo.pViewportState = viewportState;
		pipelineCreateInfo.pDepthStencilState = depthStencilState;
		pipelineCreateInfo.pDynamicState = dynamicState;
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaders.size());
		pipelineCreateInfo.pStages = shaders.data();
		pipelineCreateInfo.subpass = subpass;

		return pipelineCreateInfo;
	}

	static VkDescriptorSetLayoutCreateInfo InitDescSetLayoutCreateInfo(
		int bindingCount,
		VkDescriptorSetLayoutBinding * bindings
	) {
		VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo = {};
		descSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descSetLayoutCreateInfo.bindingCount = bindingCount;
		descSetLayoutCreateInfo.pBindings = bindings;
		return descSetLayoutCreateInfo;
	}

	static VkImageCreateInfo InitImageCreateInfo(
		VkFormat format , 
		VkImageTiling tiling ,
		int width , 
		int height , 
		int depth ,
		int arrayLayers ,
		int mipLevels , 
		uint32_t *queInd , 
		VkImageLayout imageLayout ,
		VkImageUsageFlags usageFlag ,
		VkImageViewCreateFlags createFlags = 0,
		VkImageType imageType = VK_IMAGE_TYPE_2D,
		VkSampleCountFlagBits sampleCountFlag = VK_SAMPLE_COUNT_1_BIT
	)
	{
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.flags = createFlags;
		imageCreateInfo.format = format;
		imageCreateInfo.tiling = tiling;
		imageCreateInfo.extent.width = width;
		imageCreateInfo.extent.height = height;
		imageCreateInfo.extent.depth = depth;
		imageCreateInfo.arrayLayers = arrayLayers;
		imageCreateInfo.imageType = imageType;
		imageCreateInfo.initialLayout = imageLayout;
		imageCreateInfo.mipLevels = mipLevels;
		imageCreateInfo.queueFamilyIndexCount = 1;
		imageCreateInfo.pQueueFamilyIndices = queInd;
		imageCreateInfo.samples = sampleCountFlag;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.usage = usageFlag;
		return imageCreateInfo;
	}

	static VkImageViewCreateInfo InitImageViewCreateInfo(
		VkFormat format,
		VkImage image,
		VkImageAspectFlags flagBit , 
		uint32_t baseLayer = 0 ,
		uint32_t layerCount = 1 ,
		uint32_t baseLevel = 0, 
		uint32_t levelCount = 1 ,
		VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D
	)
	{
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.format = format;
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.viewType = imageViewType;
		imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		imageViewCreateInfo.subresourceRange.aspectMask = flagBit;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = baseLayer;
		imageViewCreateInfo.subresourceRange.baseMipLevel = baseLevel;
		imageViewCreateInfo.subresourceRange.layerCount = layerCount;
		imageViewCreateInfo.subresourceRange.levelCount = levelCount;

		return imageViewCreateInfo;
	}

	static VkMemoryAllocateInfo InitMemoryAllocateInfo(
		VulkanDevice * device , 
		VkImage image 
	)
	{
		VkMemoryRequirements requirements;
		vkGetImageMemoryRequirements(device->GetDevice(), image, &requirements);

		VkMemoryAllocateInfo memoryAllocInfo = {} ;
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.allocationSize = requirements.size;
		memoryAllocInfo.memoryTypeIndex = device->GetMemoryType(requirements.memoryTypeBits , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		return memoryAllocInfo;
	}
	
	static VkAttachmentDescription InitAttachmentDescription(
		VkFormat format ,
		VkImageLayout initialImageLayout ,
		VkImageLayout finalImageLayout ,
		VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE ,
		VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VkSampleCountFlagBits sampleCountFlag = VK_SAMPLE_COUNT_1_BIT
	)
	{
		VkAttachmentDescription attachDesc = {};
		attachDesc.format = format;
		attachDesc.initialLayout = initialImageLayout;
		attachDesc.finalLayout = finalImageLayout;
		attachDesc.samples = sampleCountFlag;
		attachDesc.loadOp = loadOp;
		attachDesc.storeOp = storeOp;
		attachDesc.stencilLoadOp = stencilLoadOp;
		attachDesc.stencilStoreOp = stencilStoreOp;

		return attachDesc;
	}


	static VkSubpassDescription InitSubpassDescription( std::vector<VkAttachmentReference> & colorAttachmentReference , VkAttachmentReference * depthAttachmentReference = NULL  )
	{
		VkSubpassDescription subpassDesc = {};
		subpassDesc.colorAttachmentCount = colorAttachmentReference.size();
		subpassDesc.pColorAttachments = colorAttachmentReference.data();
		subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDesc.pDepthStencilAttachment = depthAttachmentReference;
		return subpassDesc;
	}

	static VkAttachmentReference InitAttachmentReference( int attachmentIndex , VkImageLayout imageLayout )
	{
		VkAttachmentReference reference;
		reference.attachment = attachmentIndex;
		reference.layout = imageLayout;
		return reference;
	}

	static 		std::vector<VkSubpassDependency> GetDefaultSubpassDependency( )
	{
		std::vector<VkSubpassDependency> subpassDependency;
		VkSubpassDependency dependencies[2];
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		subpassDependency.push_back(dependencies[0]);
		subpassDependency.push_back(dependencies[1]);

		return subpassDependency;
	}

	static VkRenderPassCreateInfo InitRenderPassCreateInfo( 
		std::vector<VkAttachmentDescription> & attachmentDesc , 
		std::vector<VkSubpassDescription> & subpassDesc , 
		std::vector<VkSubpassDependency> & subpassDependency )
	{
		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.subpassCount = subpassDesc.size();
		renderPassCreateInfo.pSubpasses = subpassDesc.data();
		renderPassCreateInfo.attachmentCount = attachmentDesc.size();
		renderPassCreateInfo.pAttachments = attachmentDesc.data();
		renderPassCreateInfo.dependencyCount = subpassDependency.size();
		renderPassCreateInfo.pDependencies = subpassDependency.data();

		return renderPassCreateInfo;
	}

	static VkVertexInputBindingDescription InitVertexInputBindingDescription( uint32_t binding , VkVertexInputRate vertexInputRate , uint32_t stride )
	{
		VkVertexInputBindingDescription bindingDesc;
		bindingDesc.binding = binding;
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		bindingDesc.stride = stride;

		return bindingDesc;
	}

	static VkVertexInputAttributeDescription InitVertexInputAttributeDescription( uint32_t binding , uint32_t location , VkFormat format , uint32_t offset )
	{
		VkVertexInputAttributeDescription attribute;
		attribute.binding = binding;
		attribute.location = location;
		attribute.format = format;
		attribute.offset = offset;
		return attribute;
	}

	static VkPipelineShaderStageCreateInfo InitShaderStageCreateInfo(VkShaderStageFlagBits shaderStageFlag , std::string shaderFile , VulkanDevice * device )
	{
		VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
		pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineShaderStageCreateInfo.stage = shaderStageFlag;
		pipelineShaderStageCreateInfo.module = device->LoadShader(shaderFile);
		pipelineShaderStageCreateInfo.pName = "main";
		return pipelineShaderStageCreateInfo;
	}

	static VkCommandBufferBeginInfo InitCommandBufferBeginInfo( VkCommandBufferUsageFlags usageFlag )
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo = {};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags = usageFlag;
		return commandBufferBeginInfo;
	}

	static VkFramebufferCreateInfo InitFrameBufferCreateInfo( uint32_t width , uint32_t height , uint32_t layer , uint32_t attachCount , VkImageView * imageViews , VkRenderPass renderPass  )
	{
		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.attachmentCount = attachCount;
		frameBufferCreateInfo.height = height;
		frameBufferCreateInfo.width = width;
		frameBufferCreateInfo.layers = layer;
		frameBufferCreateInfo.pAttachments = imageViews;
		frameBufferCreateInfo.renderPass = renderPass;

		return frameBufferCreateInfo;
	}

	static std::vector<VkDescriptorPoolSize> InitDescriptorPoolSizeVec( const std::vector<VkDescriptorType>& descriptorType)
	{
		std::vector<VkDescriptorPoolSize> descriptorPoolSizeVec;
		for (auto dType : descriptorType)
		{
			VkDescriptorPoolSize descriptorPoolSize;
			descriptorPoolSize.descriptorCount = 1;
			descriptorPoolSize.type = dType;
			descriptorPoolSizeVec.push_back(descriptorPoolSize);
		}

		return descriptorPoolSizeVec;
	}

	static VkDescriptorPoolCreateInfo InitDescriptorPoolCreateInfo( std::vector<VkDescriptorPoolSize> & descriptorPoolSizeVec , uint32_t maxSets )
	{
		VkDescriptorPoolCreateInfo desc = {} ;
		desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		desc.pPoolSizes = descriptorPoolSizeVec.data();
		desc.poolSizeCount = descriptorPoolSizeVec.size();
		desc.maxSets = maxSets;

		return desc;
	}

	static VkDescriptorSetAllocateInfo InitDescriptorSetAllocateInfo( uint32_t descSetCount , VkDescriptorPool descPool , VkDescriptorSetLayout * setLayout )
	{
		VkDescriptorSetAllocateInfo descSetAllocateInfo = {};
		descSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAllocateInfo.descriptorPool = descPool;
		descSetAllocateInfo.descriptorSetCount = descSetCount;
		descSetAllocateInfo.pSetLayouts = setLayout;

		return descSetAllocateInfo;
	}

	static VkWriteDescriptorSet InitWriteImageDescriptorSet( 
		uint32_t descCount ,
		VkDescriptorType descType , 
		uint32_t binding , 
		VkDescriptorSet descSet , 
		VkDescriptorImageInfo * imageInfo )
	{
		VkWriteDescriptorSet writeDescriptorSet = {};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.descriptorCount = descCount;
		writeDescriptorSet.descriptorType = descType;
		writeDescriptorSet.dstBinding = binding;
		writeDescriptorSet.dstSet = descSet;
		writeDescriptorSet.pImageInfo = imageInfo;

		return writeDescriptorSet;
	}

	static VkWriteDescriptorSet InitWriteBufferDescriptorSet(
		uint32_t descCount,
		VkDescriptorType descType,
		uint32_t binding,
		VkDescriptorSet descSet,
		VkDescriptorBufferInfo * bufferInfo)
	{
		VkWriteDescriptorSet writeDescriptorSet = {};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.descriptorCount = descCount;
		writeDescriptorSet.descriptorType = descType;
		writeDescriptorSet.dstBinding = binding;
		writeDescriptorSet.dstSet = descSet;
		writeDescriptorSet.pBufferInfo = bufferInfo;

		return writeDescriptorSet;
	}

	static VkPushConstantRange InitVkPushConstantRange(
		uint32_t offset , 
		uint32_t size , 
		VkShaderStageFlags shaderStageFlag 
	)
	{
		VkPushConstantRange pushConstantRange;
		pushConstantRange.offset = offset;
		pushConstantRange.size = size;
		pushConstantRange.stageFlags = shaderStageFlag;

		return pushConstantRange;
	}

	static VkImageMemoryBarrier InitImageMemoryBarrier( 
		VkAccessFlags srcAccess , 
		VkAccessFlags dstAccess ,
		VkImageLayout oldLayout , 
		VkImageLayout newLayout ,
		VkImageAspectFlags aspectMask ,
		int baseArrayLayer , 
		int arrayLayerCount , 
		int baseMipLevel , 
		int mipLevels , 
		VkImage image,
		int srcQueueFamilyIndex = 0 ,
		int dstQueueFamilyIndex = 0 
 )
	{
		VkImageMemoryBarrier imageMemoryBarrier = {} ;
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcAccessMask = srcAccess;
		imageMemoryBarrier.dstAccessMask = dstAccess;
		imageMemoryBarrier.oldLayout = oldLayout;
		imageMemoryBarrier.newLayout = newLayout;
		imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = baseArrayLayer;
		imageMemoryBarrier.subresourceRange.layerCount = arrayLayerCount;
		imageMemoryBarrier.subresourceRange.baseMipLevel = baseMipLevel;
		imageMemoryBarrier.subresourceRange.levelCount = mipLevels;
		imageMemoryBarrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
		imageMemoryBarrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
		imageMemoryBarrier.image = image;
		return imageMemoryBarrier;
	}

	static VkBufferImageCopy InitBufferImageCopy( 
		int bufferOffset ,
		int bufferRowLength , 
		int bufferHeight , 
		int imageWidth , 
		int imageHeight , 
		int imageDepth , 
		VkOffset3D imageOffset , 
		VkImageAspectFlags imageAspectMask , 
		int baseArrayLayer , 
		int arrayLayerCount , 
		int mipLevel 
		)
	{
		VkBufferImageCopy imageCopyRegion = {};
		imageCopyRegion.bufferOffset = bufferOffset;
		imageCopyRegion.bufferRowLength = bufferRowLength;
		imageCopyRegion.bufferImageHeight = bufferHeight;
		imageCopyRegion.imageExtent.width = imageWidth;
		imageCopyRegion.imageExtent.height = imageHeight;
		imageCopyRegion.imageExtent.depth = imageDepth;
		imageCopyRegion.imageOffset = imageOffset;
		imageCopyRegion.imageSubresource.baseArrayLayer = baseArrayLayer;
		imageCopyRegion.imageSubresource.layerCount = arrayLayerCount;
		imageCopyRegion.imageSubresource.aspectMask = imageAspectMask;
		imageCopyRegion.imageSubresource.mipLevel = mipLevel;
		
		return imageCopyRegion;
	}

	static VkComputePipelineCreateInfo InitComputePipelineCreateInfo(
		VkPipelineLayout layout , 
		VkPipelineShaderStageCreateInfo shaderStage 
	)
	{
		VkComputePipelineCreateInfo computePipelineCreateInfo = {};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.layout = layout;
		computePipelineCreateInfo.stage = shaderStage;

		return computePipelineCreateInfo;
	}

	static VkDescriptorSetLayoutBinding InitBinding(
		uint32_t binding ,
		uint32_t descCount , 
		VkDescriptorType descType , 
		VkShaderStageFlags shaderStage 
	)
	{
		VkDescriptorSetLayoutBinding sbinding = {} ;
		sbinding.binding = binding;
		sbinding.descriptorCount = descCount;
		sbinding.descriptorType = descType;
		sbinding.stageFlags = shaderStage;
		return sbinding;
	}
};

#endif
