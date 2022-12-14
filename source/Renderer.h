#pragma once
#include "Math.h"
#include "Mesh.h"
#include "Camera.h"
#include "Utils.h"
#include "MeshShaderEffect.h"
#include "TransparancyEffect.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{

	class Renderer final
	{
	public:

		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(const Timer* pTimer);
		void Render() const;
		void InitMesh(); 
		void InitCamera();
		void InitTexture();
		void SwitchTechnique();


	private:
		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		float m_AspectRatio;

		bool m_IsInitialized{ false };

		//DIRECTX
		ID3D11Device* m_pDevice;
		ID3D11DeviceContext* m_pDeviceContext;
		IDXGISwapChain* m_pSwapChain;
		ID3D11Texture2D* m_pDepthStencilBuffer;
		ID3D11DepthStencilView* m_pDepthStencilView;
		ID3D11Resource* m_pRenderTargetBuffer;
		ID3D11RenderTargetView* m_pRenderTargetView;

		//Objects
		Mesh* m_pVehicleMesh;
		Mesh* m_pFireMesh;
		Camera* m_pCamera;

		

		//Textures
		Texture* m_pTexture{ nullptr };
		Texture* m_pNormalTexture{ nullptr };
		Texture* m_pGlossinessTexture{ nullptr };
		Texture* m_pSpecularTexture{ nullptr };
		Texture* m_pFireTexture{ nullptr };

		HRESULT InitializeDirectX();
		//...
	};
}
