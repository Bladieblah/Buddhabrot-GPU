#ifndef OPENCL_H
#define OPENCL_H

// For loading shader files
#define MAX_SOURCE_SIZE (0x100000)

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <chrono>
#include <map>
#include <string>
#include <vector>

typedef struct OpenClBuffer {
    cl_mem buffer;
    size_t size;
} OpenClBuffer;

typedef struct BufferSpec {
    std::string name;
    OpenClBuffer buffer;
} BufferSpec;

typedef struct OpenClKernel {
    cl_kernel kernel;
    cl_uint work_dim;
    size_t global_size[2];
    size_t local_size[2];
    std::string name;
} OpenClKernel;

typedef struct KernelSpec {
    std::string name;
    OpenClKernel kernel;
} KernelSpec;

class OpenCl {
public:
    OpenCl(
        char *filename,
        std::vector<BufferSpec> bufferArgs,
        std::vector<KernelSpec> kernelArgs,
        bool profile = false,
        bool useGpu = true,
        bool verbose = true
    );
    void prepare(std::vector<BufferSpec> bufferArgs, std::vector<KernelSpec> kernelArgs);
    void setDevice();
    void getPlatformIds();
    void setKernelArg(std::string kernelName, cl_uint arg_index, size_t size, void *pointer);
    void setKernelBufferArg(std::string kernelName, cl_uint argIndex, std::string bufferName);
    void writeBuffer(std::string name, void *pointer);
    void step(std::string name, int count = 1);
    void readBuffer(std::string name, void *pointer);
    void cleanup();
    void flush();
    void printDeviceTypes();
    void getDeviceIds(cl_platform_id platformId);

    void startTimer();
    void getTime();
    void startFrame();

    cl_platform_id *platform_ids;
    cl_platform_id platform_id;
    
    cl_device_id *device_ids;
    cl_device_id device_id;

    cl_context context;
    cl_command_queue command_queue;

    cl_event timer_event;

    cl_program program;
    std::map<std::string, OpenClKernel> kernels;
    std::map<std::string, OpenClBuffer> buffers;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int ret;

    size_t source_size;
    char *source_str;

    char *filename;
    bool use_gpu;
    bool profile;
    bool verbose;

    std::chrono::high_resolution_clock::time_point startingTime;

    float chronoTime = 0, clTime = 0;
    unsigned printCount = 0;
};


#endif
