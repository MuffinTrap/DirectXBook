#include "d3dApp.h"
#include "d3dx11effect.h"
#include "MathHelper.h"

static const bool singleSlot = true;

struct PositionVertex
{
	XMFLOAT3	Position;
};

struct ColorVertex
{
	XMFLOAT4	Color;
};

struct CombinedVertex
{
	XMFLOAT3	Position;
	XMFLOAT4	Color;
};

class BoxApp : public D3DApp
{
public:
	BoxApp(HINSTANCE hInstance);
	~BoxApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM buttonState, int x, int y);
	void OnMouseUp(WPARAM buttonState, int x, int y);
	void OnMouseMove(WPARAM buttonState, int x, int y);

private:

	void DrawCube();
	void DrawPyramid();
	void UpdateConstantBuffer(XMFLOAT4X4 &worldMatrix);
	void ApplyTechnique(ID3DX11EffectTechnique* technique, UINT indicesAmount);

	void BuildGeometryBuffers();
	void BuildFX();
	void BuildVertexLayout();

	ID3D11Buffer* mBoxPositionVB;
	ID3D11Buffer* mBoxColorVB;
	ID3D11Buffer* mBoxIB;
	UINT mIndexAmount;

	ID3D11Buffer* mPyramidVB;
	ID3D11Buffer* mPyramidIB;
	UINT mPyramidIndexAmount;

	
	UINT posSlot;
	UINT colorSlot;

	ID3DX11Effect* mFX;
	ID3DX11EffectTechnique* mTech;
	ID3DX11EffectMatrixVariable* mfxWorldViewProj;
	ID3DX11EffectScalarVariable* mfxTime;

	ID3D11InputLayout* mInputLayout;

	XMFLOAT4X4 mPyramidWorld;

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

	BoxApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}
	return theApp.Run();

}

BoxApp::BoxApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, mBoxPositionVB(nullptr)
	, mBoxColorVB(nullptr)
	, mBoxIB(nullptr)
	, mFX(nullptr)
	, mTech(nullptr)
	, mfxWorldViewProj(nullptr)
	, mInputLayout(nullptr)
	, mTheta(1.5f * MathHelper::Pi)
	, mPhi(0.25f*MathHelper::Pi)
	, mRadius(5.0f)
{
	mMainWndCaption = "Box Demo";
	mLastMousePosition.x = 0;
	mLastMousePosition.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mWorld, I);
	XMStoreFloat4x4(&mPyramidWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	mIndexAmount = 36;
	mPyramidIndexAmount = 18;
	posSlot = 0;
	if (singleSlot)
	{
		colorSlot = 0;
	}
	else
	{
		colorSlot = 1;
	}
}

BoxApp::~BoxApp()
{
	ReleaseCOM(mBoxPositionVB);
	ReleaseCOM(mBoxColorVB);
	ReleaseCOM(mBoxIB);
	ReleaseCOM(mPyramidVB);
	ReleaseCOM(mPyramidIB);
	ReleaseCOM(mFX);
	ReleaseCOM(mInputLayout);
}

bool BoxApp::Init()
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

void BoxApp::OnResize()
{
	D3DApp::OnResize();

	// Update projection matrix
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void BoxApp::UpdateScene(float dt)
{
	mTheta += 0.2f * dt;
	mPhi += 0.2f * dt;

	// Convert sphere coordinates to Cartesian
	// float x = mRadius * sinf(mPhi) * cosf(mTheta);
	// float y = mRadius * sinf(mPhi) * sinf(mTheta);
	// float z = mRadius * cosf(mPhi);

	// Camera position
	float x = 0.0f;// mRadius * sinf(mTheta);
	float y = 1.0f;
	float z = mRadius;

	// Rotate object
	XMMATRIX rotated = XMMatrixRotationRollPitchYaw(sinf(mPhi), cosf(mTheta), cosf(mPhi) + sinf(mTheta));
	XMStoreFloat4x4(&mPyramidWorld, rotated);


	// View matrix points to origo
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);
}

void BoxApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, Colors::LightSteelBlue);
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(mInputLayout);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//DrawCube();
	DrawPyramid();

	HR(mSwapChain->Present(0, 0));
}

