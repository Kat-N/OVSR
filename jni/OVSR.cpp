#define CL_USE_DEPRECATED_OPENCL_1_1_APIS

#include <jni.h>
#include <android/bitmap.h>
#include <android/log.h>

#include <string>
#include <cstring>
#include <sstream>
#include <vector>

/*
 * needed for loadProgram function
 */
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>

#include <sys/time.h>

#include <CL/opencl.h>

#define BUILDOPT "-cl-single-precision-constant -cl-denorms-are-zero -cl-fast-relaxed-math"
#define  LOG_TAG    "OpenCLnative"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

struct OpenCLObjects
{
	cl_platform_id platform;
	cl_device_id device;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_kernel kernel;
	bool isInputBufferInitialized;
	cl_mem inputBuffer;
	cl_mem outputBuffer;
};

static OpenCLObjects openCLObjects;

/*! /brief Reads the text from a file and returns it in a string.
 * @param input is the name and full path of the file that has to be read
 * @return It returns the text from a file in a string.
 */
inline std::string loadProgram(std::string input)
{
	std::ifstream stream(input.c_str());
	if (!stream.is_open()) {
		LOGE("Cannot open input file\n");
		exit(1);
	}
	return std::string( std::istreambuf_iterator<char>(stream),
			(std::istreambuf_iterator<char>()));
}

/*! This function helps to create informative messages in
 * case when OpenCL errors occur. 
 * @param error is the error code generated by the OpenCL function
 * @return The function returns a string representation for an OpenCL error code.
 * For example, "CL_DEVICE_NOT_FOUND" instead of "-1".
 */
const char* opencl_error_to_str (cl_int error)
{
#define CASE_CL_CONSTANT(NAME) case NAME: return #NAME;

	// Suppose that no combinations are possible.
	switch(error)
	{
	CASE_CL_CONSTANT(CL_SUCCESS)
        						CASE_CL_CONSTANT(CL_DEVICE_NOT_FOUND)
        						CASE_CL_CONSTANT(CL_DEVICE_NOT_AVAILABLE)
        						CASE_CL_CONSTANT(CL_COMPILER_NOT_AVAILABLE)
        						CASE_CL_CONSTANT(CL_MEM_OBJECT_ALLOCATION_FAILURE)
        						CASE_CL_CONSTANT(CL_OUT_OF_RESOURCES)
        						CASE_CL_CONSTANT(CL_OUT_OF_HOST_MEMORY)
        						CASE_CL_CONSTANT(CL_PROFILING_INFO_NOT_AVAILABLE)
        						CASE_CL_CONSTANT(CL_MEM_COPY_OVERLAP)
        						CASE_CL_CONSTANT(CL_IMAGE_FORMAT_MISMATCH)
        						CASE_CL_CONSTANT(CL_IMAGE_FORMAT_NOT_SUPPORTED)
        						CASE_CL_CONSTANT(CL_BUILD_PROGRAM_FAILURE)
        						CASE_CL_CONSTANT(CL_MAP_FAILURE)
        						CASE_CL_CONSTANT(CL_MISALIGNED_SUB_BUFFER_OFFSET)
        						CASE_CL_CONSTANT(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST)
        						CASE_CL_CONSTANT(CL_INVALID_VALUE)
        						CASE_CL_CONSTANT(CL_INVALID_DEVICE_TYPE)
        						CASE_CL_CONSTANT(CL_INVALID_PLATFORM)
        						CASE_CL_CONSTANT(CL_INVALID_DEVICE)
        						CASE_CL_CONSTANT(CL_INVALID_CONTEXT)
        						CASE_CL_CONSTANT(CL_INVALID_QUEUE_PROPERTIES)
        						CASE_CL_CONSTANT(CL_INVALID_COMMAND_QUEUE)
        						CASE_CL_CONSTANT(CL_INVALID_HOST_PTR)
        						CASE_CL_CONSTANT(CL_INVALID_MEM_OBJECT)
        						CASE_CL_CONSTANT(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR)
        						CASE_CL_CONSTANT(CL_INVALID_IMAGE_SIZE)
        						CASE_CL_CONSTANT(CL_INVALID_SAMPLER)
        						CASE_CL_CONSTANT(CL_INVALID_BINARY)
        						CASE_CL_CONSTANT(CL_INVALID_BUILD_OPTIONS)
        						CASE_CL_CONSTANT(CL_INVALID_PROGRAM)
        						CASE_CL_CONSTANT(CL_INVALID_PROGRAM_EXECUTABLE)
        						CASE_CL_CONSTANT(CL_INVALID_KERNEL_NAME)
        						CASE_CL_CONSTANT(CL_INVALID_KERNEL_DEFINITION)
        						CASE_CL_CONSTANT(CL_INVALID_KERNEL)
        						CASE_CL_CONSTANT(CL_INVALID_ARG_INDEX)
        						CASE_CL_CONSTANT(CL_INVALID_ARG_VALUE)
        						CASE_CL_CONSTANT(CL_INVALID_ARG_SIZE)
        						CASE_CL_CONSTANT(CL_INVALID_KERNEL_ARGS)
        						CASE_CL_CONSTANT(CL_INVALID_WORK_DIMENSION)
        						CASE_CL_CONSTANT(CL_INVALID_WORK_GROUP_SIZE)
        						CASE_CL_CONSTANT(CL_INVALID_WORK_ITEM_SIZE)
        						CASE_CL_CONSTANT(CL_INVALID_GLOBAL_OFFSET)
        						CASE_CL_CONSTANT(CL_INVALID_EVENT_WAIT_LIST)
        						CASE_CL_CONSTANT(CL_INVALID_EVENT)
        						CASE_CL_CONSTANT(CL_INVALID_OPERATION)
        						CASE_CL_CONSTANT(CL_INVALID_GL_OBJECT)
        						CASE_CL_CONSTANT(CL_INVALID_BUFFER_SIZE)
        						CASE_CL_CONSTANT(CL_INVALID_MIP_LEVEL)
        						CASE_CL_CONSTANT(CL_INVALID_GLOBAL_WORK_SIZE)
        						CASE_CL_CONSTANT(CL_INVALID_PROPERTY)

	default:
		return "UNKNOWN ERROR CODE";
	}

#undef CASE_CL_CONSTANT
}


