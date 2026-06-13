#include "viewport.h"

#include <d3dcompiler.h>
#include <algorithm>
#include <cmath>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

namespace evw::gui
{
    namespace
    {
        struct Vertex { float px, py, pz; float nx, ny, nz; float u, v; };
        struct CB { float wvp[16]; float lightDir[3]; float pad; };

        const char* kShader = R"(
cbuffer CB : register(b0)
{
    row_major float4x4 wvp;
    float3 lightDir;
    float  pad;
};
Texture2D    tex0 : register(t0);
SamplerState samp0 : register(s0);
struct VSIn  { float3 pos : POSITION; float3 nrm : NORMAL; float2 uv : TEXCOORD0; };
struct VSOut { float4 pos : SV_POSITION; float3 nrm : NORMAL; float2 uv : TEXCOORD0; };
VSOut VSMain(VSIn i)
{
    VSOut o;
    o.pos = mul(float4(i.pos, 1.0), wvp);
    o.nrm = i.nrm;
    o.uv  = i.uv;
    return o;
}
float4 PSMain(VSOut i) : SV_TARGET
{
    float3 n = normalize(i.nrm);
    float ndl = saturate(dot(n, -normalize(lightDir)));
    float4 t = tex0.Sample(samp0, i.uv);
    float3 c = t.rgb * (0.35 + 0.65 * ndl);
    return float4(c, 1.0);
}
)";
    }

    bool Viewport::init(ID3D11Device* device, ID3D11DeviceContext* ctx)
    {
        device_ = device;
        ctx_ = ctx;

        ID3DBlob* vsb = nullptr; ID3DBlob* psb = nullptr; ID3DBlob* err = nullptr;
        if (FAILED(D3DCompile(kShader, std::strlen(kShader), nullptr, nullptr, nullptr,
                              "VSMain", "vs_4_0", 0, 0, &vsb, &err)))
        { if (err) err->Release(); return false; }
        if (FAILED(D3DCompile(kShader, std::strlen(kShader), nullptr, nullptr, nullptr,
                              "PSMain", "ps_4_0", 0, 0, &psb, &err)))
        { if (err) err->Release(); vsb->Release(); return false; }
        device_->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &vs_);
        device_->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, &ps_);

        D3D11_INPUT_ELEMENT_DESC il[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        device_->CreateInputLayout(il, 3, vsb->GetBufferPointer(), vsb->GetBufferSize(), &layout_);
        vsb->Release(); psb->Release();

        D3D11_BUFFER_DESC cbd{};
        cbd.ByteWidth = sizeof(CB);
        cbd.Usage = D3D11_USAGE_DYNAMIC;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        device_->CreateBuffer(&cbd, nullptr, &cbuffer_);

        D3D11_RASTERIZER_DESC rd{};
        rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_NONE; rd.DepthClipEnable = TRUE;
        device_->CreateRasterizerState(&rd, &raster_);

        D3D11_DEPTH_STENCIL_DESC dd{};
        dd.DepthEnable = TRUE; dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dd.DepthFunc = D3D11_COMPARISON_LESS;
        device_->CreateDepthStencilState(&dd, &depthState_);

        D3D11_SAMPLER_DESC sd{};
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        device_->CreateSamplerState(&sd, &sampler_);

        // 1x1 white fallback texture.
        uint32_t white = 0xFFFFFFFF;
        D3D11_TEXTURE2D_DESC wd{};
        wd.Width = 1; wd.Height = 1; wd.MipLevels = 1; wd.ArraySize = 1;
        wd.Format = DXGI_FORMAT_R8G8B8A8_UNORM; wd.SampleDesc.Count = 1;
        wd.Usage = D3D11_USAGE_IMMUTABLE; wd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA wsd{}; wsd.pSysMem = &white; wsd.SysMemPitch = 4;
        device_->CreateTexture2D(&wd, &wsd, &whiteTex_);
        device_->CreateShaderResourceView(whiteTex_, nullptr, &whiteSrv_);

        return vs_ && ps_ && layout_ && cbuffer_ && sampler_;
    }

    void Viewport::releaseTargets()
    {
        if (dsv_) { dsv_->Release(); dsv_ = nullptr; }
        if (depthTex_) { depthTex_->Release(); depthTex_ = nullptr; }
        if (srv_) { srv_->Release(); srv_ = nullptr; }
        if (rtv_) { rtv_->Release(); rtv_ = nullptr; }
        if (colorTex_) { colorTex_->Release(); colorTex_ = nullptr; }
    }

    void Viewport::releaseModel()
    {
        for (auto* s : srvList_) if (s) s->Release();
        for (auto* t : texList_) if (t) t->Release();
        srvList_.clear(); texList_.clear(); ranges_.clear();
        if (vbuffer_) { vbuffer_->Release(); vbuffer_ = nullptr; }
        if (ibuffer_) { ibuffer_->Release(); ibuffer_ = nullptr; }
        totalIndices_ = 0;
    }

    void Viewport::shutdown()
    {
        releaseTargets();
        releaseModel();
        if (whiteSrv_) { whiteSrv_->Release(); whiteSrv_ = nullptr; }
        if (whiteTex_) { whiteTex_->Release(); whiteTex_ = nullptr; }
        if (sampler_) { sampler_->Release(); sampler_ = nullptr; }
        if (depthState_) { depthState_->Release(); depthState_ = nullptr; }
        if (raster_) { raster_->Release(); raster_ = nullptr; }
        if (cbuffer_) { cbuffer_->Release(); cbuffer_ = nullptr; }
        if (layout_) { layout_->Release(); layout_ = nullptr; }
        if (ps_) { ps_->Release(); ps_ = nullptr; }
        if (vs_) { vs_->Release(); vs_ = nullptr; }
    }

    void Viewport::setModel(const evw::app::RenderModel& model)
    {
        releaseModel();

        // Upload textures.
        for (const auto& rt : model.textures)
        {
            ID3D11Texture2D* tex = nullptr;
            ID3D11ShaderResourceView* srv = nullptr;
            if (rt.width > 0 && rt.height > 0 && !rt.rgba.empty())
            {
                D3D11_TEXTURE2D_DESC td{};
                td.Width = rt.width; td.Height = rt.height; td.MipLevels = 1; td.ArraySize = 1;
                td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1;
                td.Usage = D3D11_USAGE_IMMUTABLE; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                D3D11_SUBRESOURCE_DATA sd{};
                sd.pSysMem = rt.rgba.data();
                sd.SysMemPitch = static_cast<UINT>(rt.width * 4);
                if (SUCCEEDED(device_->CreateTexture2D(&td, &sd, &tex)))
                    device_->CreateShaderResourceView(tex, nullptr, &srv);
            }
            texList_.push_back(tex);
            srvList_.push_back(srv);
        }

        std::vector<Vertex> verts;
        std::vector<uint32_t> indices;
        math::Vector3 mn(1e9f, 1e9f, 1e9f), mx(-1e9f, -1e9f, -1e9f);

        for (const auto& m : model.meshes)
        {
            uint32_t base = static_cast<uint32_t>(verts.size());
            uint32_t istart = static_cast<uint32_t>(indices.size());
            for (size_t i = 0; i < m.positions.size(); ++i)
            {
                Vertex v{};
                v.px = m.positions[i].X; v.py = m.positions[i].Y; v.pz = m.positions[i].Z;
                if (i < m.normals.size()) { v.nx = m.normals[i].X; v.ny = m.normals[i].Y; v.nz = m.normals[i].Z; }
                else { v.nx = 0; v.ny = 0; v.nz = 1; }
                if (i < m.texCoords.size()) { v.u = m.texCoords[i].X; v.v = m.texCoords[i].Y; }
                verts.push_back(v);
                mn.X = std::min(mn.X, v.px); mn.Y = std::min(mn.Y, v.py); mn.Z = std::min(mn.Z, v.pz);
                mx.X = std::max(mx.X, v.px); mx.Y = std::max(mx.Y, v.py); mx.Z = std::max(mx.Z, v.pz);
            }
            for (uint16_t idx : m.indices) indices.push_back(base + idx);
            uint32_t icount = static_cast<uint32_t>(indices.size()) - istart;
            if (icount > 0) ranges_.push_back({istart, icount, m.diffuseTexture});
        }

        if (verts.empty() || indices.empty()) return;

        center_ = math::Vector3((mn.X + mx.X) * 0.5f, (mn.Y + mx.Y) * 0.5f, (mn.Z + mx.Z) * 0.5f);
        math::Vector3 ext(mx.X - mn.X, mx.Y - mn.Y, mx.Z - mn.Z);
        radius_ = 0.5f * std::sqrt(ext.X * ext.X + ext.Y * ext.Y + ext.Z * ext.Z);
        if (radius_ < 0.001f) radius_ = 1.0f;

        D3D11_BUFFER_DESC vbd{};
        vbd.ByteWidth = static_cast<UINT>(verts.size() * sizeof(Vertex));
        vbd.Usage = D3D11_USAGE_IMMUTABLE; vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vsd{}; vsd.pSysMem = verts.data();
        device_->CreateBuffer(&vbd, &vsd, &vbuffer_);

        D3D11_BUFFER_DESC ibd{};
        ibd.ByteWidth = static_cast<UINT>(indices.size() * sizeof(uint32_t));
        ibd.Usage = D3D11_USAGE_IMMUTABLE; ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA isd{}; isd.pSysMem = indices.data();
        device_->CreateBuffer(&ibd, &isd, &ibuffer_);

        totalIndices_ = static_cast<uint32_t>(indices.size());
    }

    bool Viewport::ensureTargets(int width, int height)
    {
        if (width <= 0 || height <= 0) return false;
        if (colorTex_ && texW_ == width && texH_ == height) return true;
        releaseTargets();
        texW_ = width; texH_ = height;

        D3D11_TEXTURE2D_DESC td{};
        td.Width = width; td.Height = height; td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(device_->CreateTexture2D(&td, nullptr, &colorTex_))) return false;
        device_->CreateRenderTargetView(colorTex_, nullptr, &rtv_);
        device_->CreateShaderResourceView(colorTex_, nullptr, &srv_);

        D3D11_TEXTURE2D_DESC dd{};
        dd.Width = width; dd.Height = height; dd.MipLevels = 1; dd.ArraySize = 1;
        dd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; dd.SampleDesc.Count = 1;
        dd.Usage = D3D11_USAGE_DEFAULT; dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        if (FAILED(device_->CreateTexture2D(&dd, nullptr, &depthTex_))) return false;
        device_->CreateDepthStencilView(depthTex_, nullptr, &dsv_);
        return true;
    }

    ID3D11ShaderResourceView* Viewport::render(int width, int height, float yaw, float pitch, float distScale)
    {
        if (!ensureTargets(width, height)) return nullptr;

        const float clear[4] = {0.12f, 0.12f, 0.14f, 1.0f};
        ctx_->ClearRenderTargetView(rtv_, clear);
        ctx_->ClearDepthStencilView(dsv_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        if (totalIndices_ == 0 || !vbuffer_ || !ibuffer_) return srv_;

        float dist = radius_ * 2.5f * distScale;
        math::Vector3 eye(
            center_.X + dist * std::cos(pitch) * std::sin(yaw),
            center_.Y + dist * std::sin(pitch),
            center_.Z + dist * std::cos(pitch) * std::cos(yaw));
        math::Matrix view = math::lookAtRH(eye, center_, math::Vector3(0, 1, 0));
        math::Matrix proj = math::perspectiveFovRH(1.0472f, static_cast<float>(width) / height,
                                                   radius_ * 0.01f + 0.05f, radius_ * 10.0f + 100.0f);
        math::Matrix wvp = math::mul(view, proj);

        CB cb{};
        std::memcpy(cb.wvp, &wvp.M11, sizeof(cb.wvp));
        cb.lightDir[0] = -0.4f; cb.lightDir[1] = -0.8f; cb.lightDir[2] = -0.4f;
        D3D11_MAPPED_SUBRESOURCE ms;
        if (SUCCEEDED(ctx_->Map(cbuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)))
        { std::memcpy(ms.pData, &cb, sizeof(cb)); ctx_->Unmap(cbuffer_, 0); }

        D3D11_VIEWPORT vp{};
        vp.Width = static_cast<float>(width); vp.Height = static_cast<float>(height);
        vp.MaxDepth = 1.0f;
        ctx_->RSSetViewports(1, &vp);
        ctx_->OMSetRenderTargets(1, &rtv_, dsv_);
        ctx_->OMSetDepthStencilState(depthState_, 0);
        ctx_->RSSetState(raster_);

        UINT stride = sizeof(Vertex), offset = 0;
        ctx_->IASetInputLayout(layout_);
        ctx_->IASetVertexBuffers(0, 1, &vbuffer_, &stride, &offset);
        ctx_->IASetIndexBuffer(ibuffer_, DXGI_FORMAT_R32_UINT, 0);
        ctx_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx_->VSSetShader(vs_, nullptr, 0);
        ctx_->VSSetConstantBuffers(0, 1, &cbuffer_);
        ctx_->PSSetShader(ps_, nullptr, 0);
        ctx_->PSSetSamplers(0, 1, &sampler_);

        for (const auto& r : ranges_)
        {
            ID3D11ShaderResourceView* srv = whiteSrv_;
            if (r.texIndex >= 0 && r.texIndex < static_cast<int>(srvList_.size()) && srvList_[r.texIndex])
                srv = srvList_[r.texIndex];
            ctx_->PSSetShaderResources(0, 1, &srv);
            ctx_->DrawIndexed(r.indexCount, r.indexStart, 0);
        }

        return srv_;
    }
}
