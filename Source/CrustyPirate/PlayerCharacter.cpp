// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "Enemy.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	AttackCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("AttackCollisionBox"));
	AttackCollisionBox->SetupAttachment(RootComponent);
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	if(APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (Subsystem)
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}
	}
	OnAttackOverrideEndDelegate.BindUObject(this, &APlayerCharacter::OnAttackOverrideAnimEnd);
	AttackCollisionBox->OnComponentBeginOverlap.AddDynamic(this, &APlayerCharacter::AttackBoxOverlapBegin);
	EnableAttackCollisionBox(false);
	
	MyGameInstance = Cast<UCrustyPirateGameInstance>(GetGameInstance());
	if (MyGameInstance)
	{
		HitPoints = MyGameInstance->PlayerHP;
		if (MyGameInstance->IsDoubleJumpUnlocked)
		{
			UnlockDoubleJump();
		}
	}
	
	if (PlayerHUDClass)
	{
		PlayerHUDWidget = CreateWidget<UPlayerHUD>(UGameplayStatics::GetPlayerController(GetWorld(), 0), PlayerHUDClass);
		if (PlayerHUDWidget)
		{
			PlayerHUDWidget->AddToPlayerScreen();
			PlayerHUDWidget->SetHp(HitPoints);
			PlayerHUDWidget->SetDiamonds(MyGameInstance->CollectedDiamondCount);
			PlayerHUDWidget->SetLevel(MyGameInstance->CurrentLevelIndex);
		}
	}
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::JumpStarted);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlayerCharacter::JumpEnded);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Canceled, this, &APlayerCharacter::JumpEnded);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &APlayerCharacter::Attack);
		EnhancedInputComponent->BindAction(QuitAction, ETriggerEvent::Started, this, &APlayerCharacter::QuitGame);
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	float MoveActionValue = Value.Get<float>();
	if (IsAlive && CanMove && !IsStunned)
	{
		FVector Direction = FVector(1.0f, 0.0f, 0.0f);
		AddMovementInput(Direction, MoveActionValue);
		UpdateDirection(MoveActionValue);
	}
}

void APlayerCharacter::JumpStarted(const FInputActionValue& Value)
{
	if (IsAlive && CanMove && !IsStunned)
	{
		Jump();
	}
}

void APlayerCharacter::JumpEnded(const FInputActionValue& Value)
{
	StopJumping();
}

void APlayerCharacter::Attack(const FInputActionValue& Value)
{
	if (IsAlive && CanMove && !IsStunned)
	{
		CanMove = false;
		CanAttack = false;
		// EnableAttackCollisionBox(true);
		GetAnimInstance()->PlayAnimationOverride(AttackAnimSequence, FName("DefaultSlot"), 1.0f, 0.0f, OnAttackOverrideEndDelegate);
	}
}

void APlayerCharacter::UpdateDirection(float MoveDirection)
{
	FRotator CurrenRotation = Controller->GetControlRotation();
	if (MoveDirection < 0.0f)
	{
		if (CurrenRotation.Yaw != 180.0f)
		{
			Controller->SetControlRotation(FRotator(CurrenRotation.Pitch, 180.0f, CurrenRotation.Roll));
		}
	}
	else if (MoveDirection > 0.0f)
	{
		if (CurrenRotation.Yaw != 0.0f)
		{
			Controller->SetControlRotation(FRotator(CurrenRotation.Pitch, 0.0f, CurrenRotation.Roll));
		}
	}
}

void APlayerCharacter::OnAttackOverrideAnimEnd(bool Completed)
{
	if (IsAlive && IsActive)
	{
		CanAttack = true;
		CanMove = true;
	}

}

void APlayerCharacter::AttackBoxOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AEnemy* Enemy = Cast<AEnemy>(OtherActor);
	if (Enemy)
	{
		Enemy->TakeDamage(AttackDamage, AttackStunDuration);
	}
}

void APlayerCharacter::EnableAttackCollisionBox(bool Enabled)
{
	if (Enabled)
	{
		AttackCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AttackCollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	}
	else
	{
		AttackCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AttackCollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	}
}

void APlayerCharacter::UpdateHP(int NewHP)
{
	HitPoints = NewHP;
	PlayerHUDWidget->SetHp(HitPoints);
	MyGameInstance->SetPlayerHP(HitPoints);
}

void APlayerCharacter::TakeDamage(int DamageAmount, float StunDuration)
{
	if (!IsAlive)	return;
	if (!IsActive)	return;
	UpdateHP(HitPoints - DamageAmount);
	Stun(StunDuration);
	if (HitPoints <= 0)
	{
		UpdateHP(0);
		IsAlive = false;
		CanMove = false;
		CanAttack = false;
		GetAnimInstance()->JumpToNode(FName("JumpDie"), FName("CaptainStateMachine"));
		EnableAttackCollisionBox(false);
		float RestartDelay = 3.0f;
		GetWorldTimerManager().SetTimer(RestartGameTimer, this, &APlayerCharacter::OnRestartGameTimerTimeout, 1.0f, false, RestartDelay);
	}
	else{
		GetAnimInstance()->JumpToNode(FName("JumpHit"), FName("CaptainStateMachine"));
	}
}

void APlayerCharacter::Stun(float DurationInSeconds)
{
	IsStunned = true;
	bool IsTimerAlreadyActive = GetWorldTimerManager().IsTimerActive(StunTimer);
	if (IsTimerAlreadyActive)
	{
		GetWorldTimerManager().ClearTimer(StunTimer);
	}
	GetWorldTimerManager().SetTimer(StunTimer, this, &APlayerCharacter::OnStunTimerTimeout, 1.0f, false, DurationInSeconds);
	GetAnimInstance()->StopAllAnimationOverrides();
	EnableAttackCollisionBox(false);
}

void APlayerCharacter::OnStunTimerTimeout()
{
	IsStunned = false;
}

void APlayerCharacter::CollectItem(CollectableType ItemType)
{
	UGameplayStatics::PlaySound2D(GetWorld(), ItemPickupSound);

	switch (ItemType)
	{
		case CollectableType::HealthPotion:
		{
			int HealAmount = 25;
			UpdateHP(HitPoints + HealAmount);
		}break;
		case CollectableType::Diamond:
		{
			MyGameInstance->AddDiamond(1);
			PlayerHUDWidget->SetDiamonds(MyGameInstance->CollectedDiamondCount);
		}break;
		case CollectableType::DoubleJumpUpgrade:
		{
			if (!MyGameInstance->IsDoubleJumpUnlocked)
			{
				MyGameInstance->IsDoubleJumpUnlocked = true;
				UnlockDoubleJump();
			}
		}break;
		default:
		{
		}break;
	}
}

void APlayerCharacter::UnlockDoubleJump()
{
	JumpMaxCount = 2;
}

void APlayerCharacter::OnRestartGameTimerTimeout()
{
	MyGameInstance->RestartGame();
}

void APlayerCharacter::Deactivate()
{
	if (IsActive)
	{
		IsActive = false;
		CanMove = false;
		CanAttack = false;
		GetCharacterMovement()->StopMovementImmediately();
	}
}

void APlayerCharacter::QuitGame()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), UGameplayStatics::GetPlayerController(GetWorld(), 0), EQuitPreference::Quit, false);
}