/*! The following macro is used after each OpenCL call
 * to check if OpenCL error occurs. In the case when ERR != CL_SUCCESS
 * the macro forms an error message with OpenCL error code mnemonic,
 * puts it to LogCat, and returns from a caller function.
 */
#define SAMPLE_CHECK_ERRORS(ERR)                                                      \
		if(ERR != CL_SUCCESS)                                                             \
		{                                                                                 \
			LOGE                                                                          \
			(                                                                             \
					"OpenCL error with code %s happened in file %s at line %d. Exiting.\n",   \
					opencl_error_to_str(ERR), __FILE__, __LINE__                              \
			);                                                                            \
			\
			return;                                                                       \
		}

	/*! \brief This function picks and creates all necessary OpenCL objects
	 * to be used at each filter iteration. 
	 *
	 * The objects are:
	 * OpenCL platform, device, context, command queue, program,
	 * and kernel.
	 *
	 * @param env is a pointer to the java environment where this function is called.
	 * @param thisObject is a java object to be able to access java data from the native code
	 * @param kernelName is a java string that contains the kernel for which the OpenCL code must be initialised
	 * @param required_device_type is a OpenCL datatype that holds the type of device the context has to be build for.
	 * @param openCLObjects is the adres of the openCLObjects struct
	 */
