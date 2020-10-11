#include "ImGuiSystem.h"
#include <examples/imgui_impl_vulkan.h>

#include <iostream>

namespace {

static void checkVkResult(VkResult err)
{
    if (err == 0)
        return;
    std::cerr << "[vulkan] Error: VkResult = " << err << std::endl;
    if (err < 0)
        abort();
}

}

namespace Amano {

ImGuiSystem::ImGuiSystem(Device* device)
	: Pass(device, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV)
    , InputReader()
	, m_descriptorPool{ VK_NULL_HANDLE }
    , m_renderPass{ VK_NULL_HANDLE }
    , m_commandBuffer{ VK_NULL_HANDLE }
    , m_mouseJustPressed{}
{
    for (int i = 0; i < ImGuiMouseButton_COUNT; ++i) {
        m_mouseJustPressed[i] = false;
    }
}

ImGuiSystem::~ImGuiSystem() {
    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
    vkDestroyRenderPass(m_device->handle(), m_renderPass, nullptr);
	m_device->releaseDescriptorPool(m_descriptorPool);
    m_device->getQueue(QueueType::eGraphics)->freeCommandBuffer(m_commandBuffer);
}

bool ImGuiSystem::init() {
    return createDescriptorPool()
        && createRenderPass()
        && initImgui();
}

bool ImGuiSystem::initImgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // setup renderer bindings
    // TODO: pass this structure to Device to let it initialize
    auto pQueue = m_device->getQueue(QueueType::eGraphics);
	ImGui_ImplVulkan_InitInfo initInfo;
    initInfo.Instance = m_device->instance();
    initInfo.PhysicalDevice = m_device->physicalDevice();
	initInfo.Device = m_device->handle();
    initInfo.QueueFamily = pQueue->familyIndex();
    initInfo.Queue = pQueue->handle();
    initInfo.PipelineCache = VK_NULL_HANDLE;  // maybe not needed
    initInfo.DescriptorPool = m_descriptorPool;
    initInfo.MinImageCount = static_cast<uint32_t>(m_device->getSwapChainImageViews().size());  // swapchain size?
    initInfo.ImageCount = initInfo.MinImageCount;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT; // no MSAA in deferred
    initInfo.Allocator = nullptr;  // no custom allocator
    initInfo.CheckVkResultFn = checkVkResult;

    if (!ImGui_ImplVulkan_Init(&initInfo, m_renderPass)) {
        std::cerr << "Unable to initialize ImGui" << std::endl;
        return false;
    }

    // load and upload font
    io.Fonts->AddFontDefault();
    {
        auto queue = m_device->getQueue(QueueType::eGraphics);
        auto cmd = queue->beginSingleTimeCommands();
        ImGui_ImplVulkan_CreateFontsTexture(cmd);  // TODO: check that this succeeded
        queue->endSingleTimeCommands(cmd);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    return true;
}

// from imgui samples
bool ImGuiSystem::updateMouse(GLFWwindow* window) {
    // Update buttons
    ImGuiIO& io = ImGui::GetIO();
    for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
    {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        io.MouseDown[i] = m_mouseJustPressed[i] || glfwGetMouseButton(window, i) != 0;
        m_mouseJustPressed[i] = false;
    }

    // Update mouse position
    const ImVec2 mouse_pos_backup = io.MousePos;
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    const bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
    if (focused)
    {
        if (io.WantSetMousePos)
        {
            glfwSetCursorPos(window, (double)mouse_pos_backup.x, (double)mouse_pos_backup.y);
        }
        else
        {
            double mouse_x, mouse_y;
            glfwGetCursorPos(window, &mouse_x, &mouse_y);
            io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
        }
    }

    return ImGui::GetIO().WantCaptureMouse;
}

void ImGuiSystem::startFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
}

void ImGuiSystem::endFrame(VkFramebuffer framebuffer, uint32_t width, uint32_t height, VkFence fence) {
    // setup the buffers
    ImGui::Render();

    auto queue = m_device->getQueue(QueueType::eGraphics);
    queue->freeCommandBuffer(m_commandBuffer);
    m_commandBuffer = VK_NULL_HANDLE;
    m_commandBuffer = queue->beginSingleTimeCommands();

    // start the pass
    VkRenderPassBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = m_renderPass;
    info.framebuffer = framebuffer;
    info.renderArea.extent.width = width;
    info.renderArea.extent.height = height;
    info.clearValueCount = 0; // no clear for now
    info.pClearValues = nullptr;
    vkCmdBeginRenderPass(m_commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

    // prepare the buffers
    ImDrawData* draw_data = ImGui::GetDrawData();

    // this method calls some commands so it should be called during a command recording
    ImGui_ImplVulkan_RenderDrawData(draw_data, m_commandBuffer);

    vkCmdEndRenderPass(m_commandBuffer);

    //queue->endSingleTimeCommands(m_commandBuffer);
    // since we want to use semaphore, do not call endSingleTimeCommands
    // submit it manually, delete it at the next frame
    vkEndCommandBuffer(m_commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(m_waitSemaphores.size());
    submitInfo.pWaitSemaphores = m_waitSemaphores.data();
    submitInfo.pWaitDstStageMask = m_waitPipelineStages.data();

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_signalSemaphore;

    auto pQueue = m_device->getQueue(QueueType::eGraphics);
    pQueue->submit(&submitInfo, fence);
}

bool ImGuiSystem::createRenderPass() {
    // TODO: pass the attachment description format and general information
    VkAttachmentDescription attachment = {};
    attachment.format = m_device->getSwapChainFormat();
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // for now UI rendering hapens at the end on the swapchain
    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;
    if (vkCreateRenderPass(m_device->handle(), &info, nullptr, &m_renderPass) != VK_SUCCESS) {
        std::cerr << "Couldn't enumerate extension properties." << std::endl;
        return false;
    }

    // We do not create a pipeline by default as this is also used by examples' main.cpp,
    // but secondary viewport in multi-viewport mode may want to create one with:
    //ImGui_ImplVulkan_CreatePipeline(device, allocator, VK_NULL_HANDLE, wd->RenderPass, VK_SAMPLE_COUNT_1_BIT, &wd->Pipeline);

    return true;
}

bool ImGuiSystem::createDescriptorPool() {
    m_descriptorPool = m_device->createDetachedDescriptorPool();
    return m_descriptorPool != VK_NULL_HANDLE;
}

}
