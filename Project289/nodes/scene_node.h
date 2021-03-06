#pragma once

#include <memory>

#include <DirectXMath.h>

#include "../actors/actor.h"
#include "i_scene_node.h"
#include "../actors/base_render_component.h"
#include "ray_cast.h"
#include "../tools/memory_utility.h"

using SceneNodeList = std::vector<std::shared_ptr<ISceneNode>>;

class SceneNode : public ISceneNode {
	friend class Scene;

protected:
	SceneNodeList m_Children;
	SceneNode* m_pParent;
	SceneNodeProperties m_Props;
	WeakBaseRenderComponentPtr m_RenderComponent;

public:
	SceneNode(WeakBaseRenderComponentPtr renderComponent, RenderPass renderPass, const DirectX::XMFLOAT4X4* to, const DirectX::XMFLOAT4X4* from = nullptr, bool calulate_from = true);
	SceneNode(WeakBaseRenderComponentPtr renderComponent, RenderPass renderPass, DirectX::FXMMATRIX to, DirectX::CXMMATRIX from, bool calulate_from = true);

	virtual ~SceneNode();

	virtual const SceneNodeProperties& VGet() const override;

	virtual void VSetTransform4x4(const DirectX::XMFLOAT4X4* toWorld, const DirectX::XMFLOAT4X4* fromWorld) override;
	virtual void VSetTransform(DirectX::FXMMATRIX toWorld, DirectX::CXMMATRIX fromWorld, bool calulate_from) override;

	virtual HRESULT VOnRestore(Scene* pScene) override;
	virtual HRESULT VOnUpdate(Scene* pScene, float elapsedSeconds) override;

	virtual HRESULT VPreRender(Scene* pScene) override;
	virtual bool VIsVisible(Scene* pScene) const override;
	virtual HRESULT VRender(Scene* pScene) override;
	virtual HRESULT VRenderChildren(Scene* pScene) override;
	virtual HRESULT VPostRender(Scene* pScene) override;

	virtual bool VAddChild(std::shared_ptr<ISceneNode> kid) override;
	virtual bool VRemoveChild(ActorId aid, ComponentId cid) override;
	virtual HRESULT VOnLostDevice(Scene* pScene) override;
	virtual HRESULT VPick(Scene* pScene, RayCast* pRayCast) override;

	virtual ISceneNode* VGetParent() override;

	void SetAlpha(float alpha);
	float GetAlpha() const;

	void SetName(std::string name);
	const std::string& GetName() const;

	DirectX::XMFLOAT3 GetPosition3() const;
	DirectX::XMFLOAT4 GetPosition4() const;
	void SetPosition3(const DirectX::XMFLOAT3& pos);

	DirectX::XMFLOAT3 GetWorldPosition3() const;
	DirectX::XMVECTOR GetWorldPosition() const;

	DirectX::XMFLOAT3 GetDirection() const;

	void SetRadius(const float radius);
	void SetMaterial(const Material& mat);

	virtual ActorId VFindMyActor();
};