void initOpenCL
(
		JNIEnv* env,
		jobject thisObject,
		jstring kernelName,
		cl_device_type required_device_type,
		OpenCLObjects& openCLObjects
)
{

	using namespace std;


	openCLObjects.isInputBufferInitialized = false;

	cl_int err = CL_SUCCESS;

	/* 
	 * Step 1: Get the first platform
	 */
	cl_platform_id platform;
	err = clGetPlatformIDs(1, &platform, NULL);
	SAMPLE_CHECK_ERRORS(err);

	cl_uint i = 0;
	size_t platform_name_length = 0;
	err = clGetPlatformInfo(
			platform,
			CL_PLATFORM_NAME,
			0,
			0,
			&platform_name_length
	);
	SAMPLE_CHECK_ERRORS(err);

	openCLObjects.platform = platform;
	/* 
	 * Step 2: Create context with a device of the specified type (required_device_type).
	 */

	cl_context_properties context_props[] = {
			CL_CONTEXT_PLATFORM,
			cl_context_properties(openCLObjects.platform),
			0
	};

	openCLObjects.context =
			clCreateContextFromType
			(
					context_props,
					required_device_type,
					0,
					0,
					&err
			);
	SAMPLE_CHECK_ERRORS(err);
	/* 
	 * Step 3: Query for OpenCL device that was used for context creation.
	 */
	err = clGetContextInfo
			(
					openCLObjects.context,
					CL_CONTEXT_DEVICES,
					sizeof(openCLObjects.device),
					&openCLObjects.device,
					0
			);
	SAMPLE_CHECK_ERRORS(err);

	/*  
	 * Step 4: Create OpenCL program from its source code.
	 * The file name is passed by java.
	 * Convert the jstring to const char* and append the needed directory path.
	 */
	const char* fileName = env->GetStringUTFChars(kernelName, 0);
	std::string fileDir;
	fileDir.append("/data/data/com.denayer.ovsr/app_execdir/");
	fileDir.append(fileName);
	fileDir.append(".cl");
	std::string kernelSource = loadProgram(fileDir);
	const char* kernelSourceChar = kernelSource.c_str();

	openCLObjects.program =
			clCreateProgramWithSource
			(
					openCLObjects.context,
					1,
					&kernelSourceChar,
					0,
					&err
			);

	SAMPLE_CHECK_ERRORS(err);

	/*
	 * Build the program with defined BUILDOPT (build optimalisations).
	 */
	err = clBuildProgram(openCLObjects.program, 0, 0, BUILDOPT, 0, 0);
	jstring JavaString = (*env).NewStringUTF("Code compiled succesful.");
	if(err == CL_BUILD_PROGRAM_FAILURE)
	{
		size_t log_length = 0;
		err = clGetProgramBuildInfo(
				openCLObjects.program,
				openCLObjects.device,
				CL_PROGRAM_BUILD_LOG,
				0,
				0,
				&log_length
		);
		SAMPLE_CHECK_ERRORS(err);

		vector<char> log(log_length);

		err = clGetProgramBuildInfo(
				openCLObjects.program,
				openCLObjects.device,
				CL_PROGRAM_BUILD_LOG,
				log_length,
				&log[0],
				0
		);
		SAMPLE_CHECK_ERRORS(err);

		LOGE
		(
				"Error happened during the build of OpenCL program.\nBuild log: %s",
				&log[0]
		);
		/*
		 * sends the error log to the console text edit.
		 */
		std::string str(log.begin(),log.end());
		const char * c = str.c_str();
		JavaString = (*env).NewStringUTF(c);
		jclass MyJavaClass = (*env).FindClass("com/denayer/ovsr/OpenCL");
		if (!MyJavaClass){
			LOGD("METHOD NOT FOUND");
			return;} /* method not found */
		jmethodID setConsoleOutput = (*env).GetMethodID(MyJavaClass, "setConsoleOutput", "(Ljava/lang/String;)V");
		(*env).CallVoidMethod(thisObject, setConsoleOutput, JavaString);
		return;
	}
	/* 
	 * Step 6: Extract kernel from the built program.
	 */
	fileName = env->GetStringUTFChars(kernelName, 0);
	char result[100];   // array to hold the result.
	std::strcpy(result,fileName); // copy string one into the result.
	std::strcat(result,"Kernel"); // append string two to the result.
	openCLObjects.kernel = clCreateKernel(openCLObjects.program, result, &err);
	SAMPLE_CHECK_ERRORS(err);

	/* 
	 * Step 7: Create command queue.
	 */

	openCLObjects.queue =
			clCreateCommandQueue
			(
					openCLObjects.context,
					openCLObjects.device,
					0,     
					&err
			);
	SAMPLE_CHECK_ERRORS(err);

}

	/*! \brief This function enables the connection between initOpenCL and Java. 
	 * 
	 * @param env is a pointer to the java environment where this function is called.
	 * @param thisObject is a java object to be able to access java data from the native code
	 * @param kernelName is a java string that contains the kernel for which the OpenCL code must be initialised
	 */
extern "C" void Java_com_denayer_ovsr_OpenCL_initOpenCL
(
		JNIEnv* env,
		jobject thisObject,
		jstring kernelName,
		int dev_type
)
{
	if(dev_type==1)
	{
		LOGD("On CPU");
		initOpenCL
		(
				env,
				thisObject,
				kernelName,
				CL_DEVICE_TYPE_CPU,
				openCLObjects
		);
	}
	else
	{
		LOGD("On GPU");
		initOpenCL
		(
				env,
				thisObject,
				kernelName,
				CL_DEVICE_TYPE_GPU,
				openCLObjects
		);
	}

}
	/*! \brief This function prepares OpenCL to compile code from a Java string.
	 *
	 * It picks and creates all necessary OpenCL objects
	 * to be used at each filter iteration. The objects are:
	 * OpenCL platform, device, context, command queue, program,
	 * and kernel.
	 * 
	 * @param env is a pointer to the java environment where this function is called.
	 * @param thisObject is a java object to be able to access java data from the native code
	 * @param kernelCode is the OpenCL code to be compiled
	 * @param kernelName is a java string that contains the kernel for which the OpenCL code must be initialised
	 * @param required_device_type is a OpenCL datatype that holds the type of device the context has to be build for.
	 * @param openCLObjects is the adres of the openCLObjects struct
	 */
