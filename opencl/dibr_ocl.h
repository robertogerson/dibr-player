#ifndef OCL_COMMON_H
#define OCL_COMMON_H

#include <stdio.h>
#include <string.h>

#define FAILURE -1
#define SUCCESS 0
#define MAX_SOURCE_SIZE (0x100000)
#define MAX_PLATFORMS 2
#define NAME 10000
#define MAX_DEVICES 4

double total_frames = 0.0;
double total_time = 0;

//function to get the time difference
long int timeval_subtract( struct timeval *result,
                           struct timeval *t2,
                           struct timeval *t1 )
{
  (void) result;

  long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) -
                  (t1->tv_usec + 1000000 * t1->tv_sec);

  //result->tv_sec = diff / 1000000;
  //result->tv_usec = diff % 1000000;

  return (diff);
}

//dir containing kernel functions
#define CHECK_OPENCL_ERROR(actual, msg) \
    if(checkVal(actual, CL_SUCCESS, msg)) \
{ \
    std::cout << "Location : " << __FILE__ << ":" << __LINE__<< std::endl; \
    return -1; \
    }

#define CHECK_ALLOCATION(actual, msg) \
    if(actual == NULL) \
{ \
    error(msg); \
    std::cout << "Location : " << __FILE__ << ":" << __LINE__<< std::endl; \
    return FAILURE; \
    }


#if defined (__APPLE__) || defined(MACOSX)
static const char* CL_GL_SHARING_EXT = "cl_APPLE_gl_sharing";
#else
static const char* CL_GL_SHARING_EXT = "cl_khr_gl_sharing";
#endif

#if 1
int FILTER_SIZE = 5;
int FILTER_HALF_SIZE = 2;
float filter[25] = { 0.0396454720, 0.0399106581, 0.0399994471, 0.0399106581, 0.0396454720,
                     0.0399106581, 0.0401776181, 0.0402670009, 0.0401776181, 0.0399106581,
                     0.0399994471, 0.0402670009, 0.0403565827, 0.0402670009, 0.0399994471,
                     0.0399106581, 0.0401776181, 0.0402670009, 0.0401776181, 0.0399106581,
                     0.0396454720, 0.0399106581, 0.0399994471, 0.0399106581, 0.0396454720 };
#else
int FILTER_SIZE = 3;
int FILTER_HALF_SIZE = 1;
float filter[25] = {-1.0, 0.0, +1.0,
                    -2.0, 0.0, +2.0,
                    -1.0, 0.0, -1.0};
#endif

/* 
 * Checks if an extension is suppported
 */
int IsExtensionSupported ( const char *support_str,
                           const char *ext_string,
                           size_t ext_buffer_size )
{
  size_t offset = 0;
  const char* space_substr = strstr (ext_string + offset, " ");
  size_t space_pos = space_substr ? space_substr - ext_string : 0;

  while (space_pos < ext_buffer_size)
  {
    if (strncmp (support_str, ext_string + offset, space_pos) == 0)
    {
      // Device supports requested extension!
      printf( "Info: Found extension support ‘%s’!\n", support_str);
      return 1;
    }
    // Keep searching -- skip to next token string
    offset = space_pos + 1;
    space_substr = strstr (ext_string + offset, " ");
    space_pos = space_substr ? space_substr - ext_string : 0;
  }
  printf ("Warning: Extension not supported ‘%s’!\n", support_str);
  return 0;
}

/**
 * function to display opencl Error based on the error code
 */
