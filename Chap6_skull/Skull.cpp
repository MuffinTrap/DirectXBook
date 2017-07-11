#include "d3dApp.h"
#include "d3dx11effect.h"
#include "MathHelper.h"

// For reading model data from file
#include <fstream>
#include <iostream>
#include <string>
#include <bitset>
#include <memory>


struct SkullVertex
{
	XMFLOAT3	Position;
	XMFLOAT3	Normal;
};

class SkullApp : public D3DApp
{
public:
	SkullApp(HINSTANCE hInstance);
	~SkullApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM buttonState, int x, int y);
	void OnMouseUp(WPARAM buttonState, int x, int y);
	void OnMouseMove(WPARAM buttonState, int x, int y);

private:
	void BuildGeometryBuffers();
	void BuildFX();
	void BuildVertexLayout();

	ID3D11Buffer* mSkullVB;
	ID3D11Buffer* mSkullIB;

	UINT mSkullIndexAmount;

	ID3DX11Effect* mFX;
	ID3DX11EffectTechnique* mTech;
	ID3DX11EffectMatrixVariable* mfxWorldViewProj;

	ID3D11InputLayout* mInputLayout;

	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePosition;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	SkullApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}
	return theApp.Run();

}

SkullApp::SkullApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, mSkullVB(nullptr)
	, mSkullIB(nullptr)
	, mFX(nullptr)
	, mTech(nullptr)
	, mfxWorldViewProj(nullptr)
	, mInputLayout(nullptr)
	, mTheta(1.5f * MathHelper::Pi)
	, mPhi(0.25f*MathHelper::Pi)
	, mRadius(20.0f)
	, mSkullIndexAmount(0)
{
	mMainWndCaption = "Skull Demo";
	mLastMousePosition.x = 0;
	mLastMousePosition.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);
}

SkullApp::~SkullApp()
{
	ReleaseCOM(mSkullVB);
	ReleaseCOM(mSkullIB);
	ReleaseCOM(mFX);
	ReleaseCOM(mInputLayout);
}

bool SkullApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	BuildGeometryBuffers();
	BuildFX();
	BuildVertexLayout();

	return true;
}

void SkullApp::OnResize()
{
	D3DApp::OnResize();

	// Update projection matrix
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void SkullApp::UpdateScene(float dt)
{
	// Convert sphere coordinates to Cartesian
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float y = mRadius * sinf(mPhi) * sinf(mTheta);
	float z = mRadius * cosf(mPhi);

	// View matrix points to origo
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorSet(0.0f, 3.0f, 0.0f, 1.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);
}

void SkullApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, Colors::Black);
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(mInputLayout);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(SkullVertex);
	UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);

	// Constant buffer
	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;

	mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));

	D3DX11_TECHNIQUE_DESC techDesc;
	mTech->GetDesc(&techDesc);
	for (UINT pass = 0; pass < techDesc.Passes; pass++)
	{
		mTech->GetPassByIndex(pass)->Apply(0, md3dImmediateContext);
		md3dImmediateContext->DrawIndexed(mSkullIndexAmount, 0, 0);
	}

	HR(mSwapChain->Present(0, 0));
}

void SkullApp::OnMouseDown(WPARAM buttonState, int x, int y)
{
	mLastMousePosition.x = x;
	mLastMousePosition.y = y;

	SetCapture(mhMainWnd);

}

void SkullApp::OnMouseUp(WPARAM buttonState, int x, int y)
{
	ReleaseCapture();
}

void SkullApp::OnMouseMove(WPARAM buttonState, int x, int y)
{
	// If state has left button
	if ((buttonState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePosition.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePosition.y));

		mTheta += dx;
		mPhi += dy;

		// Restrict vertical
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((buttonState & MK_RBUTTON) != 0)
	{
		float dx = 0.005f * static_cast<float>(x - mLastMousePosition.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePosition.y);


		mRadius += dx - dy;
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePosition.x = x;
	mLastMousePosition.y = y;
}

void SkullApp::BuildGeometryBuffers()
{
	// Load vertices from file
	std::ifstream modelFile("Models/skull.txt");
	if (!modelFile)
	{
		OutputDebugStringA("Could not open file Models/skull.txt");
		return;
	}

	// Read vertex amount
	std::string headerString;
	UINT vertexAmount;
	UINT triangleCount;
	modelFile >> headerString;
	if (headerString.find("VertexCount:") != std::string::npos)
	{
		// Next is the amount
		modelFile >> vertexAmount;
	}

	// else etc...
	modelFile >> headerString;
	if (headerString.find("TriangleCount:") != std::string::npos)
	{
		modelFile >> triangleCount;
	}

	modelFile >> headerString;
	if (headerString.find("VertexList") != std::string::npos)
	{
		// ok
	}

	// Read vertices

	do {
		modelFile >> headerString;
	} while (headerString.find("{") == std::string::npos);


	std::unique_ptr<SkullVertex> skullVertices(new SkullVertex[vertexAmount]);
	float x, y, z;
	float nx, ny, nz;

	for (UINT i = 0; i < vertexAmount; i++)
	{
		modelFile >> x >> y >> z >> nx >> ny >> nz;
		skullVertices.get()[i] = { XMFLOAT3(x, y, z), XMFLOAT3(nx, ny, nz) };
	}

	do {
		modelFile >> headerString;
	} while (headerString.find("}") == std::string::npos);


	// Read triangles

	modelFile >> headerString;
	if (headerString.find("TriangleList") != std::string::npos)
	{

	}

	do {
		modelFile >> headerString;
	} while (headerString.find("{") == std::string::npos);

	mSkullIndexAmount = triangleCount * 3;
	std::unique_ptr<UINT> skullIndices(new UINT[mSkullIndexAmount]);
	for (UINT i = 0; i < mSkullIndexAmount; i++)
	{
		modelFile >> skullIndices.get()[i];
	}

	do {
		modelFile >> headerString;
	} while (headerString.find("}") == std::string::npos);

	modelFile.close();

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(SkullVertex) * vertexAmount;	// Size in bytes
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = skullVertices.get();
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mSkullVB));


	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * mSkullIndexAmount;	// Size in bytes
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = skullIndices.get();
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mSkullIB));

}

void SkullApp::BuildFX()
{
	DWORD shaderFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	shaderFlags |= D3D10_SHADER_DEBUG;
	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

	DWORD effectFlags = 0;
	ID3DBlob* errors = 0;

	HRESULT hr = D3DX11CompileEffectFromFile(L"FX/normal.fx", NULL, NULL, shaderFlags, effectFlags, md3dDevice, &mFX, &errors);

	if (errors != NULL)
	{
		MessageBoxA(0, (char*)errors->GetBufferPointer(), 0, 0);
		ReleaseCOM(errors);
	}

	if (FAILED(hr))
	{
		DXTrace(__FILEW__, (DWORD)__LINE__, hr, L"CreateEffectFromFile Failed", true);
	}

	mTech = mFX->GetTechniqueByName("NormalTech");
	mfxWorldViewProj = mFX->GetVariableByName("gWorldViewProj")->AsMatrix();
}

void SkullApp::BuildVertexLayout()
{
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		,{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D3DX11_PASS_DESC passDesc;
	mTech->GetPassByIndex(0)->GetDesc(&passDesc);
	HR(md3dDevice->CreateInputLayout(vertexDesc, 2
		, passDesc.pIAInputSignature
		, passDesc.IAInputSignatureSize
		, &mInputLayout));
}
