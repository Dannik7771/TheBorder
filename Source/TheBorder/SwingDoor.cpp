// Fill out your copyright notice in the Description page of Project Settings.


#include "SwingDoor.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ASwingDoor::ASwingDoor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Open = false;
	ReadyState = true;

	DoorFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door Frame"));
	RootComponent = DoorFrame;
	Door = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));
	Door->SetupAttachment(RootComponent);

}

// Called when the game starts or when spawned
void ASwingDoor::BeginPlay()
{
	Super::BeginPlay();

	RotateValue = -1.0f;

	if (DoorCurve)
	{
		FOnTimelineFloat TimelineCallBack;
		FOnTimelineEventStatic TimelineFinishedCallBack;

		TimelineCallBack.BindUFunction(this, FName("ControlDoor"));
		TimelineFinishedCallBack.BindUFunction(this, FName("SetState"));

		MyTimeline.AddInterpFloat(DoorCurve, TimelineCallBack);
		MyTimeline.SetTimelineFinishedFunc(TimelineFinishedCallBack);
	}

}

// Called every frame
void ASwingDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	MyTimeline.TickTimeline(DeltaTime);
}

void ASwingDoor::ControlDoor()
{
	TimelineValue = MyTimeline.GetPlaybackPosition();
	CurveFloatValue = RotateValue * DoorCurve->GetFloatValue(TimelineValue);

	FQuat NewRotation = FQuat(FRotator(0.0f, CurveFloatValue, 0.0f));
	Door->SetRelativeRotation(NewRotation);
}

void ASwingDoor::SetState()
{
	ReadyState = true;
}

void ASwingDoor::ToggleDoor()
{
	if (ReadyState)
	{
		Open = !Open;

		//	APawn* OurPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		//	FVector PawnLocation = OurPawn->GetActorLocation();
		//	FVector Direction = GetActorLocation() - PawnLocation;
		//	Direction = UKismetMathLibrary::LessLess_VectorRotator(Direction, GetActorRotation());

		DoorRotation = Door->RelativeRotation;
		if (Open)
		{
			//	if (Direction.X > 0.0f)
			//	{
			//		RotateValue = -1.0f;
			//	}
			//	else
			//	{
			//		RotateValue = -1.0f;
			//	}
			ReadyState = false;
			MyTimeline.PlayFromStart();
		}
		else
		{
			ReadyState = false;
			MyTimeline.Reverse();
		}
	}

}