template<typename T>
const char* getOpenCLErrorCodeStr(T input)
{
    int errorCode = (int)input;
    switch(errorCode)
    {
    case CL_DEVICE_NOT_FOUND:
        return "CL_DEVICE_NOT_FOUND";
    case CL_DEVICE_NOT_AVAILABLE:
        return "CL_DEVICE_NOT_AVAILABLE";
    case CL_COMPILER_NOT_AVAILABLE:
        return "CL_COMPILER_NOT_AVAILABLE";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:
        return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case CL_OUT_OF_RESOURCES:
        return "CL_OUT_OF_RESOURCES";
    case CL_OUT_OF_HOST_MEMORY:
        return "CL_OUT_OF_HOST_MEMORY";
    case CL_PROFILING_INFO_NOT_AVAILABLE:
        return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case CL_MEM_COPY_OVERLAP:
        return "CL_MEM_COPY_OVERLAP";
    case CL_IMAGE_FORMAT_MISMATCH:
        return "CL_IMAGE_FORMAT_MISMATCH";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:
        return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case CL_BUILD_PROGRAM_FAILURE:
        return "CL_BUILD_PROGRAM_FAILURE";
    case CL_MAP_FAILURE:
        return "CL_MAP_FAILURE";
    case CL_MISALIGNED_SUB_BUFFER_OFFSET:
        return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
        return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case CL_INVALID_VALUE:
        return "CL_INVALID_VALUE";
    case CL_INVALID_DEVICE_TYPE:
        return "CL_INVALID_DEVICE_TYPE";
    case CL_INVALID_PLATFORM:
        return "CL_INVALID_PLATFORM";
    case CL_INVALID_DEVICE:
        return "CL_INVALID_DEVICE";
    case CL_INVALID_CONTEXT:
        return "CL_INVALID_CONTEXT";
    case CL_INVALID_QUEUE_PROPERTIES:
        return "CL_INVALID_QUEUE_PROPERTIES";
    case CL_INVALID_COMMAND_QUEUE:
        return "CL_INVALID_COMMAND_QUEUE";
    case CL_INVALID_HOST_PTR:
        return "CL_INVALID_HOST_PTR";
    case CL_INVALID_MEM_OBJECT:
        return "CL_INVALID_MEM_OBJECT";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
        return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case CL_INVALID_IMAGE_SIZE:
        return "CL_INVALID_IMAGE_SIZE";
    case CL_INVALID_SAMPLER:
        return "CL_INVALID_SAMPLER";
    case CL_INVALID_BINARY:
        return "CL_INVALID_BINARY";
    case CL_INVALID_BUILD_OPTIONS:
        return "CL_INVALID_BUILD_OPTIONS";
    case CL_INVALID_PROGRAM:
        return "CL_INVALID_PROGRAM";
    case CL_INVALID_PROGRAM_EXECUTABLE:
        return "CL_INVALID_PROGRAM_EXECUTABLE";
    case CL_INVALID_KERNEL_NAME:
        return "CL_INVALID_KERNEL_NAME";
    case CL_INVALID_KERNEL_DEFINITION:
        return "CL_INVALID_KERNEL_DEFINITION";
    case CL_INVALID_KERNEL:
        return "CL_INVALID_KERNEL";
    case CL_INVALID_ARG_INDEX:
        return "CL_INVALID_ARG_INDEX";
    case CL_INVALID_ARG_VALUE:
        return "CL_INVALID_ARG_VALUE";
    case CL_INVALID_ARG_SIZE:
        return "CL_INVALID_ARG_SIZE";
    case CL_INVALID_KERNEL_ARGS:
        return "CL_INVALID_KERNEL_ARGS";
    case CL_INVALID_WORK_DIMENSION:
        return "CL_INVALID_WORK_DIMENSION";
    case CL_INVALID_WORK_GROUP_SIZE:
        return "CL_INVALID_WORK_GROUP_SIZE";
    case CL_INVALID_WORK_ITEM_SIZE:
        return "CL_INVALID_WORK_ITEM_SIZE";
    case CL_INVALID_GLOBAL_OFFSET:
        return "CL_INVALID_GLOBAL_OFFSET";
    case CL_INVALID_EVENT_WAIT_LIST:
        return "CL_INVALID_EVENT_WAIT_LIST";
    case CL_INVALID_EVENT:
        return "CL_INVALID_EVENT";
    case CL_INVALID_OPERATION:
        return "CL_INVALID_OPERATION";
    case CL_INVALID_GL_OBJECT:
        return "CL_INVALID_GL_OBJECT";
    case CL_INVALID_BUFFER_SIZE:
        return "CL_INVALID_BUFFER_SIZE";
    case CL_INVALID_MIP_LEVEL:
        return "CL_INVALID_MIP_LEVEL";
    case CL_INVALID_GLOBAL_WORK_SIZE:
        return "CL_INVALID_GLOBAL_WORK_SIZE";
    case CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR:
        return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
    case CL_PLATFORM_NOT_FOUND_KHR:
        return "CL_PLATFORM_NOT_FOUND_KHR";
        //case CL_INVALID_PROPERTY_EXT:
        //    return "CL_INVALID_PROPERTY_EXT";
    case CL_DEVICE_PARTITION_FAILED_EXT:
        return "CL_DEVICE_PARTITION_FAILED_EXT";
    case CL_INVALID_PARTITION_COUNT_EXT:
        return "CL_INVALID_PARTITION_COUNT_EXT";
    default:
        return "unknown error code";
    }

    return "unknown error code";
}

/**
 * OpenCL class containing common initialization functions and kernel calling
 * functions
 */
class OCLX
{
private:
  //  Round up to the nearest multiple of the group size
  size_t RoundUp(int groupSize, int globalSize)
  {
    int r = globalSize % groupSize;
    if(r == 0)
      return globalSize;
    else
      return globalSize + groupSize - r;
  }

public:
  OCLX()
  {
    flag = 0;
  }

  //function to display error message
  void error(const char* errorMsg)
  {
    std::cout<<"Error: "<<errorMsg<<std::endl;
  }

  //function to display error message
  void error(std::string errorMsg)
  {
    std::cout<<"Error: "<<errorMsg<<std::endl;
  }

  //platform information
  cl_platform_id platform;
  cl_uint num_devices;
  string platform_name;

  //device information
  cl_device_id * devices;
  vector <string> device_name;
  vector <string> device_version;
  vector <string> opencl_version;

  //execution evvironment parameters
  cl_context context;
  cl_command_queue queue;
  cl_uint maxComputeUnits;

  //pointer for kernel and programs
  cl_kernel *kernel;
  cl_program *program;

