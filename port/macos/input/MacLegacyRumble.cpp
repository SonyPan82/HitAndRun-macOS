#include <input/forceeffect.h>
#include <input/basedamper.h>
#include <input/steeringspring.h>
#include <input/wheelrumble.h>
#include <input/rumbleeffect.h>
#include <input/constanteffect.h>

// The PC wheel implementation talks directly to DirectInput force-effect
// objects. GameController devices expose optional haptics through a different
// API, so preserve the gameplay-facing state here without issuing unsupported
// DirectInput calls. Normal controller input remains fully native.
ForceEffect::ForceEffect() : mOutputPoint(nullptr), mEffectDirty(false) {}
ForceEffect::~ForceEffect() = default;
void ForceEffect::Init(IRadControllerOutputPoint* outputPoint) { mOutputPoint = outputPoint; OnInit(); }
void ForceEffect::Start() {}
void ForceEffect::Stop() {}

BaseDamper::BaseDamper() = default;
BaseDamper::~BaseDamper() = default;
void BaseDamper::OnInit() {}
void BaseDamper::SetCenterPoint(s8, u8) {}
void BaseDamper::SetDamperStrength(u8) {}
void BaseDamper::SetDamperCoefficient(s16) {}

SteeringSpring::SteeringSpring() = default;
SteeringSpring::~SteeringSpring() = default;
void SteeringSpring::OnInit() {}
void SteeringSpring::SetCenterPoint(s8, u8) {}
void SteeringSpring::SetSpringStrength(u8) {}
void SteeringSpring::SetSpringCoefficient(s16) {}

WheelRumble::WheelRumble() = default;
WheelRumble::~WheelRumble() = default;
void WheelRumble::OnInit() {}
void WheelRumble::SetMagDir(u8, u16) {}
void WheelRumble::SetPPO(u16, u16, s16) {}
void WheelRumble::SetRumbleType(u8) {}

ConstantEffect::ConstantEffect() = default;
ConstantEffect::~ConstantEffect() = default;
void ConstantEffect::OnInit() {}
void ConstantEffect::SetMagnitude(s16) {}
void ConstantEffect::SetDirection(u16) {}

RumbleEffect::RumbleEffect() : mWheelEffect(nullptr)
{
    for (unsigned i = 0; i < Input::MaxOutputMotor; ++i)
    {
        mMotors[i] = nullptr;
        mMotorUpdated[i] = false;
    }
    InitEffects();
}
RumbleEffect::~RumbleEffect() = default;
void RumbleEffect::SetWheelEffect(IRadControllerOutputPoint* wheelEffect) { mWheelEffect = wheelEffect; }
void RumbleEffect::SetMotor(unsigned whichMotor, IRadControllerOutputPoint* motor)
{
    if (whichMotor < Input::MaxOutputMotor) mMotors[whichMotor] = motor;
}
void RumbleEffect::SetEffect(Effect effect, unsigned milliseconds)
{
    if (effect < NUM_EFFECTS) mCurrentEffects[effect].mRumbleTimeLeft = milliseconds;
}
void RumbleEffect::SetDynaEffect(DynaEffect effect, unsigned milliseconds, float gain)
{
    if (effect < NUM_DYNA_EFFECTS) { mCurrentDynaEffects[effect].mRumbleTimeLeft = milliseconds; mCurrentDynaEffects[effect].mMaxGain = gain; }
}
void RumbleEffect::Update(unsigned milliseconds)
{
    for (unsigned i = 0; i < NUM_EFFECTS; ++i) UpdateEffect(static_cast<Effect>(i), milliseconds);
    for (unsigned i = 0; i < NUM_DYNA_EFFECTS; ++i) UpdateDynaEffect(static_cast<DynaEffect>(i), milliseconds, mCurrentDynaEffects[i].mMaxGain);
}
void RumbleEffect::ShutDownEffects() { OnShutDownEffects(); }
void RumbleEffect::InitEffects() { for (unsigned i = 0; i < NUM_EFFECTS; ++i) mCurrentEffects[i].mRumbleTimeLeft = 0; }
void RumbleEffect::UpdateEffect(Effect effect, unsigned milliseconds)
{
    unsigned& remaining = mCurrentEffects[effect].mRumbleTimeLeft;
    remaining = remaining > milliseconds ? remaining - milliseconds : 0;
}
void RumbleEffect::UpdateDynaEffect(DynaEffect effect, unsigned milliseconds, float)
{
    unsigned& remaining = mCurrentDynaEffects[effect].mRumbleTimeLeft;
    remaining = remaining > milliseconds ? remaining - milliseconds : 0;
}
void RumbleEffect::OnShutDownEffects() { InitEffects(); }
