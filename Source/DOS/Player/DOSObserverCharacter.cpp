#include "DOSObserverCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ADOSObserverCharacter::ADOSObserverCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	// **플레이어는 유령이므로 인물과 서로 통과한다.** 막으면 밀착 관찰이 불가능하고,
	// 그보다 더 나쁘게 - 복도나 계단에서 플레이어가 인물의 길을 막아 경로 추종이
	// Blocked로 끝나고 그 인물은 그 자리에 멈춘 채 밤을 보낸다.
	// 충돌은 양쪽이 모두 Block일 때만 성립하므로 여기 한 줄로 양방향이 풀린다.
	// 바닥/벽(WorldStatic, WorldDynamic)은 그대로 막으므로 걷기는 정상이다.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	// 마우스로 좌우를 돌리면 폰 자체가 돈다 - 컨트롤러 회전을 그대로 따른다.
	// Pitch는 폰이 아니라 카메라만 따라간다(아래 bUsePawnControlRotation).
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		// 걷는 것만 한다. 뛰지도 점프하지도 않는다.
		Move->MaxWalkSpeed = 220.f;
		Move->BrakingDecelerationWalking = 2048.f;
		Move->JumpZVelocity = 0.f;
		Move->AirControl = 0.f;
		Move->bOrientRotationToMovement = false;
		// 저택은 계단으로 층을 오간다 - 계단참을 타고 넘을 만큼은 올라갈 수 있어야 한다.
		Move->MaxStepHeight = 45.f;
		// 계단(SM_StairsCarpet)의 콜리전이 **52° 경사면**이라 50°로는 걸을 수 없는 면이 된다.
		// 인물 쪽과 같은 값을 쓴다 - 플레이어가 못 오르는 곳에 인물만 올라가면 관찰이 끊긴다.
		Move->SetWalkableFloorAngle(55.f);
	}

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f)); // 눈높이
	Camera->bUsePawnControlRotation = true;
}