  template<typename T>
  int checkVal(
          T input,
          T reference,
          std::string message,
          bool isAPIerror=true) const
  {
    if(input==reference)
    {
      return 0;
    }
    else
    {
      if(isAPIerror)
      {
        std::cout<<"Error: "<< message << " Error code : ";
        std::cout << getOpenCLErrorCodeStr(input) << std::endl;
      }
      else
      {
        std::cout << message;
      }
      return 1;
    }
  }

  /**
   * @brief randomInit method populates the matrix with random values
   * @param data
   * @param size
   */
  void randomInit(float* data, int size)
  {
    int min=0;
    int max=10;
    for (int i = 0; i < size; ++i)
      data[i] = min + (rand() % (int)(max - min + 1));
  }

  /**
   * @brief checkErr method checks for error and exits the application in case
   *        of error
   * @param clErr
   * @param filename
   * @param line
   */
  void checkErr(cl_int clErr, char* filename, int line)
  {
    if (clErr!=CL_SUCCESS)
    {
      printf("OpenCL Error %i at line %i of%s\n",clErr,line,filename);
      exit(EXIT_FAILURE);
    }
  }

  /**
   * @brief checkErr method checks for error and exits the application in case
   *        of error
   * @param clErr
   * @param filename
   * @param line
   */
  void checkErr(cl_int clErr, const char* filename, int line)
  {
    if (clErr!=CL_SUCCESS)
    {
      printf("OpenCL Error %i at line %i of%s\n",clErr,line,filename);
      exit(EXIT_FAILURE);
    }
  }

  //function to get the platform information
  cl_int getPlatform( cl_platform_id &platform,
                      int platformId,
                      bool platformIdEnabled )
  {
    cl_int clErr;
    cl_uint numPlatforms;
    clErr = clGetPlatformIDs(0, NULL, &numPlatforms);

    if( 0 < numPlatforms )
    {
      cl_platform_id* platforms = new cl_platform_id[numPlatforms];
      // get platform IDs
      clErr = clGetPlatformIDs(numPlatforms,platforms,NULL);
      checkErr(clErr,(char*)__FILE__,__LINE__);

      if(platformIdEnabled)
      {
        platform = platforms[platformId];
      }
      else
      {
        char platformName[100];
        for (unsigned i = 0; i < numPlatforms; ++i)
        {
          size_t size;
          clErr = clGetPlatformInfo( platforms[i],
                                     CL_PLATFORM_VENDOR,
                                     0,
                                     NULL,
                                     &size);

          checkErr(clErr,(char*)__FILE__,__LINE__);

          clErr = clGetPlatformInfo( platforms[i],
                                     CL_PLATFORM_VENDOR,
                                     size,
                                     platformName,
                                     NULL);
          checkErr(clErr,(char*)__FILE__,__LINE__);

          printf("Platform %i: %s\n", i, platformName);

          platform = platforms[i];
          if (!strcmp(platformName, "Advanced Micro Devices, Inc."))
          {
            break;
          }
        }
        std::cout << "Platform found : " << platformName << "\n";
      }
      delete[] platforms;
    }
    else
    {
      std::cout<<"Error: No available platform found!"<<std::endl;
      return FAILURE;
    }

    return SUCCESS;
  }

