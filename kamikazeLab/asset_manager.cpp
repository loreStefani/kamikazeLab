/* asset_manager.cpp :
 * all methods of all classes and all global functions which deal with managing assets:
 * Importing, procedural construction, maybe a bit of pre-processing
 */

#include <fstream>
#include <sstream>
#include <string>

#include"mesh.h"
#include"custom_classes.h"
#include"texture.h"
#include "shader.h"
#include "texture_cube.h"
#include "assets.h"

std::string g_assetsPath = "assets/";
std::string g_shadersPath = g_assetsPath + "shaders/";

MeshLibrary g_meshLibrary{};
TextureLibrary g_textureLibrary{};
ProgramLibrary g_programLibrary{};
TextureCubeLibrary g_textureCubeLibrary{};

void preloadAllAssets(){

	const std::string darkFighterPath = g_assetsPath + "dark_fighter/";

	CpuMesh shipMesh;
	shipMesh.import(darkFighterPath + "dark_fighter_6.obj");

	CpuTexture shipDiffuseTexture;
	shipDiffuseTexture.import(darkFighterPath + "dark_fighter_6_color.pbm");
	
	CpuTexture shipNormalTexture;
	shipNormalTexture.import(darkFighterPath + "dark_fighter_6_normal.pbm");
	shipNormalTexture.isLinear = true;
	
	CpuTexture shipSpecularTexture;
	shipSpecularTexture.import(darkFighterPath + "dark_fighter_6_specular.pbm");
	shipSpecularTexture.isLinear = true;

	g_meshLibrary.add("ShipMesh", shipMesh);
	g_textureLibrary.add("ShipDiffuseMap", shipDiffuseTexture);
	g_textureLibrary.add("ShipNormalMap", shipNormalTexture);
	g_textureLibrary.add("ShipSpecularMap", shipSpecularTexture);
	
	const std::string missilePath = g_assetsPath + "missile/";

	CpuMesh bulletMesh;
	bulletMesh.import(missilePath + "missile.obj");

	CpuTexture bulletDiffuseTexture;
	bulletDiffuseTexture.import(missilePath + "hellfire_diffuse.pbm");

	CpuTexture bulletNormalTexture;
	bulletNormalTexture.import(missilePath + "hellfire_NRM.pbm");
	bulletNormalTexture.isLinear = true;

	CpuTexture bulletSpecularTexture;
	bulletSpecularTexture.import(missilePath + "hellfire_SPEC.pbm");
	bulletSpecularTexture.isLinear = true;
	
	g_meshLibrary.add("BulletMesh", bulletMesh);
	g_textureLibrary.add("BulletDiffuseMap", bulletDiffuseTexture);
	g_textureLibrary.add("BulletNormalMap", bulletNormalTexture);
	g_textureLibrary.add("BulletSpecularMap", bulletSpecularTexture);


	const std::string floorPath = g_assetsPath + "floor/";
	
	CpuMesh floorMesh;
	floorMesh.buildGrid(1.0f, 1.0f, 10, 10);
	
	CpuTexture floorDiffuseTexture;
	floorDiffuseTexture.import(floorPath + "Foothills_of_Ariloa.pbm");
	
	CpuTexture floorNormalTexture;
	floorNormalTexture.import(floorPath + "Foothills_of_Ariloa_NRM.pbm");
	floorNormalTexture.isLinear = true;

	CpuTexture floorSpecularTexture;
	floorSpecularTexture.import(floorPath + "Foothills_of_Ariloa_SPEC.pbm");
	floorSpecularTexture.isLinear = true;

	g_meshLibrary.add("FloorMesh", floorMesh);
	g_textureLibrary.add("FloorDiffuseMap", floorDiffuseTexture);
	g_textureLibrary.add("FloorNormalMap", floorNormalTexture);
	g_textureLibrary.add("FloorSpecularMap", floorSpecularTexture);
	

	std::vector<std::string> cubeMapFaces{ "posx.pbm", "negx.pbm", "posy.pbm", "negy.pbm", "posz.pbm", "negz.pbm" };
	for (int i = 0; i < 6; ++i)
	{
		cubeMapFaces[i] = g_assetsPath + "envmap_interstellar/" + cubeMapFaces[i];
	}

	CpuTextureCube textureCube;
	textureCube.import(cubeMapFaces);

	g_textureCubeLibrary.add("SkyBox", textureCube);
}

