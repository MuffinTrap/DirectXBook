#include "d3dApp.h"
#include "GeometryGenerator.h"
#include "d3dx11effect.h"
#include "MathHelper.h"

struct Vertex
{
	XMFLOAT3	Position;
	XMFLOAT4	Color;
};

class HillsApp : public D3DApp
{
public:
	HillsApp(HINSTANCE hInstance);
	~HillsApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM buttonState, int x, int y);
	void OnMouseUp(WPARAM buttonState, int x, int y);
	void OnMouseMove(WPARAM buttonState, int x, int y);

private:

	float getHeightOnGrid(float x, float z);
	XMFLOAT4 colorByHeight(float height);

	void BuildGeometryBuffers();
	void BuildFX();
	void BuildVertexLayout();

	ID3D11Buffer* mHillVB;
	ID3D11Buffer* mHillIB;
	UINT geometryIndexBufferSize;

	ID3DX11Effect* mFX;
	ID3DX11EffectTechnique* mTech;
	ID3DX11EffectMatrixVariable* mfxWorldViewProj;

	ID3D11InputLayout* mInputLayout;

	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	float mCameraHeight;
	float mCameraDistance;
	float mCameraAngleAroundY;

	POINT mLastMousePosition;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	HillsApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}
	return theApp.Run();

}

HillsApp::HillsApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, mHillVB(nullptr)
	, mHillIB(nullptr)
	, mFX(nullptr)
	, mTech(nullptr)
	, mfxWorldViewProj(nullptr)
	, mInputLayout(nullptr)
	, mCameraHeight(0.0f)
	, mCameraDistance(10.0f)
	, mCameraAngleAroundY(0.0f)
{
	mMainWndCaption = "Hills Demo";
	mLastMousePosition.x = 0;
	mLastMousePosition.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);
}

HillsApp::~HillsApp()
{
	ReleaseCOM(mHillVB);
	ReleaseCOM(mHillIB);
	ReleaseCOM(mFX);
	ReleaseCOM(mInputLayout);
}

bool HillsApp::Init()
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

void HillsApp::OnResize()
{
	D3DApp::OnResize();

	// Update projection matrix
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void HillsApp::UpdateScene(float dt)
{
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	float y = mCameraHeight;
	float z = mCameraDistance;
	

	// View matrix points to origo
	XMVECTOR pos = XMVectorSet(0.0f, y, z, 1.0f);

	// Rotate this pos around Y axis by cameraAngleAroundY
	XMMATRIX rot = XMMatrixRotationAxis(up, mCameraAngleAroundY);
	XMVECTOR rotatedPos = XMVector4Transform(pos, rot);

	XMVECTOR target = XMVectorZero();
	

	XMMATRIX V = XMMatrixLookAtLH(rotatedPos, target, up);
	XMStoreFloat4x4(&mView, V);
}

void HillsApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, Colors::Blue);
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(mInputLayout);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mHillVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mHillIB, DXGI_FORMAT_R32_UINT, 0);

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

		md3dImmediateContext->DrawIndexed(geometryIndexBufferSize, 0, 0);
	}

	HR(mSwapChain->Present(0, 0));
}

void HillsApp::OnMouseDown(WPARAM buttonState, int x, int y)
{
	mLastMousePosition.x = x;
	mLastMousePosition.y = y;

	SetCapture(mhMainWnd);

}

void HillsApp::OnMouseUp(WPARAM buttonState, int x, int y)
{
	ReleaseCapture();
}

void HillsApp::OnMouseMove(WPARAM buttonState, int x, int y)
{
	// If state has left button
	if ((buttonState & MK_LBUTTON) != 0)
	{
		float changeSpeed = 0.05f;
		float dy = changeSpeed * static_cast<float>(y - mLastMousePosition.y);

		
		mCameraHeight += dy;

		// Restrict vertical
		mCameraHeight = MathHelper::Clamp(mCameraHeight, -100.0f, 100.0f);

		// Rotate around
		float dx = changeSpeed * static_cast<float>(x - mLastMousePosition.x);

		float radianChange = XMConvertToRadians(dx);
		mCameraAngleAroundY += radianChange;

	}
	else if ((buttonState & MK_RBUTTON) != 0)
	{
		float changeSpeed = 0.05f;
		float dy = changeSpeed * static_cast<float>(y - mLastMousePosition.y);


		mCameraDistance +=  dy;
		mCameraDistance = MathHelper::Clamp(mCameraDistance, -200.0f, 200.0f);
	}

	mLastMousePosition.x = x;
	mLastMousePosition.y = y;
}

