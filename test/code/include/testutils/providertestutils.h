/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Utilities to facilitate implementation of provider tests

    \date        04/03/2013 12:57:00


*/
/*----------------------------------------------------------------------------*/

#ifndef PROVIDERTESTUTILS_H
#define PROVIDERTESTUTILS_H

#include <scxcorelib/scxcmn.h>
#include "MI.h"
#include "base/field.h"
#include "module.h"
#include <scxcorelib/scxexception.h>
#include <map>
#include <vector>


// Helper definitions and class. Used in the case of error to printout entire call stack.
// NOTE: because of the HPUX v2 compiler we have to cast everything to const and then back.
#define CALL_LOCATION(errMsg) (const_cast<std::wostringstream &>( \
    CppUnitLocation(const_cast<const std::wostringstream &>(errMsg), SCXSRCLOCATION).GetStr()))
#define ERROR_MESSAGE (SCXCoreLib::StrToMultibyte(errMsg.str()))

class CppUnitLocation
{
    size_t m_size;
    const std::wostringstream &m_str;
public:
    CppUnitLocation(const std::wostringstream &str, const SCXCoreLib::SCXCodeLocation &loc) :
        m_size(str.str().size()), m_str(const_cast<std::wostringstream &>(str))
    {
        const_cast<std::wostringstream &>(m_str) << loc.Where();
    }
    ~CppUnitLocation()
    {   const_cast<std::wostringstream &>(m_str).str(m_str.str().substr(0, m_size));
        const_cast<std::wostringstream &>(m_str).seekp(m_size);
    }
    const std::wostringstream& GetStr() const { return m_str;}
};

class TestableInstance : public mi::Instance
{
public:
    struct PropertyInfo
    {
        // Sometimes properties are iterated through or searched by index. Here we terurn the name of the found property.
        std::wstring name;

        bool isKey;
        MI_Type type;
        MI_Boolean exists;
        MI_Uint8 flags;
        MI_Value value;
        PropertyInfo():isKey(0), type(MI_BOOLEAN), exists(0), flags(0){ memset(&value, 0, sizeof(value));}
        bool GetValue_MIBoolean(std::wostringstream &errMsg) const;
        std::wstring GetValue_MIString(std::wostringstream &errMsg) const;
        MI_Uint8 GetValue_MIUint8(std::wostringstream &errMsg) const;
        MI_Uint16 GetValue_MIUint16(std::wostringstream &errMsg) const;
        MI_Uint32 GetValue_MIUint32(std::wostringstream &errMsg) const;
        MI_Uint64 GetValue_MIUint64(std::wostringstream &errMsg) const;
        MI_Sint8 GetValue_MISint8(std::wostringstream &errMsg) const;
        MI_Sint16 GetValue_MISint16(std::wostringstream &errMsg) const;
        MI_Sint32 GetValue_MISint32(std::wostringstream &errMsg) const;
        MI_Sint64 GetValue_MISint64(std::wostringstream &errMsg) const;
        std::vector<std::wstring> GetValue_MIStringA(std::wostringstream &errMsg) const;
    };

    TestableInstance(const MI_Instance* instance) : Instance(instance->classDecl, instance, false) { }

    // Property handling methods.
    MI_Uint32 GetNumberOfProperties() const;
    MI_Result FindProperty(const char *name, struct PropertyInfo& info) const;
    MI_Result FindProperty(const wchar_t *name, struct PropertyInfo& info) const
    {
        return FindProperty(SCXCoreLib::StrToMultibyte(std::wstring(name)).c_str(), info);
    }
    MI_Result FindProperty(MI_Uint32 index, struct PropertyInfo& info, bool keysOnly = false) const;
    PropertyInfo GetProperty(const char *name, std::wostringstream &errMsg) const
    {
        PropertyInfo info;
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE + " name = " + name, MI_RESULT_OK, FindProperty(name, info));
        return info;
    }
    PropertyInfo GetProperty(const wchar_t *name, std::wostringstream &errMsg) const
    {
        return GetProperty(SCXCoreLib::StrToMultibyte(std::wstring(name)).c_str(), CALL_LOCATION(errMsg));
    }
    // Key handling methods.
    MI_Uint32 GetNumberOfKeys() const;
    std::wstring GetKey(const wchar_t *name, std::wostringstream &errMsg) const;
    std::wstring GetKey(const std::wstring &name, std::wostringstream &errMsg) const
    {
        return GetKey(name.c_str(), CALL_LOCATION(errMsg));
    }
    void GetKey(MI_Uint32 index, std::wstring &name, std::wstring &value, std::wostringstream &errMsg) const;
    std::wstring GetKeyName(MI_Uint32 index, std::wostringstream &errMsg) const;
    std::wstring GetKeyValue(MI_Uint32 index, std::wostringstream &errMsg) const;
    // Return value.
    MI_Boolean GetMIReturn_MIBoolean(std::wostringstream &errMsg) const;
};

