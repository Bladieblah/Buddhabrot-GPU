#include <map>
#include <stdio.h>
#include <string>
#include <vector>

#include "opencl.hpp"

using namespace std;

OpenCl::OpenCl(
    char *filename,
    vector<BufferSpec> bufferSpecs,
    vector<KernelSpec> kernelSpecs,
    bool useGpu
) {
    this->filename = filename;
    this->use_gpu = useGpu;
    
    this->prepare(bufferSpecs, kernelSpecs);
}

void OpenCl::prepare(vector<BufferSpec> bufferSpecs, vector<KernelSpec> kernelSpecs) {
    FILE *fp;
    fp = fopen(this->filename, "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    
    source_str = (char *)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);
    
    setDevice();

    // Create OpenCL Context
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    if (ret != CL_SUCCESS)
      fprintf(stderr, "Failed on function clCreateContext: %d\n", ret);

    // Create command queue
    command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

    // Create buffers
    for (BufferSpec bufferSpec : bufferSpecs) {
        bufferSpec.buffer.buffer = clCreateBuffer(
            context,
            CL_MEM_READ_WRITE,
            bufferSpec.buffer.size,
            NULL, &ret
        );

        if (ret != CL_SUCCESS)
            fprintf(stderr, "Failed on function clCreateBuffer for buffer %s: %d\n", bufferSpec.name.c_str(), ret);

        buffers[bufferSpec.name] = bufferSpec.buffer;
    }

    // Create kernel program from source file
    program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);
    if (ret != CL_SUCCESS)
      fprintf(stderr, "Failed on function clCreateProgramWithSource: %d\n", ret);
    
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    if (ret != CL_SUCCESS)
      fprintf(stderr, "Failed on function clBuildProgram: %d\n", ret);
    
    // Check program build info
    size_t len = 10000;
    ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
    char *buffer = (char *)calloc(len, sizeof(char));
    ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, len, buffer, NULL);
    fprintf(stderr, "%s\n", buffer);

    // Create kernels
    for (KernelSpec kernelSpec : kernelSpecs) {
        kernelSpec.kernel.kernel = clCreateKernel(program, kernelSpec.kernel.name.c_str(), &ret);

        if (ret != CL_SUCCESS)
            fprintf(stderr, "Failed on function clCreateKernel %s: %d\n", kernelSpec.name.c_str(), ret);
        
        kernels[kernelSpec.name] = kernelSpec.kernel;
    }
}

void OpenCl::setDevice() {
    getPlatformIds();
    cl_device_type target_type = use_gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU;
    
    for (int i = 0; i < ret_num_platforms; i++) {
        ret = clGetDeviceIDs(platform_ids[i], target_type, 1, &device_id, &ret_num_devices);
        if (ret != CL_SUCCESS)
            fprintf(stderr, "Failed on function clGetDeviceIDs: %d\n", ret);

        if (ret_num_devices > 0) {
            platform_id = platform_ids[i];
            return;
        }
    }

    fprintf(stderr, "No device found! The available options are:\n");
    printDeviceTypes();
}

void OpenCl::getPlatformIds() {
    ret = clGetPlatformIDs(0, NULL, &ret_num_platforms);
    if (ret != CL_SUCCESS) {
        fprintf(stderr, "Failed on function clGetPlatformIDs: %d\n", ret);
        exit(1);
    } else if (ret_num_platforms == 0) {
        fprintf(stderr, "No platform found\n");
        exit(0);
    }
    
    platform_ids = (cl_platform_id *)malloc(ret_num_platforms * sizeof(cl_platform_id));
    fprintf(stderr, "Found %d platforms\n", ret_num_platforms);

    ret = clGetPlatformIDs(ret_num_platforms, platform_ids, &ret_num_platforms);
    if (ret != CL_SUCCESS)
        fprintf(stderr, "Failed on function clGetPlatformIDs 2: %d\n", ret);
}

void OpenCl::setKernelArg(string kernelName, cl_uint argIndex, size_t size, void *pointer) {
    ret = clSetKernelArg(kernels[kernelName].kernel, argIndex, size, pointer);

    if (ret != CL_SUCCESS)
        fprintf(stderr, "Failed setting arg [%d] on kernel [%s]: [%d]\n", argIndex, kernelName.c_str(), ret);
}