float  HillsApp::getHeightOnGrid(float x, float z)
{
	return 0.3f * (z*sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT4 HillsApp::colorByHeight(float height)
{
	static const float beachLevel = -10.0f;
	static const float lightGrassLevel = 5.0f;
	static const float darkGrassLevel = 12.0f;
	static const float hillLevel = 20.0f;

	static const XMFLOAT4 sandColor(1.0f, 0.96f, 0.62f, 1.0f);
	static const XMFLOAT4 lightGrassColor(0.48f, 0.77f, 0.46f, 1.0f);
	static const XMFLOAT4 darkGrassColor(0.1f, 0.48f, 0.619f, 1.0f);
	static const XMFLOAT4 hillColor(0.45f, 0.39f, 0.34f, 1.0f);
	static const XMFLOAT4 snowColor(1.0f, 0.96f, 1.0f, 1.0f);

	if (height < beachLevel)
	{
		return sandColor;
	}
	else if (height < lightGrassLevel)
	{
		return lightGrassColor;
	}
	else if (height < darkGrassLevel)
	{
		return darkGrassColor;
	}
	else if (height < hillLevel)
	{
		return hillColor;
	}
	else
	{
		return snowColor;
	}

	return sandColor;
}

void HillsApp::BuildGeometryBuffers()
{
	GeometryGenerator::MeshData grid;
	GeometryGenerator geoGen;

	float gridWidth = 100.0f;
	float gridDepth = 100.0f;
	UINT verticesForWidth = 50;
	UINT verticesForDepth = 50;

	geoGen.CreateGrid(gridWidth, gridDepth, verticesForWidth, verticesForDepth, grid);

	UINT mGridIndexCount = grid.Indices.size();
	geometryIndexBufferSize = mGridIndexCount;

	

	// Extract from mesh to our own vertex format discarding normals and tangents
	std::vector<Vertex> gridVertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); i++)
	{
		XMFLOAT3 pos = grid.Vertices[i].Position;
		pos.y = getHeightOnGrid(pos.x, pos.z);
		gridVertices[i].Position = pos;

		gridVertices[i].Color = colorByHeight(pos.y);
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * gridVertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &gridVertices[0];
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mHillVB));


	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * mGridIndexCount;	// Size in bytes
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &grid.Indices[0];
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mHillIB));
}

void HillsApp::BuildFX()
{
	DWORD shaderFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	shaderFlags |= D3D10_SHADER_DEBUG;
	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

	DWORD effectFlags = 0;
	ID3DBlob* errors = 0;

	HRESULT hr = D3DX11CompileEffectFromFile(L"FX/color.fx", NULL, NULL, shaderFlags, effectFlags, md3dDevice, &mFX, &errors);

	if (errors != NULL)
	{
		MessageBoxA(0, (char*)errors->GetBufferPointer(), 0, 0);
		ReleaseCOM(errors);
	}

	if (FAILED(hr))
	{
		DXTrace(__FILEW__, (DWORD)__LINE__, hr, L"CreateEffectFromFile Failed", true);
	}

	mTech = mFX->GetTechniqueByName("ColorTech");
	mfxWorldViewProj = mFX->GetVariableByName("gWorldViewProj")->AsMatrix();
}

void HillsApp::BuildVertexLayout()
{
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		,{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D3DX11_PASS_DESC passDesc;
	mTech->GetPassByIndex(0)->GetDesc(&passDesc);
	HR(md3dDevice->CreateInputLayout(vertexDesc, 2, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &mInputLayout));
}