  //function to initialize the device information
  cl_int getDevice( cl_platform_id platform,
                    cl_uint & count,
                    cl_device_type deviceType,
                    cl_device_id  **devices)
  {
    cl_int status;
    char platformVendor[1024];
    status = clGetPlatformInfo( platform,
                                CL_PLATFORM_VENDOR,
                                sizeof(platformVendor),
                                platformVendor,
                                NULL);
    CHECK_OPENCL_ERROR(status, "clGetPlatformInfo failed");
    std::cout << "\nSelected Platform Vendor: "<< platformVendor << std::endl;


    // Get number of devices available
    cl_uint deviceCount = 0;
    status = clGetDeviceIDs(platform, deviceType, 0, NULL, &deviceCount);
    CHECK_OPENCL_ERROR(status, "clGetDeviceIDs failed");
    if (deviceCount == 0)
    {
      std::cout << "No device available\n" << endl;
      return FAILURE;
    }

    std::cout << "Number of Devices Found " << deviceCount << endl;
    count = deviceCount;
    //deviceIds.resize(count);

    *devices=(cl_device_id*)malloc(count*sizeof(cl_device_id));
    CHECK_ALLOCATION(*devices, "Failed to allocate memory(deviceIds)");

    // Get device ids
    status = clGetDeviceIDs(platform,deviceType,deviceCount,*devices, NULL);
    CHECK_OPENCL_ERROR(status, "clGetDeviceIDs failed");

    // Print device index and device names
    for (cl_uint i = 0; i < deviceCount; ++i)
    {
      char deviceName[1024];
      char device_version[1024];
      char opencl_version[1024];
      cl_int maxComputUnits;

      size_t size;
      status = clGetDeviceInfo( *devices[i], CL_DEVICE_NAME, 0, NULL, &size );
      status = clGetDeviceInfo( *devices[i],
                                CL_DEVICE_NAME,
                                sizeof(deviceName),
                                deviceName,
                                NULL );
      CHECK_OPENCL_ERROR(status, "clGetDeviceInfo CL_DEVICE_NAME failed");

      status = clGetDeviceInfo( *devices[i], CL_DEVICE_VERSION, 0, NULL, &size);
      status= clGetDeviceInfo( *devices[i],
                               CL_DEVICE_VERSION,
                               size,device_version,
                               NULL);
      CHECK_OPENCL_ERROR(status, "clGetDeviceInfo  CL_DEVICE_VERSION failed");

      clGetDeviceInfo( *devices[i], CL_DEVICE_OPENCL_C_VERSION, 0, NULL, &size);
      clGetDeviceInfo( *devices[i],
                       CL_DEVICE_OPENCL_C_VERSION,
                       size,
                       opencl_version,
                       NULL);

      //deviceIds.push_back(devices[i]);
      //devicename.push_back(deviceName);
      //deviceversion.push_back(device_version);;
      clGetDeviceInfo( *devices[i],
                       CL_DEVICE_MAX_COMPUTE_UNITS,
                       sizeof(maxComputUnits),
                       &maxComputUnits,
                       NULL);

      std::cout << "Device -->" << i << " : " << deviceName <<endl;
      std::cout << "Device Version -->" << device_version << std::endl;
      std::cout << "OpenCL version -->" << opencl_version << endl;
      std::cout << "max compute units -->" << maxComputUnits << endl;

      // Get string containing supported device extensions
      size_t ext_size = 1024;
      char* ext_string = (char*)malloc(ext_size);
      status = clGetDeviceInfo ( *devices[i],
                                 CL_DEVICE_EXTENSIONS,
                                 ext_size,
                                 ext_string,
                                 &ext_size);

      // Search for GL support in extension string (space delimited)
      int supported = IsExtensionSupported ( CL_GL_SHARING_EXT,
                                             ext_string,
                                             ext_size );
      if( supported )
      {
        // Device supports context sharing with OpenGL
        std::cout <<  "Found GL Sharing Support!\n" << endl;
      }
    }

    //free(devices);

    return SUCCESS;
  }

  /**
   * @brief show_platform_info function shows platform and device info
   */
  cl_int init()
  {
    cl_int status;
    getPlatform(platform, 1, false);
    getDevice(platform,num_devices,CL_DEVICE_TYPE_ALL,&devices);

    //define desired context properties list
    cl_context_properties properties[] = {CL_CONTEXT_PLATFORM,
                                          (cl_context_properties) platform,0};

    // create context for devices
    context = clCreateContext(properties,num_devices,devices,NULL,NULL,&status);
    CHECK_OPENCL_ERROR(status, "clCreateContext failed");

    // create queue for devices
    queue = clCreateCommandQueue(context,devices[0],0,&status);
    CHECK_OPENCL_ERROR(status, "clCreateCommandQueue failed");

    return SUCCESS;
  }


  /**
   * @brief destroy function releases resourcees
   */
  cl_int destroy()
  {
    cl_int status;

    free(devices);

    status = clFlush(queue);
    CHECK_OPENCL_ERROR(status, "clFlush failed");

    status = clFinish(queue);
    CHECK_OPENCL_ERROR(status, "clFinish failed");

    status = clReleaseCommandQueue(queue);
    CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed");

    status = clReleaseContext(context);
    CHECK_OPENCL_ERROR(status, "clReleaseContext failed");
    
    return status;
  }

  /**
   * @brief create_global_buffer function creates a global write buffer in
   *        device global memory
   */
  cl_mem create_write_buffer(size_t bytes)
  {
    cl_int clErr;
    cl_mem buffer = clCreateBuffer( context,
                                    CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                    bytes,
                                    NULL,
                                    &clErr );
    checkErr(clErr,(char*)__FILE__,__LINE__);
    return buffer;
  }

  /**
   * @brief create_read_buffer method creates a read buffer in device global
   *        memory
   * @param bytes
   * @return
   */
  cl_mem create_read_buffer(size_t bytes)
  {
    cl_mem_flags flags = CL_MEM_READ_ONLY ; //| CL_MEM_USE_HOST_PTR |CL_MEM_USE_PERSISTENT_MEM_AMD;

    cl_int clErr;
    cl_mem buffer = clCreateBuffer(context,flags,bytes,NULL,&clErr);
    checkErr(clErr,(char*)__FILE__,__LINE__);
    // printf("-------%d",CL_SUCCESS);
    return buffer;

  }

  /**
   * @brief creates a global  write memory which is mapped to a host memory and
   *        can be accessed by read/write to host pointer memory
   */
  cl_mem create_write_buffer(size_t bytes, void **mem, void **ptr)
  {
      cl_int status;
      cl_mem buffer = clCreateBuffer( context,
                                      CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
                                      bytes,
                                      *mem,
                                      &status );
      checkErr(status,(char*)__FILE__,__LINE__);
      *ptr = clEnqueueMapBuffer( queue,
                                 buffer,
                                 CL_FALSE,
                                 CL_MAP_READ,
                                 0,
                                 bytes,
                                 0,
                                 NULL,
                                 NULL,
                                 &status );
      checkErr(status,(char*)__FILE__,__LINE__);
      return buffer;
  }

