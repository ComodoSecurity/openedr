# Upgrading Notes
For 0.12+ all applications must call the Aws::InitAPI() function before making any other SDK calls, and the Aws::ShutdownAPI function when finished using the SDK. More information can be found here:
https://aws.amazon.com/blogs/developer/aws-sdk-for-c-simplified-configuration-and-initialization/

# AWS SDK for C++
The AWS SDK for C++ provides a modern C++ (version C++ 11 or later) interface for Amazon Web Services (AWS). It is meant to be performant and fully functioning with low- and high-level SDKs, while minimizing dependencies and providing platform portability (Windows, OSX, Linux, and mobile).  

AWS SDK for C++ is in now in General Availability and recommended for production use. We invite our customers to join
the development efforts by submitting pull requests and sending us feedback and ideas via GitHub Issues.

## Getting Help
Please use these community resources for getting help. We use the GitHub issues for tracking bugs and feature requests.
* Ask a question on StackOverflow and tag it with the `aws-sdk-cpp` tag. 
* If it turns out that you may have found a bug, please open an issue
* Open a support ticket with [AWS Support]( https://console.aws.amazon.com/support/home#/)
 
 
## Opening Issues
If you encounter a bug with the AWS SDK for C++ we would like to hear about it. Search the [existing issues]( https://github.com/aws/aws-sdk-cpp/issues) and see if others are also experiencing the issue before opening a new issue. Please include the version of AWS SDK for CPP, Compiler, Compiler Version, CMake version, and OS you’re using. Please also include repro case when appropriate.
 
The GitHub issues are intended for bug reports and feature requests. For help and questions with using AWS SDK for C++, please make use of the resources listed in the [Getting Help]( https://github.com/aws/aws-sdk-cpp#getting-help) section. Keeping the list of open issues lean we can respond in a timely manner.


### Introducing the AWS SDK for C++ from AWS re:invent 2015
The following video explains many of the core features and also high-level SDKs

[![Introducing the AWS SDK for C++](https://img.youtube.com/vi/fm4Aa3Whwos/0.jpg)](https://www.youtube.com/watch?v=fm4Aa3Whwos&list=PLhr1KZpdzuke5pqzTvI2ZxwP8-NwLACuU&index=9 "Introducing the AWS SDK for C++")

### Building the SDK:
Use the information below to build the entire source tree for your platform, run unit tests, and build integration tests.  

#### Minimum Requirements:
* Visual Studio 2015 or later
* OR GNU Compiler Collection (GCC) 4.9 or later
* OR Clang 3.3 or later
* 4GB of RAM
  * 4GB of RAM is required to build some of the larger clients. The SDK build may fail on EC2 instance types t2.micro, t2.small and other small instance types due to insufficient memory.

#### Creating an Out-of-Source Build (Recommended):
##### To create an **out-of-source build**:
1. Install CMake and the relevant build tools for your platform. Ensure these are available in your executable path.
2. Create your build directory. Replace BUILD_DIR with your build directory name:

```
cd BUILD_DIR
cmake <path-to-root-of-this-source-code>
```

You can use the following variations to create your build directory:
* For Auto Make build systems:
`make`
* For Visual Studio:
`msbuild ALL_BUILD.vcxproj`

##### To create a **release build**, do one of the following:
* For Auto Make build systems:
```
cmake -DCMAKE_BUILD_TYPE=Release  <path-to-root-of-this-source-code>
make
sudo make install
```
##### To uninstall these libraries:
```
sudo make uninstall
```
You may define a custom uninstall target when you are using SDK as a sub-project, but make sure it comes before the default definition in `CMakeLists.txt`, and you can uninstall SDK related libraries by:
```
sudo make uninstall-awssdk
```

* For Visual Studio:
```
cmake <path-to-root-of-this-source-code> -G "Visual Studio 12 Win64" -DCMAKE_BUILD_TYPE=Release
msbuild INSTALL.vcxproj /p:Configuration=Release
```

##### To build and install third party dependencies:
Starting from version 1.7.0, we added several third party dependencies, including [`aws-c-common`](https://github.com/awslabs/aws-c-common), [`aws-checksums`](https://github.com/awslabs/aws-checksums) and [`aws-c-event-stream`](https://github.com/awslabs/aws-c-event-stream). By default, they will be built and installed in `<BUILD_DIR>/.deps/install`, and copied to default system directory during SDK installation. You can change the location by specifying `CMAKE_INSTALL_PREFIX`.

However, if you want to build and install these libraries separately:
1. Download, build and install `aws-c-commom`:
    ```sh
    git clone https://github.com/awslabs/aws-c-common
    cd aws-c-common
    # checkout to a specific commit id if you want.
    git checkout <commit-id>
    mkdir build && cd build
    # without CMAKE_INSTALL_PREFIX, it will be installed to default system directory.
    cmake .. -DCMAKE_INSTALL_PREFIX=<deps-install-dir> <extra-cmake-parameters-here>
    make # or MSBuild ALL_BUILD.vcxproj on Windows
    make install # or MSBuild INSTALL.vcxproj on Windows
    ```
2. Download, build and install `aws-checksums`:
    ```sh
    git clone https://github.com/awslabs/aws-checksums
    cd aws-checksums
    # checkout to a specific commit id if you want
    git checkout <commit-id>
    mkdir build && cd build
    # without CMAKE_INSTALL_PREFIX, it will be installed to default system directory.
    cmake .. -DCMAKE_INSTALL_PREFIX=<deps-install-dir> <extra-cmake-parameters-here>
    make # or MSBuild ALL_BUILD.vcxproj on Windows
    make install # or MSBuild INSTALL.vcxproj on Windows
    ```
3. Download, build and install `aws-c-event-stream`:
    ```sh
    git clone https://github.com/awslabs/aws-c-event-stream
    cd aws-c-event-stream
    # checkout to a specific commit id if you want
    git checkout <commit-id>
    mkdir build && cd build
    # aws-c-common and aws-checksums are dependencies of aws-c-event-stream
    # without CMAKE_INSTALL_PREFIX, it will be installed to default system directory.
    cmake .. -DCMAKE_INSTALL_PREFIX=<deps-install-dir> -DCMAKE_PREFIX_PATH=<deps-install-dir> <extra-cmake-parameters-here>
    make # or MSBuild ALL_BUILD.vcxproj on Windows
    make install # or MSBuild INSTALL.vcxproj on Windows
    ```
4. Turn off `BUILD_DEPS` when building C++ SDK:
    ```sh
    cd BUILD_DIR
    cmake <path-to-root-of-this-source-code> -DBUILD_DEPS=OFF -DCMAKE_PREFIX_PATH=<deps-install-dir>
    ```
You may also find the following link helpful for including the build in your project:

https://aws.amazon.com/blogs/developer/using-cmake-exports-with-the-aws-sdk-for-c/

#### Building for Android
To build for Android, add `-DTARGET_ARCH=ANDROID` to your cmake command line.  We've included a cmake toolchain file that should cover what's needed, assuming you have the appropriate environment variables (ANDROID_NDK) set.
Currently the latest version of NDK we support is 12b.

##### Android on Windows
Building for Android on Windows requires some additional setup.  In particular, you will need to run cmake from a Visual Studio developer command prompt (2013 or higher).  Additionally, you will need 'git' and 'patch' in your path.  If you have git installed on a Windows system, then patch is likely found in a sibling directory (.../Git/usr/bin/).  Once you've verified these requirements, your cmake command line will change slightly to use nmake:

```
cmake -G "NMake Makefiles" `-DTARGET_ARCH=ANDROID` <other options> ..
```

Nmake builds targets in a serial fashion.  To make things quicker, we recommend installing JOM as an alternative to nmake and then changing the cmake invocation to:

```
cmake -G "NMake Makefiles JOM" `-DTARGET_ARCH=ANDROID` <other options> ..
```

#### General CMake Variables

##### BUILD_ONLY
Allows you to only build the clients you want to use. This will resolve low level client dependencies if you set this to a high-level sdk such as aws-cpp-sdk-transfer. This will also build integration and unit tests related to the projects you select if they exist. aws-cpp-sdk-core always builds regardless of the value of this argument. This is a list argument.
Example:
```
-DBUILD_ONLY="s3;dynamodb;cognito-identity"
```

##### ADD_CUSTOM_CLIENTS
Allows you to build any arbitrary clients based on the api definition. Simply place your definition in the code-generation/api-definitions folder. Then pass this arg to cmake. The cmake configure step will generate your client and include it as a subdirectory in your build. This is particularly useful if you want to generate a C++ client for using one of your API Gateway services. To use this feature you need to have python 2.7, java, jdk1.8, and maven installed and in your executable path. Example: -DADD_CUSTOM_CLIENTS="serviceName=myCustomService, version=2015-12-21;serviceName=someOtherService, version=2015-08-15"

##### REGENERATE_CLIENTS
This argument will wipe out all generated code and generate the client directories from the code-generation/api-definitions folder. To use this argument, you need to have python 2.7, java, jdk1.8, and maven installed in your executable path. Example: -DREGENERATE_CLIENTS=1

##### CUSTOM_MEMORY_MANAGEMENT  
To use a custom memory manager, set the value to 1. You can install a custom allocator, and all STL types will use the custom allocation interface. If the value is set to 0, you still might want to use the STL template types to help with DLL safety on Windows.

If static linking is enabled, custom memory management defaults to off. If dynamic linking is enabled, custom memory management defaults to on and avoids cross-DLL allocation and deallocation.

Note: To prevent linker mismatch errors, you must use the same value (0 or 1) throughout your build system.

##### TARGET_ARCH
To cross compile or build for a mobile platform, you must specify the target platform. By default the build detects the host operating system and builds for that operating system.
Options: WINDOWS | LINUX | APPLE | ANDROID

##### G
Use this variable to generate build artifacts, such as Visual Studio solutions and Xcode projects.

Windows example:
-G "Visual Studio 12 Win64"

For more information, see the CMake documentation for your platform.

#### General CMake Options
CMake options are variables that can either be ON or OFF, with a controllable default.  You can set an option either with CMake Gui tools or the command line via -D.

##### ENABLE_UNITY_BUILD
(Defaults to OFF) If enabled, most SDK libraries will be built as a single, generated .cpp file.  This can significantly reduce static library size as well as speed up compilation time.

##### MINIMIZE_SIZE
(Defaults to OFF) A superset of ENABLE_UNITY_BUILD, if enabled this option turns on ENABLE_UNITY_BUILD as well as some additional binary size reduction settings.  This is a work-in-progress and may change in the future (symbol stripping in particular).

##### BUILD_SHARED_LIBS
(Defaults to ON) A built-in CMake option, reexposed here for visibility.  If enabled, shared libraries will be built, otherwise static libraries will be built.

##### FORCE_SHARED_CRT
(Defaults to ON) If enabled, the SDK will link to the C runtime dynamically, otherwise it will use the BUILD_SHARED_LIBS setting (weird but necessary for backwards compatibility with older versions of the SDK)

##### SIMPLE_INSTALL
(Defaults to ON) If enabled, the install process will not insert platform-specific intermediate directories underneath bin/ and lib/.  Turn OFF if you need to make multi-platform releases under a single install directory.

##### NO_HTTP_CLIENT
(Defaults to OFF) If enabled, prevents the default platform-specific http client from being built into the library.  Turn this ON if you wish to inject your own http client implementation.

##### NO_ENCRYPTION
(Defaults to OFF) If enabled, prevents the default platform-specific cryptography implementation from being built into the library.  Turn this ON if you wish to inject your own cryptography implementation.

##### ENABLE_RTTI
(Defaults to ON) Controls whether or not the SDK is built with RTTI information

##### CPP_STANDARD
(Defaults to 11) Allows you to specify a custom c++ standard for use with C++ 14 and 17 code-bases

##### ENABLE_TESTING
(Defaults to ON) Controls whether or not the unit and integration test projects are built

#### Android CMake Variables/Options

##### NDK_DIR
An override path for where the build system should find the Android NDK.  By default, the build system will check environment variables (ANDROID_NDK) if this CMake variable is not set.

##### DISABLE_ANDROID_STANDALONE_BUILD
(Defaults to OFF) By default, Android builds will use a standalone clang-based toolchain constructed via NDK scripts.  If you wish to use your own toolchain, turn this option ON.

##### ANDROID_STL
(Defaults to libc++\_shared)  Controls what flavor of the C++ standard library the SDK will use.  Valid values are one of {libc++\_shared, libc++\_static, gnustl_shared, gnustl_static}.  There are severe performance problems within the SDK if gnustl is used, so we recommend libc++.

##### ANDROID_ABI
(Defaults to armeabi-v7a) Controls what abi to output code for.  Not all valid Android ABI values are currently supported, but we intend to provide full coverage in the future.  We welcome patches to our Openssl build wrapper that speed this process up.  Valid values are one of {arm64, armeabi-v7a, x86_64, x86, mips64, mips}.

##### ANDROID_TOOLCHAIN_NAME
(Defaults to standalone-clang) Controls which compiler is used to build the SDK.  With GCC being deprecated by Android NDK, we recommend using the default (clang).

##### ANDROID_NATIVE_API_LEVEL
(Default varies by STL choice) Controls what API level the SDK will be built against.  If you use gnustl, you have complete freedom with the choice of API level.  If you use libc++, you must use an API level of at least 21.


### Running integration tests:
Several directories are appended with \*integration-tests. After building your project, you can run these executables to ensure everything works properly.

#### Dependencies:
To compile in Linux, you must have the header files for libcurl, libopenssl. The packages are typically available in your package manager.

Debian example:
   `sudo apt-get install libcurl-dev`

### Using the SDK
After they are constructed, individual service clients are very similar to other SDKs, such as Java and .NET. This section explains how core works, how to use each feature, and how to construct an individual client.

The aws-cpp-sdk-core is the heart of the system and does the heavy lifting. You can write a client to connect to any AWS service using just the core, and the individual service clients are available to help make the process a little easier.

#### Build Defines
If you dynamically link to the SDK you will need to define the USE_IMPORT_EXPORT symbol for all build targets using the SDK.
If you wish to install your own memory manager to handle allocations made by the SDK, you will need to pass the CUSTOM_MEMORY_MANAGEMENT cmake parameter (-DCUSTOM_MEMORY_MANAGEMENT) as well as define AWS_CUSTOM_MEMORY_MANAGEMENT in all build targets dependent on the SDK.

Note, if you use our export file, this will be handled automatically for you. We recommend you use our export file to handle this for you:
https://aws.amazon.com/blogs/developer/using-cmake-exports-with-the-aws-sdk-for-c/

#### Initialization and Shutdown
We avoid global and static state where ever possible. However, on some platforms, dependencies need to be globally initialized. Also, we have a few global options such as
logging, memory management, http factories, and crypto factories. As a result, before using the SDK you MUST call our global initialization function. When you are finished using the SDK you should call our cleanup function.

All code using the AWS SDK and C++ should have at least the following:

```
#include <aws/core/Aws.h>
...

    Aws::SDKOptions options;
    Aws::InitAPI(options);

    //use the sdk

    Aws::ShutdownAPI(options);
```

Due to the way memory managers work, many of the configuration options take closures instead of pointers directly in order to ensure that the memory manager
is installed prior to any memory allocations occuring.

Here are a few recipes:

Just use defaults:
```
   Aws::SDKOptions options;
   Aws::InitAPI(options);
   .....
   Aws::ShutdownAPI(options);
```

Turn logging on using the default logger:
```
   Aws::SDKOptions options;
   options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
   Aws::InitAPI(options);
   .....
   Aws::ShutdownAPI(options);
```

Install custom memory manager:
```
    MyMemoryManager memoryManager;

    Aws::SDKOptions options;
    options.memoryManagementOptions.memoryManager = &memoryManager;
    Aws::InitAPI(options);
    .....
    Aws::ShutdownAPI(options);
```

Override default http client factory:
```
    Aws::SDKOptions options;
    options.httpOptions.httpClientFactory_create_fn = [](){ return Aws::MakeShared<MyCustomHttpClientFactory>("ALLOC_TAG", arg1); };
    Aws::InitAPI(options);
    .....
    Aws::ShutdownAPI(options);
```

#### Memory Management
The AWS SDK for C++ provides a way to control memory allocation and deallocation in a library.

Custom memory management is available only if you use a version of the library built using the compile-time constant AWS_CUSTOM_MEMORY_MANAGEMENT defined.

If you use a version of the library built without the compile-time constant, the global memory system functions such as InitializeAWSMemorySystem will not work and the global new and delete functions will be used instead.

For more information about the compile-time constant, see the STL and AWS Strings and Vectors section in this Readme.

To allocate or deallocate memory:
1. Implement the MemorySystemInterface subclass:
   aws/core/utils/memory/MemorySystemInterface.h

In the following example, the type signature for AllocateMemory can be changed as needed:

```
class MyMemoryManager : public Aws::Utils::Memory::MemorySystemInterface
{
  public:

    // ...

    virtual void* AllocateMemory(std::size_t blockSize, std::size_t alignment, const char *allocationTag = nullptr) override;
    virtual void FreeMemory(void* memoryPtr) override;

};
```

In Main:

```
int main(void)
{
  MyMemoryManager sdkMemoryManager;
  SDKOptions options;
  options.memoryManagementOptions.memoryManager = &sdkMemoryManager;
  Aws::InitAPI(options);

  // ... do stuff

  Aws::ShutdownAPI(options);

  return 0;
}
```

#### STL and AWS Strings and Vectors
When initialized with a memory manager, the AWS SDK for C++ defers all allocation and deallocation to the memory manager. If a memory manager does not exist, the SDK uses global new and delete.

If you use custom STL allocators, you must alter the type signatures for all STL objects to match the allocation policy. Because STL is used prominently in the SDK implementation and interface, a single approach in the SDK would inhibit direct passing of default STL objects into the SDK or control of STL allocation. Alternately, a hybrid approach – using custom allocators internally and allowing standard and custom STL objects on the interface – could potentially cause more difficulty when investigating memory issues.

The solution is to use the memory system’s compile-time constant AWS_CUSTOM_MEMORY_MANAGEMENT to control which STL types the SDK will use.

If the compile-time constant is enabled (on), the types resolve to STL types with a custom allocator connected to the AWS memory system.

If the compile-time constant is disabled (off), all Aws::* types resolve to the corresponding default std::* type.

Example code from the AWSAllocator.h file in the SDK:

```
#ifdef AWS_CUSTOM_MEMORY_MANAGEMENT

template< typename T >
class AwsAllocator : public std::allocator< T >
{
   ... definition of allocator that uses AWS memory system
};

#else

template< typename T > using Allocator = std::allocator<T>;

#endif
```

In the example code, the AwsAllocator can be either a custom allocator or a default allocator, depending on the compile-time constant.

Example code from the AWSVector.h file in the SDK:
`template< typename T > using Vector = std::vector< T, Aws::Allocator< T > >;`

In the example code, we define the Aws::* types.

If the compile-time constant is enabled (on), the type maps to a vector using custom memory allocation and the AWS memory system.

If the compile-time constant is disabled (off), the type maps to a regular std::vector with default type parameters.

Type aliasing is used for all std:: types in the SDK that perform memory allocation, such as containers, string stream, and string buf. The AWS SDK for C++ uses these types.

##### Remaining Issues
You can control memory allocation in the SDK; however, STL types still dominate the public interface through string parameters to the model object initialize and set methods. If you choose not to use STL and use strings and containers instead, you must create a lot of temporaries whenever you want to make a service call.

To remove most of the temporaries and allocation when service calls are made using non-STL, we have implemented the following:
* Every Init/Set function that takes a string has an overload that takes the const char*.
* Every Init/Set function that takes a container (map/vector) has an add variant that takes a single entry.
* Every Init/Set function that takes binary data has an overload that takes a pointer to the data and a length value.
* (Optional) Every Init/Set function that takes a string has an overload that takes a non-zero terminated constr char* and a length value.

##### Native SDK Developers and Memory Controls
Follow these rules in the SDK code:
* Do not use new and delete; use Aws::New<> and Aws::Delete<>.
* Do not use new[] and delete []; use Aws::NewArray<> and Aws::DeleteArray<>.
* Do not use std::make_shared; use Aws::MakeShared.
* Use Aws::UniquePtr for unique pointers to a single object. Use the Aws::MakeUnique function to create the unique pointer.
* Use Aws::UniqueArray for unique pointers to an array of objects. Use the Aws::MakeUniqueArray function to create the unique pointer.
* Do not directly use STL containers; use one of the Aws::typedefs or add a typedef for the desired container. Example: `Aws::Map<Aws::String, Aws::String> m_kvPairs;`
* Use shared_ptr for any external pointer passed into and managed by the SDK. You must initialize the shared pointer with a destruction policy that matches how the object was allocated. You can use a raw pointer if the SDK is not expected to clean up the pointer.

#### Logging
The AWS SDK for C++ includes logging support that you can configure. When initializing the logging system, you can control the filter level and the logging target (file with a name that has a configurable prefix or a stream). The log file generated by the prefix option rolls over once per hour to allow for archiving or deleting log files.

You can provide your own logger. However, it is incredibly simple to use the default logger we've already provided:

In your main function:

```
    SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
    Aws::InitAPI(options);
    //do SDK stuff;
    Aws::ShutdownAPI(options);
```
#### Client Configuration
You can use the client configuration to control most functionality in the AWS SDK for C++.

ClientConfiguration declaration:

```
struct AWS_CORE_API ClientConfiguration
{
    ClientConfiguration();

    Aws::String userAgent;
    Aws::Http::Scheme scheme;
    Aws::Region region;
    bool useDualStack;
    unsigned maxConnections;
    long requestTimeoutMs;
    long connectTimeoutMs;
    std::shared_ptr<RetryStrategy> retryStrategy;
    Aws::String endpointOverride;
    Aws::Http::Scheme proxyScheme;
    Aws::String proxyHost;
    unsigned proxyPort;
    Aws::String proxyUserName;
    Aws::String proxyPassword;
    std::shared_ptr<Aws::Utils::Threading::Executor> executor;
    bool verifySSL;
    Aws::String caPath;
    std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> writeRateLimiter;
    std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> readRateLimiter;
};
```

##### User Agent
The user agent is built in the constructor and pulls information from your operating system. Do not alter the user agent.

##### Scheme
The default value for scheme is HTTPS. You can set this value to HTTP if the information you are passing is not sensitive and the service to which you want to connect supports an HTTP endpoint. AWS Auth protects you from tampering.

##### Region
The region specifies where you want the client to communicate. Examples include us-east-1 or us-west-1. You must ensure the service you want to use has an endpoint in the region you configure.

##### UseDualStack
Sets the endpoint calculation to go to a dual stack (ipv6 enabled) endpoint. It is your responsibility to check that the service actually supports ipv6 in the region you specify.

##### Max Connections
The default value for the maximum number of allowed connections to a single server for your HTTP communications is 25. You can set this value as high as you can support the bandwidth. We recommend a value around 25.

##### Request Timeout and Connection Timeout
This value determines the length of time, in milliseconds, to wait before timing out a request. You can increase this value if you need to transfer large files, such as in Amazon S3 or CloudFront.

##### Retry Strategy
The retry strategy defaults to exponential backoff. You can override this default by implementing a subclass of RetryStrategy and passing an instance.

##### Endpoint Override
Do not alter the endpoint.

##### Proxy Scheme, Host, Port, User Name, and Password
These settings allow you to configure a proxy for all communication with AWS. Examples of when this functionality might be useful include debugging in conjunction with the Burp suite, or using a proxy to connect to the internet.

##### Executor
The default behavior for the executor is to create and detach a thread for each async call. You can change this behavior by implementing a subclass of Executor and passing an instance. We now provide a thread pooled executor as an option. For more information see this blog post: https://aws.amazon.com/blogs/developer/using-a-thread-pool-with-the-aws-sdk-for-c/

##### Verify SSL
If necessary, you can disable SSL certificate verification by setting the verify SSL value to false.

##### CA Path
You can tell the http client where to find your certificate trust store ( e.g. a directory prepared with OpenSSL c_rehash utility). This should not be necessary unless you are doing some weird symlink farm stuff for your environment. This has no effect on Windows or OSX.

##### Write Rate Limiter and Read Rate Limiter
The write and read rate limiters are used to throttle the bandwidth used by the transport layer. The default for these limiters is open. You can use the default implementation with your desired rates, or you can create your own instance by implementing a subclass of RateLimiterInterface.

#### Credentials Providers
You can use the AWSCredentialProvider interface to provide login credentials to AWS Auth. Implement this interface to provide your own method of credentials deployment. We also provide default credential providers.

##### Default Credential Provider Chain
The default credential provider chain does the following:
* Checks your environment variables for AWS Credentials
* Checks your $HOME/.aws/credentials file for a profile and credentials
* Contacts the ECS TaskRoleCredentialsProvider service to request credentials if Environment variable AWS_CONTAINER_CREDENTIALS_RELATIVE_URI has been set. Otherwise contacts the EC2MetadataInstanceProfileCredentialsProvider service to request credentials

The simplest way to communicate with AWS is to ensure we can find your credentials in one of these locations.

##### Other Methods
We also support two other methods for providing credentials:
* Provide your credentials in your client’s constructor.
* Use Amazon Cognito Identity, which is an identity management solution. You can use the CognitoCaching*CredentialsProviders classes in the identity-management project. For more information, see the *Amazon Cognito Developer Guide*.

#### Using a Service Client
You can use the default constructor, or you can use the system interfaces discussed above to construct a service client.

As an example, the following code creates an Amazon DynamoDB client using a specialized client configuration, default credentials provider chain, and default HTTP client factory:

```
auto limiter = Aws::MakeShared<Aws::Utils::RateLimits::DefaultRateLimiter<>>(ALLOCATION_TAG, 200000);

// Create a client
ClientConfiguration config;
config.scheme = Scheme::HTTPS;
config.connectTimeoutMs = 30000;
config.requestTimeoutMs = 30000;
config.readRateLimiter = m_limiter;
config.writeRateLimiter = m_limiter;

auto client = Aws::MakeShared<DynamoDBClient>(ALLOCATION_TAG, config);
```

You can also do the following to manually pass credentials:
`auto client = Aws::MakeShared<DynamoDBClient>(ALLOCATION_TAG, AWSCredentials("access_key_id", "secret_key"), config);`

Or you can do the following to use a custom credentials provider:
`auto client = Aws::MakeShared<DynamoDBClient>(ALLOCATION_TAG, Aws::MakeShared<CognitoCachingAnonymousCredentialsProvider>(ALLOCATION_TAG, "identityPoolId", "accountId"), config);`

Now you can use your Amazon DynamoDB client.

#### Error Handling
We did not use exceptions; however, you can use exceptions in your code. Every service client returns an outcome object that includes the result and an error code.

Example of handling error conditions:

```
bool CreateTableAndWaitForItToBeActive()
{
  CreateTableRequest createTableRequest;
  AttributeDefinition hashKey;
  hashKey.SetAttributeName(HASH_KEY_NAME);
  hashKey.SetAttributeType(ScalarAttributeType::S);
  createTableRequest.AddAttributeDefinitions(hashKey);
  KeySchemaElement hashKeySchemaElement;
  hashKeySchemaElement.WithAttributeName(HASH_KEY_NAME).WithKeyType(KeyType::HASH);
  createTableRequest.AddKeySchema(hashKeySchemaElement);
  ProvisionedThroughput provisionedThroughput;
  provisionedThroughput.SetReadCapacityUnits(readCap);
  provisionedThroughput.SetWriteCapacityUnits(writeCap);
  createTableRequest.WithProvisionedThroughput(provisionedThroughput);
  createTableRequest.WithTableName(tableName);

  CreateTableOutcome createTableOutcome = dynamoDbClient->CreateTable(createTableRequest);
  if (createTableOutcome.IsSuccess())
  {
     DescribeTableRequest describeTableRequest;
     describeTableRequest.SetTableName(tableName);
     bool shouldContinue = true;
     DescribeTableOutcome outcome = dynamoDbClient->DescribeTable(describeTableRequest);

     while (shouldContinue)
     {       
         if (outcome.GetResult().GetTable().GetTableStatus() == TableStatus::ACTIVE)
         {
            break;
         }
         else
         {
             std::this_thread::sleep_for(std::chrono::seconds(1));
         }
     }
     return true
  }
  else if(createTableOutcome.GetError().GetErrorType() == DynamoDBErrors::RESOURCE_IN_USE)
  {
     return true;
  }

  return false;
}
```

#### Advanced Topics
This section includes the following topics:
* Overriding Your HTTP Client
* Provided Utilities
* Controlling IOStreams Used by the HttpClient and the AWSClient

##### Overriding your Http Client
The default HTTP client for Windows is WinHTTP. The default HTTP client for all other platforms is Curl. If needed, you can create a custom HttpClientFactory, add it to the SDKOptions object which you pass to Aws::InitAPI().

##### Provided Utilities
The provided utilities include HTTP stack, string utils, hashing utils, JSON parser, and XML parser.

###### HTTP Stack
/aws/core/http/

The HTTP client provides connection pooling, is thread safe, and can be reused for your purposes. See the Client Configuration section above.

###### String Utils
/aws/core/utils/StringUtils.h

This header file provides core string functions, such as trim, lowercase, and numeric conversions.

###### Hashing Utils
/aws/core/utils/HashingUtils.h

This header file provides hashing functions, such as SHA256, MD5, Base64, and SHA256_HMAC.

###### Cryptography
/aws/core/utils/crypto/Cipher.h
/aws/core/utils/crypto/Factories.h

This header file provides access to secure random number generators, AES symmetric ciphers in CBC, CTR, and GCM modes, and the underlying Hash implementations that are used in HashingUtils.

###### JSON Parser
/aws/core/utils/json/JsonSerializer.h

This header file provides a fully functioning yet lightweight JSON parser (thin wrapper around JsonCpp).

###### XML Parser
/aws/core/utils/xml/XmlSerializer.h

This header file provides a lightweight XML parser (thin wrapper around tinyxml2). RAII pattern has been added to the interface.

##### Controlling IOStreams used by the HttpClient and the AWSClient
By default all responses use an input stream backed by a stringbuf. If needed, you can override the default behavior. For example, if you are using Amazon S3 GetObject and do not want to load the entire file into memory, you can use IOStreamFactory in AmazonWebServiceRequest to pass a lambda to create a file stream.

Example file stream request:

```
GetObjectRequest getObjectRequest;
getObjectRequest.SetBucket(fullBucketName);
getObjectRequest.SetKey(keyName);
getObjectRequest.SetResponseStreamFactory([](){ return Aws::New<Aws::FStream>( ALLOCATION_TAG, DOWNLOADED_FILENAME, std::ios_base::out ); });

auto getObjectOutcome = s3Client->GetObject(getObjectRequest);
```

### Contributing Back
\*Please Do!

##### Guidelines
* Don't make changes to generated clients directly. Make your changes in the generator. Changes to Core, Scripts, and High-Level interfaces are fine directly in the code.
* Do not use non-trivial statics anywhere. This will cause custom memory managers to crash in random places.
* Use 4 spaces for indents and never use tabs.
* No exceptions.... no exceptions. Use the Outcome pattern for returning data if you need to also return an optional error code.
* Always think about platform independence. If this is impossible, put a nice abstraction on top of it and use an abstract factory.
* Use RAII, Aws::New and Aws::Delete should only appear in constructors and destructors.
* Be sure to follow the rule of 5.
* Use the C++ 11 standard where possible.
* Use UpperCamelCase for custom type names and function names. Use m_* for member variables. Don't use statics. If you must, use UpperCammelCase for static variables
* Always be const correct, and be mindful of when you need to support r-values. We don't trust compilers to optimize this uniformly accross builds so please be explicit.
* Namespace names should be UpperCammelCase. Never put a using namespace statement in a header file unless it is scoped by a class. It is fine to use a using namespace statement in a cpp file.
* Use enum class, not enum
* Prefer `#pragma once` for include guards.
* Forward declare whenever possible.
* Use nullptr instead of NULL.
