// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerHUD.h"

void UPlayerHUD::SetHp(int NewHp)
{
	FString Str = FString::Printf(TEXT("HP: %d"), NewHp);
	HPText->SetText(FText::FromString(Str));
}

void UPlayerHUD::SetDiamonds(int Amount)
{
	FString Str = FString::Printf(TEXT("Diamonds: %d"), Amount);
	DiamondsText->SetText(FText::FromString(Str));
}

void UPlayerHUD::SetLevel(int Index)
{
	FString Str = FString::Printf(TEXT("Level: %d"), Index);
	LevelText->SetText(FText::FromString(Str));
}
