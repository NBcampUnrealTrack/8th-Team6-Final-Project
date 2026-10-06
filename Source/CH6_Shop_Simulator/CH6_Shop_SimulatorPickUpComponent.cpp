// Copyright Epic Games, Inc. All Rights Reserved.

#include "CH6_Shop_SimulatorPickUpComponent.h"

UCH6_Shop_SimulatorPickUpComponent::UCH6_Shop_SimulatorPickUpComponent()
{
	// Setup the Sphere Collision
	SphereRadius = 32.f;
}

void UCH6_Shop_SimulatorPickUpComponent::BeginPlay()
{
	Super::BeginPlay();

	// Register our Overlap Event
	OnComponentBeginOverlap.AddDynamic(this, &UCH6_Shop_SimulatorPickUpComponent::OnSphereBeginOverlap);
}

void UCH6_Shop_SimulatorPickUpComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Checking if it is a First Person Character overlapping
	ACH6_Shop_SimulatorCharacter* Character = Cast<ACH6_Shop_SimulatorCharacter>(OtherActor);
	if(Character != nullptr)
	{
		// Notify that the actor is being picked up
		OnPickUp.Broadcast(Character);

		// Unregister from the Overlap Event so it is no longer triggered
		OnComponentBeginOverlap.RemoveAll(this);
	}
}