  /**
   * @brief creates a global  read memory which is mapped to a host memory and
   *        can be accessed by read/write to host pointer mem
   */
  cl_mem create_read_buffer( size_t bytes,
                             void *buffer1,
                             uint id,
                             cl_event *event )
  {
    cl_int status;

    cl_mem_flags flags = CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR ;

    cl_mem bufferx = clCreateBuffer(context, flags, bytes, buffer1, &status);
    checkErr(status,(char*)__FILE__,__LINE__);

    void *mem = clEnqueueMapBuffer( queue,
                                    bufferx,
                                    CL_FALSE,
                                    CL_MAP_READ,
                                    0,
                                    bytes,
                                    0,
                                    NULL,
                                    NULL,
                                    &status );
    checkErr(status,(char*)__FILE__,__LINE__);
    memcpy(mem,buffer1,bytes);
    status = clEnqueueUnmapMemObject(queue, bufferx, mem, 0, NULL,&event[id]);
    checkErr(status,(char*)__FILE__,__LINE__);
    //CHECK_OPENCL_ERROR(status,"");
    //printf("-------%d",CL_SUCCESS);
    return bufferx;

  }

  /**
   * @brief creates a global  read write memory which is mapped to a host memory
   *        and can be accessed by read/write to host pointer buffer1
   */
  cl_mem create_rw_buffer(size_t bytes, void *buffer1, uint id, cl_event *event)
  {
    (void) id;
    (void) event;
    cl_int status;

    cl_mem_flags flags = CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR ;

    cl_mem bufferx = clCreateBuffer(context, flags, bytes, buffer1, &status);
    checkErr(status,(char*)__FILE__,__LINE__);

    return bufferx;
  }

  /**
   * @brief creates a global read write 2d image
   */
  cl_mem create_rw_image( size_t bytes,
                          void *buffer,
                          cl_image_format clImageFormat,
                          int width,
                          int height)
  {
    (void) bytes;

    cl_int status;
    cl_mem clImage;

    cl_mem_flags flags = CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR ;
    clImage = clCreateImage2D( context,
                               flags,
                               &clImageFormat,
                               width,
                               height,
                               0,
                               buffer,
                               &status );
    checkErr(status,(char*)__FILE__,__LINE__);

    return clImage;
  }

  /**
   * @brief function to write data to global buffer from host->device
   */
  cl_int write_buffer( cl_mem dbuffer,
                       size_t bytes,
                       void *hbuffer,
                       cl_event *event,
                       int id )
  {
    cl_int clErr;
    if(id==0)
    {
      clErr = clEnqueueWriteBuffer( queue,
                                    dbuffer,
                                    CL_FALSE,
                                    0,
                                    bytes,
                                    (void*)hbuffer,
                                    0,
                                    NULL,
                                    &event[id] );
    }
    else
    {
      //clErr = clEnqueueWriteBuffer(queue,dbuffer,CL_FALSE,0,bytes,(void*)hbuffer,NULL,NULL,&event[id]);
      clErr = clEnqueueWriteBuffer( queue,
                                    dbuffer,
                                    CL_FALSE,
                                    0,
                                    bytes,
                                    (void*)hbuffer,
                                    0,
                                    NULL,
                                    &event[id] );
    }

    checkErr(clErr,__FILE__,__LINE__);

    return clErr;
  }

  /**
   * @brief read_buffer reads data to host from the device global memory
   * @param dbuffer
   * @param bsize
   * @param hbuffer
   */
  cl_int read_buffer( cl_mem dbuffer,
                      size_t bsize,
                      void *hbuffer,
                      cl_event *event,
                      int id )
  {
    cl_int clErr;
    if(id==0)
    {
      clErr = clEnqueueReadBuffer( queue,
                                   dbuffer,
                                   CL_FALSE,
                                   0,
                                   bsize,
                                   (void*)hbuffer,
                                   0,
                                   NULL,
                                   NULL );
    }
    else
    {
      //clErr = clEnqueueReadBuffer(queue,dbuffer,CL_FALSE,0,bsize,(void*)hbuffer,NULL,NULL,&event[id]);
      clErr = clEnqueueReadBuffer(queue,
                                  dbuffer,
                                  CL_FALSE,
                                  0,
                                  bsize,
                                  (void*)hbuffer,
                                  0,
                                  NULL,
                                  &event[id] );
    }
    return clErr;
  }

