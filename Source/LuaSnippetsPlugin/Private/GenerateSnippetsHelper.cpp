// Fill out your copyright notice in the Description page of Project Settings.

#include "LuaSnippetsPluginPrivatePCH.h"
#include "GenerateSnippetsHelper.h"
#include "AssetRegistryModule.h"

#define ContinueIfTrue(p)   \
if (p)                      \
{                           \
    continue;               \
}

#define ContinueIfFalse(p)  \
if (!(p))                   \
{                           \
    continue;               \
}

static const int32 PARAM_INDEX_START    = 1;

static const TCHAR* CONTENT_BEGIN       = TEXT("{\n\t\".source.lua\": {");
static const TCHAR* CONTENT_END         = TEXT("\n\t}\n}");
static const TCHAR* CONTENT_LINE        = TEXT("_");
static const TCHAR* CONTENT_DOT         = TEXT(".");
static const TCHAR* CONTENT_COLON       = TEXT(":");
static const TCHAR* CONTENT_EMPTY       = TEXT("");

static const TCHAR* FORMAT_ELEMENT      = TEXT("\n\t\t\"%s%s%s\": {\n\t\t\t\"prefix\": \"%s%s%s\",\n\t\t\t\"body\": \"%s$0\",\n\t\t\t\"description\": \"%s %s%s%s\"\n\t\t},");
static const TCHAR* FORMAT_FUNC_PARAMS  = TEXT("${%d:%s %s}, ");
static const TCHAR* FORMAT_DESC_PARAMS  = TEXT("%s %s, ");
static const TCHAR* FORMAT_FUNC_WRAP    = TEXT("%s(%s)");

static const FString FUNC_RETURN_VOID   = "void";
static const FString ENUM_DESCRIPTION   = "Enum";

static const FString PLUGIN_DIR_VSCODE  = ".vscode/extensions/";
static const FString PLUGIN_DIR_ATOM    = ".atom/packages/";

static const FString PATH_PACKAGE       = "ue4-snippets-lua/package.json";
static const FString PATH_SNIPPET       = "ue4-snippets-lua/snippets/snippets.json";

static const FName BLUEPRINT_TYPE       = "BlueprintType";
static const FName CLASS_FLAGS          = "ClassFlags";

static const FString CONETNT_PACKAGE = "{                                   \n\
	\"name\": \"ue4-snippets-lua\",                                         \n\
	\"displayName\": \"UE4 Lua Snippets\",                                  \n\
	\"publisher\": \"AlanWalk\",                                            \n\
	\"description\": \"UE4 Code Snippets for Lua.\",                        \n\
	\"version\": \"1.0.0\",                                                 \n\
	\"engines\": {                                                          \n\
		\"vscode\": \"^0.10.1\",                                            \n\
		\"atom\": \"^0.10.1\"                                               \n\
	},                                                                      \n\
    \"repository\": {                                                       \n\
        \"type\": \"git\",                                                  \n\
        \"url\": \"git+https://github.com/AlanWalk/LuaSnippetsPlugin.git\"  \n\
    },                                                                      \n\
	\"contributes\": {                                                      \n\
		\"snippets\": [                                                     \n\
			{                                                               \n\
				\"language\": \"lua\",                                      \n\
				\"path\": \"./snippets/snippets.json\"                      \n\
			}                                                               \n\
		]                                                                   \n\
	}                                                                       \n\
}";

struct FReflectData
{
    TArray<UEnum*> EnumArray;
    TArray<UScriptStruct*> StructArray;
    TArray<UClass*> ClassArray;
};

struct FSnippetElement
{
    FString OwnerName;
    FString ElementName;
    FString Body;
    FString DescriptionType;
    FString DescriptionDetail;

    FSnippetElement()
        : OwnerName         ("")
        , ElementName       ("")
        , Body              ("")
        , DescriptionType   ("")
        , DescriptionDetail ("")
    {

    }
};

void UGenerateSnippetsHelper::GenerateSnippets()
{
    FReflectData ReflectData;
    FillLoadedReflectData(ReflectData);
    FillUnloadedReflectData(ReflectData);
    
    FString SnippetText;
    GenerateText(SnippetText, ReflectData);
    Save(SnippetText);
}

