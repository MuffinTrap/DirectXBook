#include "d3dApp.h"
#include "GeometryGenerator.h"
#include "d3dx11effect.h"
#include "MathHelper.h"

struct Vertex
{
	XMFLOAT3	Position;
	XMFLOAT4	Color;
};

class ShapesApp : public D3DApp
{
public:
	ShapesApp(HINSTANCE hInstance);
	~ShapesApp();

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

	UINT packIntoBuffer(std::vector<Vertex> &target, std::vector<GeometryGenerator::Vertex> &source, UINT startIndex, XMFLOAT4 color);

	ID3D11Buffer* mShapesVB;
	ID3D11Buffer* mShapesIB;
	UINT geometryIndexBufferSize;

	ID3DX11Effect* mFX;
	ID3DX11EffectTechnique* mTech;
	ID3DX11EffectMatrixVariable* mfxWorldViewProj;

	ID3D11InputLayout* mInputLayout;

	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	// Matrices of shapes
	XMFLOAT4X4 mSphereWorldArray[10];
	XMFLOAT4X4 mCylinderWorldArray[10];
	XMFLOAT4X4 mBoxWorld;
	XMFLOAT4X4 mGridWorld;

	// Vertex offsets and counts 
	// Everything is stored in the same buffer
	UINT mBoxVertexOffset;
	UINT mCylinderVertexOffset;
	UINT mGridVertexOffset;
	UINT mSphereVertexOffset;

	UINT mBoxIndexCount;
	UINT mCylinderIndexCount;
	UINT mGridIndexCount;
	UINT mSphereIndexCount;

	UINT mBoxIndexOffset;
	UINT mCylinderIndexOffset;
	UINT mGridIndexOffset;
	UINT mSphereIndexOffset;

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

	ShapesApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}
	return theApp.Run();

}

ShapesApp::ShapesApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, mShapesVB(nullptr)
	, mShapesIB(nullptr)
	, mFX(nullptr)
	, mTech(nullptr)
	, mfxWorldViewProj(nullptr)
	, mInputLayout(nullptr)
	, mCameraHeight(0.0f)
	, mCameraDistance(10.0f)
	, mCameraAngleAroundY(0.0f)
{
	mMainWndCaption = "Shapes Demo";
	mLastMousePosition.x = 0;
	mLastMousePosition.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	// Create scene matrices

	XMStoreFloat4x4(&mGridWorld, I);

	XMMATRIX boxScale = XMMatrixScaling(2.0f, 1.0f, 2.0f);
	XMMATRIX boxOffset = XMMatrixTranslation(0.f, 0.5f, 0.0f);
	XMStoreFloat4x4(&mBoxWorld, XMMatrixMultiply(boxScale, boxOffset));

	for (int i = 0; i < 5; i++)
	{
		XMStoreFloat4x4(&mCylinderWorldArray[i * 2 + 0], XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f));
		XMStoreFloat4x4(&mCylinderWorldArray[i * 2 + 1], XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f));

		XMStoreFloat4x4(&mSphereWorldArray[i * 2 + 0], XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f));
		XMStoreFloat4x4(&mSphereWorldArray[i * 2 + 1], XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f));
	}
}

ShapesApp::~ShapesApp()
{
	ReleaseCOM(mShapesVB);
	ReleaseCOM(mShapesIB);
	ReleaseCOM(mFX);
	ReleaseCOM(mInputLayout);
}

bool ShapesApp::Init()
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

void ShapesApp::OnResize()
{
	D3DApp::OnResize();

	// Update projection matrix
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void ShapesApp::UpdateScene(float dt)
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

void ShapesApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, Colors::Blue);
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(mInputLayout);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);



	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mShapesVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mShapesIB, DXGI_FORMAT_R32_UINT, 0);

	// Constant buffer
	
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX viewProj = view * proj;

	//mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));

	D3DX11_TECHNIQUE_DESC techDesc;
	mTech->GetDesc(&techDesc);
	for (UINT pass = 0; pass < techDesc.Passes; pass++)
	{
		// Grid
		XMMATRIX world = XMLoadFloat4x4(&mGridWorld);

		mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&(world * viewProj)));
		mTech->GetPassByIndex(pass)->Apply(0, md3dImmediateContext);
		md3dImmediateContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);

		// Box 
		world = XMLoadFloat4x4(&mBoxWorld);

		mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&(world * viewProj)));
		mTech->GetPassByIndex(pass)->Apply(0, md3dImmediateContext);
		md3dImmediateContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

		// Cylinders
		for (int i = 0; i < 10; i++)
		{
			world = XMLoadFloat4x4(&mCylinderWorldArray[i]);

			mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&(world * viewProj)));
			mTech->GetPassByIndex(pass)->Apply(0, md3dImmediateContext);
			md3dImmediateContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
		}
		// Spheres
		for (int i = 0; i < 10; i++)
		{
			world = XMLoadFloat4x4(&mSphereWorldArray[i]);

			mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&(world * viewProj)));
			mTech->GetPassByIndex(pass)->Apply(0, md3dImmediateContext);
			md3dImmediateContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);

		}

		
	}

	HR(mSwapChain->Present(0, 0));
}