  /**
   * @brief function to read the source file containing kernel code and
   *        performing compilation of kernel code
   * @param path
   * @param name
   * @return
   */
  cl_int read_program(char * path,cl_program *program)
  {
    //cl_kernel kernel;
    FILE *fp;
    char *source = (char*)malloc(MAX_SOURCE_SIZE);

    cl_int status;
    fp = fopen(path, "r");
    if (!fp) {
      fprintf(stderr, "Failed to load kernel %s\n",path);
      exit(1);
    }
    cerr << "Reading Kernel File " << path << endl;

    //read the kernel source from the file
    size_t size = fread( source, 1, MAX_SOURCE_SIZE, fp);
    fflush(fp);
    fclose( fp );
    if(size <=0)
      return FAILURE;

    // Create a program from the kernel source
    *program = clCreateProgramWithSource( context,
                                          1,
                                          (const char **)&source,
                                          (const size_t *)&size,
                                          &status );
    CHECK_OPENCL_ERROR(status, "clCreateProgramWithSource");

    //JIT compilation of kernel
    status = clBuildProgram(*program, 1, &devices[0], NULL, NULL, NULL);
    if (status == CL_BUILD_PROGRAM_FAILURE)
    {
      // Determine the size of the log
      size_t log_size;
      clGetProgramBuildInfo( *program,
                             devices[0],
                             CL_PROGRAM_BUILD_LOG,
                             0,
                             NULL,
                             &log_size );

      // Allocate memory for the log
      char *log = (char *) malloc(log_size);
      // Get the log
      clGetProgramBuildInfo( *program,
                             devices[0],
                             CL_PROGRAM_BUILD_LOG,
                             log_size,
                             log,
                             NULL);

      // Print the log
      printf("%s\n", log);
    }
    CHECK_OPENCL_ERROR(status, "");

    //get build infor log
    size_t lsize;
    status = clGetProgramBuildInfo( *program,
                                    devices[0],
                                    CL_PROGRAM_BUILD_LOG,
                                    0,
                                    NULL,
                                    &lsize );
    CHECK_OPENCL_ERROR(status, "");

    char build_log[lsize];
    status = clGetProgramBuildInfo( *program,
                                    devices[0],
                                    CL_PROGRAM_BUILD_LOG,
                                    lsize,
                                    build_log,
                                    NULL);
    CHECK_OPENCL_ERROR(status,"");
    printf("\nBuild Log:\n\n%s\n\n", build_log);

    return SUCCESS;
  }

  /**
   * @brief function to create kernel.
   */
  cl_int read_kernel(cl_program program,cl_kernel *kernel,char *name)
  {
    // Create the OpenCL kernel
    cl_int status;
    *kernel = clCreateKernel(program,name, &status);
    CHECK_OPENCL_ERROR(status,"");

    return SUCCESS;
  }

  /**
   * @brief fuction to release kernel and program data structures
   */
  cl_int release(cl_kernel k,cl_program p)
  {
      cl_int  status;

      status=clReleaseKernel(k);
      CHECK_OPENCL_ERROR(status, "");

      status=clReleaseProgram(p);
      CHECK_OPENCL_ERROR(status, "");
  }

  /**
   * @brief function to the kernel and program for the demo application.
   */
  cl_int load_demo(cl_program *program, cl_kernel *kernel)
  {
    string source="./convolution.cl";
    string name[10];
    name[0] = "convolute";
    name[1] = "dibr";
    name[2] = "hole_filling";

    read_program((char *)source.c_str(), program);
    read_kernel(*program, &kernel[0], (char *)name[0].c_str());
    read_kernel(*program, &kernel[1], (char *)name[1].c_str());
    read_kernel(*program, &kernel[2], (char *)name[2].c_str());

    return SUCCESS;
  }

  //function to call the launch kernel on the device and set the arguments for kernel execution
  //commmon host memory pointer
  void *memory1;
  //common global memory pointer
  cl_mem  dimageIn, dimageOut,
  dimageDepth, dimageDepthOut,
  dimagePixelMutex,
  dimageDepthFiltered,
  dimageFilter,
  dShiftLookup,
  dimageMask;

  int flag; //initialization flag for code to be run during first execution

  cl_int conv(Mat src, Mat out, cl_kernel *ke, cl_program program)
  {
    (void) program;

    cl_kernel kernel = ke[0];
    cl_int status;
    cl_event event[5];

    size_t bytes = src.rows * src.step * sizeof(unsigned char);
    size_t filter_bytes = FILTER_SIZE * FILTER_SIZE * sizeof(float);

    if(flag == 0)
    {
      dimageIn = create_rw_buffer(bytes, src.data, 0, NULL);
      dimageOut = create_rw_buffer(bytes, out.data, 0, NULL);

      dimageFilter = create_rw_buffer(filter_bytes, filter, 0, NULL);
      status = write_buffer(dimageFilter, filter_bytes, filter, event, 5);
      CHECK_OPENCL_ERROR(status, "");

      flag = 1;
    }
    else
    {
      status = write_buffer(dimageIn, bytes, src.data, event, 5);
      CHECK_OPENCL_ERROR(status, "");
    }

    size_t local[3] = { 16, 9, 1 };
    size_t global[3] =  { RoundUp(local[0], src.cols),
                          RoundUp(local[1], src.rows),
                          1 };
    int s = 0, channels = 0;
    int arg = -1;

    //setting the kernel arguments
    status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimageIn);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimageOut);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &src.rows);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &src.cols);
    CHECK_OPENCL_ERROR(status, "");

    s = src.step;
    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &s);
    CHECK_OPENCL_ERROR(status, "");

    channels = (int)src.channels();
    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &channels);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &FILTER_HALF_SIZE);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimageFilter);
    CHECK_OPENCL_ERROR(status, "");

    status = clEnqueueNDRangeKernel( queue,
                                     kernel,
                                     2,
                                     NULL,
                                     global,
                                     local,
                                     0,
                                     NULL,
                                     &event[0] );
    CHECK_OPENCL_ERROR(status, "");

    //waiting for the kernel to complete
    status = clWaitForEvents(1, &event[0]);
    CHECK_OPENCL_ERROR(status, "");
    clFinish(queue);

    status = read_buffer(dimageOut, bytes, out.data, event, 1);
    CHECK_OPENCL_ERROR(status, "");

    clFinish(queue);

    return SUCCESS;
  }