void UGenerateSnippetsHelper::GenerateText(FString& SnippetText, const FReflectData& ReflectData)
{
    SnippetText.Append(CONTENT_BEGIN);

    // Enum
    for (UEnum* Enum : ReflectData.EnumArray)
    {
        AppendEnum(SnippetText, Enum);
    }

    // Struct
    for (UScriptStruct* Struct : ReflectData.StructArray)
    {
        AppendStruct(SnippetText, Struct);
    }

    // Class
    for (UClass* Class : ReflectData.ClassArray)
    {
        AppendClass(SnippetText, Class);
    }

    SnippetText.RemoveFromEnd(TEXT(","));
    SnippetText.Append(CONTENT_END);
}

void UGenerateSnippetsHelper::Save(const FString& SnippetText)
{
    auto IsSlashOrBackslash = [](TCHAR C) { return C == TEXT('/') || C == TEXT('\\'); };

    FString DirUser = FPlatformProcess::UserDir();
    FString DirHome = DirUser.Left(DirUser.FindLastCharByPredicate(IsSlashOrBackslash, DirUser.Len() - 1) + 1);

    FString DirVScode = DirHome + PLUGIN_DIR_VSCODE;
    FString DirATom = DirHome + PLUGIN_DIR_ATOM;

    SaveIfPluginDirExist(DirVScode, SnippetText);
    SaveIfPluginDirExist(DirATom, SnippetText);
}

void UGenerateSnippetsHelper::SaveIfPluginDirExist(const FString& PluginDir, const FString& SnippetText)
{
    if (!FPaths::DirectoryExists(PluginDir))
    {
        return;
    }
    FString PackageName = PluginDir + PATH_PACKAGE;
    FString SnippetName = PluginDir + PATH_SNIPPET;

    FFileHelper::SaveStringToFile(CONETNT_PACKAGE, *PackageName);
    FFileHelper::SaveStringToFile(SnippetText, *SnippetName);
}

void UGenerateSnippetsHelper::AppendEnum(FString& SnippetText, const UEnum* Enum)
{
    for (uint8 i = 0; i < Enum->GetMaxEnumValue(); ++i)
    {
        FSnippetElement Ele;
        Ele.OwnerName = Enum->GetName();
        Ele.ElementName = Enum->GetNameByIndex(i).ToString();
        Ele.ElementName.RemoveFromStart(Ele.OwnerName + "::", ESearchCase::CaseSensitive);
        Ele.DescriptionType = ENUM_DESCRIPTION;

        AppendElement(SnippetText, true, Ele);
    }
}

void UGenerateSnippetsHelper::AppendStruct(FString& SnippetText, const UStruct* Struct)
{
    for (TFieldIterator<UProperty> PropIt(Struct); PropIt; ++PropIt)
    {
        UProperty* Property = *PropIt;
        ContinueIfFalse(CanExportProperty(Property));

        FSnippetElement Ele;
        Ele.OwnerName = Struct->GetName();
        Ele.ElementName = Property->GetName();
        Ele.DescriptionType = Property->GetCPPType();

        AppendElement(SnippetText, Property->GetOwnerStruct() == Struct, Ele);
    }
}

void UGenerateSnippetsHelper::AppendClass(FString& SnippetText, const UClass* Class)
{    
    // AppendProperty
    AppendStruct(SnippetText, Class);

    for (TFieldIterator<UFunction> FuncIt(Class); FuncIt; ++FuncIt)
    {
        UFunction* Function = *FuncIt;
        ContinueIfFalse(CanExportFunction(Function));

        FSnippetElement Ele;
        Ele.OwnerName = Class->GetName();
        Ele.ElementName = Function->GetName();
        Ele.DescriptionType = FUNC_RETURN_VOID;
        int32 index = PARAM_INDEX_START;
        for (TFieldIterator<UProperty> PropIt(Function); PropIt; ++PropIt)
        {
            UProperty* Property = *PropIt;
            if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
            {
                Ele.DescriptionType = Property->GetCPPType();
            }
            else
            {
                Ele.Body.Append(FString::Printf(FORMAT_FUNC_PARAMS, index++, *Property->GetCPPType(), *Property->GetName()));
                Ele.DescriptionDetail.Append(FString::Printf(FORMAT_DESC_PARAMS, *Property->GetCPPType(), *Property->GetName()));
            }
        }
        if (index != PARAM_INDEX_START)
        {
            // Remove needless char
            Ele.Body.RemoveFromEnd(", ");
            Ele.DescriptionDetail.RemoveFromEnd(", ");

            Ele.Body = FString::Printf(FORMAT_FUNC_WRAP, *Ele.ElementName, *Ele.Body);
            Ele.DescriptionDetail = FString::Printf(FORMAT_FUNC_WRAP, *Ele.ElementName, *Ele.DescriptionDetail);
        }
        
        AppendElement(SnippetText, Function->GetOwnerClass() == Class, Ele);
    }
}

