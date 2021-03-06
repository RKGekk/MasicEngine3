#include "scene_node.h"
#include "scene.h"
#include "../engine/engine.h"
#include "../actors/transform_component.h"
#include "camera_node.h"

SceneNode::SceneNode(WeakBaseRenderComponentPtr renderComponent, RenderPass renderPass, const DirectX::XMFLOAT4X4* to, const DirectX::XMFLOAT4X4* from, bool calulate_from) {
	DirectX::XMFLOAT4X4 to4x4;
	if (to == nullptr) {
		DirectX::XMStoreFloat4x4(&to4x4, DirectX::XMMatrixIdentity());
	}
	else {
		to4x4 = *to;
	}

	VSetTransform4x4(&to4x4, from);
	SetRadius(0);
	m_RenderComponent = renderComponent;

	m_pParent = nullptr;

	m_Props.m_ActorId = (renderComponent) ? renderComponent->GetOwnerId() : 0;
	m_Props.m_Name = (renderComponent) ? renderComponent->VGetName() : "SceneNode";
	m_Props.m_RenderPass = renderPass;
	m_Props.m_AlphaType = AlphaType::AlphaOpaque;
}

SceneNode::SceneNode(WeakBaseRenderComponentPtr renderComponent, RenderPass renderPass, DirectX::FXMMATRIX to, DirectX::CXMMATRIX from, bool calulate_from) {
	VSetTransform(to, from, calulate_from);
	SetRadius(0);
	m_RenderComponent = renderComponent;

	m_pParent = nullptr;

	m_Props.m_ActorId = (renderComponent) ? renderComponent->GetOwnerId() : 0;
	m_Props.m_Name = (renderComponent) ? renderComponent->VGetName() : "SceneNode";
	m_Props.m_RenderPass = renderPass;
	m_Props.m_AlphaType = AlphaType::AlphaOpaque;
}

SceneNode::~SceneNode() {}

const SceneNodeProperties& SceneNode::VGet() const {
	return m_Props;
}

void SceneNode::VSetTransform4x4(const DirectX::XMFLOAT4X4* toWorld, const DirectX::XMFLOAT4X4* fromWorld) {
	m_Props.m_ToWorld = *toWorld;
	if (!fromWorld) {
		DirectX::XMStoreFloat4x4(&m_Props.m_FromWorld, DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&m_Props.m_ToWorld)));
	}
	else {
		m_Props.m_FromWorld = *fromWorld;
	}
}

void SceneNode::VSetTransform(DirectX::FXMMATRIX toWorld, DirectX::CXMMATRIX fromWorld, bool calulate_from) {
	DirectX::XMStoreFloat4x4(&m_Props.m_ToWorld, toWorld);
	if (calulate_from) {
		DirectX::XMStoreFloat4x4(&m_Props.m_FromWorld, DirectX::XMMatrixInverse(nullptr, toWorld));
	}
	else {
		DirectX::XMStoreFloat4x4(&m_Props.m_FromWorld, fromWorld);
	}
}

HRESULT SceneNode::VOnRestore(Scene* pScene) {
	//DirectX::XMFLOAT4 color = (m_RenderComponent) ? m_RenderComponent->GetColor() : DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	//m_Props.m_Material.SetDiffuse(color);

	SceneNodeList::iterator i = m_Children.begin();
	SceneNodeList::iterator end = m_Children.end();
	while (i != end) {
		(*i)->VOnRestore(pScene);
		++i;
	}
	return S_OK;
}

HRESULT SceneNode::VOnUpdate(Scene* pScene, float elapsedSeconds) {
	auto i = m_Children.begin();
	auto end = m_Children.end();
	while (i != end) {
		(*i)->VOnUpdate(pScene, elapsedSeconds);
		++i;
	}
	return S_OK;
}