void OpenCl::setKernelBufferArg(string kernelName, string bufferName, int argIndex) {
    ret = clSetKernelArg(kernels[kernelName].kernel, argIndex, sizeof(cl_mem), (void *)&(buffers[bufferName].buffer));

    if (ret != CL_SUCCESS)
        fprintf(stderr, "Failed setting buffer [%s] arg [%d] on kernel [%s]: %d\n", bufferName.c_str(), argIndex, kernelName.c_str(), ret);
}

void OpenCl::writeBuffer(string name, void *pointer) {
    ret = clEnqueueWriteBuffer(
        command_queue,
        buffers[name].buffer,
        CL_TRUE,
        0,
        buffers[name].size,
        pointer,
        0, NULL, NULL
    );
    
    if (ret != CL_SUCCESS) {
      fprintf(stderr, "Failed writing buffer [%s]: %d\n", name.c_str(), ret);
    }
}

void OpenCl::step(string name) {
    OpenClKernel kernel = kernels[name];
	ret = clEnqueueNDRangeKernel(command_queue, kernel.kernel, kernel.work_dim, NULL, kernel.item_size, NULL, 0, NULL, NULL);
    
    if (ret != CL_SUCCESS) {
      fprintf(stderr, "Failed executing kernel [%s]: %d\n", name.c_str(), ret);
      exit(1);
    }
}

void OpenCl::readBuffer(string name, void *pointer) {
    ret = clEnqueueReadBuffer(
        command_queue,
        buffers[name].buffer,
        CL_TRUE,
        0,
        buffers[name].size,
        pointer,
        0, NULL, NULL
    );
    
    if (ret != CL_SUCCESS) {
      fprintf(stderr, "Failed reading buffer %s: %d\n", name.c_str(), ret);
    }
}

void OpenCl::cleanup() {
    map<string, OpenClKernel>::iterator kernelIter;
    map<string, OpenClBuffer>::iterator bufferIter;
    
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseProgram(program);

    for (kernelIter = kernels.begin(); kernelIter != kernels.end(); kernelIter++) {
        ret = clReleaseKernel(kernelIter->second.kernel);
    }

    for (bufferIter = buffers.begin(); bufferIter != buffers.end(); bufferIter++) {
        ret = clReleaseMemObject(bufferIter->second.buffer);
    }
    
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
    
    free(source_str);
}

void OpenCl::flush() {
    clFlush(command_queue);
}

void OpenCl::printDeviceTypes() {
    for (int i = 0; i < ret_num_platforms; i++) {
        getDeviceIds(platform_ids[i]);
    }

    exit(0);
}

void OpenCl::getDeviceIds(cl_platform_id platformId) {
    ret = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_ALL, 0, NULL, &ret_num_devices);
    if (ret != CL_SUCCESS)
        fprintf(stderr, "Failed on function clGetDeviceIDs: %d\n", ret);
    
    device_ids = (cl_device_id *)malloc(ret_num_platforms * sizeof(cl_device_id));
    fprintf(stderr, "Found %d devices\n", ret_num_platforms);

    ret = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_ALL, ret_num_devices, device_ids, &ret_num_devices);
    if (ret != CL_SUCCESS)
        fprintf(stderr, "Failed on function clGetDeviceIDs 2: %d\n", ret);

    for (int i = 0; i < ret_num_devices; i++) {
        cl_device_type devType;

        ret = clGetDeviceInfo(device_ids[i], CL_DEVICE_TYPE, sizeof(cl_device_type), &devType, NULL);
        if (ret != CL_SUCCESS)
            fprintf(stderr, "Failed on function clGetDeviceInfo: %d\n", ret);

        switch (devType) {
            case CL_DEVICE_TYPE_CPU:
                fprintf(stderr, "CL_DEVICE_TYPE_CPU\n");
                break;
            case CL_DEVICE_TYPE_GPU:
                fprintf(stderr, "CL_DEVICE_TYPE_GPU\n");
                break;
            case CL_DEVICE_TYPE_ACCELERATOR:
                fprintf(stderr, "CL_DEVICE_TYPE_ACCELERATOR\n");
                break;
            case CL_DEVICE_TYPE_CUSTOM:
                fprintf(stderr, "CL_DEVICE_TYPE_CPU\n");
                break;
            
            default:
                fprintf(stderr, "Could not match type %llu\n", devType);
                break;
        }
    }
}