class TestableContext
{
private:
    static MI_Result MI_CALL PostResult(MI_Context* context, MI_Result result);
    static MI_Result MI_CALL PostInstance(MI_Context *context, const MI_Instance* instance);
    static MI_Result MI_CALL RefuseUnload(MI_Context* context);

public:
    TestableContext();
    ~TestableContext();
    void Reset();

    operator mi::Context&();

    const mi::PropertySet& GetPropertySet() { return *m_pPropertySet; }

    MI_Result GetResult() { return m_result; }
    const std::vector<TestableInstance>& GetInstances() { return m_inst; }
    bool WasRefuseUnloadCalled() { return m_wasRefuseUnloadCalled; }
    size_t Size() const { return m_inst.size();}
    const TestableInstance& operator[](size_t i) const { return m_inst[i];}
    void Print() const;
private:
    MI_Context m_miContext;
    MI_ContextFT m_contextFT;
    mi::Context *m_pCppContext;

    MI_PropertySet m_miPropertySet;
    mi::PropertySet* m_pPropertySet;

    // Return values of interest
    MI_Result m_result;
    bool m_wasRefuseUnloadCalled;

    // Colllection of instances (should eventually be freed by calling MI_Instance_Delete())
    //std::vector<MI_Instance *> m_inst;
    std::vector<TestableInstance> m_inst;

    static std::map<MI_Context *, TestableContext *> ms_map;
};

/*----------------------------------------------------------------------------*/
//! Helper, checks if prerequisites are met, such as process running as a root.
//! \param[in]  testName    name of the test running, will be sent to stdout on failure.
//! \returns    true if prerequisites are met.
bool MeetsPrerequisites(std::wstring testName);

/*----------------------------------------------------------------------------*/
//! Helper, gets full host name.
//! \param[in]  errMsg    error message to be output on failure.
//! \returns    fully qualified host name.
std::wstring GetFQHostName(std::wostringstream &errMsg);

/*----------------------------------------------------------------------------*/
//! Helper, gets OS distribution name.
//! \param[in]  errMsg    error message to be output on failure.
//! \returns    distribution name.
std::wstring GetDistributionName(std::wostringstream &errMsg);

template<class T> void SetUpAgent(std::wostringstream &errMsg)
{
    mi::Module Module;
    T agent(&Module);
    TestableContext context;

    // Call load, verify result is okay, and verify RefuseUnload() was called.
    agent.Load(context);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_RESULT_OK, context.GetResult() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, context.WasRefuseUnloadCalled() );
}

template<class T> void TearDownAgent(std::wostringstream &errMsg)
{
    mi::Module Module;
    T agent(&Module);
    TestableContext context;

    // Call unload, verify result is okay, verify RefuseUnload() wasn't called.
    agent.Unload(context);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_RESULT_OK, context.GetResult() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, false, context.WasRefuseUnloadCalled() );
}

/*----------------------------------------------------------------------------*/
//! Enumerates all instances and returns vector with instance data.
//! \param      T Type of object to be enumerated.
//! \param[out]  context Object containing returned instances data and the return code.
//! \param[in]  errMsg       Stream containing error messages.
//! \param[in]  keysOnly True if only keys should be returned, otherwise all properties are returned.
template<class T> void EnumInstances(TestableContext &context, std::wostringstream &errMsg, bool keysOnly = false)
{
    mi::Module Module;
    T agent(&Module);
    agent.EnumerateInstances(context, NULL, context.GetPropertySet(), keysOnly, NULL);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_RESULT_OK, context.GetResult() );
}

