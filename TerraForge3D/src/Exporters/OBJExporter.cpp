#include "Exporters/OBJExporter.h"
#include "Base/BinaryFileWriter.h"
#include "Utils/Utils.h"

#include <string>
#include <sstream>

OBJExporter::OBJExporter()
{
}

OBJExporter::~OBJExporter()
{
}

bool OBJExporter::Export(const std::string& path, Mesh* mesh, float* progress)
{
	if (!progress) progress = &m_Progress; *progress = 0.0f;
	BinaryFileWriter writer(path);
	if (!writer.IsOpen()) return false;
	std::stringstream out_strm;
	if (!this->WriteHeader(out_strm, mesh, progress)) return false; *progress = 0.05f;
	if (!this->WriteVertices(out_strm, mesh, progress)) return false; *progress = 0.2f;
	if (!this->WriteNormals(out_strm, mesh, progress)) return false; *progress = 0.4f;
	if (!this->WriteTextureCoordinates(out_strm, mesh, progress)) return false; *progress = 0.6f;
	if (!this->WriteFaces(out_strm, mesh, progress)) return false; *progress = 0.8f;
	const std::string out_str = out_strm.str();
	writer.Write(out_str.data(), out_str.size());
	*progress = 1.0f;
	return true;
}

bool OBJExporter::WriteHeader(std::stringstream& out_strm, Mesh* mesh, float* progress)
{
	out_strm << "# Generated by TerraForge3D Gen 2\n";
	out_strm << "# Generation time : " << GetTimeStamp() << "\n\n";
	return true;
}

bool OBJExporter::WriteVertices(std::stringstream& out_strm, Mesh* mesh, float* progress)
{
	out_strm << "# Vertices\n";
	for (size_t i = 0; i < mesh->vertexCount; i++)
	{
		const auto& pos = mesh->vert[i].position;
		sprintf(buffer, "v %f %f %f\n", pos.x, pos.y, pos.z);
		out_strm << buffer;
	}
	out_strm << '\n';
	return true;
}

bool OBJExporter::WriteNormals(std::stringstream& out_strm, Mesh* mesh, float* progress)
{
	out_strm << "# Normals\n";
	for (size_t i = 0; i < mesh->vertexCount; i++)
	{
		const auto& nor = mesh->vert[i].normal;
		sprintf(buffer, "vn %f %f %f\n", nor.x, nor.y, nor.z);
		out_strm << buffer;
	}
	out_strm << '\n';
	return true;
}

bool OBJExporter::WriteTextureCoordinates(std::stringstream& out_strm, Mesh* mesh, float* progress)
{
	out_strm << "# Texture Coordinates\n";
	for (size_t i = 0; i < mesh->vertexCount; i++)
	{
		const auto& tex = mesh->vert[i].texCoord;
		sprintf(buffer, "vt %f %f\n", tex.x, tex.y);
		out_strm << buffer;
	}
	out_strm << '\n';
	return true;
}

bool OBJExporter::WriteFaces(std::stringstream& out_strm, Mesh* mesh, float* progress)
{
	out_strm << "# Faces\n";
	for (size_t i = 0; i < mesh->indexCount; i+=3)
	{
		auto index_a = mesh->indices[i + 0] + 1;
		auto index_b = mesh->indices[i + 1] + 1;
		auto index_c = mesh->indices[i + 2] + 1;
		sprintf(buffer, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",
						index_a, index_a, index_a,
						index_b, index_b, index_b,
						index_c, index_c, index_c);
		out_strm << buffer;
	}
	out_strm << '\n';
	return true;
}