void CpuMesh::addQuad(int i, int j, int k, int h){
	tris.push_back( Tri(i,j,k) );
	tris.push_back( Tri(k,h,i) );
}

void CpuMesh::buildTorus(int ni, int nj, float innerRadius, float outerRadius){

	verts.reserve( ni*nj );
	tris.reserve( ni*nj*2 );

	for (int j=0; j<nj; j++)
	for (int i=0; i<ni; i++) {

		// construct geometry
		double alpha = 2*3.1415 * i / ni;
		float x = (float)cos(alpha) * innerRadius;
		float y = (float)sin(alpha) * innerRadius;
		x += outerRadius;

		double beta = 2*3.1415 * j / nj;

		Vertex v;
		v.pos.x = (float)cos(beta) * x;
		v.pos.y = y;
		v.pos.z = (float)sin(beta) * x;

		verts.push_back( v );

		// construct connectivity
		addQuad( j        * ni +  i       ,
				 j        * ni + (i+1)%ni ,
				 (j+1)%nj * ni + (i+1)%ni ,
				 (j+1)%nj * ni +  i       );
	}

}

void CpuMesh::buildGrid(float width, float depth, int m, int n)
{
	//Adapted from '3D Game Programming with DirectX 11' by Frank Luna (http://www.d3dcoder.net/d3d11.htm)
		
	const int vertexCount = m*n;
	const int faceCount = (m - 1)*(n - 1) * 2;

	verts.clear();
	tris.clear();
	verts.resize(vertexCount);
	tris.resize(faceCount);

	const float halfWidth = 0.5f*width;
	const float halfDepth = 0.5f*depth;

	const float dx = width / (n - 1);
	const float dz = depth / (m - 1);
	const float du = 1.0f / (n - 1);
	const float dv = 1.0f / (m - 1);

	for (int i = 0; i < m; i++)
	{
		const float z = - halfDepth + i*dz;
		for (int j = 0; j < n; j++)
		{
			Vertex& vertex = verts[i*n + j];

			//position
			const float x = -halfWidth + j*dx;
			glm::vec3& v = vertex.pos;
			v.x = x;
			v.y = 0.0f;
			v.z = z;

			//normal
			glm::vec3& n = vertex.norm;
			n.x = 0.0f;
			n.y = 1.0f,
			n.z = 0.0f;

			//textcoord
			glm::vec2& textCoord = vertex.uv;
			textCoord.x = j*du;
			textCoord.y = 1.0f - i*dv;
			
			//tangent
			glm::vec4& tangentWithHandedness = vertex.tang;
			tangentWithHandedness = glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f};
		}
	}
			
	for (int i = 0, k = 0; i < m - 1; i++)
	{
		const int rowIndex = i*n;
		const int nextRowIndex = rowIndex + n;
		for (int j = 0; j < n - 1; j++, k += 2)
		{
			Tri& firstTri = tris[k];
			firstTri.i = rowIndex + j;
			firstTri.j = nextRowIndex + j;
			firstTri.k = rowIndex + j + 1;

			Tri& secondTri = tris[k + 1];
			secondTri.i = nextRowIndex + j;
			secondTri.j = nextRowIndex + j + 1;
			secondTri.k = rowIndex + j + 1;
		}
	}
}