HRESULT SceneNode::VPreRender(Scene* pScene) {
	StrongActorPtr pActor = MakeStrongPtr(g_pApp->GetGameLogic()->VGetActor(m_Props.m_ActorId));
	if (pActor) {
		std::shared_ptr<TransformComponent> pTc = MakeStrongPtr(pActor->GetComponent<TransformComponent>("TransformComponent"));
		if (pTc) {
			m_Props.m_ToWorld = pTc->GetTransform4x4f();
		}
	}

	pScene->PushAndSetMatrix4x4(m_Props.m_ToWorld);
	return S_OK;
}

bool SceneNode::VIsVisible(Scene* pScene) const {
	DirectX::XMMATRIX toWorld = pScene->GetCamera()->VGet().ToWorld();
	DirectX::XMMATRIX fromWorld = pScene->GetCamera()->VGet().FromWorld();

	DirectX::XMVECTOR pos = DirectX::XMVectorSetW(GetWorldPosition(), 1.0f);
	DirectX::XMVECTOR fromWorldPos = DirectX::XMVector4Transform(pos, fromWorld);

	Frustum const& frustum = pScene->GetCamera()->GetFrustum();

	bool isVisible = frustum.Inside(fromWorldPos, VGet().Radius());
	return isVisible;
}

HRESULT SceneNode::VRender(Scene* pScene) {
	return S_OK;
}

HRESULT SceneNode::VRenderChildren(Scene* pScene) {
	SceneNodeList::iterator i = m_Children.begin();
	SceneNodeList::iterator end = m_Children.end();

	while (i != end) {
		if ((*i)->VPreRender(pScene) == S_OK) {
			if ((*i)->VIsVisible(pScene)) {
				float alpha = (*i)->VGet().m_Material.GetAlpha();
				if (alpha == 1.0f) {
					(*i)->VRender(pScene);
				}
				else if (alpha != 0.0f) {
					AlphaSceneNode* asn = new AlphaSceneNode;
					asn->m_pNode = *i;
					//asn->m_Concat = pScene->GetTopInvMatrix4x4f();
					asn->m_Concat = pScene->GetTopMatrix4x4f();
					DirectX::XMVECTOR worldPos = DirectX::XMVectorSet(asn->m_Concat._41, asn->m_Concat._42, asn->m_Concat._43, asn->m_Concat._44);
					DirectX::XMMATRIX fromWorld = pScene->GetCamera()->VGet().FromWorld();
					DirectX::XMVECTOR screenPos = DirectX::XMVector4Transform(worldPos, fromWorld);

					asn->m_ScreenZ = DirectX::XMVectorGetZ(screenPos);

					pScene->AddAlphaSceneNode(asn);
				}
				(*i)->VRenderChildren(pScene);
			}
		}
		(*i)->VPostRender(pScene);
		++i;
	}

	return S_OK;
}

HRESULT SceneNode::VPostRender(Scene* pScene) {
	pScene->PopMatrix();
	return S_OK;
}

bool SceneNode::VAddChild(std::shared_ptr<ISceneNode> ikid) {
	m_Children.push_back(ikid);

	std::shared_ptr<SceneNode> kid = std::static_pointer_cast<SceneNode>(ikid);
	kid->m_pParent = this;
	//DirectX::XMVECTOR kidPos = kid->VGet().ToWorld().r[3];
	//float newRadius = DirectX::XMVectorGetX(DirectX::XMVector3Length(kidPos)) + kid->VGet().Radius();
	//if (newRadius > m_Props.m_Radius) { m_Props.m_Radius = newRadius; }
	if (kid->VGet().Radius() > m_Props.m_Radius) {
		m_Props.m_Radius = kid->VGet().Radius();
	}

	return true;
}

bool SceneNode::VRemoveChild(ActorId aid, ComponentId cid) {
	for (SceneNodeList::iterator i = m_Children.begin(); i != m_Children.end(); ++i) {
		const SceneNodeProperties& pProps = (*i)->VGet();
		if (pProps.ActorId() != INVALID_ACTOR_ID && aid == pProps.ActorId() && cid == pProps.ComponentId()) {
			i = m_Children.erase(i);
			return true;
		}
	}
	return false;
}