void ShapesApp::OnMouseDown(WPARAM buttonState, int x, int y)
{
	mLastMousePosition.x = x;
	mLastMousePosition.y = y;

	SetCapture(mhMainWnd);

}

void ShapesApp::OnMouseUp(WPARAM buttonState, int x, int y)
{
	ReleaseCapture();
}

void ShapesApp::OnMouseMove(WPARAM buttonState, int x, int y)
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


		mCameraDistance += dy;
		mCameraDistance = MathHelper::Clamp(mCameraDistance, -200.0f, 200.0f);
	}

	mLastMousePosition.x = x;
	mLastMousePosition.y = y;
}

UINT ShapesApp::packIntoBuffer(std::vector<Vertex> &target, std::vector<GeometryGenerator::Vertex> &source, UINT startIndex, XMFLOAT4 color)
{
	for (size_t i = 0; i < source.size(); i++, startIndex++)
	{
		target[startIndex].Position = source[i].Position;
		target[startIndex].Color = color;
	}

	return startIndex;
}

void ShapesApp::BuildGeometryBuffers()
{
	GeometryGenerator::MeshData grid;
	GeometryGenerator::MeshData box;
	GeometryGenerator::MeshData cylinder;
	GeometryGenerator::MeshData sphere;

	GeometryGenerator geoGen;

	float gridWidth = 100.0f;
	float gridDepth = 100.0f;
	UINT verticesForWidth = 50;
	UINT verticesForDepth = 50;

	geoGen.CreateGrid(gridWidth, gridDepth, verticesForWidth, verticesForDepth, grid);
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);
	geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20, cylinder);
	geoGen.CreateSphere(0.5f, 20, 20, sphere);

	mBoxVertexOffset = 0;
	mGridVertexOffset = mBoxVertexOffset + box.Vertices.size();
	mCylinderVertexOffset = mGridVertexOffset + grid.Vertices.size();
	mSphereVertexOffset = mCylinderVertexOffset + cylinder.Vertices.size();

	mBoxIndexCount = box.Indices.size();
	mGridIndexCount = grid.Indices.size();
	mCylinderIndexCount = cylinder.Indices.size();
	mSphereIndexCount = sphere.Indices.size();

	mBoxIndexOffset = 0;
	mGridIndexOffset = mBoxIndexOffset + mBoxIndexCount;
	mCylinderIndexOffset = mGridIndexOffset + mGridIndexCount;
	mSphereIndexOffset = mCylinderIndexOffset + mCylinderIndexCount;

	UINT totalVertexCount = mSphereVertexOffset + sphere.Vertices.size();
	UINT totalIndexCount = mSphereIndexOffset + mSphereIndexCount;

	std::vector<Vertex> vertices(totalVertexCount);

	XMFLOAT4 black(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 red(1.0f, 0.6f, 0.6f, 1.0f);
	XMFLOAT4 darkRed(0.6f, 0.2f, 0.2f, 1.0f);
	XMFLOAT4 white(0.8f, 0.8f, 0.8f, 1.0f);


	UINT bi = 0; // Buffer index
	bi = packIntoBuffer(vertices, box.Vertices, bi, red);
	bi = packIntoBuffer(vertices, grid.Vertices, bi, black);
	bi = packIntoBuffer(vertices, cylinder.Vertices, bi, darkRed);
	bi = packIntoBuffer(vertices, sphere.Vertices, bi, white);



	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * totalVertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mShapesVB));

	std::vector<UINT> indices;
	indices.reserve(totalIndexCount);
	indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());
	indices.insert(indices.end(), grid.Indices.begin(), grid.Indices.end());
	indices.insert(indices.end(), cylinder.Indices.begin(), cylinder.Indices.end());
	indices.insert(indices.end(), sphere.Indices.begin(), sphere.Indices.end());



	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * totalIndexCount;	// Size in bytes
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mShapesIB));
}

void ShapesApp::BuildFX()
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

void ShapesApp::BuildVertexLayout()
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