void CpuMesh::buildSphere(float radius, int sliceCount, int stackCount)
{
	//Adapted from '3D Game Programming with DirectX 11' by Frank Luna (http://www.d3dcoder.net/d3d11.htm)

	verts.clear();
	tris.clear();

	Vertex northPole;
	northPole.pos = glm::vec3{ 0.0f, radius, 0.0f };
	northPole.norm = glm::vec3{0.0f, 1.0f, 0.0};
	northPole.uv = glm::vec2{0.0f, 1.0f};
	northPole.tang = glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};

	verts.push_back(northPole);

	const float pi = glm::pi<float>();
	const float _2pi = 2.0f*pi;

	const float phiStep = pi / stackCount;
	const float thetaStep = _2pi / sliceCount;

	for (int i = 1; i <= stackCount - 1; i++)
	{
		const float phi = i*phiStep;

		for (int j = 0; j <= sliceCount; j++)
		{
			Vertex v;

			const float theta = j*thetaStep;

			const float radiusTimesSinPhi = radius*std::sinf(phi);
			const float x = radiusTimesSinPhi*std::cosf(theta);
			const float y = radius*std::cosf(phi);
			const float z = radiusTimesSinPhi*std::sinf(theta);
			
			v.pos = glm::vec3{ x, y, z };
			v.norm = glm::normalize(glm::vec3{x, y, z});
			v.uv = glm::vec2{ theta / _2pi, 1.0f - phi / pi };
			v.tang = glm::normalize(glm::vec4{z, 0.0f, -x, 0.0f}); //-dPos/dTheta (u coordinate decreases as theta increases)
			v.tang.w = 1.0f,
			
			verts.push_back(v);
		}
	}

	Vertex southPole;
	southPole.pos = glm::vec3{ 0.0f, -radius, 0.0f };
	southPole.norm = glm::vec3{ 0.0f, -1.0f, 0.0f };
	southPole.uv = glm::vec2{ 0.0f , 0.0f };
	southPole.tang = glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f };
	
	verts.push_back(southPole);

	for (int i = 1; i <= sliceCount; i++)
	{
		Tri tri;
		tri.i = 0;
		tri.j = i;
		tri.k = i + 1;

		tris.push_back(tri);
	}

	int baseIndex = 1;
	int ringVertexCount = sliceCount + 1;
	for (int i = 0; i < stackCount - 2; i++)
	{
		const int ringIndex = baseIndex + i*ringVertexCount;
		const int nextRingIndex = ringIndex + ringVertexCount;
		for (int j = 0; j < sliceCount; j++)
		{
			Tri tri{ ringIndex + j, nextRingIndex + j, ringIndex + j + 1 };
			tris.push_back(tri);

			tri.i = nextRingIndex + j;
			tri.j = nextRingIndex + j + 1;
			tri.k = ringIndex + j + 1;
			tris.push_back(tri);
		}
	}

	const int southPoleIndex = (int)verts.size() - 1;
	baseIndex = southPoleIndex - ringVertexCount;

	for (int i = 0; i < sliceCount; ++i)
	{
		Tri tri;
		tri.i = southPoleIndex;
		tri.j = baseIndex + i + 1;
		tri.k = baseIndex + i;
		tris.push_back(tri);
	}
}

void CpuMesh::buildFullScreenQuad()
{
	verts.clear();
	tris.clear();

	verts.resize(4);
	tris.resize(2);

	Vertex& v0 = verts[0];
	Vertex& v1 = verts[1];
	Vertex& v2 = verts[2];
	Vertex& v3 = verts[3];

	v0.pos = glm::vec3{ -1.0f, -1.0f, -1.0f };// Lower left
	v1.pos = glm::vec3{1.0f, -1.0f, -1.0f};// Lower right
	v2.pos = glm::vec3{ 1.0, 1.0, -1.0}; //Upper right
	v3.pos = glm::vec3{ -1.0, 1.0, -1.0 }; // Upper left

	v0.uv = glm::vec2{ 0.0f, 0.0f };
	v1.uv = glm::vec2{ 1.0f, 0.0f };
	v2.uv = glm::vec2{ 1.0f, 1.0f };
	v3.uv = glm::vec2{ 0.0f, 1.0f };

	for (int i = 0; i < 4; ++i)
	{
		verts[i].norm = glm::vec3{ 0.0f, 0.0f, -1.0f }; //ndc space is left handed
		verts[i].tang = glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f };
	}
	
	tris[0] = Tri{ 0, 1, 2 };
	tris[1] = Tri{ 0, 2, 3 };	
}