/*----------------------------------------------------------------------------*/
//! Helper, verifies that theres one to one mapping between instance property names and given list of names.
//! \param[in]  instance                Instance to be verified
//! \param[in]  expectedPropertiesList  List of expected property names.
//! \param[out] expectedPropertiesCnt   Number of names in expectedProperties.
//! \param[in]  errMsg       Stream containing error messages.
void VerifyInstancePropertyNames(const TestableInstance &instance,
    const std::wstring* expectedPropertiesList, size_t expectedPropertiesCnt, std::wostringstream &errMsg);

/*----------------------------------------------------------------------------*/
//! Helper, tries to find the field by the given field name.
//! \param[in]  instance    Instance in which to look for the field.
//! \param[in]  name        Name of the field to be searched for.
//! \param[out] foundField  Pointer to the returned field.
//! \returns    MI_RESULT_OK on success, otherwise error code.
MI_Result FindFieldString(mi::Instance &instance, const char* name, Field* &foundField);

/*----------------------------------------------------------------------------*/
//! Returns single instance data for the object with given key values.
//! \param      T Type of object to be enumerated.
//! \param      TN Type of instance name object to be used.
//! \param[in]  keyNames Vector of the keys describing the requested object instance.
//! \param[in]  keyValues Vector of the key values describing the requested object instance.
//! \param[in]  context        Object containing all retrieved instance. Only one instance is returned.
//! \param[in]  errMsg       Stream containing error messages.
//! \returns    MI_RESULT_OK on success, otherwise error code.
template<class T, class TN> MI_Result GetInstance(
    const std::vector<std::wstring>& keyNames, const std::vector<std::wstring>& keyValues, TestableContext &context,
    std::wostringstream &errMsg)
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, keyNames.size(), keyValues.size());
    
    TN instanceName;

    // Set all key values.
    size_t k;
    for (k = 0; k < keyNames.size(); k++)
    {
        std::string keyName = SCXCoreLib::StrToMultibyte(keyNames[k]);
        Field* field;
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_RESULT_OK, FindFieldString(instanceName, keyName.c_str(), field));
        mi::Field<mi::String> *miField = reinterpret_cast<mi::Field<mi::String> *>(field);
        mi::String keyValue(SCXCoreLib::StrToMultibyte(keyValues[k]).c_str());
        miField->Set(keyValue);
    }

    // Get instance with this keys.
    mi::Module Module;
    T agent(&Module);
    agent.GetInstance(context, NULL, instanceName, context.GetPropertySet());
    if (context.GetResult() != MI_RESULT_OK)
    {
        return context.GetResult();
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, 1u, context.Size());
    // Verify we got the proper instance back.
    for (k = 0; k < keyNames.size(); k++)
    {
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, keyNames[k], context[0].GetKeyName(static_cast<MI_Uint32>(k),
            CALL_LOCATION(errMsg)));
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, keyValues[k], context[0].GetKeyValue(static_cast<MI_Uint32>(k),
            CALL_LOCATION(errMsg)));
    }    
    return MI_RESULT_OK;
}

