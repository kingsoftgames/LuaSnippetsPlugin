// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UObject.h"
#include "GenerateSnippetsHelper.generated.h"

struct FReflectData;
struct FSnippetElement;

UCLASS()
class UGenerateSnippetsHelper : public UObject
{
	GENERATED_BODY()
public:

    UFUNCTION(BlueprintCallable, Category = GenerateSnippetsHelper)
    static void GenerateSnippets();
    
private:
    
    static void GenerateText(FString& SnippetText, const FReflectData& ReflectData);
    static void Save(const FString& SnippetText);
    static void SaveIfPluginDirExist(const FString& PluginDir, const FString& SnippetText);
    
    static void FillLoadedReflectData(FReflectData& ReflectData);
    static void FillUnloadedReflectData(FReflectData& ReflectData);
    
    static void AppendEnum(FString& SnippetText, const UEnum* Enum);
    static void AppendStruct(FString& SnippetText, const UStruct* Struct);
    static void AppendClass(FString& SnippetText, const UClass* Class);
    static void AppendElement(FString& SnippetText, bool IsOriginOwner, FSnippetElement& Ele);
    
    static bool CanExportEnum(const UEnum* Enum);
    static bool CanExportStruct(const UScriptStruct* Struct);
    static bool CanExportClass(const UClass* Class);
    static bool CanExportFunction(const UFunction* Function);
    static bool CanExportProperty(const UProperty* Property);
};