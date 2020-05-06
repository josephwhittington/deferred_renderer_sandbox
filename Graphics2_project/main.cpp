// Graphics2_project.cpp : Defines the entry point for the application.

#include <vector>
#include <string>
#include <fstream>
#include <ctime>

#include "framework.h"
#include "Graphics2_project.h"

#include <d3d11.h>
#include <DirectXMath.h>

#include "assets/StoneHenge.h"
#include "Camera.h"
#include "Enums_globals.h"
#include "InputManager.h"

#include "base_vs.csh"
#include "base_ps.csh"

#include "post_vs.csh"
#include "post_gm.csh"
#include "post_ps.csh"

using namespace DirectX;

// Globals
// For initialization and utility
ID3D11Device* device;
IDXGISwapChain* swapchain;
ID3D11DeviceContext* deviceContext;

// States
ID3D11RasterizerState* rasterizerStateDefault;
ID3D11RasterizerState* rasterizerStateWireframe;
ID3D11SamplerState* sampler_state;

// For drawing
ID3D11RenderTargetView* renderTargetView;
D3D11_VIEWPORT viewport;
float aspectRatio = 1;
int screen_width = 0;
int screen_height = 0;

// Camera
int camdir = 1;
// Camera

// Shader variables
ID3D11Buffer* constantBuffer;
// Shader variables

// Z buffer
ID3D11Texture2D* zBuffer;
ID3D11DepthStencilView* depthStencil;
// Z buffer

// Geometry buffers
ID3D11Texture2D* gbuffer_depth;
ID3D11Texture2D* gbuffer_diffuse;
ID3D11Texture2D* gbuffer_specular;
ID3D11Texture2D* gbuffer_normal;
ID3D11Texture2D* gbuffer_position;

ID3D11DepthStencilView* gbuffer_stencil_depth;

ID3D11ShaderResourceView* gbuffer_resource_depth;
ID3D11ShaderResourceView* gbuffer_resource_diffuse;
ID3D11ShaderResourceView* gbuffer_resource_specular;
ID3D11ShaderResourceView* gbuffer_resource_normal;
ID3D11ShaderResourceView* gbuffer_resource_position;

ID3D11RenderTargetView* gbuffer_target_diffuse;
ID3D11RenderTargetView* gbuffer_target_specular;
ID3D11RenderTargetView* gbuffer_target_normal;
ID3D11RenderTargetView* gbuffer_target_position;
// Geometry buffers

// Camera shit
FPSCamera camera;

bool g_wireframeEnabled = false;

// Object shit
ID3D11VertexShader* vertexShader;
ID3D11PixelShader* pixelShader;

struct Model
{
	std::vector<SimpleVertex> vertices;
	std::vector<int> indices;

	XMFLOAT3 position;

	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;

	ID3D11InputLayout* vertexBufferLayout;
};

struct BUFFERS
{
	ID3D11Buffer* c_bufer;
	ID3D11Buffer* l_buffer;
};

enum class LIGHTTYPE
{
	DIRECTIONAL = 0,
	POINT = 1,
	SPOT = 2
};

struct Light
{
	XMFLOAT4 position, lightDirection;
	XMFLOAT4 ambient, diffuse, specular;
	unsigned int lightType;
	float ambientIntensity, diffuseIntensity, specularIntensity;
	float cosineInnerCone, cosineOuterCone;
	float lightRadius;
	int lightOn;
};

// Math globals
struct WorldViewProjection
{
	XMFLOAT4X4 WorldMatrix;
	XMFLOAT4X4 ViewMatrix;
	XMFLOAT4X4 ProjectionMatrix;
	XMFLOAT4 camera_position;
}WORLD;	
// Models
Model model;
std::vector<Model> models;
std::vector<Light> lights;
BUFFERS buffers;
struct
{
	XMFLOAT4 campos;
	int mode;
}POST;

// Full screen quad
struct QVertex
{
	FLOAT3 position;
	FLOAT2 uv;
};
struct FullScreenQuad
{
	std::vector<QVertex> positions;
	ID3D11Buffer* vertexBuffer;

	ID3D11VertexShader* vertexShader;
	ID3D11GeometryShader* geometryShader;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* vertexBufferLayout;
} QUAD;

// Globals

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


// FUNCTION DECLARATION
void HandleInput();
void LoadWobjectMesh(const char* meshname, std::vector<SimpleVertex>& vertices, std::vector<int>& indices);
void InitializeModel(Model& model, const char* modelname, XMFLOAT3 position, const BYTE* _vs, const BYTE* _ps, int vs_size, int ps_size);
void InitializeFullscreenQuad(const BYTE* _vs, const BYTE* _gs, const BYTE* _ps, int vs_size, int gs_size, int ps_size);
void RenderModel(Model& model);
void CleanupModel(Model& model);
void CleanupFullscreenQuad();

void RenderFullscreenQuad();

// Geometry shit
void InitializeGubffers();
void CleanupGbuffers();

void GbufferPass();

// Helpers
float RandFloat(float LO, float HI)
{
	return LO + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (HI - LO)));
}

XMFLOAT3 RandFLOAT3(float LO, float HI)
{
	return XMFLOAT3(RandFloat(LO, HI), RandFloat(LO, HI), RandFloat(LO, HI));
}

