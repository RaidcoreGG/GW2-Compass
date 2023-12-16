#include "render3D.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#ifdef _MSC_VER
#pragma comment(lib, "d3dcompiler") // Automatically link with d3dcompiler.lib as we are using D3DCompile() below.
#endif
using namespace DirectX;

HRESULT hr;

ID3D11RenderTargetView* renderTargetView;
ID3D11Buffer* squareIndexBuffer;
ID3D11DepthStencilView* depthStencilView;
ID3D11Texture2D* depthStencilBuffer;
ID3D11Buffer* squareVertBuffer;
ID3D11VertexShader* VS;
ID3D11PixelShader* PS;
ID3D10Blob* VS_Buffer;
ID3D10Blob* PS_Buffer;
ID3D11InputLayout* vertLayout;
ID3D11Buffer* cbPerObjectBuffer;

XMMATRIX WVP;
XMMATRIX worldView;
XMMATRIX camView;
XMMATRIX camProjection;

XMVECTOR camPosition;
XMVECTOR camTarget;
XMVECTOR camUp;

XMMATRIX Rotation;
XMMATRIX Scale;
XMMATRIX Translation;
float rot = 0.00f;


//Create effects constant buffer's structure//
struct cbPerObject
{
	XMMATRIX  WVP;
};

cbPerObject cbPerObj;

//Vertex Structure and Vertex Layout (Input Layout)//
struct Vertex	//Overloaded Vertex Structure
{
	Vertex() {}
	Vertex(float x, float y, float z,
		float cr, float cg, float cb, float ca)
		: pos(x, y, z), color(cr, cg, cb, ca) {}

	XMFLOAT3 pos;
	XMFLOAT4 color;
};

D3D11_INPUT_ELEMENT_DESC layout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
UINT numElements = ARRAYSIZE(layout);


void init_app() {
	ImGuiIO& io = ImGui::GetIO();

	//Describe our SwapChain Buffer
	DXGI_MODE_DESC bufferDesc;

	ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));

	bufferDesc.Width = io.DisplaySize.x;
	bufferDesc.Height = io.DisplaySize.y;
	bufferDesc.RefreshRate.Numerator = 60;
	bufferDesc.RefreshRate.Denominator = 1;
	bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	//Create our BackBuffer
	ID3D11Texture2D* BackBuffer;
	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);

	//Create our Render Target
	hr = device->CreateRenderTargetView(BackBuffer, NULL, &renderTargetView);
	BackBuffer->Release();

	//Describe our Depth/Stencil Buffer
	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width = io.DisplaySize.x;
	depthStencilDesc.Height = io.DisplaySize.y;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	//Create the Depth/Stencil View
	device->CreateTexture2D(&depthStencilDesc, NULL, &depthStencilBuffer);
	device->CreateDepthStencilView(depthStencilBuffer, NULL, &depthStencilView);

	//Set our Render Target
	deviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
}


