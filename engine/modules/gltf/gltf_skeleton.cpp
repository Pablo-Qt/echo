#include "gltf_skeleton.h"
#include "engine/core/main/Engine.h"

namespace Echo
{
	GltfSkeleton::GltfSkeleton()
		: m_animations("")
	{
	}

	GltfSkeleton::~GltfSkeleton()
	{
	}

	void GltfSkeleton::bindMethods()
	{
		CLASS_BIND_METHOD(GltfSkeleton, getGltfRes, DEF_METHOD("getGltfRes"));
		CLASS_BIND_METHOD(GltfSkeleton, setGltfRes, DEF_METHOD("setGltfRes"));
		CLASS_BIND_METHOD(GltfSkeleton, getAnim, DEF_METHOD("getAnim"));
		CLASS_BIND_METHOD(GltfSkeleton, setAnim, DEF_METHOD("setAnim"));

		CLASS_REGISTER_PROPERTY(GltfSkeleton, "Gltf", Variant::Type::ResourcePath, "getGltfRes", "setGltfRes");
		CLASS_REGISTER_PROPERTY(GltfSkeleton, "Anim", Variant::Type::StringOption, "getAnim", "setAnim");
	}

	// set gltf resource
	void GltfSkeleton::setGltfRes(const ResourcePath& path)
	{
		if (m_assetPath.setPath(path.getPath()))
		{
			m_asset = GltfRes::create(m_assetPath);	
			if (m_asset)
			{
				for (GltfAnim& gltfAnim : m_asset->m_animations)
				{
					AnimClip* clip = gltfAnim.m_clip;
					if (clip)
					{
						if (clip->m_name.empty())
							generateUniqueName(clip->m_name);

						m_clips.push_back(clip);
						m_animations.addOption(clip->m_name);
					}
				}
			}
		}
	}

	// play anim
	void GltfSkeleton::setAnim(const StringOption& animName)
	{
		m_animations.setValue(animName.getValue());
	}

	// get current anim clip
	const AnimClip* GltfSkeleton::getAnimClip()
	{
		return m_animations.isValid() ? m_clips[m_animations.getIdx()] : nullptr;
	}

	// update self
	void GltfSkeleton::update_self()
	{
		if (m_animations.isValid())
		{
			float deltaTime = Engine::instance()->getFrameTime();
			AnimClip* clip = m_clips[m_animations.getIdx()];
			if (clip)
			{
				clip->update(deltaTime);

				extractClipData(clip);
			}
		}
	}

	// is anim exist
	bool GltfSkeleton::isAnimExist(const char* animName)
	{
		for (AnimClip* clip : m_clips)
		{
			if (clip->m_name == animName)
				return true;
		}

		return false;
	}

	// generate unique name
	void GltfSkeleton::generateUniqueName(String& oName)
	{
		char name[128] = "anim_";
		for (i32 i = 0; i < 65535; i++)
		{
			itoa(i, name + 5, 10);
			if (!isAnimExist(name))
			{
				oName = name;
				break;
			}
		}
	}

	//  query clip data
	void GltfSkeleton::extractClipData(AnimClip* clip)
	{
		if (clip)
		{
			for (AnimNode* animNode : clip->m_nodes)
			{
				i32 nodeIdx = any_cast<i32>(animNode->m_userData);
				for (AnimProperty* property : animNode->m_properties)
				{
					GltfAnimChannel::Path channelPath = any_cast<GltfAnimChannel::Path>(property->m_userData);
					for (i32 i = 0; i < property->getCurverCount(); i++)
					{
					}
				}
			}
		}
	}
}