XMFLOAT4 RandFLOAT4XYZ(float LO, float HI)
{
	return XMFLOAT4(RandFloat(LO, HI), RandFloat(LO, HI), RandFloat(LO, HI), 1);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GRAPHICS2PROJECT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GRAPHICS2PROJECT));

    MSG msg;

    // Main message loop:
	while (true)
	{
		PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
			break;
		if (GetAsyncKeyState(VK_ESCAPE)) break;

		// Input shit
		HandleInput();

		// Move all the lights
		float thresh = .1;
		for (int i = 0; i < lights.size(); i++)
		{
			int axis = 2;
			float r = RandFloat(0, 3);
			if (r < 1) axis = 0;
			else if (r < 2) axis = 1;

			switch (axis)
			{
			case 0:
				lights[i].position.x += RandFloat(-thresh, thresh);
				break;
			case 1:
				lights[i].position.y += RandFloat(-thresh, thresh);
				break;
			case 2:
				lights[i].position.z += RandFloat(-thresh, thresh);
				break;
			default:
				lights[i].position.x += RandFloat(-thresh, thresh);
				break;
			}
		}

		/*float move_thresh = .5;
		XMFLOAT3 direction = camera.GetLook();
		direction.x *= -move_thresh;
		direction.y *= -move_thresh;
		direction.z *= -move_thresh;
		camera.Move(direction);*/

		// Move all the lights

		// DONT FUCK WITH THIS
		// Rendering shit right here
		// Output merger

		float color[4] = { 0, 0, 0, 1 };
		deviceContext->ClearRenderTargetView(renderTargetView, color);
		deviceContext->ClearRenderTargetView(gbuffer_target_diffuse, color);
		deviceContext->ClearRenderTargetView(gbuffer_target_specular, color);
		deviceContext->ClearRenderTargetView(gbuffer_target_normal, color);
		deviceContext->ClearRenderTargetView(gbuffer_target_position, color);
		deviceContext->ClearDepthStencilView(gbuffer_stencil_depth, D3D11_CLEAR_DEPTH, 1, 0);

		// Unset the the shaders
		ID3D11ShaderResourceView* sr[] = {
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr
		};
		deviceContext->PSSetShaderResources(0, 5, sr);

		ID3D11RenderTargetView* rtvs[] = {
			renderTargetView,
			gbuffer_target_diffuse,
			gbuffer_target_specular,
			gbuffer_target_normal,
			gbuffer_target_position
		};
		deviceContext->OMSetRenderTargets(ARRAYSIZE(rtvs), rtvs, gbuffer_stencil_depth);

		// Hacky AF - fix this
		UINT teapotstrides[] = { sizeof(SimpleVertex) };
		UINT teapotoffsets[] = { 0 };
		ID3D11Buffer* teapotVertexBuffers[] = { models[0].vertexBuffer };
		deviceContext->IASetVertexBuffers(0, 1, teapotVertexBuffers, teapotstrides, teapotoffsets);
		deviceContext->IASetIndexBuffer(models[0].indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		// Hacky AF - fix this

		// Rasterizer
		deviceContext->RSSetViewports(1, &viewport);
		deviceContext->PSSetSamplers(0, 1, &sampler_state);

		deviceContext->VSSetShader(vertexShader, 0, 0);
		deviceContext->GSSetShader(nullptr, 0, 0);
		deviceContext->PSSetShader(pixelShader, 0, 0);
		// TEAPOT
		// DONT FUCK WITH THIS

		for (int i = 0; i < models.size(); i++)
		{
			RenderModel(models[i]);
		}

		// Gbuffer shit
		RenderFullscreenQuad();
		// Gbuffer shit

		swapchain->Present(0, 0);
    }

	// Release a million fucking things
	D3DSAFE_RELEASE(swapchain);
	D3DSAFE_RELEASE(deviceContext);
	D3DSAFE_RELEASE(device);
	D3DSAFE_RELEASE(renderTargetView);
	D3DSAFE_RELEASE(constantBuffer);
	D3DSAFE_RELEASE(buffers.l_buffer);

	D3DSAFE_RELEASE(rasterizerStateDefault);
	D3DSAFE_RELEASE(rasterizerStateWireframe);
	D3DSAFE_RELEASE(sampler_state);

	D3DSAFE_RELEASE(zBuffer);
	D3DSAFE_RELEASE(depthStencil);

	D3DSAFE_RELEASE(vertexShader);
	D3DSAFE_RELEASE(pixelShader);

	for (int i = 0; i < models.size(); i++)
	{
		CleanupModel(models[i]);
	}
	CleanupFullscreenQuad();
	CleanupGbuffers();

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GRAPHICS2PROJECT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_GRAPHICS2PROJECT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(
	   szWindowClass, 
	   szTitle,
	   WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
	   CW_USEDEFAULT, 
	   0, 
	   CW_USEDEFAULT, 
	   0, 
	   nullptr, 
	   nullptr, 
	   hInstance, 
	   nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   RECT rect;
   GetClientRect(hWnd, &rect);
   screen_width = rect.right - rect.left;
   screen_height = rect.bottom - rect.top;;

   // Attach d3d to the window
   D3D_FEATURE_LEVEL DX11 = D3D_FEATURE_LEVEL_11_0;
   DXGI_SWAP_CHAIN_DESC swap;
   ZeroMemory(&swap, sizeof(DXGI_SWAP_CHAIN_DESC));
   swap.BufferCount = 1;
   swap.OutputWindow = hWnd;
   swap.Windowed = true;
   swap.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
   swap.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
   swap.BufferDesc.Width = screen_width;
   swap.BufferDesc.Height = screen_height;
   swap.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
   swap.SampleDesc.Count = 1;

   aspectRatio = swap.BufferDesc.Width / (float)swap.BufferDesc.Height;

   HRESULT result;

#ifdef _DEBUG
   result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, &DX11, 1, D3D11_SDK_VERSION, &swap, &swapchain, &device, 0, &deviceContext);
#else   
   result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, &DX11, 1, D3D11_SDK_VERSION, &swap, &swapchain, &device, 0, &deviceContext);