#define PER_LINE 1
  cl_int dibr( cl_kernel *ke, cl_program program,
               Mat &src,
               Mat &depth,
               Mat &filter_out,
               Mat &out,
               Mat &depth_out,
               Mat &mask,
               int *shift_table_lookup,
               int *pixelMutex,
               bool with_lock = true,
               bool with_hole_filling = true)
  {
    (void) program;
    cl_kernel kernel = ke[0];
    cl_int status;
    cl_event event[8];

    // input bytes
    size_t bytes = src.rows * src.step * sizeof(unsigned char);
    size_t depth_bytes = depth.rows * depth.step * sizeof(unsigned char);
    size_t filter_bytes = FILTER_SIZE * FILTER_SIZE * sizeof(float);
    size_t shift_lookup_table_bytes = 256 * sizeof (int);

    // Output bytes
    size_t out_bytes = out.rows * out.step * sizeof(unsigned char);
    size_t mask_bytes = mask.rows * mask.step * sizeof (unsigned char);
    size_t pixel_mutex_bytes = out.rows * out.cols * sizeof (int);


    struct timeval end, result, now;
    long int diff;
    gettimeofday(&now, NULL);

    // Begin filter
    if(flag == 0)
    {
      dimageDepth = create_rw_buffer(depth_bytes, depth.data, 0, NULL);
      dimageDepthFiltered = create_rw_buffer(depth_bytes, filter_out.data, 0, NULL);

      dimageFilter = create_rw_buffer(filter_bytes, filter, 0, NULL);
      status = write_buffer(dimageFilter, filter_bytes, filter, event, 5); // I need to write this just one time
      CHECK_OPENCL_ERROR(status, "");

      dimageIn = create_rw_buffer(bytes, src.data, 0, NULL);
      dimageOut = create_rw_buffer(out_bytes, out.data, 0, NULL);
      dimageDepthOut = create_rw_buffer(out_bytes, depth_out.data, 0, NULL);

      if(with_lock)
      {
        dimagePixelMutex = create_rw_buffer( pixel_mutex_bytes,
                                             pixelMutex,
                                             0,
                                             NULL );

      }

      dShiftLookup = create_rw_buffer( shift_lookup_table_bytes,
                                       shift_table_lookup,
                                       0,
                                       NULL );

      dimageMask = create_rw_buffer(mask_bytes, mask.data, 0, NULL);
      flag = 1;
    }

    //Write buffers!!
    status = write_buffer( dShiftLookup,
                           shift_lookup_table_bytes,
                           shift_table_lookup,
                           event,
                           1 );
    CHECK_OPENCL_ERROR(status, "");

    status = write_buffer(dimageDepth, depth_bytes, depth.data, event, 2);
    CHECK_OPENCL_ERROR(status, "");

    status = write_buffer(dimageIn, bytes, src.data, event, 3);
    CHECK_OPENCL_ERROR(status, "");

    // The following buffer should be cleared! Probably we can do better using
    // the GPU to do so.
    status = write_buffer(dimageOut, out_bytes, out.data, event, 4);
    CHECK_OPENCL_ERROR(status, "");

    status = write_buffer(dimageDepthOut, out_bytes, depth_out.data, event, 5);
    CHECK_OPENCL_ERROR(status, "");

    if(with_lock)
    {
      status = write_buffer( dimagePixelMutex,
                             pixel_mutex_bytes,
                             pixelMutex,
                             event,
                             6);
      CHECK_OPENCL_ERROR(status, "");
    }

    status = write_buffer(dimageMask, mask_bytes, mask.data, event, 7);
    CHECK_OPENCL_ERROR(status, "");

    clFinish(queue);

    // Ok! Let's start to compute now!
#if PER_LINE
    size_t local[3] = { 9, 1, 1 };
    size_t global[3] = { (size_t) src.rows,
                         1,
                         1 };

#else
    size_t local[3] = { 16, 9, 1 };
    size_t global[3] = { (size_t) src.cols,
                         (size_t) src.rows,
                         1 };
#endif
    int s = 0, channels = 0;
    int arg = -1;

    //setting the kernel arguments
    /*
    status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimageDepth);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimageDepthFiltered);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &src.cols);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &src.rows);
    CHECK_OPENCL_ERROR(status, "");

    s = src.step;
    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &s);
    CHECK_OPENCL_ERROR(status, "");

    channels = (int)src.channels();
    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &channels);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &FILTER_HALF_SIZE);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimageFilter);
    CHECK_OPENCL_ERROR(status, "");

    // Enqueue filter
    // status = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, local, 0, NULL, &event[0]);
    // CHECK_OPENCL_ERROR(status, "");
    // End filter
    */

    // start dibr
    kernel = ke[1];
    int S = 20;

#if PER_LINE
    local[0] = 9;
    global[0] = (size_t) src.rows;
#else
    local[0] = 16;
    local[1] = 9;
    global[0] = (size_t) src.cols;
    global[1] = (size_t) src.rows;
#endif

    arg = -1;
    //setting the kernel arguments
    status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimageIn);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimageDepth);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimageOut);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimageDepthOut);
    CHECK_OPENCL_ERROR(status, "");

    if(with_lock)
    {
      status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimagePixelMutex);
      CHECK_OPENCL_ERROR(status, "");
    }

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimageMask);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &src.rows);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &src.cols);
    CHECK_OPENCL_ERROR(status, "");

    s = src.step;
    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &s);
    CHECK_OPENCL_ERROR(status, "");

    s = out.step;
    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &s);
    CHECK_OPENCL_ERROR(status, "");

    channels = (int)src.channels();
    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &channels);
    CHECK_OPENCL_ERROR(status, "");

    s = mask.step;
    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &s);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dShiftLookup);
    CHECK_OPENCL_ERROR(status, "");

    status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &S);
        CHECK_OPENCL_ERROR(status, "");