bool CpuMesh::import(const std::string& filename){
	std::ifstream infile(filename);
	if (!infile.is_open()) return false;
	std::string line;

	std::vector< vec3 > tmpV;
	std::vector< vec3 > tmpN;

	while ( std::getline(infile, line) ){

		std::istringstream iss(line);

		std::string code;
		iss >> code;
		if (code=="v") {
			float x,y,z;
			iss >> x >> y >> z;
			tmpV.push_back( vec3(x,z,y) );
		} else if (code=="vn") {
			float x,y,z;
			iss >> x >> y >> z;
			tmpN.push_back( vec3(x,z,y) );
		} else if (code=="vt") {
			float x,y;
			iss >> x >> y;
			Vertex nv;
			nv.uv = vec2(x,1.0f-y); // NB: flipping the y, different conventions about UV space
			verts.push_back( nv );
			// v.pos e v.norm: ci penso dopo, quando leggero' le facce
		} else if (code=="f") {
			std::string st_i, st_j, st_k;
			iss >> st_i >> st_j >> st_k;
			int i0,i1,i2;
			int j0,j1,j2;
			int k0,k1,k2;
			sscanf_s( st_i.c_str() , "%d/%d/%d" , &i0,&i1,&i2 );
			sscanf_s( st_j.c_str() , "%d/%d/%d" , &j0,&j1,&j2 );
			sscanf_s( st_k.c_str() , "%d/%d/%d" , &k0,&k1,&k2 );

			//  Obj indices start from 1 not 0!
			i0--;j0--;k0--;
			i1--;j1--;k1--;
			i2--;j2--;k2--;

			Tri nt( i1, j1, k1 );
			tris.push_back( nt );

			verts[ i1 ].pos = tmpV[ i0 ];
			verts[ i1 ].norm = tmpN[ i2 ];
			verts[ j1 ].pos = tmpV[ j0 ];
			verts[ j1 ].norm = tmpN[ j2 ];
			verts[ k1 ].pos = tmpV[ k0 ];
			verts[ k1 ].norm = tmpN[ k2 ];

		}

	}

	computeTangents();
	return true;
}

