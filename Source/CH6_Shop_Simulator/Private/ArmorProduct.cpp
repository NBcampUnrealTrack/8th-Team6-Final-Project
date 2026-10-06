// Fill out your copyright notice in the Description page of Project Settings.


#include "ArmorProduct.h"

AArmorProduct::AArmorProduct()
{
	// 부모인 AProductBase가 가진 변수의 기본값을 'Armor(방어구)'으로 강제 고정합니다.
	ProductType = EProductType::Armor;
	Price = 300; // (예시) 방어구 상자 기본 가격
}
