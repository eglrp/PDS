
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>

#include "cldev.h"
#include "clBufferEx.h"

using namespace std;

//��ʼ��CL�豸, ���Ҽ�����ϵ�����openCLƽ̨���豸
int cldev::init(bool dispInfo)
{
    cl::Platform::get(&platforms);
    if (platforms.size() == 0)
    {
        std::cout << "Platform size 0\n";
        return -1;
    }
    for (int i = 0; i < platforms.size(); i++)
    {
        cl::STRING_CLASS str;
        //��ȡ�汾��ֵ
        platforms[i].getInfo((cl_platform_info)CL_PLATFORM_VERSION, &str);
        float versionNum;
#ifdef _MSC_VER
        sscanf_s(str.c_str() + 6, "%f", &versionNum);
#else
        sscanf(str.c_str() + 6, "%f", &versionNum);
#endif // _MSC_VER
        pfVersions.push_back(versionNum);

        if (dispInfo)
        {
            //*
            cout << "==========Platform " << i << "==========" << endl;
            cout << "Version: " << versionNum << endl;
            platforms[i].getInfo((cl_platform_info)CL_PLATFORM_NAME, &str);
            cout<<"Name: " << str << endl;
            platforms[i].getInfo((cl_platform_info)CL_PLATFORM_PROFILE, &str);
            cout << "Profile: " << str << endl;

            vector<cl::Device> pDevs;
            platforms[i].getDevices(CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU, &pDevs);
            for (int j = 0; j < pDevs.size(); j++)
            {
                cout << "=====Device " << j <<"====="<< endl;
                string str;
                pDevs[j].getInfo(CL_DEVICE_NAME, &str);
                cout << "Device Name: " << str << endl;
                cl_device_type type;
                pDevs[j].getInfo(CL_DEVICE_TYPE, &type);
                cout << "Device Type: ";
                if (type== CL_DEVICE_TYPE_CPU)
                    cout << "CPU" << endl;
                else if(type == CL_DEVICE_TYPE_GPU)
                    cout<< "GPU" << endl;
                //��ȡ�豸��local memory size
                cl_ulong lm_size;
                pDevs[j].getInfo(CL_DEVICE_LOCAL_MEM_SIZE, &lm_size);
                cout << "Local mem size:" << lm_size << endl;

                //�ж��豸��SVM֧��
                cl_device_svm_capabilities caps;
                pDevs[j].getInfo(CL_DEVICE_SVM_CAPABILITIES,&caps);
                if (caps & CL_DEVICE_SVM_COARSE_GRAIN_BUFFER)
                {
                    cout << "Coarse grain SVM is supported" << endl;
                    SVMmode = MODE_COARSE_SVM;
                }
                else
                {
                    cout << "SVM is not supported." << endl;
                    SVMmode = MODE_NO_SVM;
                }
                cl_command_queue_properties cqps;
                pDevs[j].getInfo(CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES, &cqps);
                if ( (cqps & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) && (cqps & CL_QUEUE_PROFILING_ENABLE) ){
                    printf("This device does support QUEUE_ON_DEVICE\n");
                }
                else{
                    printf("This device does not support QUEUE_ON_DEVICE\n");
                }

            }
            cout << endl;
            //*/
        }
    }
    return 0;
}

