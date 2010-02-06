#include "jdyncall.hpp"
#include <jni.h>
#include "Exceptions.h"

char __cdecl doJavaToNativeCallHandler(DCArgs* args, DCValue* result, MethodCallInfo *info)
{
	void* callback;
	char returnVal = DC_SIGCHAR_VOID;
	size_t iParam;
	size_t nParams = info->nParams;
	JNIEnv *env = (JNIEnv*)dcbArgPointer(args);
	jobject objOrClass = (jobject)dcbArgPointer(args);
	THREAD_STATIC DCCallVM* vm = NULL;
	
	if (!vm) {
		vm = dcNewCallVM(1024);
	} else {
		// reset is done by dcMode anyway ! dcReset(vm);
	}

	dcMode(vm, info->fDCMode);

	for (iParam = 0; iParam < nParams; iParam++) {
		ValueType type = info->fParamTypes[iParam];
		switch (type) {
			case eIntValue:
				dcArgInt(vm, dcbArgInt(args));
				break;
			case eCLongValue:
				dcArgLong(vm, (long)dcbArgLongLong(args));
				break;
			case eSizeTValue:
				if (sizeof(size_t) == 4)
					dcArgInt(vm, (int)dcbArgLongLong(args));
				else
					dcArgLongLong(vm, dcbArgLongLong(args));
				break;
			case eLongValue:
				dcArgLongLong(vm, dcbArgLongLong(args));
				break;
			case eShortValue:
				dcArgShort(vm, dcbArgShort(args));
				break;
			case eByteValue:
				dcArgChar(vm, dcbArgChar(args));
				break;
			case eFloatValue:
				dcArgFloat(vm, dcbArgFloat(args));
				break;
			case eDoubleValue:
				dcArgDouble(vm, dcbArgDouble(args));
				break;
		}
	}
	callback = info->fForwardedSymbol;
	switch (info->fReturnType) {
#define CALL_CASE(valueType, capCase, hiCase, uni) \
		case valueType: \
			result->uni = dcCall ## capCase(vm, callback); \
			returnVal = DC_SIGCHAR_ ## hiCase; \
			break;
		CALL_CASE(eIntValue, Int, INT, i)
		CALL_CASE(eLongValue, Long, LONGLONG, l)
		CALL_CASE(eShortValue, Short, SHORT, s)
		CALL_CASE(eFloatValue, Float, FLOAT, f)
		CALL_CASE(eDoubleValue, Double, DOUBLE, d)
		CALL_CASE(eByteValue, Char, CHAR, c)
		case eCLongValue:
			result->l = dcCallLong(vm, callback);
			returnVal = DC_SIGCHAR_LONG;
			break;
		case eSizeTValue:
			result->l = (sizeof(size_t) == 4) ? dcCallInt(vm, callback) : dcCallLongLong(vm, callback);
			returnVal = DC_SIGCHAR_LONG;
			break;
		case eVoidValue:
			dcCallVoid(vm, callback);
			returnVal = DC_SIGCHAR_VOID;
			break;
		case eWCharValue:
			// TODO
		default:
			//cerr << "Return ValueType not supported yet: " << (int)info->fReturnType << " !\n";
			returnVal = DC_SIGCHAR_VOID;
	}

	return returnVal;
}

char __cdecl JavaToNativeCallHandler(DCCallback* callback, DCArgs* args, DCValue* result, void* userdata)
{
	MethodCallInfo *info = NULL;
	if (!userdata) {
		//cerr << "MethodCallHandler was called with a null userdata !!!\n";
		return DC_SIGCHAR_VOID;
	}
	
	info = (MethodCallInfo*)userdata;

	BEGIN_TRY();
	return doJavaToNativeCallHandler(args, result, info);
	END_TRY_RET(info->fEnv, 0);
}