void UGenerateSnippetsHelper::AppendElement(FString& SnippetText, bool IsOriginOwner, FSnippetElement& Ele)
{
    Ele.Body = Ele.Body.IsEmpty() ? Ele.ElementName : Ele.Body;
    Ele.DescriptionDetail = Ele.DescriptionDetail.IsEmpty() ? Ele.ElementName : Ele.DescriptionDetail;

    if (IsOriginOwner)
    {
        FString RegularStr = FString::Printf(FORMAT_ELEMENT, *Ele.OwnerName, CONTENT_DOT, *Ele.ElementName, CONTENT_EMPTY, CONTENT_EMPTY, *Ele.ElementName, *Ele.Body, *Ele.DescriptionType, *Ele.OwnerName, CONTENT_COLON, *Ele.DescriptionDetail);
        SnippetText.Append(RegularStr);
    }

    FString FuzzyStr = FString::Printf(FORMAT_ELEMENT, *Ele.OwnerName, CONTENT_LINE, *Ele.ElementName, *Ele.OwnerName, CONTENT_LINE, *Ele.ElementName, *Ele.Body, *Ele.DescriptionType, CONTENT_EMPTY, CONTENT_EMPTY, *Ele.DescriptionDetail);
    SnippetText.Append(FuzzyStr);
}

void UGenerateSnippetsHelper::FillLoadedReflectData(FReflectData& ReflectData)
{
    // Type = UEnum
    for (TObjectIterator<UEnum> EnumIt; EnumIt; ++EnumIt)
    {
        UEnum* CurrentEnum = *EnumIt;
        ContinueIfFalse(CanExportEnum(CurrentEnum));
        ReflectData.EnumArray.Add(CurrentEnum);
    }

    // Type = UStruct
    for (TObjectIterator<UScriptStruct> StructIt; StructIt; ++StructIt)
    {
        UScriptStruct* CurrentStruct = *StructIt;
        ContinueIfFalse(CanExportStruct(CurrentStruct));
        ReflectData.StructArray.Add(CurrentStruct);
    }

    // Type = UClass
    for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
    {
        UClass* CurrentClass = *ClassIt;
        ContinueIfFalse(CanExportClass(CurrentClass));
        ReflectData.ClassArray.Add(CurrentClass);
    }
}