void BoxApp::DrawCube()
{
	// Two input slots
	// 0: position
	// 1: color

	UINT offset = 0;
	UINT comboStride = sizeof(CombinedVertex);
	UINT posStride = sizeof(PositionVertex);
	UINT colorStride = sizeof(ColorVertex);

	if (singleSlot)
	{
		md3dImmediateContext->IASetVertexBuffers(posSlot, 1, &mBoxPositionVB, &comboStride, &offset);
	}
	else
	{
		md3dImmediateContext->IASetVertexBuffers(posSlot, 1, &mBoxPositionVB, &posStride, &offset);
		md3dImmediateContext->IASetVertexBuffers(colorSlot, 1, &mBoxColorVB, &colorStride, &offset);
	}

	md3dImmediateContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);

	UpdateConstantBuffer(mWorld);
	ApplyTechnique(mTech, mIndexAmount);

}

void BoxApp::DrawPyramid()
{
	// Only works in single slot mode
	if (!singleSlot)
	{
		return;
	}

	UINT offset = 0;
	UINT comboStride = sizeof(CombinedVertex);

	md3dImmediateContext->IASetVertexBuffers(posSlot, 1, &mPyramidVB, &comboStride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mPyramidIB, DXGI_FORMAT_R32_UINT, 0);

	UpdateConstantBuffer(mPyramidWorld);
	ApplyTechnique(mTech, mPyramidIndexAmount);
}

void BoxApp::UpdateConstantBuffer(XMFLOAT4X4 &worldMatrix)
{
	// Constant buffer
	XMMATRIX world = XMLoadFloat4x4(&worldMatrix);
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;

	mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
	mfxTime->SetFloat(mTimer.TotalTime());
}

void BoxApp::ApplyTechnique(ID3DX11EffectTechnique* technique, UINT indicesAmount)
{
	D3DX11_TECHNIQUE_DESC techDesc;
	technique->GetDesc(&techDesc);
	for (UINT pass = 0; pass < techDesc.Passes; pass++)
	{
		mTech->GetPassByIndex(pass)->Apply(0, md3dImmediateContext);

		md3dImmediateContext->DrawIndexed(indicesAmount, 0, 0);
	}
}

void BoxApp::OnMouseDown(WPARAM buttonState, int x, int y)
{
	mLastMousePosition.x = x;
	mLastMousePosition.y = y;

	SetCapture(mhMainWnd);

}

void BoxApp::OnMouseUp(WPARAM buttonState, int x, int y)
{
	ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM buttonState, int x, int y)
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