void initOpenCLFromInput
(
		JNIEnv* env,
		jobject thisObject,
		jstring kernelCode,
		jstring kernelName,
		cl_device_type required_device_type,
		OpenCLObjects& openCLObjects
)
{
	using namespace std;

	openCLObjects.isInputBufferInitialized = false;
	cl_int err = CL_SUCCESS;

	cl_platform_id platform;
	err = clGetPlatformIDs(1, &platform, NULL);
	SAMPLE_CHECK_ERRORS(err);

	cl_uint i = 0;
	size_t platform_name_length = 0;
	err = clGetPlatformInfo(
			platform,
			CL_PLATFORM_NAME,
			0,
			0,
			&platform_name_length
	);
	SAMPLE_CHECK_ERRORS(err);

	openCLObjects.platform = platform;

	cl_context_properties context_props[] = {
			CL_CONTEXT_PLATFORM,
			cl_context_properties(openCLObjects.platform),
			0
	};

	openCLObjects.context =
			clCreateContextFromType
			(
					context_props,
					required_device_type,
					0,
					0,
					&err
			);
	SAMPLE_CHECK_ERRORS(err);

	err = clGetContextInfo
			(
					openCLObjects.context,
					CL_CONTEXT_DEVICES,
					sizeof(openCLObjects.device),
					&openCLObjects.device,
					0
			);
	SAMPLE_CHECK_ERRORS(err);

	const char* fileName = env->GetStringUTFChars(kernelCode, 0);

	openCLObjects.program =
			clCreateProgramWithSource
			(
					openCLObjects.context,
					1,
					&fileName,
					0,
					&err
			);

	SAMPLE_CHECK_ERRORS(err);

	err = clBuildProgram(openCLObjects.program, 0, 0, BUILDOPT, 0, 0);
	jstring JavaString = (*env).NewStringUTF("Code compiled succesful.");
	if(err == CL_BUILD_PROGRAM_FAILURE)
	{
		size_t log_length = 0;
		err = clGetProgramBuildInfo(
				openCLObjects.program,
				openCLObjects.device,
				CL_PROGRAM_BUILD_LOG,
				0,
				0,
				&log_length
		);
		SAMPLE_CHECK_ERRORS(err);

		vector<char> log(log_length);

		err = clGetProgramBuildInfo(
				openCLObjects.program,
				openCLObjects.device,
				CL_PROGRAM_BUILD_LOG,
				log_length,
				&log[0],
				0
		);
		SAMPLE_CHECK_ERRORS(err);

		LOGE
		(
				"Error happened during the build of OpenCL program.\nBuild log: %s",
				&log[0]
		);
		/*
		 * sends the error log to the console text edit.
		 */
		std::string str(log.begin(),log.end());
		const char * c = str.c_str();
		JavaString = (*env).NewStringUTF(c);
		jclass MyJavaClass = (*env).FindClass("com/denayer/ovsr/OpenCL");
		if (!MyJavaClass){
			LOGD("METHOD NOT FOUND");
			return;} /* method not found */
		jmethodID setConsoleOutput = (*env).GetMethodID(MyJavaClass, "setConsoleOutput", "(Ljava/lang/String;)V");
		(*env).CallVoidMethod(thisObject, setConsoleOutput, JavaString);
		return;
	}

	fileName = env->GetStringUTFChars(kernelName, 0);
	char result[100];   // array to hold the result.
	std::strcpy(result,fileName); //place the given kernel name into a string
	openCLObjects.kernel = clCreateKernel(openCLObjects.program, result, &err);
	SAMPLE_CHECK_ERRORS(err);

	openCLObjects.queue =
			clCreateCommandQueue
			(
					openCLObjects.context,
					openCLObjects.device,
					0,    // Creating queue properties, refer to the OpenCL specification for details.
					&err
			);
	SAMPLE_CHECK_ERRORS(err);
}

	/*! \brief This function enables the connection between initOpenCLFromInput and Java. 
	 * 
	 * @param env is a pointer to the java environment where this function is called.
	 * @param thisObject is a java object to be able to access java data from the native code
	 * @param OpenCLCode is a java string that contains the OpenCL code to be excecuted
	 * @param kernelName is a java string that contains the kernel for which the OpenCL code must be initialised
	 */
