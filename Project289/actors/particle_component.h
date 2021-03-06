#pragma once

#include <string>
#include <memory>

#include <DirectXMath.h>

#include "actor_component.h"
#include "../engine/i_engine_physics.h"
#include "../physics/particle.h"

using namespace std::literals;

class ParticleComponent : public ActorComponent {
public:
    static const std::string g_Name;
    virtual const std::string& VGetName() const override;

public:
    ParticleComponent();
    virtual ~ParticleComponent();

    virtual TiXmlElement* VGenerateXml() override;

    virtual bool VInit(TiXmlElement* pData) override;
    virtual void VPostInit() override;
    virtual void VUpdate(float deltaMs) override;

    virtual Particle& VGetParticle();
    virtual Particle* VGetParticlePtr();

protected:
    Particle m_particle;
    IEnginePhysics* m_pGamePhysics;
};