void init_scene() {
	ImGuiIO& io = ImGui::GetIO();

	// Shared Vertex and pixel shader
	const CHAR* shaderString =
		"cbuffer cbPerObject\n"
		"{\n"
		"    float4x4 WVP;\n"
		"};\n"
		"\n"
		"struct VS_OUTPUT\n"
		"{\n"
		"    float4 Pos : SV_POSITION;\n"
		"    float4 Color : COLOR;\n"
		"};\n"
		"\n"
		"VS_OUTPUT VS(float4 inPos : POSITION, float4 inColor : COLOR)\n"
		"{\n"
		"    VS_OUTPUT output;\n"
		"    output.Pos = mul(inPos, WVP);\n"
		"    output.Color = inColor;\n"
		"\n"
		"    return output;\n"
		"}\n"
		"\n"
		"float4 PS(VS_OUTPUT input) : SV_TARGET\n"
		"{\n"
		"    return input.Color;\n"
		"}\n";

	//Compile Shaders from shader file
	HRESULT hr = D3DCompile(
		shaderString,            // Shader source code as a string
		strlen(shaderString),    // Length of the shader source string
		nullptr,                 // Source name (used for error messages)
		nullptr,                 // Macro definitions (if any)
		nullptr,                 // Include interface (if any)
		"VS",                    // Entry point function name for vertex shader
		"vs_4_0",                // Shader profile for vertex shader (e.g., vs_4_0)
		0,                       // Compile options
		0,                       // Effect flags
		&VS_Buffer,              // Output compiled shader bytecode
		nullptr                  // Error messages (not used in this example)
	);

	hr = D3DCompile(
		shaderString,            // Shader source code as a string
		strlen(shaderString),    // Length of the shader source string
		nullptr,                 // Source name (used for error messages)
		nullptr,                 // Macro definitions (if any)
		nullptr,                 // Include interface (if any)
		"PS",                    // Entry point function name for pixel shader
		"ps_4_0",                // Shader profile for pixel shader (e.g., ps_4_0)
		0,                       // Compile options
		0,                       // Effect flags
		&PS_Buffer,              // Output compiled shader bytecode
		nullptr                  // Error messages (not used in this example)
	);

	//Create the Shader Objects
	hr = device->CreateVertexShader(VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), NULL, &VS);
	hr = device->CreatePixelShader(PS_Buffer->GetBufferPointer(), PS_Buffer->GetBufferSize(), NULL, &PS);

	//Set Vertex and Pixel Shaders
	deviceContext->VSSetShader(VS, 0, 0);
	deviceContext->PSSetShader(PS, 0, 0);

	///////////////**************new**************////////////////////
	//Create the vertex buffer
	Vertex v[] =
	{
		// 0
		Vertex(
			-1.0f, -1.0f, -1.0f,    // position
			1.0f, 0.0f, 0.0f, 1.0f   // color
		),
		// 1
		Vertex(
			-1.0f, +1.0f, -1.0f,    // position
			0.0f, 1.0f, 0.0f, 1.0f  // color
		),
		// 2
		Vertex(
			+1.0f, +1.0f, -1.0f,    // position
			0.0f, 0.0f, 1.0f, 1.0f   // color
		),
		// 3
		Vertex(
			+1.0f, -1.0f, -1.0f,    // position
			1.0f, 1.0f, 0.0f, 1.0f  // color
		),
		// 4
		Vertex(
			-1.0f, -1.0f, +1.0f,    // position
			1.0f, 1.0f, 0.0f, 1.0f  // color
		),
		// 5
		Vertex(
			-1.0f, +1.0f, +1.0f,    // position
			1.0f, 0.0f, 1.0f, 1.0f   // color
		),
		// 6
		Vertex(
			+1.0f, +1.0f, +1.0f,    // position
			0.0f, 1.0f, 1.0f, 1.0f   // color
		),
		// 7
		Vertex(
			+1.0f, -1.0f, +1.0f,    // position
			0.5f, 0.5f, 0.5f, 1.0f  // color
		),
	};

	DWORD indices[] = {
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(DWORD) * 12 * 3;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;

	iinitData.pSysMem = indices;
	device->CreateBuffer(&indexBufferDesc, &iinitData, &squareIndexBuffer);

	deviceContext->IASetIndexBuffer(squareIndexBuffer, DXGI_FORMAT_R32_UINT, 0);


	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * 8;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	///////////////**************new**************////////////////////

	D3D11_SUBRESOURCE_DATA vertexBufferData;

	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
	vertexBufferData.pSysMem = v;
	hr = device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &squareVertBuffer);

	//Set the vertex buffer
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &squareVertBuffer, &stride, &offset);

	//Create the Input Layout
	hr = device->CreateInputLayout(layout, numElements, VS_Buffer->GetBufferPointer(),
		VS_Buffer->GetBufferSize(), &vertLayout);

	//Set the Input Layout
	deviceContext->IASetInputLayout(vertLayout);

	//Set Primitive Topology
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//Create the Viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = io.DisplaySize.x;
	viewport.Height = io.DisplaySize.y;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	//Set the Viewport
	deviceContext->RSSetViewports(1, &viewport);

	//Create the buffer to send to the cbuffer in effect file
	D3D11_BUFFER_DESC cbbd;
	ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));

	cbbd.Usage = D3D11_USAGE_DEFAULT;
	cbbd.ByteWidth = sizeof(cbPerObject);
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;

	hr = device->CreateBuffer(&cbbd, NULL, &cbPerObjectBuffer);


	//Camera information

	XMVECTOR cameraPosition = XMVectorSet(MumbleLink->CameraPosition.X, MumbleLink->CameraPosition.Y, MumbleLink->CameraPosition.Z, 0.0f);
	XMVECTOR cameraFront = XMVectorSet(MumbleLink->CameraFront.X, MumbleLink->CameraFront.Y, MumbleLink->CameraFront.Z, 0.0f);
	XMVECTOR targetPosition = cameraPosition + cameraFront;

	camView = XMMatrixLookAtLH(cameraPosition, targetPosition, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

	//Set the Projection matrix
	camProjection = XMMatrixPerspectiveFovLH(XM_PIDIV4, io.DisplaySize.x / io.DisplaySize.y, 0.01f, 1000.0f);
}