void UGenerateSnippetsHelper::FillUnloadedReflectData(FReflectData& ReflectData)
{
    const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

    // Type = UEnum
    {
        TArray<FAssetData> AssetData;
        AssetRegistryModule.Get().GetAssetsByClass(UUserDefinedEnum::StaticClass()->GetFName(), AssetData);
        for (const FAssetData& Asset : AssetData)
        {
            ContinueIfFalse(Asset.IsValid() && !Asset.IsAssetLoaded());
            UEnum* CurrentEnum = Cast<UEnum>(Asset.GetAsset());
            ReflectData.EnumArray.Add(CurrentEnum);
        }
    }

    // Type = UStruct
    {
        TArray<FAssetData> AssetData;
        AssetRegistryModule.Get().GetAssetsByClass(UUserDefinedStruct::StaticClass()->GetFName(), AssetData);
        for (const FAssetData& Asset : AssetData)
        {
            ContinueIfFalse(Asset.IsValid() && !Asset.IsAssetLoaded());
            UScriptStruct* CurrentStruct = Cast<UScriptStruct>(Asset.GetAsset());
            ReflectData.StructArray.Add(CurrentStruct);
        }
    }

    // Type = UClass
    {
        const FString BPNormalTypeAllowed(TEXT("BPTYPE_Normal"));

        TArray<FAssetData> AssetData;
        AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), AssetData);
        for (const FAssetData& Asset : AssetData)
        {
            ContinueIfFalse(Asset.IsValid() && !Asset.IsAssetLoaded());
            const FString* BlueprintTypeStr = Asset.TagsAndValues.Find(BLUEPRINT_TYPE);
            const bool bNormalBP = BlueprintTypeStr && (*BlueprintTypeStr == BPNormalTypeAllowed);

            ContinueIfFalse(bNormalBP);
            uint32 ClassFlags = 0;
            const FString* ClassFlagsStr = Asset.TagsAndValues.Find(CLASS_FLAGS);

            ContinueIfFalse(ClassFlagsStr);
            ClassFlags = FCString::Atoi(**ClassFlagsStr);

            ContinueIfTrue(ClassFlags & CLASS_Deprecated);
            UClass* CurrentClass = (Cast<UBlueprint>(Asset.GetAsset()))->GeneratedClass;
            ReflectData.ClassArray.Add(CurrentClass);
        }
    }
}

bool UGenerateSnippetsHelper::CanExportEnum(const UEnum* Enum)
{
    return Enum && (Enum->GetBoolMetaData(BLUEPRINT_TYPE) || Enum->IsA<UUserDefinedEnum>());
}

bool UGenerateSnippetsHelper::CanExportStruct(const UScriptStruct* Struct)
{
    if (auto UDStruct = Cast<const UUserDefinedStruct>(Struct))
    {
        if (EUserDefinedStructureStatus::UDSS_UpToDate != UDStruct->Status.GetValue())
        {
            return false;
        }
    }
    return Struct && (Struct->GetBoolMetaDataHierarchical(BLUEPRINT_TYPE));
}

bool UGenerateSnippetsHelper::CanExportClass(const UClass* Class)
{
    bool bCanExport = (Class->ClassFlags & (CLASS_RequiredAPI | CLASS_MinimalAPI)) != 0; // Don't export classes that don't export DLL symbols
    return bCanExport;
}

bool UGenerateSnippetsHelper::CanExportFunction(const UFunction* Function)
{
    // We don't support delegates and non-public functions
    if ((Function->FunctionFlags & FUNC_Delegate))
    {
        return false;
    }

    // Reject if any of the parameter types is unsupported yet
    for (TFieldIterator<UProperty> ParamIt(Function); ParamIt; ++ParamIt)
    {
        UProperty* Param = *ParamIt;
        if (Param->IsA(UArrayProperty::StaticClass()) ||
            Param->ArrayDim > 1 ||
            Param->IsA(UDelegateProperty::StaticClass()) ||
            Param->IsA(UMulticastDelegateProperty::StaticClass()) ||
            Param->IsA(UWeakObjectProperty::StaticClass()) ||
            Param->IsA(UInterfaceProperty::StaticClass()))
        {
            return false;
        }
    }

    return true;
}

bool UGenerateSnippetsHelper::CanExportProperty(const UProperty* Property)
{
    // Only public, editable properties can be exported
    if (!Property->HasAnyFlags(RF_Public) ||
        (Property->PropertyFlags & CPF_Protected) ||
        !(Property->PropertyFlags & CPF_Edit))
    {
        return false;
    }

    // Reject if it's one of the unsupported types (yet)
    if (Property->IsA(UArrayProperty::StaticClass()) ||
        Property->ArrayDim > 1 ||
        Property->IsA(UDelegateProperty::StaticClass()) ||
        Property->IsA(UMulticastDelegateProperty::StaticClass()) ||
        Property->IsA(UWeakObjectProperty::StaticClass()) ||
        Property->IsA(UInterfaceProperty::StaticClass()) ||
        Property->IsA(UStructProperty::StaticClass()))
    {
        return false;
    }

    return true;
}