extern "C" void Java_com_denayer_ovsr_OpenCL_initOpenCLFromInput
(
		JNIEnv* env,
		jobject thisObject,
		jstring OpenCLCode,
		jstring kernelName,
		int dev_type
)
{
	if(dev_type==1)
	{
		initOpenCLFromInput
		(
				env,
				thisObject,
				OpenCLCode,
				kernelName,
				CL_DEVICE_TYPE_CPU,
				openCLObjects
		);
	}
	else
	{
		initOpenCLFromInput
		(
				env,
				thisObject,
				OpenCLCode,
				kernelName,
				CL_DEVICE_TYPE_GPU,
				openCLObjects
		);
	}
}

	/*! \brief This function prepares OpenCL to compile code from a Java string.
	 *
	 * This is a regular sequence of calls to deallocate
	 * all created OpenCL resources in bootstrapOpenCL.
	 *
	 * You can call these deallocation procedures in the middle
	 * of your application execution (not at the end) if you don't
	 * need OpenCL runtime any more.
	 * Use deallocation, for example, to free memory or recreate
	 * OpenCL objects with different parameters.
	 *
	 * Calling deallocation in the end of application
	 * execution might be not so useful, as upon killing
	 * an application, which is a common thing in the Android OS,
	 * all OpenCL resources are deallocated automatically.
	 * 
	 * @param openCLObjects is the adres of the openCLObjects struct
	 */
void shutdownOpenCL (OpenCLObjects& openCLObjects)
{
	LOGD("SHUTTING DOWN");
	cl_int err = CL_SUCCESS;

	if(openCLObjects.isInputBufferInitialized)
	{
		err = clReleaseMemObject(openCLObjects.inputBuffer);
		SAMPLE_CHECK_ERRORS(err);
	}

	err = clReleaseKernel(openCLObjects.kernel);
	SAMPLE_CHECK_ERRORS(err);

	err = clReleaseProgram(openCLObjects.program);
	SAMPLE_CHECK_ERRORS(err);

	err = clReleaseCommandQueue(openCLObjects.queue);
	SAMPLE_CHECK_ERRORS(err);

	err = clReleaseContext(openCLObjects.context);
	SAMPLE_CHECK_ERRORS(err);

	/* There is no procedure to deallocate OpenCL devices or
	 * platforms as both are not created at the startup,
	 * but queried from the OpenCL runtime.
	 */
}

	/*! \brief This function enables the connection between shutdownOpenCL and Java. 
	 * 
	 * @param env is a pointer to the java environment where this function is called.
	 * @param thisObject is a java object to be able to access java data from the native code
	 */
extern "C" void Java_com_denayer_ovsr_OpenCL_shutdownOpenCL
(
		JNIEnv* env,
		jobject thisObject
)
{
	shutdownOpenCL(openCLObjects);
}
	/*! \brief Excecutes an OpenCL kernel. Makes no use of image2d
	 *  
	 * @param env is a pointer to the java environment where this function is called.
	 * @param thisObject is a java object to be able to access java data from the native code
	 * @param openCLObjects is the adres of the openCLObjects struct
	 * @param inputBitmap is the Android bitmap that has to be processed
	 * @param outputBitmap is the result of the OpenCL kernel
	*/