HRESULT SceneNode::VOnLostDevice(Scene* pScene) {
	SceneNodeList::iterator i = m_Children.begin();
	SceneNodeList::iterator end = m_Children.end();
	while (i != end) {
		(*i)->VOnLostDevice(pScene);
		++i;
	}
	return S_OK;
}

HRESULT SceneNode::VPick(Scene* pScene, RayCast* pRayCast) {
	for (SceneNodeList::const_iterator i = m_Children.begin(); i != m_Children.end(); ++i) 	{
		HRESULT hr = (*i)->VPick(pScene, pRayCast);
		if (hr == E_FAIL) { return E_FAIL; }
	}

	return S_OK;
}

ISceneNode* SceneNode::VGetParent() {
	return m_pParent;
}

void SceneNode::SetAlpha(float alpha) {
	m_Props.SetAlpha(alpha);
	for (SceneNodeList::const_iterator i = m_Children.begin(); i != m_Children.end(); ++i) 	{
		std::shared_ptr<SceneNode> sceneNode = std::static_pointer_cast<SceneNode>(*i);
		sceneNode->SetAlpha(alpha);
	}
}

float SceneNode::GetAlpha() const {
	return m_Props.Alpha();
}

void SceneNode::SetName(std::string name) {
	m_Props.m_Name = name;
}

const std::string& SceneNode::GetName() const {
	return m_Props.m_Name;
}

DirectX::XMFLOAT3 SceneNode::GetPosition3() const {
	return DirectX::XMFLOAT3(m_Props.m_ToWorld.m[3][0], m_Props.m_ToWorld.m[3][1], m_Props.m_ToWorld.m[3][2]);
}

DirectX::XMFLOAT4 SceneNode::GetPosition4() const {
	return DirectX::XMFLOAT4(m_Props.m_ToWorld.m[3][0], m_Props.m_ToWorld.m[3][1], m_Props.m_ToWorld.m[3][2], 1.0f);
}

void SceneNode::SetPosition3(const DirectX::XMFLOAT3& pos) {
	m_Props.m_ToWorld.m[3][0] = pos.x;
	m_Props.m_ToWorld.m[3][1] = pos.y;
	m_Props.m_ToWorld.m[3][2] = pos.z;
	m_Props.m_ToWorld.m[3][3] = 1.0f;
}

DirectX::XMFLOAT3 SceneNode::GetWorldPosition3() const {
	DirectX::XMFLOAT3 pos = GetPosition3();
	if (m_pParent) {
		DirectX::XMFLOAT3 wp1 = m_pParent->GetWorldPosition3();
		pos.x += wp1.x;
		pos.y += wp1.y;
		pos.z += wp1.z;
	}
	return pos;
}

DirectX::XMVECTOR SceneNode::GetWorldPosition() const {
	DirectX::XMFLOAT3 res = GetWorldPosition3();
	return DirectX::XMLoadFloat3(&res);
}

DirectX::XMFLOAT3 SceneNode::GetDirection() const {
	DirectX::XMFLOAT4X4 justRot4x4 = m_Props.m_ToWorld;
	justRot4x4.m[3][0] = 0.0f;
	justRot4x4.m[3][1] = 0.0f;
	justRot4x4.m[3][2] = 0.0f;
	justRot4x4.m[3][3] = 1.0f;
	DirectX::XMMATRIX justRot = DirectX::XMLoadFloat4x4(&justRot4x4);

	DirectX::XMVECTOR forward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	DirectX::XMVECTOR out = DirectX::XMVector4Transform(forward, justRot);
	DirectX::XMFLOAT3 result;
	DirectX::XMStoreFloat3(&result, out);

	return result;
}

void SceneNode::SetRadius(const float radius) {
	m_Props.m_Radius = radius;
}

void SceneNode::SetMaterial(const Material& mat) {
	m_Props.m_Material = mat;
}

ActorId SceneNode::VFindMyActor() {
	ActorId act = VGet().ActorId();
	if (act != 0) { return act; }

	ISceneNode* parent = m_pParent;
	while (parent) {
		act = parent->VGet().ActorId();
		if (act != 0) { return act; }
		parent = parent->VGetParent();
	}
	return 0;
}
