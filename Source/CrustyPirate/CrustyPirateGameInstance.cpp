// Fill out your copyright notice in the Description page of Project Settings.


#include "CrustyPirateGameInstance.h"
#include "Kismet/GameplayStatics.h"


void UCrustyPirateGameInstance::SetPlayerHP(int NewHP)
{
	PlayerHP = NewHP;
}

void UCrustyPirateGameInstance::AddDiamond(int Amount)
{
	CollectedDiamondCount += Amount;
}

void UCrustyPirateGameInstance::ChangeLevel(int LevelIndex)
{
	if (LevelIndex <= 0)	return;
	CurrentLevelIndex = LevelIndex;
	FString LevelNameString = FString::Printf(TEXT("Level_%d"), LevelIndex);
	UGameplayStatics::OpenLevel(GetWorld(), FName(LevelNameString));
}

void UCrustyPirateGameInstance::RestartGame()
{
	PlayerHP = 100;
	CollectedDiamondCount = 0;
	IsDoubleJumpUnlocked = false;
	CurrentLevelIndex = 1;
	ChangeLevel(CurrentLevelIndex);
}