#if PER_LINE
    status = clEnqueueNDRangeKernel( queue,
                                     kernel,
                                     1,
                                     NULL,
                                     global,
                                     local,
                                     0,
                                     NULL,
                                     &event[0] );
    CHECK_OPENCL_ERROR(status, "");
#else
    status = clEnqueueNDRangeKernel( queue,
                                     kernel,
                                     2,
                                     NULL,
                                     global,
                                     local,
                                     0,
                                     NULL,
                                     &event[0] );
    CHECK_OPENCL_ERROR(status, "");
#endif
    // End dibr

    if(with_hole_filling)
    {
      // Start hole-filling
      kernel = ke[2];
      arg = -1;

      local[0] = 16;
      local[1] = 9;
      global[0] = (size_t) src.cols * 2;
      global[1] = (size_t) src.rows;

      //setting the kernel arguments
      status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimageIn);
      CHECK_OPENCL_ERROR(status, "");

      status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimageOut);
      CHECK_OPENCL_ERROR(status, "");

      status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimageDepthOut);
      CHECK_OPENCL_ERROR(status, "");

      status = clSetKernelArg(kernel, ++arg, sizeof(cl_mem), &dimageMask);
      CHECK_OPENCL_ERROR(status, "");

      status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &src.rows);
      CHECK_OPENCL_ERROR(status, "");

      status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &src.cols);
      CHECK_OPENCL_ERROR(status, "");

      s = src.step;
      status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &s);
      CHECK_OPENCL_ERROR(status, "");

      s = out.step;
      status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &s);
      CHECK_OPENCL_ERROR(status, "");

      channels = (int)out.channels();
      status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &channels);
      CHECK_OPENCL_ERROR(status, "");

      s = mask.step;
      status = clSetKernelArg(kernel, ++arg, sizeof(cl_int), &s);
      CHECK_OPENCL_ERROR(status, "");

      int INTERPOLATION_HALF_SIZE_WINDOW = 15;
      status = clSetKernelArg( kernel,
                               ++arg,
                               sizeof(cl_int),
                               &INTERPOLATION_HALF_SIZE_WINDOW );
      CHECK_OPENCL_ERROR(status, "");

//#if PER_LINE
      //status = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global, local, 0, NULL, &event[0]);
      //CHECK_OPENCL_ERROR(status, "");
//#else
      status = clEnqueueNDRangeKernel( queue,
                                       kernel,
                                       2,
                                       NULL,
                                       global,
                                       local,
                                       0,
                                       NULL,
                                       &event[1] );
      CHECK_OPENCL_ERROR(status, "");
//#endif
          // End hole-filling
    }

    clFinish(queue); // We need to think in change that for a callback
                     // (read Heterogeneous Computing with OpenCL page 173)

    gettimeofday(&end, NULL);
    diff = timeval_subtract(&result, &end, &now);

    total_frames += 1.0;
    total_time += (double)(diff) / 1000.0;
    cout << (float)diff << "\n";

    status = read_buffer(dimageOut, out_bytes, out.data, event, 1);
    CHECK_OPENCL_ERROR(status, "");
    status = clWaitForEvents (1, &event[2]);
    clFinish(queue); // We need to think in change that for a callback
                     // (read Heterogeneous Computing with OpenCL page 173)

    return SUCCESS;
  }
};


#endif // OCL_COMMON_H