#endif

   ID3D11Resource* backbuffer;
   result = swapchain->GetBuffer(0, __uuidof(backbuffer), (void**)&backbuffer);
   result = device->CreateRenderTargetView(backbuffer, NULL, &renderTargetView);

   // Release the resource to decrement the counter by one
   // This is necessary to keep the thing from leaking memory
   backbuffer->Release();

   // Setup viewport
   viewport.Width = swap.BufferDesc.Width;
   viewport.Height = swap.BufferDesc.Height;
   viewport.TopLeftY = viewport.TopLeftX = 0;
   viewport.MinDepth = 0;
   viewport.MaxDepth = 1;

   // Rasterizer state
   D3D11_RASTERIZER_DESC rdesc;
   ZeroMemory(&rdesc, sizeof(D3D11_RASTERIZER_DESC));
   rdesc.FrontCounterClockwise = false;
   rdesc.DepthBiasClamp = 1;
   rdesc.DepthBias = rdesc.SlopeScaledDepthBias = 0;
   rdesc.DepthClipEnable = true;
   rdesc.FillMode = D3D11_FILL_SOLID;
   rdesc.CullMode = D3D11_CULL_BACK;
   rdesc.AntialiasedLineEnable = false;
   rdesc.MultisampleEnable = false;

   device->CreateRasterizerState(&rdesc, &rasterizerStateDefault);

   // Wire frame Rasterizer State
   ZeroMemory(&rdesc, sizeof(D3D11_RASTERIZER_DESC));
   rdesc.FillMode = D3D11_FILL_WIREFRAME;
   rdesc.CullMode = D3D11_CULL_NONE;
   rdesc.DepthClipEnable = true;

   device->CreateRasterizerState(&rdesc, &rasterizerStateWireframe);

   deviceContext->RSSetState(rasterizerStateDefault);

   // Initialize camera
   camera.SetPosition(XMFLOAT3(0, 0, -50));
   camera.SetFOV(30);

   // Create texture sample state
   D3D11_SAMPLER_DESC sdesc;
   ZeroMemory(&sdesc, sizeof(D3D11_SAMPLER_DESC));
   sdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
   sdesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
   sdesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
   sdesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
   sdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
   sdesc.MinLOD = 0;
   sdesc.MaxLOD = D3D11_FLOAT32_MAX;
   // Create the sampler state
   result = device->CreateSamplerState(&sdesc, &sampler_state);
   ASSERT_HRESULT_SUCCESS(result);

   srand(time(NULL));

   // Lights
   // Set up directional light
   Light dirLight1;
   ZeroMemory(&dirLight1, sizeof(Light));
   XMFLOAT4 color(0, 0, 1, 1);
   dirLight1.lightType = (unsigned int)LIGHTTYPE::POINT;
   dirLight1.diffuse = color;
   dirLight1.specular = color;
   dirLight1.ambientIntensity = 0.1f;
   dirLight1.diffuseIntensity = 0.2;
   dirLight1.specularIntensity = 0.4;
   dirLight1.position = XMFLOAT4(0, 0, 10, 1);
   dirLight1.lightDirection = XMFLOAT4(-1, -1, 1, 1);
   dirLight1.lightRadius = 20;

   // 10 lights
   int LIGHT_COUNT = 250;
   lights.resize((size_t)LIGHT_COUNT);

   for (int i = 0; i < LIGHT_COUNT; i++)
   {
	   dirLight1.lightRadius = RandFloat(1, 10);
	   dirLight1.diffuseIntensity = RandFloat(0, .01);
	   dirLight1.specularIntensity = RandFloat(0, .05);
	   dirLight1.ambientIntensity = RandFloat(0, .05);
	   dirLight1.position = RandFLOAT4XYZ(-30, 30);
	   dirLight1.diffuse = dirLight1.specular = dirLight1.ambient = RandFLOAT4XYZ(0, 1);
	   lights[i] = dirLight1;
   }

   // Resize to avoid buffer wanring
   if (lights.size() < 4) lights.resize(4);

   // Lights

   // TEAPOT
   Model temp;
   int AXIS_LIMIT = 5;
   float SEP_FACTOR = 3;

   // Load shaders
   result = device->CreateVertexShader(base_vs, sizeof(base_vs), nullptr, &vertexShader);
   ASSERT_HRESULT_SUCCESS(result);
   result = device->CreatePixelShader(base_ps, sizeof(base_ps), nullptr, &pixelShader);
   ASSERT_HRESULT_SUCCESS(result);

   for (int x = 0; x < AXIS_LIMIT; x++)
   {
	   for (int y = 0; y < AXIS_LIMIT; y++)
	   {
		   for (int z = 0; z < AXIS_LIMIT; z++)
		   {
			   InitializeModel(temp, "assets/models/teapot.wobj", XMFLOAT3(x * SEP_FACTOR, y * SEP_FACTOR, z * SEP_FACTOR), base_vs, base_ps, sizeof(base_vs), sizeof(base_ps));
			   models.push_back(temp);
			   
			   InitializeModel(temp, "assets/models/teapot.wobj", XMFLOAT3(x * -SEP_FACTOR, y * SEP_FACTOR, z * SEP_FACTOR), base_vs, base_ps, sizeof(base_vs), sizeof(base_ps));
			   models.push_back(temp);
			   
			   InitializeModel(temp, "assets/models/teapot.wobj", XMFLOAT3(x * SEP_FACTOR, y * -SEP_FACTOR, z * SEP_FACTOR), base_vs, base_ps, sizeof(base_vs), sizeof(base_ps));
			   models.push_back(temp);
			   
			   InitializeModel(temp, "assets/models/teapot.wobj", XMFLOAT3(x * -SEP_FACTOR, y * -SEP_FACTOR, z * SEP_FACTOR), base_vs, base_ps, sizeof(base_vs), sizeof(base_ps));
			   models.push_back(temp);
		   }
	   }
   }

   InitializeFullscreenQuad(post_vs, post_gm, post_ps, sizeof(post_vs), sizeof(post_gm), sizeof(post_ps));
   // TEAPOT

   // Gbuffer shit
   InitializeGubffers();
   // Gbuffer shit

   deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   // Create constant buffer
   D3D11_BUFFER_DESC bDesc;
   D3D11_SUBRESOURCE_DATA subdata;
   ZeroMemory(&bDesc, sizeof(D3D11_BUFFER_DESC));
   ZeroMemory(&subdata, sizeof(D3D11_SUBRESOURCE_DATA));

   bDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
   bDesc.ByteWidth = sizeof(WorldViewProjection);
   bDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
   bDesc.MiscFlags = 0;
   bDesc.StructureByteStride = 0;
   bDesc.Usage = D3D11_USAGE_DYNAMIC;

   result = device->CreateBuffer(&bDesc, nullptr, &constantBuffer);
   ASSERT_HRESULT_SUCCESS(result);

   // Create light buffer
   D3D11_BUFFER_DESC lbuffer;
   D3D11_SUBRESOURCE_DATA lsubdata;
   ZeroMemory(&lbuffer, sizeof(D3D11_BUFFER_DESC));
   ZeroMemory(&lsubdata, sizeof(D3D11_SUBRESOURCE_DATA));

   lbuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
   lbuffer.ByteWidth = sizeof(Light) * lights.size();
   lbuffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
   lbuffer.MiscFlags = 0;
   lbuffer.StructureByteStride = 0;
   lbuffer.Usage = D3D11_USAGE_DYNAMIC;

   result = device->CreateBuffer(&lbuffer, nullptr, &buffers.l_buffer);
   ASSERT_HRESULT_SUCCESS(result);

   // Light buffer
   D3D11_MAPPED_SUBRESOURCE gpuBuffer;
   result = deviceContext->Map(buffers.l_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &gpuBuffer);
   memcpy(gpuBuffer.pData, lights.data(), sizeof(Light) * lights.size());
   deviceContext->Unmap(buffers.l_buffer, 0);
   // Connect constant buffer to the pipeline
   ID3D11Buffer* lightbuffers[] = { buffers.l_buffer };
   deviceContext->PSSetConstantBuffers(0, 1, lightbuffers);
   // Light buffer

   return TRUE;
}

