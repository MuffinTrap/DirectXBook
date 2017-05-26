
#include "d3dApp.h"
#include <dxgi.h>
#include <iostream>

class InitDirect3DApp : public D3DApp
{
public:
	InitDirect3DApp(HINSTANCE hInstance);
	~InitDirect3DApp();

	virtual bool Init();
	virtual void UpdateScene(float delta);
	virtual void DrawScene(void);
	virtual void OnResize(void);
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	// Memory check
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	InitDirect3DApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	ID3D11Device *device = theApp.GetDevice();


	IDXGIDevice* dxgiDevice = 0;
	HR(device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));

	IDXGIAdapter* dxgiAdapter = 0;
	HR(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter));

	IDXGIFactory* dxgiFactory = 0;
	HR(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory));

	// Do not react to ALT + ENTER
	dxgiFactory->MakeWindowAssociation(theApp.MainWnd(), DXGI_MWA_NO_WINDOW_CHANGES);

	// Get amount of adapters
	UINT adapterIndex = 0;

	IDXGIAdapter* adapter;
	DXGI_ADAPTER_DESC adapterDesc;

	OutputDebugString("Enumerating Display adapters ...\n");
	for (adapterIndex = 0; dxgiFactory->EnumAdapters(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND; adapterIndex++)
	{
		adapter->GetDesc(&adapterDesc);
		OutputDebugStringW(adapterDesc.Description);
		OutputDebugString("\n");

		OutputDebugString("Enumerating outputs for adapter:\n");
		UINT outputIndex = 0;
		IDXGIOutput* output;
		DXGI_OUTPUT_DESC outputDesc;
		for (outputIndex = 0; adapter->EnumOutputs(outputIndex, &output) != DXGI_ERROR_NOT_FOUND; outputIndex++)
		{
			output->GetDesc(&outputDesc);
			OutputDebugStringW(outputDesc.DeviceName);
			OutputDebugString("\n");
			UINT numModes;
			output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numModes, NULL);
			DXGI_MODE_DESC *modes = new DXGI_MODE_DESC[numModes];
			output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numModes, modes);
			for (UINT i = 0; i < numModes; i++)
			{
				std::stringstream debug;
				debug << "Mode " << i << ": Rate "<< modes[i].RefreshRate.Numerator << "/" << modes[i].RefreshRate.Denominator << " " << modes[i].Width << "x" << modes[i].Height << std::endl;
				OutputDebugString(debug.str().c_str());
			}
		}
	}

	ReleaseCOM(dxgiDevice);
	ReleaseCOM(dxgiAdapter);
	ReleaseCOM(dxgiFactory);


    return theApp.Run();
}

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
    // nop
}

InitDirect3DApp::~InitDirect3DApp()
{

}

bool InitDirect3DApp::Init()
{
    if(D3DApp::Init() == false)
    {
        return false;
    }
    return true;
}

void InitDirect3DApp::UpdateScene(float delta) {}
void InitDirect3DApp::DrawScene(void) 
{
	assert(md3dImmediateContext);
	assert(mSwapChain);

	// Clear back buffer
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Blue));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	HR(mSwapChain->Present(0, 0));
}
void InitDirect3DApp::OnResize(void) 
{
	D3DApp::OnResize();
}