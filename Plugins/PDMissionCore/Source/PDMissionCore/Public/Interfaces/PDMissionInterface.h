/*
 * @copyright Permafrost Development (MIT license) 
 * Authors: Ario Amin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. 
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "PDMissionInterface.generated.h"

UINTERFACE(MinimalAPI)
class UPDMissionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PDMISSIONCORE_API IPDMissionInterface : public IInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void GrantMissionToActor(AActor* CallingActor, FName MissionName);
	virtual void GrantMissionToActor_Implementation(AActor* CallingActor, FName MissionName);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void RemoveMissionFromActor(AActor* CallingActor, FName MissionName);
	virtual void RemoveMissionFromActor_Implementation(AActor* CallingActor, FName MissionName);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void AddTagsToContainer(TArray<FGameplayTag>& Tags);
	virtual void AddTagsToContainer_Implementation(TArray<FGameplayTag>& Tags);	

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void RemoveTagsToContainer(TArray<FGameplayTag>& Tags);
	virtual void RemoveTagsToContainer_Implementation(TArray<FGameplayTag>& DeleteTags);	

	const TSet<FGameplayTag>& GetTagContainer() const { return TagContainer; }
protected:
	
	TSet<FGameplayTag> TagContainer;  
};
