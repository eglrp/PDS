#pragma once

#define __CL_ENABLE_EXCEPTIONS
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include<map>

using namespace std;

class cldev
{
private:
	std::vector<cl::Platform> platforms;
	std::vector<float> pfVersions;			//ƽ̨�İ汾

	int selectedPf;							//ѡ��ʹ�õ�ƽ̨
	std::vector<cl::Device> selectedDevs;	//ѡ���ƽ̨�µ��豸
	std::vector<int> selDevCUsize;			//ѡ����豸��CU����
	std::vector<bool> devQueSupFlag;		//ѡ���ƽ̨�µ��豸�Ƿ�֧���豸����

	cl::Context context;
	cl::Program program;
	std::map<string, cl::Kernel> kernels;
	std::vector<cl::CommandQueue> queues;
	int SVMmode;
public:
	cldev() {};

	int init(bool dispInfo);	//��ʼ��CL�豸
	int selectPfWithMostDev(cl_device_type useType, float versionReq, int SVMsupport);	//ѡ��������Device,�汾���ڵ���version��ƽ̨��Ϊ����ƽ̨
	int createKernels(vector<string> kernelFiles, vector<string> kernelNames);		//���ļ�����kernel
	int createKernelsFromStr(vector<std::pair<string, string> >, vector<string> kernelNames);
	int fileToString(const char *filename, std::string& s);
	bool getSVMmode() { return SVMmode; }

	cl::Context get_context() { return context; }
	cl::Device get_device(int devIdx) { return selectedDevs[devIdx]; }
	cl::CommandQueue get_queue(int devIdx) { return queues[devIdx]; }
	int get_CUsize(int devIdx) { return selDevCUsize[devIdx]; }
	int get_prefer_localsize(int devIdx) { return 32; }
	cl::Kernel* get_kernel(string name) {
		if (kernels.find(name) != kernels.end())
			return &kernels[name];
		else
			return NULL;
	}
	void getKernelInfo(string kernelName);

	void test();
};
