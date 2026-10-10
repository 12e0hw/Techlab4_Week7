#pragma once

inline void to_json(json& J, const FVector& V)
{
	J = json::array({ V.X, V.Y, V.Z });
}

inline void from_json(const json& J, FVector& V)
{
	for (int32 i = 0; i < 3; i++)
	{
		V[i] = J[i].get<float>();
	}
}

inline void to_json(json& J, const FRotator& V)
{
	J = json::array({ V.Pitch, V.Yaw, V.Roll });
}

inline void from_json(const json& J, FRotator& V)
{
	for (int32 i = 0; i < 3; i++)
	{
		V[i] = J[i].get<float>();
	}
}

inline void to_json(json& J, const FVector4& V)
{
	J = json::array({ V.X, V.Y, V.Z, V.W });
}

inline void from_json(const json& J, FVector4& V)
{
	for (int32 i = 0; i < 4; i++)
	{
		V[i] = J[i].get<float>();
	}
}

// 메모리 순서(BGRA)가 아니라 읽기 쉬운 RGBA 순서로 저장한다
inline void to_json(json& J, const FColor& C)
{
	J = json::array({ C.R, C.G, C.B, C.A });
}

inline void from_json(const json& J, FColor& C)
{
	C = FColor(J[0].get<uint8>(), J[1].get<uint8>(), J[2].get<uint8>(), J[3].get<uint8>());
}

inline void to_json(json& J, const FLinearColor& C)
{
	J = json::array({ C.R, C.G, C.B, C.A });
}

inline void from_json(const json& J, FLinearColor& C)
{
	C = FLinearColor(J[0].get<float>(), J[1].get<float>(), J[2].get<float>(), J[3].get<float>());
}

inline void to_json(json& J, const FTransform& T)
{
	J = json{ {"Location", T.Location}, {"Rotation", T.Rotation}, {"Scale", T.Scale} };
}

inline void from_json(const json& J, FTransform& T)
{
	J.at("Location").get_to(T.Location);
	J.at("Rotation").get_to(T.Rotation);
	J.at("Scale").get_to(T.Scale);
}