/*----------------------------------------------------------------------------*/
//! Verify that it's possible to get instances by using a complete key, and
//! that the instances returned are ok.
//! \param      T            Type of object to be enumerated.
//! \param      TN           Type of instance name object to be used.
//! \param[in]  allKeyNames  Names of key properties.
//! \param[in]  invalidKey   Index of a key to be set to invalid value, -1 if all valid.
//! \param[in]  errMsg       Stream containing error messages.
//! \returns    MI_RESULT_OK on success, otherwise error code.
template<class T, class TN> MI_Result VerifyGetInstanceByCompleteKeySuccess(
    const std::vector<std::wstring>& allKeyNames, size_t invalidKey, std::wostringstream &errMsg)
{
    TestableContext originalContext;
    EnumInstances<T>(originalContext, CALL_LOCATION(errMsg));
    const std::vector<TestableInstance> &originalInstances = originalContext.GetInstances();
    CPPUNIT_ASSERT_MESSAGE(ERROR_MESSAGE, originalInstances.size() != 0);
    const TestableInstance &originalInstance = originalInstances[0];

    // Get all key values.
    std::vector<std::wstring> allKeyValues;
    for (size_t nr = 0; nr < allKeyNames.size(); nr++)
    {
        allKeyValues.push_back(originalInstance.GetKey(allKeyNames[nr], CALL_LOCATION(errMsg)));
    }

    // Set the invalid key value.
    CPPUNIT_ASSERT_MESSAGE(ERROR_MESSAGE, (invalidKey == static_cast<size_t>(-1)) || (invalidKey < allKeyNames.size()));
    if (invalidKey != static_cast<size_t>(-1))
    {
        allKeyValues[invalidKey] = L"InvalidKeyValue";
    }

    // Get instance.
    TestableContext context;
    MI_Result result = GetInstance<T, TN>(allKeyNames, allKeyValues, context, CALL_LOCATION(errMsg));
    if (result != MI_RESULT_OK)
    {
        return result;
    }   
    const TestableInstance &instance = context[0];

    // Number of keys must be same.
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, originalInstance.GetNumberOfKeys(), instance.GetNumberOfKeys());

    // And keys must be the same.
    for (MI_Uint32 nr = 0; nr < originalInstance.GetNumberOfKeys(); nr++)
    {
        std::wstring originalKeyName = originalInstance.GetKeyName(nr, CALL_LOCATION(errMsg));
        std::wstring originalKeyValue = originalInstance.GetKeyValue(nr, CALL_LOCATION(errMsg));
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, originalKeyValue, instance.GetKey(originalKeyName,
            CALL_LOCATION(errMsg)));
    }

    // Number of properties must be the same.
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, originalInstance.GetNumberOfProperties(), instance.GetNumberOfProperties());
    // And properties must be the same.
    for (MI_Uint32 nr = 0; nr < originalInstance.GetNumberOfProperties(); nr++)
    {
        TestableInstance::PropertyInfo originalProperty;
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_RESULT_OK, originalInstance.FindProperty(nr, originalProperty));
        TestableInstance::PropertyInfo property;
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_RESULT_OK,
            instance.FindProperty(originalProperty.name.c_str(), property));

        // Here we could also check if the property values of originalInstance and instance are the same. To do this we must
        // first define the TestableInstance::PropertyInfo == or != operators for all of the possible MI_Value's, 
        // which we don't have at the moment.
    }

    return result;
}

/*----------------------------------------------------------------------------*/
//! Verify that it's not possible to get instances without using a complete key.
//! \param      T            Type of object to be enumerated.
//! \param      TN           Type of instance name object to be used.
//! \param[in]  allKeyNames  Names of key properties.
//! \param[in]  errMsg       Stream containing error messages.
template<class T, class TN> void VerifyGetInstanceByPartialKeyFailure(const std::vector<std::wstring>& allKeyNames,
    std::wostringstream &errMsg)
{
    for (size_t nr = 0; nr < allKeyNames.size(); nr++)
    {
        std::vector<std::wstring> notAllKeyNames(allKeyNames);
        notAllKeyNames.erase(notAllKeyNames.begin() + nr);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE + " Didn't detect missing key " +
            SCXCoreLib::StrToMultibyte(allKeyNames[nr]), MI_RESULT_INVALID_PARAMETER,
            (VerifyGetInstanceByCompleteKeySuccess<T, TN>(
            notAllKeyNames, static_cast<size_t>(-1), CALL_LOCATION(errMsg))));
    }
}

