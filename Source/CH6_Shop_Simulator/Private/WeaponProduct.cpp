// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponProduct.h"

AWeaponProduct::AWeaponProduct()
{
	// 부모인 AProductBase가 가진 변수의 기본값을 'Weapon(무기)'으로 강제 고정합니다.
	ProductType = EProductType::Weapon;
	Price = 500; // (예시) 무기 상자 기본 가격
}