void CpuMesh::computeTangents()
{
	assert((verts.size() > 0 && tris.size() > 0) || (verts.size() == 0 && tris.size() == 0));

	/**
	*Adapted from 'Mathematics for 3D Game Programming and Computer Graphics' by Eric Lengyel
	*(http://www.mathfor3dgameprogramming.com/)
	*/

	const size_t count = verts.size();
	std::vector<glm::vec3> perVertexBitangent(count, glm::vec3{});
	const size_t triangleCount = tris.size();

	for (size_t i = 0; i < triangleCount; i++)
	{
		int t0 = tris[i].i;
		int t1 = tris[i].j;
		int t2 = tris[i].k;

		const glm::vec3& p0 = verts[t0].pos;
		const glm::vec3& p1 = verts[t1].pos;
		const glm::vec3& p2 = verts[t2].pos;

		const glm::vec2& uv0 = verts[t0].uv;
		const glm::vec2& uv1 = verts[t1].uv;
		const glm::vec2& uv2 = verts[t2].uv;

		/*
		* let T and B be the tangent and bitangent vector of the triangle, then in tangent space for any position Q
		* with texture coordinates (u,v) inside the triangle it holds that:
		* Q - Pi = (u - ui)*T + (v - vi)*B   where Pi is the position of any of the triangle's vertices and (ui, vi) its texture coordinates
		* Using p0 as Pi, p1 and p2 as Q :
		* p1 - p0 = (uv1.x - uv0.x) * T +  (uv1.y - uv0.y) * B
		* p2 - p0 = (uv2.x - uv0.x) * T +  (uv2.y - uv0.y) * B
		* this is a linear system with six unknowns and six equations :
		*   u1 - u0   v1 - v0      Tx   Ty   Tz     p1x - p0x   p1y - p0y   p1z - p0z
		*                       *                =
		*   u2 - u0   v2 - v0      Bx   By   Bz     p2x - p0x   p2y - p0y   p2z - p0z
		*
		*/

		/*
		* invert the coefficients matrix
		* a   b                d   -b
		*        =  1.0/det *
		* c   d               -c    a
		*
		*/

		/*
		*
		*   Tx   Ty   Tz                v2 - v0   -(v1 - v0)       p1x - p0x   p1y - p0y   p1z - p0z
		*                = 1.0/det *                          *
		*   Bx   By   Bz              -(u2 - u0)    u1 - u0        p2x - p0x   p2y - p0y   p2z - p0z
		*
		*
		*/

		// only the first row is required for the tangent, but the bitangent is necessary to compute the tangent space handedness

		const float u1_minus_u0 = uv1.x - uv0.x;
		const float v1_minus_v0 = uv1.y - uv0.y;
		const float u2_minus_u0 = uv2.x - uv0.x;
		const float v2_minus_v0 = uv2.y - uv0.y;

		const float det = u1_minus_u0 * v2_minus_v0 - v1_minus_v0*u2_minus_u0;
		const float invDet = 1.0f / det;

		//coefficients matrix inverse's rows
		const glm::vec2 coffMatInvRow0 = glm::vec2{ v2_minus_v0, -v1_minus_v0 } *invDet;
		const glm::vec2 coffMatInvRow1 = glm::vec2{ -u2_minus_u0, u1_minus_u0 } *invDet;

		//position deltas matrix's columns
		const glm::vec2 posDeltasMatCol0 = glm::vec2{ p1.x - p0.x, p2.x - p0.x };
		const glm::vec2 posDeltasMatCol1 = glm::vec2{ p1.y - p0.y, p2.y - p0.y };
		const glm::vec2 posDeltasMatCol2 = glm::vec2{ p1.z - p0.z, p2.z - p0.z };

		const float tangentX = glm::dot(coffMatInvRow0, posDeltasMatCol0);
		const float tangentY = glm::dot(coffMatInvRow0, posDeltasMatCol1);
		const float tangentZ = glm::dot(coffMatInvRow0, posDeltasMatCol2);

		const float bitangentX = glm::dot(coffMatInvRow1, posDeltasMatCol0);
		const float bitangentY = glm::dot(coffMatInvRow1, posDeltasMatCol1);
		const float bitangentZ = glm::dot(coffMatInvRow1, posDeltasMatCol2);
				
		const glm::vec4 tangent{ tangentX, tangentY, tangentZ, 0.0f };

		verts[t0].tang += tangent;
		verts[t1].tang += tangent;
		verts[t2].tang += tangent;

		const glm::vec3 bitangent{ bitangentX, bitangentY, bitangentZ};

		perVertexBitangent[t0] += bitangent;
		perVertexBitangent[t1] += bitangent;
		perVertexBitangent[t2] += bitangent;
	}
		
	for (size_t i = 0; i < count; i++)
	{
		//orthonormalize
		const glm::vec3& normal = verts[i].norm;		
		glm::vec4& tangentWithHandedness = verts[i].tang;

		glm::vec3 tangent{ tangentWithHandedness.x, tangentWithHandedness.y, tangentWithHandedness.z };
		
		glm::vec3 tangentNormalProjection = glm::dot(tangent, normal) * normal;
		tangent -= tangentNormalProjection;
		tangent = glm::normalize(tangent);
		
		//compute handedness and store it in the tangent's w component
		glm::vec3 bitangent = glm::normalize(perVertexBitangent[i]);

		float handedNess = glm::dot(glm::cross(normal, tangent), bitangent) < 0.0f ? -1.0f : 1.0f;
				
		tangentWithHandedness = glm::vec4{ tangent.x, tangent.y, tangent.z, handedNess };
	}
}


bool CpuTexture::import(std::string filename){
	std::ifstream infile(filename,std::ios::binary);
	if (!infile.is_open()) return false;

	int depth;
	std::string token;
	infile >> token >> sizeX >> sizeY >> depth;
	// TODO: some checking please!

	infile.ignore(); //read new line character after depth

	data.reserve( sizeX*sizeY );

	for (int i=0; i<sizeX*sizeY; i++) {
		char rgb[3];
		infile.read(rgb, 3 );
		Texel t;
		t.r = rgb[0];
		t.g = rgb[1];
		t.b = rgb[2];
		data.push_back( t );
	}

	return true;
}

void CpuTexture::createRandom(int size){
	sizeX = sizeY = size;
	data.resize(sizeX * sizeY);
	for (Texel &t : data) {
		t.r = rand()%256;
		t.g = rand()%256;
		t.b = rand()%256;
		t.a = 255;
	}
}

