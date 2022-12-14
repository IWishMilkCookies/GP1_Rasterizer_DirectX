#include "pch.h"
#include "Renderer.h"

namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
		m_AspectRatio = static_cast<float>(m_Width) / m_Height;

		//Initialize DirectX pipeline
		const HRESULT result = InitializeDirectX();

		if (result == S_OK)
		{
			m_IsInitialized = true;
			std::cout << "DirectX is initialized and ready!\n";

		}
		else
		{
			std::cout << "DirectX initialization failed!\n";
		}
		
		InitCamera();
		InitTexture();
		InitMesh();
	}

	Renderer::~Renderer()
	{
		if (m_pDeviceContext)
		{
			m_pDeviceContext->ClearState();
			m_pDeviceContext->Flush();
			m_pDeviceContext->Release();
		}

		//Mesh
		delete m_pVehicleMesh;
		m_pVehicleMesh = nullptr;

		delete m_pFireMesh;
		m_pFireMesh = nullptr;

		//Object
		delete m_pCamera;
		m_pCamera = nullptr;

		//Texture
		delete m_pTexture;
		m_pTexture = nullptr;

		delete m_pNormalTexture;
		m_pNormalTexture = nullptr;

		delete m_pGlossinessTexture;
		m_pGlossinessTexture = nullptr;

		delete m_pSpecularTexture;
		m_pSpecularTexture = nullptr;

		delete m_pFireTexture;
		m_pFireTexture = nullptr;


		m_pRenderTargetView->Release();
		m_pRenderTargetBuffer->Release();
		m_pDepthStencilView->Release();
		m_pDepthStencilBuffer->Release();
		m_pSwapChain->Release();
		m_pDevice->Release();

	}

	void Renderer::Update(const Timer* pTimer)
	{
		m_pCamera->Update(pTimer);

		const float meshRotation{ 45.0f * pTimer->GetElapsed() * TO_RADIANS };
		m_pVehicleMesh->RotateMesh(meshRotation);
		m_pFireMesh->RotateMesh(meshRotation);
	}


	void Renderer::Render() const
	{
		if (!m_IsInitialized)
			return;

		m_pCamera->CalculateViewMatrix();


		//1. CLEAR RTV & DSV
		ColorRGB clearColor = {0.f, 0.f, 0.3f};
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, &clearColor.r);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		//2. SET PIPELINE + INVOKE DRAWCALLS (=RENDER)

		auto worldViewProjectionMatix = m_pVehicleMesh->GetWorldMatrix() * m_pCamera->GetViewMatrix() * m_pCamera->GetProjectionMatrix();
		m_pVehicleMesh->Render(m_pDeviceContext, worldViewProjectionMatix, m_pCamera->GetInvViewMatrix());
		m_pFireMesh->Render(m_pDeviceContext, worldViewProjectionMatix, m_pCamera->GetInvViewMatrix());

		//3. PRESENT BACKBUFFER (SWAP)
		m_pSwapChain->Present(0, 0);


	}

	void Renderer::InitMesh()
	{
		//Set up mesh data for vehicle
		std::vector<Vertex> vertices{};
		
		std::vector<uint32_t> indices{};
		MeshShaderEffect* shaderEffect = new MeshShaderEffect(m_pDevice, L"Resources/MeshShader.fx");

		if (m_pTexture)
			shaderEffect->SetDiffuseMap(m_pTexture);

		if (m_pNormalTexture)
			shaderEffect->SetNormalMap(m_pNormalTexture);

		if (m_pGlossinessTexture)
			shaderEffect->SetGlossinessMap(m_pGlossinessTexture);

		if (m_pSpecularTexture)
			shaderEffect->SetSpecularMap(m_pSpecularTexture);
		
		Utils::ParseOBJ("Resources/vehicle.obj", vertices, indices);
		m_pVehicleMesh = new Mesh(m_pDevice, vertices, indices, shaderEffect);
		const Vector3 position{ m_pCamera->GetOrigin() + Vector3{0, 0, 50}};
		const Vector3 rotation{ };
		const Vector3 scale{ Vector3{ 1, 1, 1 } };
		Matrix worldMatrix{ Matrix::CreateScale(scale) * Matrix::CreateRotation(rotation) * Matrix::CreateTranslation(position) };

		m_pVehicleMesh->SetWorldMatrix(worldMatrix);




		//Set up mesh data for Fire
		std::vector<Vertex> fireVertices{};

		std::vector<uint32_t> fireIndices{};
		Utils::ParseOBJ("Resources/fireFX.obj", vertices, indices);
		TransparancyEffect* transparancyEffect = new TransparancyEffect(m_pDevice, L"Resources/Transparancy.fx");
		m_pFireMesh = new Mesh(m_pDevice, vertices, indices, transparancyEffect);

		if (m_pFireTexture)
			transparancyEffect->SetDiffuseMap(m_pFireTexture);

		m_pFireMesh->SetWorldMatrix(worldMatrix);
	}

	void Renderer::InitCamera()
	{
		m_pCamera = new Camera({ 0.f, 0.f, -10.f }, m_AspectRatio, 45.f);
		m_pCamera->CalculateProjectionMatrix();
	}

	void Renderer::InitTexture()
	{
		m_pTexture = Texture::LoadFromFile("Resources/vehicle_diffuse.png", m_pDevice);
		m_pNormalTexture = Texture::LoadFromFile("Resources/vehicle_normal.png", m_pDevice);
		m_pGlossinessTexture = Texture::LoadFromFile("Resources/vehicle_gloss.png", m_pDevice);
		m_pSpecularTexture = Texture::LoadFromFile("Resources/vehicle_specular.png", m_pDevice);
		m_pFireTexture = Texture::LoadFromFile("Resources/fireFX_diffuse.png", m_pDevice);
	}

	void Renderer::SwitchTechnique()
	{
		m_pVehicleMesh->GetEffect()->SwitchCurrentTechnique();
		m_pFireMesh->GetEffect()->SwitchCurrentTechnique();
	}


	HRESULT Renderer::InitializeDirectX()
	{
		//1. Create Device & DeviceContext
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
		uint32_t createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, &featureLevel, 1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext);

		if (FAILED(result))
			return result;

		//Create DXGI Factory
		IDXGIFactory1* pDxgiFactory{};
		result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pDxgiFactory));

		if (FAILED(result))
			return result;

		//2. Create Swapchain
		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferDesc.Width = m_Width;
		swapChainDesc.BufferDesc.Height = m_Height;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;

		//Get the handle (HWND) from the SDL Backbuffer
		SDL_SysWMinfo sysWMInfo{};
		SDL_VERSION(&sysWMInfo.version);
		SDL_GetWindowWMInfo(m_pWindow, &sysWMInfo);
		swapChainDesc.OutputWindow = sysWMInfo.info.win.window;

		//Create SwapChain
		result = pDxgiFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);

		if (FAILED(result))
			return result;

		//3. Create DepthStencil (DS) & DepthStencilView (DSV)

		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = m_Width;
		depthStencilDesc.Height = m_Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;


		//View
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = depthStencilDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);
		if (FAILED(result))
			return result;

		result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
		if (FAILED(result))
			return result;

		//4. Create renderTarget (RT) & RenderTargetView (RTV)

		//Resource 
		result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));
		if (FAILED(result))
			return result;

		//View
		result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);

		//5. Bind RTV & DSV to output Merger Stage
		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

		//6. Set viewport
		D3D11_VIEWPORT viewPort{};
		viewPort.Width = static_cast<float>(m_Width);
		viewPort.Height = static_cast<float>(m_Height);
		viewPort.TopLeftX = 0.f;
		viewPort.TopLeftY = 0.f;
		viewPort.MinDepth = 0.f;
		viewPort.MaxDepth = 1.f;
		m_pDeviceContext->RSSetViewports(1, &viewPort);
		
		pDxgiFactory->Release();
		return S_OK;
	}
}