void BoxApp::BuildGeometryBuffers()
{
	//
	// CUBE
	//

	const UINT uniqueVertexAmount = 8;

	PositionVertex positionVertices[uniqueVertexAmount] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f) },
	};

	ColorVertex colorVertices[uniqueVertexAmount] =
	{
		{ DirectX::XMFLOAT4A(1.0f, 1.0f, 1.0f, 1.0f) },
		{ DirectX::XMFLOAT4A(0.0f, 1.0f, 1.0f, 1.0f) },
		{ DirectX::XMFLOAT4A(1.0f, 0.0f, 1.0f, 1.0f) },
		{ DirectX::XMFLOAT4A(1.0f, 1.0f, 0.0f, 1.0f) },
		{ DirectX::XMFLOAT4A(0.0f, 0.0f, 1.0f, 1.0f) },
		{ DirectX::XMFLOAT4A(1.0f, 0.0f, 0.0f, 1.0f) },
		{ DirectX::XMFLOAT4A(0.0f, 1.0f, 0.0f, 1.0f) },
		{ DirectX::XMFLOAT4A(0.0f, 0.0f, 0.0f, 1.0f) }
	};

	CombinedVertex comboVertices[uniqueVertexAmount]
	{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4A(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4A(0.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4A(1.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4A(1.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4A(0.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4A(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4A(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4A(0.0f, 0.0f, 0.0f, 1.0f) }
	};

	D3D11_BUFFER_DESC positionVbd;
	positionVbd.Usage = D3D11_USAGE_IMMUTABLE;
	if (singleSlot)
	{
		positionVbd.ByteWidth = sizeof(CombinedVertex) * uniqueVertexAmount;	// Size in bytes
	}
	else
	{
		positionVbd.ByteWidth = sizeof(PositionVertex) * uniqueVertexAmount;	// Size in bytes
	}
	positionVbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	positionVbd.CPUAccessFlags = 0;
	positionVbd.MiscFlags = 0;
	positionVbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vinitDataPosition;
	if (singleSlot)
	{
		vinitDataPosition.pSysMem = comboVertices;
	}
	else
	{
		vinitDataPosition.pSysMem = positionVertices;
	}
	HR(md3dDevice->CreateBuffer(&positionVbd, &vinitDataPosition, &mBoxPositionVB));

	D3D11_BUFFER_DESC ColorVbd;
	ColorVbd.Usage = D3D11_USAGE_IMMUTABLE;
	ColorVbd.ByteWidth = sizeof(ColorVertex) * uniqueVertexAmount;	// Size in bytes
	ColorVbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	ColorVbd.CPUAccessFlags = 0;
	ColorVbd.MiscFlags = 0;
	ColorVbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA vinitDataColor;
	vinitDataColor.pSysMem = colorVertices;
	HR(md3dDevice->CreateBuffer(&ColorVbd, &vinitDataColor, &mBoxColorVB));


	// Index buffer
	UINT indices[] = {
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

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * mIndexAmount;	// Size in bytes
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = indices;
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));

	//
	// PYRAMID 
	// 
	// Left hand 

	const UINT pyramidVertexAmount = 5;
	CombinedVertex pyramidVertices[pyramidVertexAmount] =
	{
		{ XMFLOAT3(-1.0f, 0.0f, +1.0f), DirectX::XMFLOAT4A(0.0f, 1.0f, 0.2f, 1.0f) }, // 0 
		{ XMFLOAT3(+1.0f, 0.0f, +1.0f), DirectX::XMFLOAT4A(0.0f, 1.0f, 0.2f, 1.0f) }, // 1
		{ XMFLOAT3(+1.0f, 0.0f, -1.0f), DirectX::XMFLOAT4A(0.0f, 1.0f, 0.2f, 1.0f) }, // 2
		{ XMFLOAT3(-1.0f, 0.0f, -1.0f), DirectX::XMFLOAT4A(0.0f, 1.0f, 0.2f, 1.0f) }, // 3 
		{ XMFLOAT3(+0.0f, +2.0f, +0.0f), DirectX::XMFLOAT4A(1.0f, 0.4f, 0.0f, 1.0f) }  // 4
	};

	D3D11_BUFFER_DESC pyramidVbd;
	pyramidVbd.Usage = D3D11_USAGE_IMMUTABLE;
	pyramidVbd.ByteWidth = sizeof(CombinedVertex) * pyramidVertexAmount;
	pyramidVbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	pyramidVbd.CPUAccessFlags = 0;
	pyramidVbd.MiscFlags = 0;
	pyramidVbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA pyramidVertexInitDataPosition;
	pyramidVertexInitDataPosition.pSysMem = pyramidVertices;
	HR(md3dDevice->CreateBuffer(&pyramidVbd, &pyramidVertexInitDataPosition, &mPyramidVB));

	UINT pyramidIndices[] =
	{
		0, 2, 1,
		2, 0, 3,

		4, 0, 1,
		4, 1, 2,
		4, 2, 3,
		4, 3, 0
	};

	D3D11_BUFFER_DESC pIbd;
	pIbd.Usage = D3D11_USAGE_IMMUTABLE;
	pIbd.ByteWidth = sizeof(UINT) * mPyramidIndexAmount;	// Size in bytes
	pIbd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	pIbd.CPUAccessFlags = 0;
	pIbd.MiscFlags = 0;
	pIbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA piinitData;
	piinitData.pSysMem = pyramidIndices;
	HR(md3dDevice->CreateBuffer(&pIbd, &piinitData, &mPyramidIB));

	
}

void BoxApp::BuildFX()
{
	DWORD shaderFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	shaderFlags |= D3D10_SHADER_DEBUG;
	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

	DWORD effectFlags = 0;
	ID3DBlob* errors = 0;

	HRESULT hr = D3DX11CompileEffectFromFile(L"FX/color.fx",NULL, NULL, shaderFlags, effectFlags, md3dDevice, &mFX, &errors);

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
	mfxTime = mFX->GetVariableByName("gTime")->AsScalar();
}

void BoxApp::BuildVertexLayout()
{

	UINT byteOffset = 0;
	if (singleSlot)
	{
		byteOffset = sizeof(PositionVertex);
	}
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, posSlot, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
		, {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, colorSlot, byteOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D3DX11_PASS_DESC passDesc;
	mTech->GetPassByIndex(0)->GetDesc(&passDesc);
	HR(md3dDevice->CreateInputLayout(vertexDesc, 2, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &mInputLayout));
}