float fX = 1.0f;
float fZ = 1.0f;
Vector3 ag{};
int lastMap = 0;

float xOriginWorld = 0.0f;
float zOriginWorld = 0.0f;

/* converts a map unit to a world one */
float convToWorld(float mapUnit)
{
	return ((mapUnit * 100) / 2.54f) / 24;
}
/* converts a world unit to a map one */
float convToMap(float worldUnit)
{
	return ((worldUnit * 24) * 2.54f) / 100;
}

void draw_scene() {
	Vector2 world = MumbleLink->Context.Compass.PlayerPosition;

	// also trigger if negative/positive changed and handle accordingly
	if (MumbleLink->Context.MapID != lastMap)
	{
		lastMap = MumbleLink->Context.MapID;

		if (MumbleLink->AvatarPosition.X >= 0) { xOriginWorld = world.X - convToWorld(MumbleLink->AvatarPosition.X); }
		else { xOriginWorld = world.X + abs(convToWorld(MumbleLink->AvatarPosition.X)); }

		if (MumbleLink->AvatarPosition.Z >= 0) { zOriginWorld = world.Y + convToWorld(MumbleLink->AvatarPosition.Z); }
		else { zOriginWorld = world.Y - abs(convToWorld(MumbleLink->AvatarPosition.Z)); }

		/*char buff[4096];
		char* p = &buff[0];
		p += _snprintf_s(p, 400, _TRUNCATE, "X: %f | Z: %f\n", xOriginWorld, zOriginWorld);
		APIDefs->Log(ELogLevel_CRITICAL, &buff[0]);*/
	}

	ag = {};
	ag.X = convToMap(world.X - xOriginWorld);
	ag.Y = MumbleLink->AvatarPosition.Y;
	ag.Z = convToMap(world.Y - zOriginWorld) * -1;

	/*char buff2[4096];
	char* p = &buff2[0];
	p += _snprintf_s(p, 400, _TRUNCATE, "x%f | z%f", ag.X, ag.Z);
	APIDefs->Log(ELogLevel_CRITICAL, &buff2[0]);*/

	//Clear our backbuffer
	//float bgColor[4] = { (0.0f, 0.0f, 0.0f, 0.0f) };
	//deviceContext->ClearRenderTargetView(renderTargetView, bgColor);

	//Refresh the Depth/Stencil view
	deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	worldView = XMMatrixIdentity();

	// Scale
	XMFLOAT3 scale = XMFLOAT3(1.0f, 0.0f, 1.0f);
	XMMATRIX scaleMatrix = XMMatrixScalingFromVector(XMLoadFloat3(&scale));

	worldView = XMMatrixMultiply(worldView, scaleMatrix);

	// Translate
	//XMFLOAT3 position = XMFLOAT3(-690.5883f, 50.3751f, -349.9916f);
	XMFLOAT3 position = XMFLOAT3(ag.X, ag.Y + 2.5f, ag.Z);
	XMMATRIX translationMatrix = XMMatrixTranslationFromVector(XMLoadFloat3(&position));

	worldView = XMMatrixMultiply(worldView, translationMatrix);

	// Rotation (example: rotating around the y-axis)
	//float angle = 0.0f; // Specify your desired rotation angle in radians
	float angle = atan2f(MumbleLink->CameraFront.X, MumbleLink->CameraFront.Z);
	XMMATRIX rotationMatrix = XMMatrixRotationY(angle);

	XMMATRIX objectWorldView = scaleMatrix * rotationMatrix * translationMatrix;

	//Set the WVP matrix and send it to the constant buffer in effect file
	WVP = objectWorldView * camView * camProjection;
	cbPerObj.WVP = XMMatrixTranspose(WVP);
	deviceContext->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &cbPerObj, 0, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);

	//Draw the first cube
	deviceContext->DrawIndexed(36, 0, 0);
}

void cleanup() {
	renderTargetView->Release();
	squareVertBuffer->Release();
	squareIndexBuffer->Release();
	VS->Release();
	PS->Release();
	VS_Buffer->Release();
	PS_Buffer->Release();
	vertLayout->Release();
	depthStencilView->Release();
	depthStencilBuffer->Release();
	cbPerObjectBuffer->Release();
}


void draw_cube() {
	init_app();
	init_scene();
	draw_scene();
	cleanup();
}