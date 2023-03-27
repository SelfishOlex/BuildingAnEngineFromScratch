#pragma once

#include <fbxsdk.h>
#include <vector>
#include <Vertex.h>

namespace Asset
{
    class FbxLoader
    {
    public:
        explicit FbxLoader( const char* pathToFbxFile );

        struct Mesh
        {
            std::vector<Vertex> m_vertices;
            std::vector<uint16_t> m_indices;
        };

        [[nodiscard]] const std::vector<Mesh>& GetMeshes() const { return m_meshes; }

    private:
        std::vector<Mesh> m_meshes;
        Mesh ReadMesh( FbxNodeAttribute* pAttribute );

        /* Tab character ("\t") counter */
        int m_numTabs = 0;

        void PrintNode( FbxNode* pNode );
        void PrintTabs();
        void PrintAttribute( FbxNodeAttribute* pAttribute );
        FbxString GetAttributeTypeName( FbxNodeAttribute::EType type );
    };
}
