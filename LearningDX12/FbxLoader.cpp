#include "FbxLoader.h"

#include <WinString.h>

namespace Olex
{
    FbxLoader::FbxLoader( const char* pathToFbxFile )
    {
        // Initialize the SDK manager. This object handles memory management.
        FbxManager* lSdkManager = FbxManager::Create();

        // Create the IO settings object.
        FbxIOSettings* ios = FbxIOSettings::Create( lSdkManager, IOSROOT );
        lSdkManager->SetIOSettings( ios );

        // Create an importer using the SDK manager.
        FbxImporter* lImporter = FbxImporter::Create( lSdkManager, "" );

        // Use the first argument as the filename for the importer.
        if ( !lImporter->Initialize( pathToFbxFile, -1, lSdkManager->GetIOSettings() ) )
        {
            throw std::exception( lImporter->GetStatus().GetErrorString() );
        }

        // Create a new scene so that it can be populated by the imported file.
        FbxScene* lScene = FbxScene::Create( lSdkManager, "myScene" );

        // Import the contents of the file into the scene.
        lImporter->Import( lScene );

        // The file is imported, so get rid of the importer.
        lImporter->Destroy();

        // Print the nodes of the scene and their attributes recursively.
        // Note that we are not printing the root node because it should
        // not contain any attributes.
        FbxNode* lRootNode = lScene->GetRootNode();
        if ( lRootNode )
        {
            for ( int i = 0; i < lRootNode->GetChildCount(); i++ )
            {
                PrintNode( lRootNode->GetChild( i ) );
            }
        }
        // Destroy the SDK manager and all the other objects it was handling.
        lSdkManager->Destroy();
    }

    struct Log
    {
        template <typename ...Args>
        static void Message( const char* format, Args ...args )
        {
            char buffer[1000];
            sprintf_s( buffer, _countof( buffer ), format, std::forward<Args>( args )... );
            OutputDebugStringA( buffer );
        }
    };

    /**
     * Print a node, its attributes, and all its children recursively.
     */
    void FbxLoader::PrintNode( FbxNode* pNode )
    {
        PrintTabs();
        const char* nodeName = pNode->GetName();
        FbxDouble3 translation = pNode->LclTranslation.Get();
        FbxDouble3 rotation = pNode->LclRotation.Get();
        FbxDouble3 scaling = pNode->LclScaling.Get();

        // Print the contents of the node.
        Log::Message( "<node name='%s' translation='(%f, %f, %f)' rotation='(%f, %f, %f)' scaling='(%f, %f, %f)'>\n",
            nodeName,
            translation[0], translation[1], translation[2],
            rotation[0], rotation[1], rotation[2],
            scaling[0], scaling[1], scaling[2]
        );

        numTabs++;

        // Print the node's attributes.
        for ( int i = 0; i < pNode->GetNodeAttributeCount(); i++ )
            PrintAttribute( pNode->GetNodeAttributeByIndex( i ) );

        // Recursively print the children.
        for ( int j = 0; j < pNode->GetChildCount(); j++ )
            PrintNode( pNode->GetChild( j ) );

        numTabs--;
        PrintTabs();
        Log::Message( "</node>\n" );
    }

    void FbxLoader::PrintTabs()
    {
        for ( int i = 0; i < numTabs; i++ )
            Log::Message( "\t" );
    }

    void FbxLoader::PrintAttribute( FbxNodeAttribute* pAttribute )
    {
        if ( !pAttribute ) return;

        FbxString typeName = GetAttributeTypeName( pAttribute->GetAttributeType() );
        FbxString attrName = pAttribute->GetName();
        PrintTabs();

        // Note: to retrieve the character array of a FbxString, use its Buffer() method.
        Log::Message( "<attribute type='%s' name='%s'/>\n", typeName.Buffer(), attrName.Buffer() );

        if ( pAttribute->GetAttributeType() == FbxNodeAttribute::eMesh )
        {
            m_meshes.push_back( ReadMesh( pAttribute ) );
        }
    }