// Gbuffer shit
void GbufferPass()
{

}

void InitializeGubffers()
{
	HRESULT result;

	// Depth
	{
		// Texture
		D3D11_TEXTURE2D_DESC depth_tex_desc;
		ZeroMemory(&depth_tex_desc, sizeof(D3D11_TEXTURE2D_DESC));
		depth_tex_desc.Width = screen_width;
		depth_tex_desc.Height = screen_height;
		depth_tex_desc.MipLevels = 1;
		depth_tex_desc.ArraySize = 1;
		depth_tex_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		depth_tex_desc.SampleDesc.Count = 1;
		depth_tex_desc.SampleDesc.Quality = 0;
		depth_tex_desc.Usage = D3D11_USAGE_DEFAULT;
		depth_tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

		result = device->CreateTexture2D(&depth_tex_desc, nullptr, &gbuffer_depth);
		ASSERT_HRESULT_SUCCESS(result);

		// Create depth stencil
		D3D11_DEPTH_STENCIL_VIEW_DESC depth_ds_desc;
		ZeroMemory(&depth_ds_desc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
		depth_ds_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depth_ds_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depth_ds_desc.Texture2D.MipSlice = 0;

		result = device->CreateDepthStencilView(gbuffer_depth, &depth_ds_desc, &gbuffer_stencil_depth);
		ASSERT_HRESULT_SUCCESS(result);

		// Shader resource view
		D3D11_SHADER_RESOURCE_VIEW_DESC depth_sr_desc;
		ZeroMemory(&depth_sr_desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

		depth_sr_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		depth_sr_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		depth_sr_desc.Texture2D.MostDetailedMip = 0;
		depth_sr_desc.Texture2D.MipLevels = 1;

		result = device->CreateShaderResourceView(gbuffer_depth, &depth_sr_desc, &gbuffer_resource_depth);
		ASSERT_HRESULT_SUCCESS(result);
	}

	// Diffuse
	{
		// Texture
		D3D11_TEXTURE2D_DESC diffuse_tex_desc;
		ZeroMemory(&diffuse_tex_desc, sizeof(D3D11_TEXTURE2D_DESC));
		diffuse_tex_desc.Width = screen_width;
		diffuse_tex_desc.Height = screen_height;
		diffuse_tex_desc.MipLevels = 1;
		diffuse_tex_desc.ArraySize = 1;
		diffuse_tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		diffuse_tex_desc.SampleDesc.Count = 1;
		diffuse_tex_desc.SampleDesc.Quality = 0;
		diffuse_tex_desc.Usage = D3D11_USAGE_DEFAULT;
		diffuse_tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		result = device->CreateTexture2D(&diffuse_tex_desc, nullptr, &gbuffer_diffuse);
		ASSERT_HRESULT_SUCCESS(result);

		// Shader resource view
		D3D11_SHADER_RESOURCE_VIEW_DESC diffuse_sr_desc;
		ZeroMemory(&diffuse_sr_desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

		diffuse_sr_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		diffuse_sr_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		diffuse_sr_desc.Texture2D.MostDetailedMip = 0;
		diffuse_sr_desc.Texture2D.MipLevels = 1;

		result = device->CreateShaderResourceView(gbuffer_diffuse, &diffuse_sr_desc, &gbuffer_resource_diffuse);
		ASSERT_HRESULT_SUCCESS(result);

		// Create render target
		D3D11_RENDER_TARGET_VIEW_DESC RT_DESC;
		ZeroMemory(&RT_DESC, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));

		RT_DESC.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		RT_DESC.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		RT_DESC.Texture2D.MipSlice = 0;

		result = device->CreateRenderTargetView(gbuffer_diffuse, &RT_DESC, &gbuffer_target_diffuse);
		ASSERT_HRESULT_SUCCESS(result);
	}

	// Specular
	{
		// Texture
		D3D11_TEXTURE2D_DESC specular_tex_desc;
		ZeroMemory(&specular_tex_desc, sizeof(D3D11_TEXTURE2D_DESC));
		specular_tex_desc.Width = screen_width;
		specular_tex_desc.Height = screen_height;
		specular_tex_desc.MipLevels = 1;
		specular_tex_desc.ArraySize = 1;
		specular_tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		specular_tex_desc.SampleDesc.Count = 1;
		specular_tex_desc.SampleDesc.Quality = 0;
		specular_tex_desc.Usage = D3D11_USAGE_DEFAULT;
		specular_tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		result = device->CreateTexture2D(&specular_tex_desc, nullptr, &gbuffer_specular);
		ASSERT_HRESULT_SUCCESS(result);

		// Shader resource view
		D3D11_SHADER_RESOURCE_VIEW_DESC specular_sr_desc;
		ZeroMemory(&specular_sr_desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

		specular_sr_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		specular_sr_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		specular_sr_desc.Texture2D.MostDetailedMip = 0;
		specular_sr_desc.Texture2D.MipLevels = 1;

		result = device->CreateShaderResourceView(gbuffer_specular, &specular_sr_desc, &gbuffer_resource_specular);
		ASSERT_HRESULT_SUCCESS(result);

		// Create render target
		D3D11_RENDER_TARGET_VIEW_DESC RT_DESC;
		ZeroMemory(&RT_DESC, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));

		RT_DESC.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		RT_DESC.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		RT_DESC.Texture2D.MipSlice = 0;

		result = device->CreateRenderTargetView(gbuffer_specular, &RT_DESC, &gbuffer_target_specular);
		ASSERT_HRESULT_SUCCESS(result);
	}

	// Normal
	{
		// Texture
		D3D11_TEXTURE2D_DESC normal_tex_desc;
		ZeroMemory(&normal_tex_desc, sizeof(D3D11_TEXTURE2D_DESC));
		normal_tex_desc.Width = screen_width;
		normal_tex_desc.Height = screen_height;
		normal_tex_desc.MipLevels = 1;
		normal_tex_desc.ArraySize = 1;
		normal_tex_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		normal_tex_desc.SampleDesc.Count = 1;
		normal_tex_desc.SampleDesc.Quality = 0;
		normal_tex_desc.Usage = D3D11_USAGE_DEFAULT;
		normal_tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		result = device->CreateTexture2D(&normal_tex_desc, nullptr, &gbuffer_normal);
		ASSERT_HRESULT_SUCCESS(result);

		// Shader resource view
		D3D11_SHADER_RESOURCE_VIEW_DESC normal_sr_desc;
		ZeroMemory(&normal_sr_desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

		normal_sr_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		normal_sr_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		normal_sr_desc.Texture2D.MostDetailedMip = 0;
		normal_sr_desc.Texture2D.MipLevels = 1;

		result = device->CreateShaderResourceView(gbuffer_normal, &normal_sr_desc, &gbuffer_resource_normal);
		ASSERT_HRESULT_SUCCESS(result);

		// Create render target
		D3D11_RENDER_TARGET_VIEW_DESC RT_DESC;
		ZeroMemory(&RT_DESC, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));

		RT_DESC.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		RT_DESC.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		RT_DESC.Texture2D.MipSlice = 0;

		result = device->CreateRenderTargetView(gbuffer_normal, &RT_DESC, &gbuffer_target_normal);
		ASSERT_HRESULT_SUCCESS(result);
	}

	// Position
	{
		// Texture
		D3D11_TEXTURE2D_DESC position_tex_desc;
		ZeroMemory(&position_tex_desc, sizeof(D3D11_TEXTURE2D_DESC));
		position_tex_desc.Width = screen_width;
		position_tex_desc.Height = screen_height;
		position_tex_desc.MipLevels = 1;
		position_tex_desc.ArraySize = 1;
		position_tex_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		position_tex_desc.SampleDesc.Count = 1;
		position_tex_desc.SampleDesc.Quality = 0;
		position_tex_desc.Usage = D3D11_USAGE_DEFAULT;
		position_tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		result = device->CreateTexture2D(&position_tex_desc, nullptr, &gbuffer_position);
		ASSERT_HRESULT_SUCCESS(result);

		// Shader resource view
		D3D11_SHADER_RESOURCE_VIEW_DESC position_sr_desc;
		ZeroMemory(&position_sr_desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

		position_sr_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		position_sr_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		position_sr_desc.Texture2D.MostDetailedMip = 0;
		position_sr_desc.Texture2D.MipLevels = 1;

		result = device->CreateShaderResourceView(gbuffer_position, &position_sr_desc, &gbuffer_resource_position);
		ASSERT_HRESULT_SUCCESS(result);

		// Create render target
		D3D11_RENDER_TARGET_VIEW_DESC RT_DESC;
		ZeroMemory(&RT_DESC, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));

		RT_DESC.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		RT_DESC.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		RT_DESC.Texture2D.MipSlice = 0;

		result = device->CreateRenderTargetView(gbuffer_position, &RT_DESC, &gbuffer_target_position);
		ASSERT_HRESULT_SUCCESS(result);
	}
}

void CleanupGbuffers()
{
	D3DSAFE_RELEASE(gbuffer_depth);
	D3DSAFE_RELEASE(gbuffer_diffuse);
	D3DSAFE_RELEASE(gbuffer_specular);
	D3DSAFE_RELEASE(gbuffer_normal);
	D3DSAFE_RELEASE(gbuffer_position);
	
	D3DSAFE_RELEASE(gbuffer_resource_depth);
	D3DSAFE_RELEASE(gbuffer_resource_diffuse);
	D3DSAFE_RELEASE(gbuffer_resource_specular);
	D3DSAFE_RELEASE(gbuffer_resource_normal);
	D3DSAFE_RELEASE(gbuffer_resource_position);
	
	D3DSAFE_RELEASE(gbuffer_target_diffuse);
	D3DSAFE_RELEASE(gbuffer_target_specular);
	D3DSAFE_RELEASE(gbuffer_target_normal);
	D3DSAFE_RELEASE(gbuffer_target_position);
	
	D3DSAFE_RELEASE(gbuffer_stencil_depth);
}
// Gbuffer shit

void CleanupModel(Model& _model)
{
	// teapot cleanup
	D3DSAFE_RELEASE(_model.indexBuffer);
	D3DSAFE_RELEASE(_model.vertexBuffer);
	D3DSAFE_RELEASE(_model.vertexBufferLayout);
	// teapot cleanup
}

void CleanupFullscreenQuad()
{
	D3DSAFE_RELEASE(QUAD.pixelShader);
	D3DSAFE_RELEASE(QUAD.vertexShader);
	D3DSAFE_RELEASE(QUAD.geometryShader);
	D3DSAFE_RELEASE(QUAD.vertexBufferLayout);
	D3DSAFE_RELEASE(QUAD.vertexBuffer);
}

void RenderModel(Model& _model)
{
	// TEAPOT
		// World matrix projection
	XMMATRIX temp = XMMatrixIdentity();
	temp = XMMatrixMultiply(temp, XMMatrixScaling(1, 1, 1));
	temp = XMMatrixMultiply(temp, XMMatrixTranslation(_model.position.x, _model.position.y, _model.position.z));
	XMStoreFloat4x4(&WORLD.WorldMatrix, temp);
	// View
	camera.GetViewMatrix(temp);
	XMStoreFloat4x4(&WORLD.ViewMatrix, temp);
	// Projection
	temp = XMMatrixPerspectiveFovLH(camera.GetFOV(), aspectRatio, 0.1f, 1000);
	XMStoreFloat4x4(&WORLD.ProjectionMatrix, temp);
	// Camera position
	WORLD.camera_position = camera.GetPosition();

	// Send the matrix to constant buffer
	D3D11_MAPPED_SUBRESOURCE gpuBuffer;
	HRESULT result = deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &gpuBuffer);
	memcpy(gpuBuffer.pData, &WORLD, sizeof(WORLD));
	deviceContext->Unmap(constantBuffer, 0);
	// Connect constant buffer to the pipeline
	ID3D11Buffer* teapotCBuffers[] = { constantBuffer };
	deviceContext->VSSetConstantBuffers(0, 1, teapotCBuffers);

	deviceContext->IASetInputLayout(_model.vertexBufferLayout);
	deviceContext->DrawIndexed(_model.indices.size(), 0, 0);
}

void RenderFullscreenQuad()
{
	// Send the matrix to constant buffer
	POST.campos = camera.GetPosition();

	D3D11_MAPPED_SUBRESOURCE gpuBuffer;
	HRESULT result = deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &gpuBuffer);
	memcpy(gpuBuffer.pData, &POST, sizeof(POST));
	deviceContext->Unmap(constantBuffer, 0);
	// Connect constant buffer to the pipeline
	ID3D11Buffer* teapotCBuffers[] = { constantBuffer };
	deviceContext->VSSetConstantBuffers(0, 1, teapotCBuffers);

	ID3D11RenderTargetView* rtvs[] = {
		renderTargetView,
		nullptr,
		nullptr,
		nullptr,  
		nullptr
	}; 

	deviceContext->OMSetRenderTargets(ARRAYSIZE(rtvs), rtvs, nullptr);

	ID3D11ShaderResourceView* gbuffer[] = {
			gbuffer_resource_depth,
			gbuffer_resource_diffuse,
			gbuffer_resource_normal,
			gbuffer_resource_specular,
			gbuffer_resource_position
	};
	deviceContext->PSSetShaderResources(0, ARRAYSIZE(gbuffer), gbuffer);

	UINT quadstrides[] = { sizeof(QVertex) };
	UINT quadoffsets[] = { 0 };
	ID3D11Buffer* quadvbuffer[] = { QUAD.vertexBuffer };
	deviceContext->IASetVertexBuffers(0, 1, quadvbuffer, quadstrides, quadoffsets);
	deviceContext->VSSetShader(QUAD.vertexShader, 0, 0);
	deviceContext->PSSetShader(QUAD.pixelShader, 0, 0);
	deviceContext->IASetInputLayout(QUAD.vertexBufferLayout);

	deviceContext->Draw(QUAD.positions.size(), 0);
}