/*----------------------------------------------------------------------------*/
//! Verify that it's not possible to get instances by using a key with invalid value.
//! \param      T            Type of object to be enumerated.
//! \param      TN           Type of instance name object to be used.
//! \param[in]  allKeyNames  Names of key properties.
//! \param[in]  errMsg       Stream containing error messages.
template<class T, class TN> void VerifyGetInstanceByInvalidKeyFailure(const std::vector<std::wstring>& allKeyNames,
    std::wostringstream &errMsg)
{
    for (size_t nr = 0; nr < allKeyNames.size(); nr++)
    {
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE + " Didn't detect invalid key " +
            SCXCoreLib::StrToMultibyte(allKeyNames[nr]), MI_RESULT_NOT_FOUND,
            (VerifyGetInstanceByCompleteKeySuccess<T, TN>(allKeyNames, nr, CALL_LOCATION(errMsg))));
    }
}

/*------------------------------------------------------------------------------

Standard tests, usualy run on every provider.

------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
//! Verifies that provider correctly enumerates instances, but returns keys only.
//! \param      T               Type of object to be enumerated.
//! \param[in]  allKeyNames     List of all expected key names.
//! \param[in]  context         Object containing all retrieved instances. Can be used
//!                             for further provider specific validation.
//! \param[in]  errMsg          Stream containing error messages.
template<class T> void StandardTestEnumerateKeysOnly(const std::vector<std::wstring>& allKeyNames,
    TestableContext &context, std::wostringstream &errMsg)
{
    EnumInstances<T>(context, CALL_LOCATION(errMsg), true);// Second parameter is true to get keys only.
    const std::vector<TestableInstance> &instances = context.GetInstances();
    CPPUNIT_ASSERT_MESSAGE(ERROR_MESSAGE, 1 <= instances.size());// We should always receive at least one instance.
    for (size_t i = 0; i < instances.size(); i++)
    {
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, allKeyNames.size(), instances[i].GetNumberOfKeys());
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, instances[i].GetNumberOfKeys(), instances[i].GetNumberOfProperties());
        for (size_t k = 0; k < allKeyNames.size(); k++)
        {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, allKeyNames[k], instances[i].GetKeyName(
                static_cast<MI_Uint32>(k), CALL_LOCATION(errMsg)));
        }
    }
}

/*----------------------------------------------------------------------------*/
//! Verifies that keys have expecteed values.
//! \param      T               Type of object to be enumerated.
//! \param[in]  keyNames        List of keys to check.
//! \param[in]  keyValues       List of expected key values.
//! \param[in]  keysSame        List of keys that should have same values for all instances.
//! \param[in]  context         Object containing all retrieved instances. Can be used
//!                             for further provider specific validation.
//! \param[in]  errMsg          Stream containing error messages.
template<class T> void StandardTestCheckKeyValues(const std::vector<std::wstring>& keyNames,
    const std::vector<std::wstring>& keyValues, const std::vector<std::wstring>& keysSame,
    TestableContext &context, std::wostringstream &errMsg)
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, keyNames.size(), keyValues.size());
    EnumInstances<T>(context, CALL_LOCATION(errMsg), true);// Second parameter is true to get keys only.
    const std::vector<TestableInstance> &instances = context.GetInstances();
    CPPUNIT_ASSERT_MESSAGE(ERROR_MESSAGE, 1 <= instances.size());// We should always receive at least one instance.
    
    // First check if keys in keyNames have expected keyValues.
    for (size_t i = 0; i < instances.size(); i++)
    {
        // We asked for keys only, this should be all of the properties returned.
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, instances[i].GetNumberOfKeys(), instances[i].GetNumberOfProperties());
        for (size_t k = 0; k < keyNames.size(); k++)
        {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, keyValues[k], instances[i].GetKey(keyNames[k],
                CALL_LOCATION(errMsg)));
        }
    }
    
    // Now check if keys in keysSame have same value in all the instances.
    for (size_t i = 1; i < instances.size(); i++)
    {
        for (size_t k = 0; k < keysSame.size(); k++)
        {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, instances[0].GetKey(keysSame[k], CALL_LOCATION(errMsg)),
                instances[i].GetKey(keysSame[k], CALL_LOCATION(errMsg)));
        }
    }
}