void nativeBasicOpenCL
(
		JNIEnv* env,
		jobject thisObject,
		OpenCLObjects& openCLObjects,
		jobject inputBitmap,
		jobject outputBitmap
)
{
	using namespace std;

	timeval start;
	timeval end;

//	gettimeofday(&start, NULL);

	AndroidBitmapInfo bitmapInfo;
	AndroidBitmap_getInfo(env, inputBitmap, &bitmapInfo);

	size_t bufferSize = bitmapInfo.height * bitmapInfo.stride;

	cl_uint rowPitch = bitmapInfo.stride / 4;

	cl_int err = CL_SUCCESS;


	if(openCLObjects.isInputBufferInitialized)
	{

		err = clReleaseMemObject(openCLObjects.inputBuffer);
		SAMPLE_CHECK_ERRORS(err);
	}

	void* inputPixels = 0;
	AndroidBitmap_lockPixels(env, inputBitmap, &inputPixels);

	openCLObjects.inputBuffer =
			clCreateBuffer
			(
					openCLObjects.context,
					CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
					bufferSize,   // Buffer size in bytes.
					inputPixels,  // Bytes for initialization.
					&err
			);
	SAMPLE_CHECK_ERRORS(err);

	openCLObjects.isInputBufferInitialized = true;

	AndroidBitmap_unlockPixels(env, inputBitmap);

	void* outputPixels = 0;
	AndroidBitmap_lockPixels(env, outputBitmap, &outputPixels);

	cl_mem outputBuffer =
			clCreateBuffer
			(
					openCLObjects.context,
					CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
					bufferSize,    // Buffer size in bytes, same as the input buffer.
					outputPixels,  // Area, above which the buffer is created.
					&err
			);
	SAMPLE_CHECK_ERRORS(err);

	err = clSetKernelArg(openCLObjects.kernel, 0, sizeof(openCLObjects.inputBuffer), &openCLObjects.inputBuffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(openCLObjects.kernel, 1, sizeof(outputBuffer), &outputBuffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(openCLObjects.kernel, 2, sizeof(cl_uint), &rowPitch);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(openCLObjects.kernel, 3, sizeof(cl_uint), &bitmapInfo.width);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(openCLObjects.kernel, 4, sizeof(cl_uint), &bitmapInfo.height);
	SAMPLE_CHECK_ERRORS(err);

	size_t globalSize[2] = { bitmapInfo.width, bitmapInfo.height };

	err =
			clEnqueueNDRangeKernel
			(
					openCLObjects.queue,
					openCLObjects.kernel,
					2,
					0,
					globalSize,
					0,
					0, 0, 0
			);
	SAMPLE_CHECK_ERRORS(err);

	err = clFinish(openCLObjects.queue);
	SAMPLE_CHECK_ERRORS(err);

	err = clEnqueueReadBuffer (openCLObjects.queue,
			outputBuffer,
			true,
			0,
			bufferSize,
			outputPixels,
			0,
			0,
			0);
	SAMPLE_CHECK_ERRORS(err);

	// Call clFinish to guarantee that the output region is updated.
	err = clFinish(openCLObjects.queue);
	SAMPLE_CHECK_ERRORS(err);

	err = clReleaseMemObject(outputBuffer);
	SAMPLE_CHECK_ERRORS(err);

	// Make the output content be visible at the Java side by unlocking
	// pixels in the output bitmap object.
	AndroidBitmap_unlockPixels(env, outputBitmap);

//	gettimeofday(&end, NULL);
//
//	float ndrangeDuration =
//			(end.tv_sec + end.tv_usec * 1e-6) - (start.tv_sec + start.tv_usec * 1e-6);

	//LOGD("nativeBasicOpenCL ends successfully");

//	jclass MyJavaClass = (*env).FindClass("com/denayer/ovsr/OpenCL");
//	if (!MyJavaClass){
//		return;} /* method not found */
//	jmethodID setTimeFromJNI = (*env).GetMethodID(MyJavaClass, "setTimeFromJNI", "(F)V");
//	(*env).CallVoidMethod(thisObject, setTimeFromJNI, ndrangeDuration);
	//LOGD("Done");
}
	/*! \brief This function enables the connection between nativeBasicOpenCL and Java. 
	 * 
	 * @param env is a pointer to the java environment where this function is called.
	 * @param thisObject is a java object to be able to access java data from the native code
	 * @param inputBitmap is the Android bitmap that has to be processed
	 * @param outputBitmap is the result of the OpenCL kernel
	 */
extern "C" void Java_com_denayer_ovsr_OpenCL_nativeBasicOpenCL
(
		JNIEnv* env,
		jobject thisObject,
		jobject inputBitmap,
		jobject outputBitmap
)
{
	nativeBasicOpenCL
	(
			env,
			thisObject,
			openCLObjects,
			inputBitmap,
			outputBitmap
	);
}
	/*! \brief Excecutes an OpenCL kernel. Makes use of the image2d_t data type.
	 *  
	 * @param env is a pointer to the java environment where this function is called.
	 * @param thisObject is a java object to be able to access java data from the native code
	 * @param openCLObjects is the adres of the openCLObjects struct
	 * @param inputBitmap is the Android bitmap that has to be processed
	 * @param outputBitmap is the result of the OpenCL kernel
	*/
void nativeImage2DOpenCL
(
		JNIEnv* env,
		jobject thisObject,
		OpenCLObjects& openCLObjects,
		jobject inputBitmap,
		jobject outputBitmap
)
{
	using namespace std;

	timeval start;
	timeval end;

	gettimeofday(&start, NULL);

	AndroidBitmapInfo bitmapInfo;
	AndroidBitmap_getInfo(env, inputBitmap, &bitmapInfo);

	size_t bufferSize = bitmapInfo.height * bitmapInfo.stride;
	LOGD("height: %d",bitmapInfo.height);
	cl_uint rowPitch = bitmapInfo.stride / 4;

	cl_int err = CL_SUCCESS;

	void* inputPixels = 0;
	AndroidBitmap_lockPixels(env, inputBitmap, &inputPixels);

	cl_image_format image_format;
	image_format.image_channel_data_type=CL_UNORM_INT8;
	image_format.image_channel_order=CL_RGBA;

	openCLObjects.inputBuffer =
			clCreateImage2D(openCLObjects.context,
					CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
					&image_format,
					bitmapInfo.width,
					bitmapInfo.height,
					0,
					inputPixels,
					&err);
	SAMPLE_CHECK_ERRORS(err);

	openCLObjects.isInputBufferInitialized = true;

	AndroidBitmap_unlockPixels(env, inputBitmap);

	void* outputPixels = 0;
	AndroidBitmap_lockPixels(env, outputBitmap, &outputPixels);

	cl_mem outputBuffer =
			clCreateImage2D(openCLObjects.context,
					CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
					&image_format,
					bitmapInfo.width,
					bitmapInfo.height,
					0,
					outputPixels,
					&err);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(openCLObjects.kernel, 0, sizeof(openCLObjects.inputBuffer), &openCLObjects.inputBuffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(openCLObjects.kernel, 1, sizeof(outputBuffer), &outputBuffer);
	SAMPLE_CHECK_ERRORS(err);

	size_t globalSize[2] = { bitmapInfo.width, bitmapInfo.height };

	err = clEnqueueNDRangeKernel
			(
					openCLObjects.queue,
					openCLObjects.kernel,
					2,
					0,
					globalSize,
					0,
					0, 0, 0
			);
	SAMPLE_CHECK_ERRORS(err);

	err = clFinish(openCLObjects.queue);
	SAMPLE_CHECK_ERRORS(err);

    const size_t origin[3] = {0, 0, 0};
    const size_t region[3] = {bitmapInfo.width, bitmapInfo.height, 1};

	err = clEnqueueReadImage(
			openCLObjects.queue,
			outputBuffer,
			true,
			origin,
			region,
			0,
			0,
			outputPixels,
			0,
			0,
			0);
	SAMPLE_CHECK_ERRORS(err);


	// Call clFinish to guarantee that the output region is updated.
	err = clFinish(openCLObjects.queue);
	SAMPLE_CHECK_ERRORS(err);

	err = clReleaseMemObject(outputBuffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clReleaseMemObject(openCLObjects.inputBuffer);
	SAMPLE_CHECK_ERRORS(err);
	openCLObjects.isInputBufferInitialized = false;
	// Make the output content be visible at the Java side by unlocking
	// pixels in the output bitmap object.
	AndroidBitmap_unlockPixels(env, outputBitmap);

	gettimeofday(&end, NULL);

	float ndrangeDuration =
			(end.tv_sec + end.tv_usec * 1e-6) - (start.tv_sec + start.tv_usec * 1e-6);

	LOGD("nativeBasicOpenCL ends successfully");

	jclass MyJavaClass = (*env).FindClass("com/denayer/ovsr/OpenCL");
	if (!MyJavaClass){
		LOGD("Method not found in OVSR.cpp on line 897");
		return;} /* method not found */
	jmethodID setTimeFromJNI = (*env).GetMethodID(MyJavaClass, "setTimeFromJNI", "(F)V"); //argument is float, return time is void
	(*env).CallVoidMethod(thisObject, setTimeFromJNI, ndrangeDuration);
}
	/*! \brief This function enables the connection between nativeImage2DOpenCL and Java. 
	 * 
	 * @param env is a pointer to the java environment where this function is called.
	 * @param thisObject is a java object to be able to access java data from the native code
	 * @param inputBitmap is the Android bitmap that has to be processed
	 * @param outputBitmap is the result of the OpenCL kernel
	 */
extern "C" void Java_com_denayer_ovsr_OpenCL_nativeImage2DOpenCL
(
		JNIEnv* env,
		jobject thisObject,
		jobject inputBitmap,
		jobject outputBitmap
)
{
	nativeImage2DOpenCL
	(
			env,
			thisObject,
			openCLObjects,
			inputBitmap,
			outputBitmap
	);
}
	/*! \brief Excecutes a saturation OpenCL kernel. Makes use of the image2d_t data type.
	 *  
	 * This function is specially made for saturation kernels to be excecuted. It has an extra argument "saturatie" that is a value between 0 and 200.
	 *
	 * @param env is a pointer to the java environment where this function is called.
	 * @param thisObject is a java object to be able to access java data from the native code
	 * @param openCLObjects is the adres of the openCLObjects struct
	 * @param inputBitmap is the Android bitmap that has to be processed
	 * @param outputBitmap is the result of the OpenCL kernel
	 * @param saturatie is the saturation value needed to process the kernel
	*/
void nativeSaturatieImage2DOpenCL
(
		JNIEnv* env,
		jobject thisObject,
		OpenCLObjects& openCLObjects,
		jobject inputBitmap,
		jobject outputBitmap,
		jfloat saturatie
)
{
	using namespace std;

//	timeval start;
//	timeval end;
//
//	gettimeofday(&start, NULL);


	AndroidBitmapInfo bitmapInfo;
	AndroidBitmap_getInfo(env, inputBitmap, &bitmapInfo);

	size_t bufferSize = bitmapInfo.height * bitmapInfo.stride;

	cl_uint rowPitch = bitmapInfo.stride / 4;

	cl_int err = CL_SUCCESS;

	if(openCLObjects.isInputBufferInitialized)
	{

		err = clReleaseMemObject(openCLObjects.inputBuffer);
		SAMPLE_CHECK_ERRORS(err);
	}

	void* inputPixels = 0;
	AndroidBitmap_lockPixels(env, inputBitmap, &inputPixels);

	cl_image_format image_format;
	image_format.image_channel_data_type=CL_UNORM_INT8;
	image_format.image_channel_order=CL_RGBA;

	//        http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clCreateImage2D.html
	openCLObjects.inputBuffer =
			clCreateImage2D(openCLObjects.context,
					CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
					&image_format,
					bitmapInfo.width,
					bitmapInfo.height,
					0,
					inputPixels,
					&err);
	SAMPLE_CHECK_ERRORS(err);

	openCLObjects.isInputBufferInitialized = true;

	AndroidBitmap_unlockPixels(env, inputBitmap);

	void* outputPixels = 0;
	AndroidBitmap_lockPixels(env, outputBitmap, &outputPixels);

	cl_mem outputBuffer =
			clCreateImage2D(openCLObjects.context,
					CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
					&image_format,
					bitmapInfo.width,
					bitmapInfo.height,
					0,
					outputPixels,
					&err);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(openCLObjects.kernel, 0, sizeof(openCLObjects.inputBuffer), &openCLObjects.inputBuffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(openCLObjects.kernel, 1, sizeof(outputBuffer), &outputBuffer);
	SAMPLE_CHECK_ERRORS(err);
	cl_float saturatieVal = saturatie / 100 ;
	err = clSetKernelArg(openCLObjects.kernel, 2, sizeof(cl_float), &saturatieVal);
	SAMPLE_CHECK_ERRORS(err);

	size_t globalSize[2] = { bitmapInfo.width, bitmapInfo.height };

	err = clEnqueueNDRangeKernel
			(
					openCLObjects.queue,
					openCLObjects.kernel,
					2,
					0,
					globalSize,
					0,
					0, 0, 0
			);
	SAMPLE_CHECK_ERRORS(err);

	err = clFinish(openCLObjects.queue);
	SAMPLE_CHECK_ERRORS(err);

    const size_t origin[3] = {0, 0, 0};
    const size_t region[3] = {bitmapInfo.width, bitmapInfo.height, 1};

	err = clEnqueueReadImage(
			openCLObjects.queue,
			outputBuffer,
			true,
			origin,
			region,
			0,
			0,
			outputPixels,
			0,
			0,
			0);
	SAMPLE_CHECK_ERRORS(err);


	// Call clFinish to guarantee that the output region is updated.
	err = clFinish(openCLObjects.queue);
	SAMPLE_CHECK_ERRORS(err);

	err = clReleaseMemObject(outputBuffer);
	SAMPLE_CHECK_ERRORS(err);

	// Make the output content be visible at the Java side by unlocking
	// pixels in the output bitmap object.
	AndroidBitmap_unlockPixels(env, outputBitmap);

//	gettimeofday(&end, NULL);
//
//	float ndrangeDuration =
//			(end.tv_sec + end.tv_usec * 1e-6) - (start.tv_sec + start.tv_usec * 1e-6);
//
//	LOGD("nativeBasicOpenCL ends successfully");
//
//	jclass MyJavaClass = (*env).FindClass("com/denayer/ovsr/OpenCL");
//	if (!MyJavaClass){
//		LOGD("Aj :(");
//		return;} /* method not found */
//	jmethodID setTimeFromJNI = (*env).GetMethodID(MyJavaClass, "setTimeFromJNI", "(F)V"); //argument is float, return time is void
//	(*env).CallVoidMethod(thisObject, setTimeFromJNI, ndrangeDuration);
}
	/*! \brief This function enables the connection between nativeSaturatieImage2DOpenCL and Java. 
	 * 
	 * @param env is a pointer to the java environment where this function is called.
	 * @param thisObject is a java object to be able to access java data from the native code
	 * @param inputBitmap is the Android bitmap that has to be processed
	 * @param outputBitmap is the result of the OpenCL kernel
	 * @param saturatie is the value to saturate with
	 */
extern "C" void Java_com_denayer_ovsr_OpenCL_nativeSaturatieImage2DOpenCL
(
		JNIEnv* env,
		jobject thisObject,
		jobject inputBitmap,
		jobject outputBitmap,
		jfloat saturatie
)
{
	nativeSaturatieImage2DOpenCL
	(
			env,
			thisObject,
			openCLObjects,
			inputBitmap,
			outputBitmap,
			saturatie
	);
}
