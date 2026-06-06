#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3_shadercross/SDL_shadercross.h>

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

struct UniformBuffer {
    float time;
    // you can add other properties here
};


// CCW order
Vertex vertices[] = {
    {0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f}, // top vertex
    {-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f}, // bottom left vertex
    {0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}, // bottom right vertex
};

int main() {
    // TODO Non-portable
    system("mkdir -p shaders");
    if (0 != system("glslc -fshader-stage=vertex ../src/shaders/vertex.glsl -o shaders/vertex.spv")) {
        std::cerr << "Vertex shader compilation failed\n";
        std::exit(-1);
    }
    if (0 != system("glslc -fshader-stage=fragment ../src/shaders/fragment.glsl -o shaders/fragment.spv")) {
        std::cerr << "Fragment shader compilation failed\n";
        std::exit(-1);
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        printf("ERROR: SDL_Init(): %s\n", SDL_GetError());
    }
    SDL_Window *sdl_window = SDL_CreateWindow("myengine2", 800, 600, SDL_WINDOW_RESIZABLE);
    if (!sdl_window) {
        std::cerr << "Failed to create SDL3 window" << std::endl;
        std::exit(-1);
    }

    // SDL_GPU_SHADERFORMAT_SPIRV requires MoltenVK on MacOS
    SDL_SetHint(SDL_HINT_VULKAN_LIBRARY, "/usr/local/lib/libMoltenVK.dylib");
    SDL_GPUDevice *sdl_gpuDevice = SDL_CreateGPUDevice(
        SDL_ShaderCross_GetSPIRVShaderFormats(), true, nullptr);;
    if (!sdl_gpuDevice) {
        std::cerr << "Failed to create GPU device" << std::endl;
        std::exit(-1);
    }
    SDL_ClaimWindowForGPUDevice(sdl_gpuDevice, sdl_window);


    // Create GPU Buffer
    SDL_GPUBufferCreateInfo gpuBufferCreateInfo{};
    gpuBufferCreateInfo.size = sizeof(Vertex) * std::size(vertices);
    gpuBufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    SDL_GPUBuffer *vertexBuffer = SDL_CreateGPUBuffer(sdl_gpuDevice, &gpuBufferCreateInfo);

    // Create Transfer Buffer
    SDL_GPUTransferBufferCreateInfo gpuTransferBufferCreateInfo{};
    gpuTransferBufferCreateInfo.size = gpuBufferCreateInfo.size;
    gpuTransferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    SDL_GPUTransferBuffer *gpuTransferBuffer = SDL_CreateGPUTransferBuffer(sdl_gpuDevice, &gpuTransferBufferCreateInfo);

    // Map transfer buffer
    Vertex *data = (Vertex *) SDL_MapGPUTransferBuffer(sdl_gpuDevice, gpuTransferBuffer, false);
    SDL_memcpy(data, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(sdl_gpuDevice, gpuTransferBuffer);

    // Copy Pass
    SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(sdl_gpuDevice);
    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    // Source
    SDL_GPUTransferBufferLocation transferBufferLocation{};
    transferBufferLocation.transfer_buffer = gpuTransferBuffer;
    transferBufferLocation.offset = 0;

    // Destination
    SDL_GPUBufferRegion region{};
    region.buffer = vertexBuffer;
    region.size = sizeof(Vertex) * std::size(vertices);
    region.offset = 0;

    SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &region, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    size_t vertexCodeSize{};
    Uint8 *vertexCode = static_cast<Uint8 *>(SDL_LoadFile("shaders/vertex.spv", &vertexCodeSize));
    SDL_ShaderCross_SPIRV_Info vertexInfo{};
    vertexInfo.bytecode = vertexCode;
    vertexInfo.bytecode_size = vertexCodeSize;
    vertexInfo.entrypoint = "main";
    vertexInfo.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;

    SDL_ShaderCross_GraphicsShaderMetadata *vertexMetadata = SDL_ShaderCross_ReflectGraphicsSPIRV(
        vertexCode, vertexCodeSize, 0);
    SDL_GPUShader *vertexShader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
        sdl_gpuDevice, &vertexInfo, &vertexMetadata->resource_info, 0);

    SDL_free(vertexCode); // Free the file

    size_t fragmentCodeSize{};
    Uint8 *fragmentCode = static_cast<Uint8 *>(SDL_LoadFile("shaders/fragment.spv", &fragmentCodeSize));
    SDL_ShaderCross_SPIRV_Info fragmentInfo{};
    fragmentInfo.bytecode = fragmentCode;
    fragmentInfo.bytecode_size = fragmentCodeSize;
    fragmentInfo.entrypoint = "main";
    fragmentInfo.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;;
    SDL_ShaderCross_GraphicsShaderMetadata *fragmentMetadata = SDL_ShaderCross_ReflectGraphicsSPIRV(
        fragmentCode, fragmentCodeSize, 0);
    SDL_GPUShader *fragmentShader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
        sdl_gpuDevice, &fragmentInfo, &fragmentMetadata->resource_info, 0);
    SDL_free(fragmentCode); // Free the file


    std::cout << SDL_GetError();

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    SDL_GPUVertexBufferDescription vbDescriptions[1]{};

    vbDescriptions[0].slot = 0;
    vbDescriptions[0].pitch = sizeof(Vertex);
    vbDescriptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vbDescriptions[0].instance_step_rate = 0;

    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = vbDescriptions;


    SDL_GPUVertexAttribute vertexAttributes[2]{};

    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].location = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; // XYZ
    vertexAttributes[0].offset = 0;


    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].location = 1;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4; // RGBA
    vertexAttributes[1].offset = 3 * sizeof(float);

    pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
    pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

    SDL_GPUColorTargetDescription colorTargetDescriptions[1]{};
    colorTargetDescriptions[0].format = SDL_GetGPUSwapchainTextureFormat(sdl_gpuDevice, sdl_window);

    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;


    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(sdl_gpuDevice, &pipelineInfo);
    SDL_ReleaseGPUShader(sdl_gpuDevice, vertexShader);
    SDL_ReleaseGPUShader(sdl_gpuDevice, fragmentShader);


    UniformBuffer timeUniform{};
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.key.down) {
                switch (event.key.key) {
                    case SDLK_ESCAPE:
                        done = true;
                        break;
                    default: break;
                }
            }
        }

        SDL_GPUCommandBuffer *sdl_gpuCommnadBuffer = SDL_AcquireGPUCommandBuffer(sdl_gpuDevice);
        SDL_GPUTexture *swapchainTexture;
        Uint32 width, height;
        SDL_WaitAndAcquireGPUSwapchainTexture(sdl_gpuCommnadBuffer, sdl_window, &swapchainTexture, &width, &height);
        if (!swapchainTexture) {
            printf(":\\\n");
            SDL_SubmitGPUCommandBuffer(sdl_gpuCommnadBuffer);
            continue;
        }

        SDL_GPUColorTargetInfo colorTargetInfo{};
        colorTargetInfo.clear_color = {0.0f, 0.0f, 0.0f, 0.0f};
        colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
        colorTargetInfo.texture = swapchainTexture;

        SDL_GPURenderPass *sdl_renderPass = SDL_BeginGPURenderPass(sdl_gpuCommnadBuffer, &colorTargetInfo, 1, nullptr);


        timeUniform.time = SDL_GetTicksNS() / 1e9f;
        SDL_PushGPUVertexUniformData(sdl_gpuCommnadBuffer, 0, &timeUniform, sizeof(UniformBuffer));
        SDL_BindGPUGraphicsPipeline(sdl_renderPass, pipeline);

        SDL_GPUBufferBinding bufferBindings[1]{};
        bufferBindings[0].buffer = vertexBuffer;
        bufferBindings[0].offset = 0;

        SDL_BindGPUVertexBuffers(sdl_renderPass, 0, bufferBindings, 1);

        SDL_DrawGPUPrimitives(sdl_renderPass, 3, 1, 0, 0);

        SDL_EndGPURenderPass(sdl_renderPass);
        SDL_SubmitGPUCommandBuffer(sdl_gpuCommnadBuffer);
    }

    SDL_ReleaseGPUGraphicsPipeline(sdl_gpuDevice, pipeline);
    SDL_ReleaseGPUBuffer(sdl_gpuDevice, vertexBuffer);
    SDL_ReleaseGPUTransferBuffer(sdl_gpuDevice, gpuTransferBuffer);
    SDL_DestroyGPUDevice(sdl_gpuDevice);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
    return 0;
}
