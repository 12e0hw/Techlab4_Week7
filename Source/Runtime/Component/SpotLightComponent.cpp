#include "EnginePCH.h"
#include "SpotLightComponent.h"
#include "Engine/LightSceneProxy.h"
#include "Render/LineBatcher.h"

namespace
{
	// 너무 높이면 라인 배처 정점을 빨리 소모한다.
	constexpr int32 CONE_SEGMENT_COUNT = 24;

	// 감쇠는 광원으로부터의 거리 기준이라 빛이 닿는 경계는 모선 길이가 Range인 원뿔이다.
	// 그래서 밑면을 Forward * Range가 아니라 Forward * cos(θ) * Range에 둔다.
	void AddWireCone(
		FLineBatcher* LineBatcher,
		const FVector& Apex,
		const FVector& Forward,
		const FVector& Right,
		const FVector& Up,
		float HalfAngleDegrees,
		float Range,
		const FVector4& Color)
	{
		const float HalfAngle = FMath::DegreesToRadians(HalfAngleDegrees);
		const FVector BaseCenter = Apex + Forward * (std::cos(HalfAngle) * Range);
		const float BaseRadius = std::sin(HalfAngle) * Range;

		FVector PreviousPoint;
		for (int32 i = 0; i <= CONE_SEGMENT_COUNT; ++i)
		{
			const float Angle = (2.0f * PI * i) / CONE_SEGMENT_COUNT;
			const FVector Point =
				BaseCenter +
				Right * (std::cos(Angle) * BaseRadius) +
				Up * (std::sin(Angle) * BaseRadius);

			if (i > 0)
			{
				LineBatcher->AddLine(PreviousPoint, Point, Color);
			}
			if (i < CONE_SEGMENT_COUNT)
			{
				LineBatcher->AddLine(Apex, Point, Color);
			}
			PreviousPoint = Point;
		}
	}
}

void USpotLightComponent::PostEditChangeProperty(const FProperty& Property)
{
	Super::PostEditChangeProperty(Property);

	// 언리얼처럼 Outer는 1~80도로, Inner는 0~Outer로 제한한다. Inner가 Outer보다 크면 경계 계산이 뒤집힌다.
	OuterConeAngle = FMath::Clamp(OuterConeAngle, 1.0f, 80.0f);
	InnerConeAngle = FMath::Clamp(InnerConeAngle, 0.0f, OuterConeAngle);
}

FSpotLightInfo USpotLightComponent::GetSpotLightInfo() const
{
	FSpotLightInfo Info;
	Info.Color = GetShaderColorWithIntensity();
	Info.AttenuationRadius = AttenuationRadius;
	Info.Position = GetWorldLocation();
	Info.CosInnerCone = std::cos(FMath::DegreesToRadians(InnerConeAngle));
	Info.CosOuterCone = std::cos(FMath::DegreesToRadians(OuterConeAngle));
	Info.Direction = GetLightDirection();
	return Info;
}

FLightSceneProxy* USpotLightComponent::CreateSceneProxy()
{
	return new FSpotLightSceneProxy(this);
}

void USpotLightComponent::DrawDebug(FLineBatcher* LineBatcher) const
{
	if (!LineBatcher) return;

	const FVector Apex = GetWorldLocation();
	const FVector Forward = GetLightDirection();

	const FVector UpGuide = (std::abs(Forward.Z) > 0.99f) ? FVector(0.0f, 1.0f, 0.0f) : FVector(0.0f, 0.0f, 1.0f);
	const FVector Right = FVector::Cross(Forward, UpGuide).GetSafeNormal();
	const FVector Up = FVector::Cross(Right, Forward).GetSafeNormal();

	// 디버그 라인은 오버레이라 화면에 보이는 sRGB 값을 감마 변환 없이 그대로 쓴다
	const FLinearColor LineColor = LightColor.ReinterpretAsLinear();
	const FVector4 OuterColor(LineColor.R, LineColor.G, LineColor.B, 1.0f);
	const FVector4 InnerColor(LineColor.R * 0.5f, LineColor.G * 0.5f, LineColor.B * 0.5f, 1.0f);

	AddWireCone(LineBatcher, Apex, Forward, Right, Up, OuterConeAngle, AttenuationRadius, OuterColor);
	AddWireCone(LineBatcher, Apex, Forward, Right, Up, InnerConeAngle, AttenuationRadius, InnerColor);
}