void InitializeFullscreenQuad(const BYTE* _vs, const BYTE* _gs, const BYTE* _ps, int vs_size, int gs_size, int ps_size)
{
	// Generate positions
	QUAD.positions.push_back({ FLOAT3{ -1, -1, 0 }, FLOAT2{ 0, 1 } });
	QUAD.positions.push_back({ FLOAT3{ -1,  1, 0 }, FLOAT2{ 0, 0 } });
	QUAD.positions.push_back({ FLOAT3{  1,  1, 0 }, FLOAT2{ 1, 0 } });

	QUAD.positions.push_back({ FLOAT3{ -1, -1, 0 }, FLOAT2{ 0, 1 } });
	QUAD.positions.push_back({ FLOAT3{  1,  1, 0 }, FLOAT2{ 1, 0 } });
	QUAD.positions.push_back({ FLOAT3{  1, -1, 0 }, FLOAT2{ 1, 1 } });

	// Vertex buffer
	D3D11_BUFFER_DESC bufferDesc;
	D3D11_SUBRESOURCE_DATA subdata;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&subdata, sizeof(D3D11_SUBRESOURCE_DATA));
	// Actually load data
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.ByteWidth = sizeof(QVertex) * QUAD.positions.size();
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	subdata.pSysMem = QUAD.positions.data();

	HRESULT result = device->CreateBuffer(&bufferDesc, &subdata, &QUAD.vertexBuffer);
	ASSERT_HRESULT_SUCCESS(result);

	// Load shaders
	result = device->CreateVertexShader(_vs, vs_size, nullptr, &QUAD.vertexShader);
	ASSERT_HRESULT_SUCCESS(result);
	result = device->CreateGeometryShader(_gs, gs_size, nullptr, &QUAD.geometryShader);
	ASSERT_HRESULT_SUCCESS(result);
	result = device->CreatePixelShader(_ps, ps_size, nullptr, &QUAD.pixelShader);
	ASSERT_HRESULT_SUCCESS(result);

	// Make input layout for vertex buffer
	D3D11_INPUT_ELEMENT_DESC tempInputElementDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	result = device->CreateInputLayout(tempInputElementDesc, 2, _vs, vs_size, &QUAD.vertexBufferLayout);
	ASSERT_HRESULT_SUCCESS(result);
}

