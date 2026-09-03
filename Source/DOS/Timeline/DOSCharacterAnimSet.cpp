#include "DOSCharacterAnimSet.h"

#include "Animation/AnimMontage.h"

UAnimMontage* UDOSCharacterAnimSet::Find(FGameplayTag ActionTag) const
{
	if (!ActionTag.IsValid())
	{
		return nullptr;
	}

	const TSoftObjectPtr<UAnimMontage>* Found = ByActionTag.Find(ActionTag);
	if (!Found || Found->IsNull())
	{
		return nullptr;
	}

	return Found->LoadSynchronous();
}
