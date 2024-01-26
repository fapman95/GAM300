#pragma once
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "Tools/CompilerSettings.h"
#include "TDSMath.h"
#include "GraphicsResource/GeomStruct.h"
#include "GraphicsResource/MaterialInfo.h"
#include "../GAM300Engine/Include/GraphicsResource/GraphicsResourceDefines.h"
#include "pch.h"


namespace TDS
{
	struct GeomDescriptor;
	struct RawMeshData;
	struct MaterialDataLoaded;




	aiVector3D ExtractEulerAngles(const aiMatrix4x4& mat);

	void ExtractTranslation(const aiMatrix4x4& mat, aiVector3D& translation);

	void ExtractScale(const aiMatrix4x4& mat, aiVector3D& scale);


	class MeshLoader
	{
		public:
			static constexpr int						MAX_MESH_LOADER = 4;

			inline static std::shared_ptr<MeshLoader>	m_Instance = nullptr;

			inline static MeshLoader& GetInstance()
			{
				if (m_Instance == nullptr)
					m_Instance = std::make_shared<MeshLoader>();

				return *m_Instance;
			}


			struct AnimationMesh
			{
				struct Skeleton
				{
					struct Bone
					{
						std::uint32_t		m_ID;
						std::uint32_t		m_ParentID;
						std::string			m_Name;
						std::string			m_ParentName;

						aiMatrix4x4			m_BoneTransform;;

					};

					std::vector<Bone>		m_BoneList;
				};

				struct AnimationFrameData
				{
					aiVector3D				m_Scale;
					aiVector3D				m_Translate;
					aiQuaternion			m_Rotate;
				};

				struct AnimationChannel
				{
					std::string						m_Name;
					int								m_FPS;
					float							m_FrameTime;
					std::vector<AnimationFrameData>	m_FrameData;
				};

				Skeleton							m_SkeletonData;
				std::vector<AnimationChannel>		m_Animations;

			};
			struct GeomData
			{
				struct lod
				{
					std::vector<std::uint32_t> m_Indices;
				};
				struct SubMesh
				{
					std::vector<TDSModel::Vertex>	m_Vertex;
					std::vector<std::uint32_t>		m_Indices;
					std::vector<lod>				m_Lods;
					std::string						m_SubMeshName;
					Vec3							m_ScenePos;
					Vec3							m_SceneRotate;
					Vec3							m_SceneScale;
					std::int32_t					m_iMaterial;
	
				};

				struct Mesh
				{
					std::string				m_Name = "";
					std::string				m_NodeName;
					std::string				m_ParentNode;
					std::vector<SubMesh>	m_Submeshes;
				};

				std::vector<Mesh>			m_Meshes;

				void ConvertToTDSModel(TDSModel& model);
			};

			struct Request
			{
				std::string			m_FileName;
				std::string			m_OutFile;
				GeomDescriptor		currSetting;
				TDSModel			m_Output;
				MaterialList		m_MaterialList;
				AnimationMesh		m_AnimationMesh;
			};

			struct AssimpSceneInfo
			{
				std::atomic_bool							m_NotAvail = false;
				const aiScene*								m_Scene;
				aiMatrix4x4									m_AppliedTransformation;

				struct Ref
				{
					std::vector<const aiNode*> m_Nodes;

				};
				std::vector<Ref>	m_MeshRef;

				
				bool MeshValidityCheck(Request& request);

			};
				

			MeshLoader();

			~MeshLoader();

			void RunCompiler(Request& request);

		private:
			
		
			std::array<AssimpSceneInfo, MAX_MESH_LOADER>	m_AssimpLoaders;
			std::deque<Request>								m_RequestLoad;



			void	AddRequests(const std::string& fileName, const std::string& outFile, GeomDescriptor& desc);

			bool	AssimpLoad(Request& request, AssimpSceneInfo& assimp);


			//Mesh Importing
			void	ImportMeshData(Request& request, AssimpSceneInfo& assimp, std::vector<RawMeshData>& assimpData);

			void	ProcessScene(std::vector<RawMeshData>& assimpData, const aiNode& Node, const aiScene& Scene, aiMatrix4x4& ParentTransform, std::string_view ParentName, Request& request);

			void	MergeMesh(Request& request, std::vector<RawMeshData>& assimpData);

			void	OptimizeMesh(std::vector<RawMeshData>& assimpData);

			void	CreateLODs(Request& request, std::vector<RawMeshData>& InputNodes);
	
			void	CreateFinalGeom(const std::vector<RawMeshData>& rawMesh, GeomData& geom);
			
			//Load Animation Data
			void	LoadAnimationData(Request& request, AssimpSceneInfo& assimp);

			void	LoadAnimations(Request& request, AssimpSceneInfo& assimp, int SamplingFPS = 60);


	};



}