void InitializeModel(Model& _model, const char* modelname, XMFLOAT3 position, const BYTE* _vs, const BYTE* _ps, int vs_size, int ps_size)
{
	// TEAPOT
   // Set position
	_model.position = position;
	// Load mesh data
	D3D11_BUFFER_DESC bufferDesc;
	D3D11_SUBRESOURCE_DATA subdata;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&subdata, sizeof(D3D11_SUBRESOURCE_DATA));
	// Actually load data
	LoadWobjectMesh(modelname, _model.vertices, _model.indices);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.ByteWidth = sizeof(SimpleVertex) * _model.vertices.size();
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	subdata.pSysMem = _model.vertices.data();

	HRESULT result = device->CreateBuffer(&bufferDesc, &subdata, &_model.vertexBuffer);
	ASSERT_HRESULT_SUCCESS(result);

	// Index Buffer
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.ByteWidth = sizeof(int) * _model.indices.size();

	subdata.pSysMem = _model.indices.data();
	result = device->CreateBuffer(&bufferDesc, &subdata, &_model.indexBuffer);
	ASSERT_HRESULT_SUCCESS(result);

	// Make input layout for vertex buffer
	D3D11_INPUT_ELEMENT_DESC tempInputElementDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	result = device->CreateInputLayout(tempInputElementDesc, 3, _vs, vs_size, &_model.vertexBufferLayout);
	ASSERT_HRESULT_SUCCESS(result);
	// TEAPOT
}