/*----------------------------------------------------------------------------*/
//! Verifies that provider correctly enumerates instances.
//! \param      T              Type of object to be enumerated.
//! \param[in]  allKeyNames    List of all expected key names.
//! \param[in]  context        Object containing all retrieved instances. Can be used
//!                            for further provider specific validation.
//! \param[in]  errMsg         Stream containing error messages.
template<class T> void StandardTestEnumerateInstances(const std::vector<std::wstring>& allKeyNames,
    TestableContext &context, std::wostringstream &errMsg)
{
    EnumInstances<T>(context, CALL_LOCATION(errMsg));
    const std::vector<TestableInstance> &instances = context.GetInstances();
    CPPUNIT_ASSERT_MESSAGE(ERROR_MESSAGE, 1 <= instances.size());// We should always receive at least one instance.
    for (size_t i = 0; i < instances.size(); i++)
    {
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, allKeyNames.size(), instances[i].GetNumberOfKeys());
        for (size_t k = 0; k < allKeyNames.size(); k++)
        {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, allKeyNames[k], instances[i].GetKeyName(
                static_cast<MI_Uint32>(k), CALL_LOCATION(errMsg)));
        }
    }
}

/*----------------------------------------------------------------------------*/
//! Verifies that provider returns valid instance GetInstance() method is called.
//! \param      T            Type of object to be enumerated.
//! \param      TN           Type of instance name object to be used.
//! \param[in]  context      Object containing all retrieved instances. Can be used
//!                          for further provider specific validation.
//! \param[in]  numberOfKeys Number of keys instance should have.
//! \param[in]  errMsg       Stream containing error messages.
template<class T, class TN> void StandardTestGetInstance(TestableContext &context, size_t numberOfKeys,
    std::wostringstream &errMsg)
{
    // First retrieve all instance names just to know what instance
    // to ask for.
    TestableContext originalContext;
    EnumInstances<T>(originalContext, CALL_LOCATION(errMsg), true);
    const std::vector<TestableInstance> &instances = originalContext.GetInstances();
    CPPUNIT_ASSERT_MESSAGE(ERROR_MESSAGE, 1 <= instances.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, numberOfKeys, instances[0].GetNumberOfKeys());

    // Prepare key names and values.
    std::vector<std::wstring> keyNames;
    std::vector<std::wstring> keyValues;
    for (size_t i = 0; i < numberOfKeys; i++)
    {
        std::wstring name = instances[0].GetKeyName(static_cast<MI_Uint32>(i), CALL_LOCATION(errMsg));
        std::wstring value = instances[0].GetKeyValue(static_cast<MI_Uint32>(i), CALL_LOCATION(errMsg));

        keyNames.push_back(name);
        keyValues.push_back(value);
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_RESULT_OK,
        (GetInstance<T, TN>(keyNames, keyValues, context, CALL_LOCATION(errMsg))));
}

/*----------------------------------------------------------------------------*/
//! Verifies that provider returns correct instance when complete key is provided
//! and that it's not possible to get instances without using a complete key.
//! \param      T            Type of object to be enumerated.
//! \param      TN           Type of instance name object to be used.
//! \param[in]  allKeyNames  Names of key properties.
//! \param[in]  errMsg       Stream containing error messages.
template<class T, class TN> void StandardTestVerifyGetInstanceKeys(const std::vector<std::wstring>& allKeyNames,
    std::wostringstream &errMsg)
{
    try {
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_RESULT_OK, (VerifyGetInstanceByCompleteKeySuccess<T, TN>(
            allKeyNames, static_cast<size_t>(-1), CALL_LOCATION(errMsg))));
        VerifyGetInstanceByPartialKeyFailure<T, TN>(allKeyNames, CALL_LOCATION(errMsg));
        VerifyGetInstanceByInvalidKeyFailure<T, TN>(allKeyNames, CALL_LOCATION(errMsg));
    } catch (SCXCoreLib::SCXAccessViolationException&) {
        // Skip access violations because some properties
        // require root access.
        SCXUNIT_WARNING(L"Skipping test - need root access");
        SCXUNIT_RESET_ASSERTION();
    }
}

#endif // PROVIDERTESTUTILS_H

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