/*
versionReq: �汾Ҫ�󣬱�����ڵ���versionReq
SVMsupport: SVMsupport��֧��Ҫ��0����SVM֧�֣�1=�������SVM 2=��ϸ����SVM
*/
int cldev::selectPfWithMostDev(cl_device_type useType, float versionReq, int SVMsupport)
{
    cl_int err;
    cl_device_svm_capabilities caps;
    cl_command_queue_properties cqps;
	bool deviceQueueSupported;
    int idx=-1, maxDevs=0;
    vector<cl::Device> pDevs;
    vector<pair<int, cl::Device>> candidates;	//<platform index, dev>
	vector<bool> devQueSupport;
    int* devCntForPf = new int[platforms.size()];
    for (int i = 0; i < platforms.size(); i++)
    {
        devCntForPf[i] = 0;
        //����汾Ҫ��
        if (pfVersions[i]<versionReq) continue;
        try
        {
            platforms[i].getDevices(useType, &pDevs);
        }
        catch (exception ex)
        {
            continue;
        }
        //����SVMҪ���Լ�DEVICE_QUEUEҪ��
        int svm ;
        deviceQueueSupported=false;
        for (int j = 0; j < pDevs.size(); j++)
        {
            svm = 0;
            pDevs[j].getInfo(CL_DEVICE_SVM_CAPABILITIES, &caps);
            if (caps & CL_DEVICE_SVM_COARSE_GRAIN_BUFFER) svm = 1;
            else {}
            pDevs[j].getInfo(CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES, &cqps);
            if ( (cqps & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) && (cqps & CL_QUEUE_PROFILING_ENABLE) ){
                printf("This device does support QUEUE_ON_DEVICE\n");
				deviceQueueSupported = true;
            }
            else{
                printf("This device does not support QUEUE_ON_DEVICE\n");
            }
            if (svm >= SVMsupport)
            {
                candidates.push_back(make_pair(i, pDevs[j]));
				if (deviceQueueSupported)
					devQueSupport.push_back(true);
				else
					devQueSupport.push_back(false);
                devCntForPf[i]++;
            }
        }
        if (devCntForPf[i] > maxDevs)
        {
            maxDevs = devCntForPf[i];
            idx = i;
        }
    }
    delete[] devCntForPf;

    selectedPf = idx;
    if (idx < 0) return -1;
    //��д�豸
    for (int i = 0; i < candidates.size(); i++)
    {
        if (candidates[i].first == selectedPf)
        {
            selectedDevs.push_back(candidates[i].second);
			devQueSupFlag.push_back(devQueSupport[i]);
            cl_int cu_size;
            candidates[i].second.getInfo(CL_DEVICE_MAX_COMPUTE_UNITS, &cu_size);
            selDevCUsize.push_back(cu_size);
            cout << "CU size:" << cu_size << endl;
        }
    }

    if (candidates.size() < 1)
    {
        cout << "There is no device meet the requirements." << endl;
        return -1;
    }
    cout << "Selected platform " << idx << endl;

    //����������
    cl::Context context_(selectedDevs, NULL,NULL);
    context = context_;

    //Ϊÿ���豸�����������
    for (int i = 0; i < selectedDevs.size(); i++)
    {
        cl::CommandQueue queue(context, selectedDevs[i], 0, &err);
        if (err != 0) return -1;
        queues.push_back(queue);

        /* ���豸�ϴ���һ��������� */
		if(false){
		//if (devQueSupFlag[i]) {
			cl_queue_properties properties[] =
			{
				CL_QUEUE_PROPERTIES, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_ON_DEVICE | CL_QUEUE_ON_DEVICE_DEFAULT,
				CL_QUEUE_SIZE, 65536, //8192,
				0
			};
			clCreateCommandQueueWithProperties(context(), selectedDevs[i](), properties, &err);
		}
		//else
		//	clCreateCommandQueueWithProperties(context(), selectedDevs[i](), NULL, &err);

    }
    return 0;
}


