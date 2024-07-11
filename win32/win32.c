
#include "..\base.h"
#include "..\os.h"
#include "..\math.h"

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>

#pragma comment (lib, "kernel32")
#pragma comment (lib, "gdi32")
#pragma comment (lib, "user32")
#pragma comment (lib, "dxguid")
#pragma comment (lib, "dxgi")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "d3dcompiler")

static u64 AllocationGranularity = 65536;
static u64 NumberOfProcessors = 1;
static LARGE_INTEGER PerformanceFrequency;
static memory_arena TempArena = {0};

static HWND WindowHandle;
bool ShouldWindowClose = false;
static u32 WindowWidth = 1280, WindowHeight = 720;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

memory_arena AllocateArenaFromOS(u32 Capacity, u64 StartingAddress) {
    memory_arena Result = {0};
    Result.Capacity = Capacity;
    u32 SizeRoundedUp = RoundUp32(Capacity, AllocationGranularity);
    Result.Start = VirtualAlloc((void *)StartingAddress, SizeRoundedUp, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    Result.Offset = Result.Start;

    Assert(Result.Start != 0);
    return Result;
}


typedef struct {
	void *Data;
	u64 Size;
} buffer;

static buffer FileRead(memory_arena *Arena, const char *Path) {
    HANDLE FileHandle = CreateFileA(Path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    Assert(FileHandle != INVALID_HANDLE_VALUE);

    LARGE_INTEGER FileSize = {0};
    GetFileSizeEx(FileHandle, &FileSize);

    void *Buffer = ArenaPush(Arena, FileSize.LowPart);
    BOOL ReadResult = ReadFile(FileHandle, Buffer, FileSize.LowPart, NULL, NULL);
    Assert(ReadResult);
    (void)ReadResult;

    CloseHandle(FileHandle);
    buffer Result = {
        .Data = Buffer,
        .Size = FileSize.LowPart
    };
    return Result;
}

static bool WindowShouldClose() {

	MSG Message;
	while (PeekMessage(&Message, WindowHandle, 0, 0, PM_REMOVE)) {
		DispatchMessage(&Message);
	}

	// HWND ActiveWindow = GetActiveWindow();
	// if (ActiveWindow != WindowHandle) {
    // }

    return ShouldWindowClose;
}


[[noreturn]]
void AppMain() {

	{
        // Init OS Properties
        SYSTEM_INFO SystemInfo;
        GetSystemInfo(&SystemInfo);
        AllocationGranularity = SystemInfo.dwAllocationGranularity;
        NumberOfProcessors = SystemInfo.dwNumberOfProcessors;
        Assert(PopCount64(AllocationGranularity) == 1);

        // rdtsc / profiling
        BOOL Success = QueryPerformanceFrequency(&PerformanceFrequency);
        Assert(Success);
        (void)Success;

        _mm_setcsr(_mm_getcsr() | (_MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON));

        u64 StartingAddress = 0;
#if defined(_DEBUG)
        StartingAddress = TB(0x4);
#endif
        TempArena = AllocateArenaFromOS(MB(32), StartingAddress);
	}

	init_params InitParams = {
        .WindowWidth = WindowWidth,
        .WindowHeight = WindowHeight
    };
	OnInit(&InitParams);

    HINSTANCE hInstance = GetModuleHandle(0);
    {
        WNDCLASSEXW WindowClass = {
            .cbSize = sizeof(WindowClass),
            .lpfnWndProc = WindowProc,
            .hInstance = hInstance,
            .hIcon = LoadIcon(NULL, IDI_APPLICATION),
            .hCursor = LoadCursor(NULL, IDC_ARROW),
            .lpszClassName = L"D3D11_Window_Class"
        };
        ATOM Atom = RegisterClassExW(&WindowClass);
        Assert(Atom);
        (void)Atom;

        DWORD ExStyle = WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP;
        DWORD Style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;

        RECT WindowSize = {0};
        WindowSize.right = (LONG)(InitParams.WindowWidth);
        WindowSize.bottom = (LONG)(InitParams.WindowHeight);
        AdjustWindowRectEx(&WindowSize, Style, FALSE, 0);
        u32 AdjustedWidth = WindowSize.right - WindowSize.left;
        u32 AdjustedHeight = WindowSize.bottom - WindowSize.top;

        WindowHandle = CreateWindowExW(
            ExStyle, WindowClass.lpszClassName, L"D3D11Window", Style,
            CW_USEDEFAULT, CW_USEDEFAULT, AdjustedWidth, AdjustedHeight,
            NULL, NULL, hInstance, NULL
        );
        Assert(WindowHandle);
    }

    HRESULT HR;

    ID3D11Device *Device = 0;
    ID3D11DeviceContext *DeviceContext = 0;
    IDXGISwapChain1 *SwapChain = 0;
    {
        UINT Flags = 0;
#if defined(_DEBUG)
        Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL Levels[] = { D3D_FEATURE_LEVEL_11_0 };
        HR = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, Flags, Levels, ArrayLength(Levels),
            D3D11_SDK_VERSION, &Device, NULL, &DeviceContext
        );

        Assert(Device);
        Assert(SUCCEEDED(HR));
    }

#if defined(_DEBUG)
    {
        ID3D11InfoQueue *Info = 0;
        ID3D11Device_QueryInterface(Device, &IID_ID3D11InfoQueue, (void **)&Info);
        ID3D11InfoQueue_SetBreakOnSeverity(Info, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        ID3D11InfoQueue_SetBreakOnSeverity(Info, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        ID3D11InfoQueue_Release(Info);
    }

    {
        IDXGIInfoQueue* DxgiInfo;
        HR = DXGIGetDebugInterface1(0, &IID_IDXGIInfoQueue, (void**)&DxgiInfo);
        Assert(SUCCEEDED(HR));
        IDXGIInfoQueue_SetBreakOnSeverity(DxgiInfo, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        IDXGIInfoQueue_SetBreakOnSeverity(DxgiInfo, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
        IDXGIInfoQueue_Release(DxgiInfo);
    }
#endif

    {
        IDXGIDevice *DxgiDevice = 0;
        HR = ID3D11Device_QueryInterface(Device, &IID_IDXGIDevice, (void **)&DxgiDevice);
        Assert(SUCCEEDED(HR));

        IDXGIAdapter *DxgiAdapter = 0;
        HR = IDXGIDevice_GetAdapter(DxgiDevice, &DxgiAdapter);
        Assert(SUCCEEDED(HR));

        IDXGIFactory2 *Factory = 0;
        HR = IDXGIAdapter_GetParent(DxgiAdapter, &IID_IDXGIFactory2, (void**)&Factory);
        Assert(SUCCEEDED(HR));

        DXGI_SWAP_CHAIN_DESC1 Description = {
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = { 1, 0 },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 2,
            .Scaling = DXGI_SCALING_NONE,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD
        };

        HR = IDXGIFactory2_CreateSwapChainForHwnd(Factory, (IUnknown *)Device, WindowHandle, &Description, NULL, NULL, &SwapChain);
        Assert(SUCCEEDED(HR));

        IDXGIFactory_MakeWindowAssociation(Factory, WindowHandle, DXGI_MWA_NO_ALT_ENTER);

        IDXGIFactory2_Release(Factory);
        IDXGIAdapter_Release(DxgiAdapter);
        IDXGIDevice_Release(DxgiDevice);
    }

    ID3D11Buffer *VertexBuffer;
    {
        f32 Vertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 1.0f,
             1.0f,  1.0f, 1.0f, 1.0f,

            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
        };

        D3D11_BUFFER_DESC BufferDescription = {
            .ByteWidth = sizeof(Vertices),
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER
        };

        D3D11_SUBRESOURCE_DATA Initial = { .pSysMem = Vertices };
        ID3D11Device_CreateBuffer(Device, &BufferDescription, &Initial, &VertexBuffer);
    }

    ID3D11InputLayout *Layout;
    ID3D11VertexShader *VertexShader = 0;
    ID3D11PixelShader *PixelShader = 0;
    ID3D11ComputeShader *CCAShader = 0;
    ID3D11ComputeShader *RNGShader = 0;

    {
        D3D11_INPUT_ELEMENT_DESC Description[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(f32) * 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            { "UV",       0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(f32) * 2, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        memory_arena FileArena = ArenaScratch(&TempArena);
        buffer VertexShaderByteCode = FileRead(&FileArena, "blit.vertex.cso");
        ID3D11Device_CreateVertexShader(Device, VertexShaderByteCode.Data, VertexShaderByteCode.Size, NULL, &VertexShader);
        Assert(VertexShader != 0);

        buffer PixelShaderByteCode = FileRead(&FileArena, "blit.pixel.cso");
        ID3D11Device_CreatePixelShader(Device, PixelShaderByteCode.Data, PixelShaderByteCode.Size, NULL, &PixelShader);
        Assert(PixelShader != 0);

        ID3D11Device_CreateInputLayout(Device, Description, ARRAYSIZE(Description), VertexShaderByteCode.Data, VertexShaderByteCode.Size, &Layout);
    }

    {
        memory_arena FileArena = ArenaScratch(&TempArena);
        buffer ByteCode = FileRead(&FileArena, "cca.compute.cso");
        ID3D11Device_CreateComputeShader(Device, ByteCode.Data, ByteCode.Size, NULL, &CCAShader);
        Assert(CCAShader != 0);
    }

    {
        memory_arena FileArena = ArenaScratch(&TempArena);
        buffer ByteCode = FileRead(&FileArena, "rng.compute.cso");
        ID3D11Device_CreateComputeShader(Device, ByteCode.Data, ByteCode.Size, NULL, &RNGShader);
        Assert(RNGShader != 0);
    }

    typedef struct {
        ID3D11Texture2D *Texture;
        ID3D11ShaderResourceView *ResourceView;
        ID3D11UnorderedAccessView *UAV;
    } RWTexture;

    RWTexture Textures[2] = {0};
    {
        u32 Width = WindowWidth;
        u32 Height = WindowHeight;

        D3D11_TEXTURE2D_DESC Description = {
            .Width = Width,
            .Height = Height,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_R32_FLOAT,
            .SampleDesc = { 1, 0 },
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS 
        };

        ID3D11Device_CreateTexture2D(Device, &Description, NULL, &Textures[0].Texture);
        ID3D11Device_CreateShaderResourceView(Device, (ID3D11Resource *)Textures[0].Texture, NULL, &Textures[0].ResourceView);

        ID3D11Device_CreateTexture2D(Device, &Description, NULL, &Textures[1].Texture);
        ID3D11Device_CreateShaderResourceView(Device, (ID3D11Resource *)Textures[1].Texture, NULL, &Textures[1].ResourceView);
        // ID3D11Texture2D_Release(Texture);
    }

    ID3D11SamplerState *SamplerState;
    {
        D3D11_SAMPLER_DESC SamplerDescription = {
            .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
            .AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
            .AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
            .AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
            .MipLODBias = 0,
            .MaxAnisotropy = 1,
            .MinLOD = 0,
            .MaxLOD = D3D11_FLOAT32_MAX
        };

        ID3D11Device_CreateSamplerState(Device, &SamplerDescription, &SamplerState);
    }

    ID3D11BlendState *BlendState = 0;
    {
        D3D11_BLEND_DESC BlendDescription = {
            .RenderTarget[0] = {
                .BlendEnable = TRUE,
                .SrcBlend = D3D11_BLEND_SRC_ALPHA,
                .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
                .BlendOp = D3D11_BLEND_OP_ADD,
                .SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA,
                .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
                .BlendOpAlpha = D3D11_BLEND_OP_ADD,
                .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
            }
        };
        ID3D11Device_CreateBlendState(Device, &BlendDescription, &BlendState);
    }

    ID3D11RasterizerState *RasterizerState = 0;
    {
        D3D11_RASTERIZER_DESC Description =
        {
            .FillMode = D3D11_FILL_SOLID,
            .CullMode = D3D11_CULL_NONE,
            .DepthClipEnable = TRUE,
        };
        ID3D11Device_CreateRasterizerState(Device, &Description, &RasterizerState);
    }

    ID3D11DepthStencilState *DepthState = 0;
    {
        D3D11_DEPTH_STENCIL_DESC DepthDescription =
        {
            .DepthEnable = FALSE,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = D3D11_COMPARISON_LESS,
            .StencilEnable = FALSE,
            .StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
            .StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
        };
        ID3D11Device_CreateDepthStencilState(Device, &DepthDescription, &DepthState);
    }

    ID3D11RenderTargetView *RenderTargetView = 0;
    ID3D11DepthStencilView *DepthStencilView = 0;

    {
        RECT Rect;
        GetClientRect(WindowHandle, &Rect);
        WindowWidth = Rect.right - Rect.left;
        WindowHeight = Rect.bottom - Rect.top;
        // HR = IDXGISwapChain1_ResizeBuffers(SwapChain, 0, WindowWidth, WindowHeight, DXGI_FORMAT_UNKNOWN, 0);
        // Assert(SUCCEEDED(HR));
        ID3D11Texture2D *BackBuffer = 0;
        IDXGISwapChain1_GetBuffer(SwapChain, 0, &IID_ID3D11Texture2D, (void**)&BackBuffer);
        ID3D11Device_CreateRenderTargetView(Device, (ID3D11Resource *)BackBuffer, NULL, &RenderTargetView);
        ID3D11Texture2D_Release(BackBuffer);

        D3D11_TEXTURE2D_DESC DepthDesc = {
            .Width = WindowWidth,
            .Height = WindowHeight,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_D32_FLOAT, // or use DXGI_FORMAT_D32_FLOAT_S8X24_UINT if you need stencil
            .SampleDesc = { 1, 0 },
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_DEPTH_STENCIL,
        };

        // create new depth stencil texture & DepthStencil view
        ID3D11Texture2D* DepthTexture;
        ID3D11Device_CreateTexture2D(Device, &DepthDesc, NULL, &DepthTexture);
        ID3D11Device_CreateDepthStencilView(Device, (ID3D11Resource*)DepthTexture, NULL, &DepthStencilView);
        ID3D11Texture2D_Release(DepthTexture);
    }

    D3D11_VIEWPORT Viewport = {
        .TopLeftX = 0,
        .TopLeftY = 0,
        .Width = (FLOAT)WindowWidth,
        .Height = (FLOAT)WindowHeight,
        .MinDepth = 0,
        .MaxDepth = 1,
    };

    ID3D11Device_CreateUnorderedAccessView(Device, (ID3D11Resource *)Textures[0].Texture, NULL, &Textures[0].UAV);
    ID3D11Device_CreateUnorderedAccessView(Device, (ID3D11Resource *)Textures[1].Texture, NULL, &Textures[1].UAV);

    bool TextureSwap = false;

    typedef struct {
        u32 States;
        u32 Threshold;
        u32 Search;
        u32 UseNeummanSearch;
    } constant_buffer;

    static constant_buffer ConstantBuffer;
    ID3D11Buffer *D3D11ConstantBuffer = NULL;
    {
        ConstantBuffer.States = 15;
        ConstantBuffer.Threshold = 1;
        ConstantBuffer.Search = 1;
        ConstantBuffer.UseNeummanSearch = 1;

        D3D11_BUFFER_DESC ConstantBufferDescription = {
            .ByteWidth = sizeof(constant_buffer),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            .MiscFlags = 0,
            .StructureByteStride = 0
        };

        D3D11_SUBRESOURCE_DATA InitialData = { .pSysMem = &ConstantBuffer };
        HR = ID3D11Device_CreateBuffer(Device, &ConstantBufferDescription, &InitialData, &D3D11ConstantBuffer);
        Assert(SUCCEEDED(HR));
    }
    ID3D11DeviceContext_CSSetConstantBuffers(DeviceContext, 0, 1, &D3D11ConstantBuffer);
    ID3D11DeviceContext_PSSetConstantBuffers(DeviceContext, 0, 1, &D3D11ConstantBuffer);

    {
        // Initialize
        ID3D11DeviceContext_CSSetShader(DeviceContext, RNGShader, NULL, 0);
        ID3D11DeviceContext_CSSetUnorderedAccessViews(DeviceContext, 0, 1, &Textures[TextureSwap].UAV, NULL);
        ID3D11DeviceContext_Dispatch(DeviceContext, WindowWidth / 32, WindowHeight / 32, 1);
        ID3D11UnorderedAccessView *NullUSRV[] = { 0 };
        ID3D11DeviceContext_CSSetUnorderedAccessViews(DeviceContext, 0, 1, NullUSRV, NULL);
        ID3D11DeviceContext_CSSetShader(DeviceContext, NULL, NULL, 0);
    }


    while (!WindowShouldClose()) {
        // OnRender();
        
        FLOAT ClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        ID3D11DeviceContext_ClearRenderTargetView(DeviceContext, RenderTargetView, ClearColor);
        ID3D11DeviceContext_ClearDepthStencilView(DeviceContext, DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0.0);

        // Input Assembler
        ID3D11DeviceContext_IASetInputLayout(DeviceContext, Layout);
        ID3D11DeviceContext_IASetPrimitiveTopology(DeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        UINT Stride = sizeof(f32) * 4;
        UINT Offset = 0;
        ID3D11DeviceContext_IASetVertexBuffers(DeviceContext, 0, 1, &VertexBuffer, &Stride, &Offset);

        // Compute Shader
        ID3D11DeviceContext_CSSetShader(DeviceContext, CCAShader, NULL, 0);
        ID3D11DeviceContext_CSSetUnorderedAccessViews(DeviceContext, 0, 1, &Textures[TextureSwap].UAV, NULL);
        ID3D11DeviceContext_CSSetUnorderedAccessViews(DeviceContext, 1, 1, &Textures[!TextureSwap].UAV, NULL);
        ID3D11DeviceContext_Dispatch(DeviceContext, WindowWidth / 32, WindowHeight / 32, 1);
        static ID3D11UnorderedAccessView *NullUSRV[] = { 0, 0 };
        ID3D11DeviceContext_CSSetUnorderedAccessViews(DeviceContext, 0, 2, NullUSRV, NULL);
        ID3D11DeviceContext_CSSetShader(DeviceContext, NULL, NULL, 0);

        // Vertex Shader
        // ID3D11DeviceContext_VSSetConstantBuffers(DeviceContext, 0, 1, &ConstantBuffer);
        ID3D11DeviceContext_VSSetShader(DeviceContext, VertexShader, NULL, 0);

        // Rasterization
        ID3D11DeviceContext_RSSetViewports(DeviceContext, 1, &Viewport);
        ID3D11DeviceContext_RSSetState(DeviceContext, RasterizerState);

        // Pixel Shader
        ID3D11DeviceContext_PSSetSamplers(DeviceContext, 0, 1, &SamplerState);
        ID3D11DeviceContext_PSSetShaderResources(DeviceContext, 0, 1, &Textures[!TextureSwap].ResourceView);
        ID3D11DeviceContext_PSSetShader(DeviceContext, PixelShader, NULL, 0);

        // Output Merger
        ID3D11DeviceContext_OMSetBlendState(DeviceContext, BlendState, NULL, ~0ULL);
        ID3D11DeviceContext_OMSetDepthStencilState(DeviceContext, DepthState, 0);
        ID3D11DeviceContext_OMSetRenderTargets(DeviceContext, 1, &RenderTargetView, DepthStencilView);

        // Draw
        ID3D11DeviceContext_Draw(DeviceContext, 6, 0);

        IDXGISwapChain1_Present(SwapChain, 3, 0);
        static ID3D11ShaderResourceView NullSRV[] = { 0 };
        ID3D11DeviceContext_PSSetShaderResources(DeviceContext, 0, 1, (ID3D11ShaderResourceView**)NullSRV);

        TextureSwap = !TextureSwap;
        // Sleep(0);
    }

    ExitProcess(0);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
        case WM_CLOSE:
        case WM_QUIT: {
            ShouldWindowClose = true;
            return 0;
        }
        case WM_SIZE: {
            UINT NewWidth = LOWORD(lParam);
            UINT NewHeight = HIWORD(lParam);
            WindowWidth = NewWidth;
            WindowHeight = NewHeight;
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}