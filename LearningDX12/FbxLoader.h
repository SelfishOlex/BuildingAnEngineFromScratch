#pragma once

#include <DirectXMath.h>
#include <fbxsdk.h>
#include <vector>

namespace Olex
{
    class FbxLoader
    {
    public:
        explicit FbxLoader( const char* pathToFbxFile );

        struct Mesh
        {
            struct VertexInfo
            {
                DirectX::XMFLOAT3 m_position;
                //DirectX::XMFLOAT3 m_normal;
                DirectX::XMFLOAT2 m_uv;
            };

            std::vector<VertexInfo> m_vertices;
            std::vector<DirectX::XMINT3> m_indices;
        };

        [[nodiscard]] const std::vector<Mesh>& GetMeshes() const { return m_meshes; }

    private:
        std::vector<Mesh> m_meshes;
        Mesh ReadMesh( FbxNodeAttribute* pAttribute );

        /* Tab character ("\t") counter */
        int numTabs = 0;

        void PrintNode( FbxNode* pNode );
        void PrintTabs();
        void PrintAttribute( FbxNodeAttribute* pAttribute );
        FbxString GetAttributeTypeName( FbxNodeAttribute::EType type );
    };
}