/*
���ļ�����kernel
======���======
==-1 ���� ==0 OK
*/
int cldev::createKernels(vector<string> kernelFiles, vector<string> kernelNames)
{
    string combFileContent, singFileContent;
    try
    {
        int err;
        //������cl�ļ����ݽ��кϲ�
        for (int i = 0; i < kernelFiles.size(); i++)
        {
            //err = fileToString(kernelFiles[i].c_str(), singFileContent); //���ļ�����openCL����
            //if (err != 0) return i;
            combFileContent.append("#include \"");
            combFileContent.append(kernelFiles[i].c_str());
            combFileContent.append("\"\n");
        }

        //����cl�ļ�����
        cl::Program::Sources source(1, std::make_pair(combFileContent.c_str(), combFileContent.size()));
        cl::Program program_(context, source, &err);
        program = program_;
        err = program.build(selectedDevs, "-cl-std=CL2.0", NULL,NULL); //

        //����kernel
        for (int i = 0; i < kernelNames.size(); i++)
        {
            cl::Kernel kernel(program, kernelNames[i].c_str(), &err);
            if (err != 0) return -i;
            kernels[kernelNames[i]] = kernel;
        }
    }
    catch (cl::Error err)
    {
        char *program_log;
        size_t program_size, log_size;
        cout << "Err Code:" << err.err() << endl;
        /* Find size of log and print to std output */
        clGetProgramBuildInfo(program(), selectedDevs[0](), CL_PROGRAM_BUILD_LOG,
                              0, NULL, &log_size);
        program_log = (char*)malloc(log_size + 1);
        program_log[log_size] = '\0';
        clGetProgramBuildInfo(program(), selectedDevs[0](), CL_PROGRAM_BUILD_LOG,
                              log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
        system("pause");
        exit(1);
    }
    return 0;
}


/*
���ļ�����kernel
kernelContents		==>firstΪ�ļ���, secondΪ�ļ�����
======���======
==-1 ���� ==0 OK
*/
int cldev::createKernelsFromStr(vector<std::pair<string, string>> kernelContents, vector<string> kernelNames)
{
    vector<string> kernelFiles;
    vector<std::pair<string, string>>::iterator iter = kernelContents.begin();
    for (; iter != kernelContents.end(); iter++)
    {
        kernelFiles.push_back(iter->first);
        //д���ļ�
        std::fstream f(iter->first, (std::fstream::out | std::fstream::binary));
        if (f.is_open())
        {
            f.write(iter->second.c_str(), iter->second.size());
        }
        f.close();
    }

    return createKernels(kernelFiles, kernelNames);
}

/** ��ȡ�ļ�������תΪ�ַ��� */
int cldev::fileToString(const char *filename, std::string& s)
{
    size_t size;
    char*  str;
    std::fstream f(filename, (std::fstream::in | std::fstream::binary));
    if (f.is_open())
    {
        size_t fileSize;
        f.seekg(0, std::fstream::end);
        size = fileSize = (size_t)f.tellg();
        f.seekg(0, std::fstream::beg);
        str = new char[size + 1];
        if (!str)
        {
            f.close();
            return 0;
        }
        f.read(str, fileSize);
        f.close();
        str[size] = '\0';
        s = str;
        delete[] str;
        return 0;
    }
    cout << "Error: failed to open file\n:" << filename << endl;
    return -1;
}

void cldev::getKernelInfo(string kernelName)
{
    size_t size[3];
    //kernels[kernelName].getWorkGroupInfo(selectedDevs[0], CL_KERNEL_GLOBAL_WORK_SIZE, &size[0]);
    //printf("Max size of global work items:%d,%d,%d", size[0], size[1], size[2]);
    kernels[kernelName].getWorkGroupInfo(selectedDevs[0], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, &size[0]);
    printf("Prefered size of work group:%d\n", size[0]);
    kernels[kernelName].getWorkGroupInfo(selectedDevs[0], CL_KERNEL_WORK_GROUP_SIZE, &size[0]);
    printf("Max work group:%d\n", size[0]);



}




void cldev::test()
{
    cl_int err;
    float data[5];
    float *x;

    //��������SVM
    //����pts3D, Pts2D, Poses�����ݻ���
    map<string, clBufferEx<float>> buffers;
    buffers["test1"]=clBufferEx<float>(context, queues[0], 10, MODE_COARSE_SVM);

    data[0] = 1.23;
    data[1] = 2.34;
    data[2] = 3.45;
    data[3] = 4.56;
    data[4] = 5.67;
    buffers["test1"].write(0, data, 5);

    cl::Kernel kernel = kernels["test1"];
    buffers["test1"].SetArgForKernel(kernel, 0);

    //�ڵڶ���device�ϵ����ں�
    cl::Event event;
    try
    {
        queues[0].enqueueNDRangeKernel(
            kernel,
            cl::NullRange,
            cl::NDRange(4),
            cl::NullRange,
            NULL,
            NULL);
        //event.wait();
        queues[0].finish();

        clBufferPtr<float> ptr = buffers["test1"].get_ptr(false);
        float *ptr2=ptr.get();

        buffers["test1"].read(0, data, 5);
    }
    catch (cl::Error err)
    {
        std::cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << std::endl;
    }

    buffers["test2"] = clBufferEx<float>(context, queues[0], 100, MODE_COARSE_SVM);
}


void cldev_test()
{
    cldev cd;
    cd.init(false);
    if (cd.selectPfWithMostDev(CL_DEVICE_TYPE_ALL, 2.0, 1))
    {
        cout << "No devices satisfy the requirement.";
    }

    vector<string> kernelFiles;
    vector<string> kernelNames;
    kernelNames.push_back("test1");
    kernelFiles.push_back("E:\\sync_directory\\workspace\\PSBA\\CL_files\\test1.cl");
    kernelNames.push_back("test2");
    kernelFiles.push_back("E:\\sync_directory\\workspace\\PSBA\\CL_files\\test2.cl");
    if (cd.createKernels(kernelFiles, kernelNames) != 0)
        cout << "cldev_test(): create Kernel failed" << endl;
    cd.test();
}

