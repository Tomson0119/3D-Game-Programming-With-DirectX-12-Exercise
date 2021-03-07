GeometryGenerator::MeshData ShapesApp::CreateSkull()
{
    GeometryGenerator::MeshData mesh;

    std::ifstream file("skull.txt");
    std::string line;

    if (!file.is_open())
    {
        MessageBox(0, L"File not found.", 0, 0);
        assert(false);
    }

    int vertexCount = 0;
    int triangleCount = 0;
    std::string ignore;

    file >> ignore >> vertexCount;
    file >> ignore >> triangleCount;
    file >> ignore >> ignore >> ignore >> ignore;

    int indexCount = triangleCount * 3;

    mesh.Vertices.resize(vertexCount);
    mesh.Indices32.resize(indexCount);

    for (size_t i = 0; i < vertexCount; ++i)
    {
        file >> mesh.Vertices[i].Position.x >> mesh.Vertices[i].Position.y
            >> mesh.Vertices[i].Position.z;
        file >> mesh.Vertices[i].Normal.x >> mesh.Vertices[i].Normal.y
            >> mesh.Vertices[i].Normal.z;
        mesh.Vertices[i].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);
        mesh.Vertices[i].TexC = XMFLOAT2(0.0f, 0.0f);
    }

    file >> ignore >> ignore >> ignore;

    for (size_t i = 0; i < indexCount; ++i)
        file >> mesh.Indices32[i];

    file.close();

    return mesh;
}
