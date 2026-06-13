// D3D11 offscreen 3D viewport for rendering Drawable meshes. Renders to a
// texture whose shader-resource-view is shown inside an ImGui window.
#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <d3d11.h>

#include <vector>

#include "evw/app/render_mesh.h"
#include "evw/math/math.h"

namespace evw::gui
{
    class Viewport
    {
    public:
        bool init(ID3D11Device* device, ID3D11DeviceContext* ctx);
        void shutdown();

        // Replaces the rendered model (geometry + textures). Computes bounds for framing.
        void setModel(const evw::app::RenderModel& model);

        // Renders the scene at the given size with an orbit camera and returns the
        // shader-resource-view to display (as ImTextureID). yaw/pitch in radians,
        // distScale is camera distance multiplier.
        ID3D11ShaderResourceView* render(int width, int height, float yaw, float pitch, float distScale);

        size_t triangleCount() const { return totalIndices_ / 3; }
        bool hasGeometry() const { return totalIndices_ > 0; }

    private:
        struct DrawRange { uint32_t indexStart; uint32_t indexCount; int texIndex; };

        void releaseTargets();
        void releaseModel();
        bool ensureTargets(int width, int height);

        ID3D11Device* device_ = nullptr;
        ID3D11DeviceContext* ctx_ = nullptr;

        ID3D11VertexShader* vs_ = nullptr;
        ID3D11PixelShader* ps_ = nullptr;
        ID3D11InputLayout* layout_ = nullptr;
        ID3D11Buffer* cbuffer_ = nullptr;
        ID3D11Buffer* vbuffer_ = nullptr;
        ID3D11Buffer* ibuffer_ = nullptr;
        ID3D11RasterizerState* raster_ = nullptr;
        ID3D11DepthStencilState* depthState_ = nullptr;
        ID3D11SamplerState* sampler_ = nullptr;

        // Per-mesh textures + a 1x1 white fallback.
        std::vector<ID3D11Texture2D*> texList_;
        std::vector<ID3D11ShaderResourceView*> srvList_;
        ID3D11ShaderResourceView* whiteSrv_ = nullptr;
        ID3D11Texture2D* whiteTex_ = nullptr;
        std::vector<DrawRange> ranges_;

        // Offscreen target.
        int texW_ = 0, texH_ = 0;
        ID3D11Texture2D* colorTex_ = nullptr;
        ID3D11RenderTargetView* rtv_ = nullptr;
        ID3D11ShaderResourceView* srv_ = nullptr;
        ID3D11Texture2D* depthTex_ = nullptr;
        ID3D11DepthStencilView* dsv_ = nullptr;

        uint32_t totalIndices_ = 0;
        math::Vector3 center_;
        float radius_ = 1.0f;
    };
}