    FbxLoader::Mesh FbxLoader::ReadMesh( FbxNodeAttribute* pAttribute )
    {
        Mesh mesh;

        if ( FbxMesh* fbxMesh = pAttribute->GetNode()->GetMesh() )
        {
            {
                const FbxVector4* vertexBuffer = fbxMesh->GetControlPoints();
                const int vertexCount = fbxMesh->GetControlPointsCount();
                mesh.m_vertices.resize( vertexCount );

                for ( int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex )
                {
                    const double* buffer = vertexBuffer[vertexIndex].Buffer();
                    const DirectX::XMFLOAT3 vertex = { static_cast<float>( buffer[0] ), static_cast<float>( buffer[1] ), static_cast<float>( buffer[2] ) };
                    mesh.m_vertices[vertexIndex].m_position = vertex;
                }
            }

            const bool check = fbxMesh->IsTriangleMesh();
            if ( check == false )
            {
                throw std::exception( "Only supported triangles in fbx mesh!" );
            }

            const bool hasUV = fbxMesh->GetElementUVCount() > 0;

            FbxStringList lUVNames;
            fbxMesh->GetUVSetNames( lUVNames );
            const auto uvCount = lUVNames.GetCount();
            const char* uvName = lUVNames[0]; ///

            const int polygonCount = fbxMesh->GetPolygonCount();
            mesh.m_indices.reserve( polygonCount );

            for ( int polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex )
            {
                const int vertexIndex0 = fbxMesh->GetPolygonVertex( polygonIndex, 0 );
                const int vertexIndex1 = fbxMesh->GetPolygonVertex( polygonIndex, 1 );
                const int vertexIndex2 = fbxMesh->GetPolygonVertex( polygonIndex, 2 );

                mesh.m_indices.emplace_back( vertexIndex0, vertexIndex2, vertexIndex1 );

                bool unmapped;
                FbxVector2 uv;
                bool result = fbxMesh->GetPolygonVertexUV( polygonIndex, 0, uvName, uv, unmapped );
                mesh.m_vertices[vertexIndex0].m_uv = { static_cast<float>( uv.Buffer()[0] ), static_cast<float>( uv.Buffer()[1] ) };
                result = fbxMesh->GetPolygonVertexUV( polygonIndex, 1, uvName, uv, unmapped );
                mesh.m_vertices[vertexIndex1].m_uv = { static_cast<float>( uv.Buffer()[0] ), static_cast<float>( uv.Buffer()[1] ) };
                result = fbxMesh->GetPolygonVertexUV( polygonIndex, 2, uvName, uv, unmapped );
                mesh.m_vertices[vertexIndex2].m_uv = { static_cast<float>( uv.Buffer()[0] ), static_cast<float>( uv.Buffer()[1] ) };

                FbxVector4 normal;
                result = fbxMesh->GetPolygonVertexNormal( polygonIndex, 0, normal );
                mesh.m_vertices[vertexIndex0].m_normal = { static_cast<float>( normal.Buffer()[0] ), static_cast<float>( normal.Buffer()[1] ), static_cast<float>( normal.Buffer()[2] ) };
                result = fbxMesh->GetPolygonVertexNormal( polygonIndex, 1, normal );
                mesh.m_vertices[vertexIndex1].m_normal = { static_cast<float>( normal.Buffer()[0] ), static_cast<float>( normal.Buffer()[1] ), static_cast<float>( normal.Buffer()[2] ) };
                result = fbxMesh->GetPolygonVertexNormal( polygonIndex, 2, normal );
                mesh.m_vertices[vertexIndex2].m_normal = { static_cast<float>( normal.Buffer()[0] ), static_cast<float>( normal.Buffer()[1] ), static_cast<float>( normal.Buffer()[2] ) };
            }
        }

        return mesh;
    }

    FbxString FbxLoader::GetAttributeTypeName( FbxNodeAttribute::EType type )
    {
        switch ( type ) {
        case FbxNodeAttribute::eUnknown: return "unidentified";
        case FbxNodeAttribute::eNull: return "null";
        case FbxNodeAttribute::eMarker: return "marker";
        case FbxNodeAttribute::eSkeleton: return "skeleton";
        case FbxNodeAttribute::eMesh: return "mesh";
        case FbxNodeAttribute::eNurbs: return "nurbs";
        case FbxNodeAttribute::ePatch: return "patch";
        case FbxNodeAttribute::eCamera: return "camera";
        case FbxNodeAttribute::eCameraStereo: return "stereo";
        case FbxNodeAttribute::eCameraSwitcher: return "camera switcher";
        case FbxNodeAttribute::eLight: return "light";
        case FbxNodeAttribute::eOpticalReference: return "optical reference";
        case FbxNodeAttribute::eOpticalMarker: return "marker";
        case FbxNodeAttribute::eNurbsCurve: return "nurbs curve";
        case FbxNodeAttribute::eTrimNurbsSurface: return "trim nurbs surface";
        case FbxNodeAttribute::eBoundary: return "boundary";
        case FbxNodeAttribute::eNurbsSurface: return "nurbs surface";
        case FbxNodeAttribute::eShape: return "shape";
        case FbxNodeAttribute::eLODGroup: return "lodgroup";
        case FbxNodeAttribute::eSubDiv: return "subdiv";
        default: return "unknown";
        }
    }
}