static size_t istreamSize(std::istream& is)
{
	auto initGetPointerPosition = is.tellg();
	is.seekg(0, std::ios::end);

	size_t streamSize = static_cast<size_t>(is.tellg());
	is.seekg(initGetPointerPosition);

	return streamSize;
}

static bool loadShader(const std::string& shaderFilePath, std::string& outShaderSource)
{
	std::ifstream shaderFile(shaderFilePath, std::ios::binary);

	if (!shaderFile.is_open())
	{
		return false;
	}

	size_t shaderSize = istreamSize(shaderFile);

	char* buff = new char[shaderSize + 1];

	shaderFile.read(buff, shaderSize);

	buff[shaderSize] = '\0';

	outShaderSource = buff;

	delete[] buff;

	return true;
}

bool ShaderSource::import(const std::string& shaderFilePath)
{
	shaderSource = "";
	if (!loadShader(shaderFilePath, shaderSource))
	{
		shaderSource = "";
		return false;
	}

	return true;
}

bool CpuProgram::import(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath)
{
	vertexShaderSources.clear();
	fragmentShaderSources.clear();

	vertexShaderSources.resize(1);

	if (!loadShader(vertexShaderFilePath, vertexShaderSources[0]))
	{
		vertexShaderSources.clear();
		return false;
	}

	fragmentShaderSources.resize(1);

	if (!loadShader(fragmentShaderFilePath, fragmentShaderSources[0]))
	{
		vertexShaderSources.clear();
		fragmentShaderSources.clear();
		return false;
	}

	return true;
}

void CpuProgram::addVertexShaderInclude(const ShaderSource& shaderSource, bool append)
{
	assert(!vertexShaderSources.empty());
	if (append)
	{
		size_t mainIndex = vertexShaderSources.size() - 1;
		vertexShaderSources.insert(vertexShaderSources.begin() + mainIndex, shaderSource.shaderSource + "\n");
	}
	else
	{
		vertexShaderSources.insert(vertexShaderSources.begin(), shaderSource.shaderSource + "\n");
	}
}

void CpuProgram::addFragmentShaderInclude(const ShaderSource& shaderSource, bool append)
{
	assert(!fragmentShaderSources.empty());
	if (append)
	{
		size_t mainIndex = fragmentShaderSources.size() - 1;
		fragmentShaderSources.insert(fragmentShaderSources.begin() + mainIndex, shaderSource.shaderSource + "\n");
	}
	else
	{
		fragmentShaderSources.insert(fragmentShaderSources.begin(), shaderSource.shaderSource + "\n");
	}
}

bool CpuProgram::addVertexShaderInclude(const std::string& shaderSourceFilePath, bool append)
{
	ShaderSource shaderSource;
	if (shaderSource.import(shaderSourceFilePath))
	{
		addVertexShaderInclude(shaderSource, append);
		return true;
	}
	return false;
}

bool CpuProgram::addFragmentShaderInclude(const std::string& shaderSourceFilePath, bool append)
{
	ShaderSource shaderSource;
	if (shaderSource.import(shaderSourceFilePath))
	{
		addFragmentShaderInclude(shaderSource, append);
		return true;
	}
	return false;
}


bool CpuTextureCube::import(const std::vector<std::string>& cubeFacesFileNames)
{
	constexpr size_t cubeFacesCount = 6;
	assert(cubeFacesFileNames.size() >= cubeFacesCount);

	facesData.clear();
	facesData.resize(cubeFacesCount);

	CpuTexture positiveX;
	bool importFace = positiveX.import(cubeFacesFileNames[0]);

	if (!importFace)
	{
		facesData.clear();
		return false;
	}

	sizeX = positiveX.sizeX;
	sizeY = positiveX.sizeY;

	facesData[0] = std::move(positiveX.data);

	for (size_t i = 1; i < cubeFacesCount; ++i)
	{
		CpuTexture faceCpuTexture;
		importFace = faceCpuTexture.import(cubeFacesFileNames[i]);
		if (!importFace)
		{
			facesData.clear();
			return false;
		}
		assert(faceCpuTexture.sizeX == sizeX && faceCpuTexture.sizeY == sizeY);
		facesData[i] = std::move(faceCpuTexture.data);
	}

	return true;
}