void LoadWobjectMesh(const char* meshname, std::vector<SimpleVertex>& vertices, std::vector<int>& indices)
{
	MeshHeader header;

	std::string smeshname = meshname;

	assert(smeshname.find(".fbx") == std::string::npos);

	std::ifstream file;
	file.open(meshname, std::ios::binary | std::ios::in);
	assert(file.is_open());

	// Read in the header
	file.read((char*)&header, sizeof(MeshHeader));
	std::string s = header.texturename;

	// Create a buffer to hold the data
	char* buffer = new char[header.vertexcount * (size_t)sizeof(SimpleVertex)];

	// Read in the indices
	file.read((char*)buffer, header.indexcount * (size_t)sizeof(int));
	indices.resize(header.indexcount);
	memcpy(indices.data(), buffer, sizeof(int) * header.indexcount);

	// Read in the vertices
	file.read((char*)buffer, header.vertexcount * (size_t)sizeof(SimpleVertex));
	vertices.resize(header.vertexcount);
	memcpy(vertices.data(), buffer, sizeof(SimpleVertex) * header.vertexcount);

	// Free memory
	delete[] buffer;

	file.close();
}

void HandleInput()
{
	static float move_thresh = .1;
	static float look_thresh = .09;
	XMFLOAT3 direction;

	if (InputManager::IsKeyDown((int)BUTTONS::NUM_THREE))
	{
		POST.mode = 3;
	}
	if (InputManager::IsKeyDown((int)BUTTONS::NUM_FOUR))
	{
		POST.mode = 4;
	}
	if (InputManager::IsKeyDown((int)BUTTONS::NUM_FIVE))
	{
		POST.mode = 5;
	}
	if (InputManager::IsKeyDown((int)BUTTONS::NUM_SIX))
	{
		POST.mode = 6;
	}
	if (InputManager::IsKeyDown((int)BUTTONS::NUM_SEVEN))
	{
		POST.mode = 7;
	}

	if(InputManager::IsKeyDown((int)BUTTONS::NUM_ONE))
	{
		deviceContext->RSSetState(rasterizerStateWireframe);
	}
	else if (InputManager::IsKeyDown((int)BUTTONS::NUM_TWO))
	{
		deviceContext->RSSetState(rasterizerStateDefault);
	}
	// Movement
	if (InputManager::IsKeyDown((int)BUTTONS::LETTER_W))
	{
		direction = camera.GetLook();
		direction.x *= move_thresh;
		direction.y *= move_thresh;
		direction.z *= move_thresh;
		camera.Move(direction);
	}
	else if (InputManager::IsKeyDown((int)BUTTONS::LETTER_S))
	{
		direction = camera.GetLook();
		direction.x *= -move_thresh;
		direction.y *= -move_thresh;
		direction.z *= -move_thresh;
		camera.Move(direction);
	} 
	else if (InputManager::IsKeyDown((int)BUTTONS::LETTER_A))
	{
		direction = camera.GetRight();
		direction.x *= move_thresh;
		direction.y *= move_thresh;
		direction.z *= move_thresh;
		camera.Move(direction);
	}
	else if (InputManager::IsKeyDown((int)BUTTONS::LETTER_D))
	{
		direction = camera.GetRight();
		direction.x *= -move_thresh;
		direction.y *= -move_thresh;
		direction.z *= -move_thresh;
		camera.Move(direction);
	}
	else if (InputManager::IsKeyDown((int)BUTTONS::LETTER_Z))
	{
		camera.Move(XMFLOAT3(0, move_thresh, 0));
	}
	else if (InputManager::IsKeyDown((int)BUTTONS::LETTER_X))
	{
		camera.Move(XMFLOAT3(0, -move_thresh, 0));
	}

	// Looking
	if (InputManager::IsKeyDown(VK_UP))
	{
		camera.Rotate(0, look_thresh);
	}
	else if (InputManager::IsKeyDown(VK_DOWN))
	{
		camera.Rotate(0, -look_thresh);
	}
	else if (InputManager::IsKeyDown(VK_LEFT))
	{
		camera.Rotate(look_thresh, 0);
	}
	else if (InputManager::IsKeyDown(VK_RIGHT))
	{
		camera.Rotate(-look_thresh, 0);
	}
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned int thing;
    switch (message)
    {
	case WM_KEYDOWN:
	{
		thing = (unsigned int)wParam;
		InputManager::SetKeyDown((unsigned int)(wParam));
		break;
	}
	case WM_KEYUP:
	{
		thing = (unsigned int)wParam;
		InputManager::SetKeyUp((unsigned int)(wParam));
		break;
	}
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}