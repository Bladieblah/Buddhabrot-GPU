#ifndef OPENCL_H
#define OPENCL_H

// For loading shader files
#define MAX_SOURCE_SIZE (0x100000)

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <map>
#include <string>
#include <vector>

typedef struct OpenClBuffer {
    cl_mem buffer;
    size_t size;
} OpenClBuffer;

typedef struct BufferArgument {
    std::string name;
    OpenClBuffer buffer;
} BufferArgument;

typedef struct OpenClKernel {
    cl_kernel kernel;
    cl_uint work_dim;
    size_t item_size[2];
    std::string name;
} OpenClKernel;

typedef struct KernelArgument {
    std::string name;
    OpenClKernel kernel;
} KernelArgument;

class OpenCl {
public:
    OpenCl(char *filename, std::vector<BufferArgument> bufferArgs, std::vector<KernelArgument> kernelArgs, bool useGpu = true);
    void prepare(std::vector<BufferArgument> bufferArgs, std::vector<KernelArgument> kernelArgs);
    void setDevice();
    void getPlatformIds();
    void setKernelArg(std::string kernelName, cl_uint arg_index, size_t size, void *pointer);
    void setKernelBufferArg(std::string kernelName, std::string bufferName, int argIndex);
    void setKernelBufferArgLocal(std::string kernelName, std::string bufferName, int argIndex);
    void writeBuffer(std::string name, void *pointer);
    void swapBuffers(std::string buffer1, std::string buffer2);
    void step(std::string name);
    void readBuffer(std::string name, void *pointer);
    void cleanup();
    void flush();
    void printDeviceTypes();
    void getDeviceIds(cl_platform_id platformId);

    cl_platform_id *platform_ids;
    cl_platform_id platform_id;
    
    cl_device_id *device_ids;
    cl_device_id device_id;

    cl_context context;
    cl_command_queue command_queue;


    cl_program program;
    std::map<std::string, OpenClKernel> kernels;
    std::map<std::string, OpenClBuffer> buffers;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int ret;

    size_t source_size;
    char *source_str;

    // Kernel size for parallelisation
    size_t global_item_size[1];
    size_t local_item_size[1];

    char *filename;
    bool use_gpu;
};


#endif
