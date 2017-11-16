#pragma once

#define __CL_ENABLE_EXCEPTIONS
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include<vector>
#include<memory>
#include<iostream>
#include"mod_shared_ptr.hpp"

#define MODE_NO_SVM			0
#define MODE_COARSE_SVM		1
//#define MODE_FINE_SVM		2

using namespace std;


template<class T>
class clBufferPtr
{
	int mode;
	int size;
	cl::CommandQueue queue;
	cl::Buffer buffer;
	shared_ptr<T> ptr;				//��SVM buffer��ָ��
	mod_shared_ptr<T> bufferPtr;	//SVM buffer
	bool write;
public:
	clBufferPtr(cl::CommandQueue queue, cl::Buffer buffer, shared_ptr<T> ptr,  int size, bool write) {
		mode = MODE_NO_SVM;
		this->ptr = ptr;
		this->size = size;
		this->queue = queue;
		this->buffer = buffer;
		this->write = write;
	}
	clBufferPtr(cl::CommandQueue queue, T* ptr,  int size, bool write) {
		mode = MODE_COARSE_SVM;
		this->queue = queue;
		this->size = size;
		if (write)
			clEnqueueSVMMap(queue(), CL_TRUE, CL_MAP_WRITE,
				ptr, size * sizeof(T), 0, 0, 0);
		else
			clEnqueueSVMMap(queue(), CL_TRUE, CL_MAP_READ,
				ptr, size * sizeof(T), 0, 0, 0);
		bufferPtr = mod_shared_ptr<T>(ptr);
	}
	~clBufferPtr() {
		if (mode == MODE_NO_SVM) {
			if (write) {
				//��ptr�����ݿ�����buffer
				queue.enqueueWriteBuffer(buffer, CL_TRUE, 0, size * sizeof(T), ptr.get(), 0, 0);
				queue.finish();
			}
		}
		else if (mode == MODE_COARSE_SVM) {
			if (bufferPtr.use_count() == 1 && mode == 1)
				clEnqueueSVMUnmap(queue(), bufferPtr.get(), 0, 0, 0);
		}
	}
	T* get() {
		if (mode == MODE_NO_SVM) return ptr.get();
		else if (mode == MODE_COARSE_SVM) return bufferPtr.get();
		else return NULL;
	}
};


template<class T>
class clBufferEx
{
private:
	cl::Buffer buffer;
	//shared_ptr<T> ptr;				//��SVM buffer���ʱʹ��
	mod_shared_ptr<T> bufferPtr;	//SVM buffer
	int bufferSize;			//buffer�ĳߴ磨ʵ�ʵ��ֽ�����Ҫ����sizeof(T)��
	int svmMode;		//0=��SVMģʽ��1=������SVMģʽ
	cl::CommandQueue queue;
	cl::Context context;
public:
	clBufferEx() { }
	/*
	clBufferEx(const clBufferEx<T> &buffer) {
		printf("copy func %x\n", this);
		this->buffer=buffer.buffer;
		this->bufferPtr=buffer.bufferPtr;	//SVM buffer
		
		count++;
	} */

	/*
	clBufferEx<T> & clBufferEx<T>::operator = (const clBufferEx<T>& rhs) {
		//printf("operator = %x\n", this);
		this->buffer = rhs.buffer;
		this->bufferPtr = rhs.bufferPtr;	//SVM buffer
		return *this;
	}
	*/
	clBufferEx(cl::Context, cl::CommandQueue queue, int size, int mode);		//����ָ�����͵�Buffer
	~clBufferEx();

	int write(int pos, T* src, int size);		//��buffer��ָ��λ��д������src
	int writeBlocks(int pos, vector<T*> *src, int block_size);
	int read(int pos, T* dst, int size);			//��buffer��ָ��λ�õ����ݶ�����dst
	cl_int SetArgForKernel(cl::Kernel, int num);		//��Ϊkernel�ĵ�num������
	clBufferPtr<T> get_ptr(bool write);		//����ptr; ��ʹ�÷�SVM��ʽʱ������ͽ�ʹ�á�
	int size() { return bufferSize; }
};


template<class T>
clBufferEx<T>::~clBufferEx()
{
	//printf("deconstruct func %x.\t\t", this);
	if (svmMode == MODE_NO_SVM) {
		//����Ҫ�ֶ�����Buffer������Զ��ͷ�
	}
	else {
		if (bufferPtr.use_count() == 1) {
			void *ptr = (void*)bufferPtr.get();
			//printf("free SVM %x\n", ptr);
			clSVMFree(context(), ptr);
		}
	}
}

template<class T>
clBufferEx<T>::clBufferEx(cl::Context context, cl::CommandQueue queue, int size, int mode)		//����ָ�����͵�Buffer
{
	cl_int err;

	//printf("construct func %x\n", this);
	this->context = context;
	this->queue = queue;
	this->svmMode = mode;
	this->bufferSize = size;
	if (mode == MODE_NO_SVM) {
		buffer = cl::Buffer(context,CL_MEM_READ_WRITE, size * sizeof(T), NULL, &err);
		cl::detail::errHandler(err, "Buffer create error.");
	}
	else {
		try {
			T* ptr = (T*)clSVMAlloc(context(), CL_MEM_READ_WRITE, size * sizeof(T), 0);
			bufferPtr = mod_shared_ptr<T>(ptr);
			//printf("create SVM %x\n", ptr);
		}
		catch (cl::Error err) {
			std::cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << std::endl;
		}
	}
}

//��buffer��ָ��λ��д������src
template<class T>
int clBufferEx<T>::write(int pos, T* src, int size)
{
	cl_int err;
	T *ptr, *ptr_org;
	if (svmMode == MODE_NO_SVM) {
		queue.enqueueWriteBuffer(buffer, CL_TRUE, pos*sizeof(T), size * sizeof(T), src, 0, 0);
		queue.finish();
	}
	else if (svmMode == MODE_COARSE_SVM) {
		ptr_org = bufferPtr.get();
		//ӳ�䵽����
		err = clEnqueueSVMMap(queue(), CL_TRUE, CL_MAP_WRITE,
			(void*)ptr_org, bufferSize * sizeof(T), 0, 0, 0);
		//��������
		ptr = ptr_org + pos;
		memcpy((void*)ptr, (void*)src, size * sizeof(T));
		//���ӳ��
		err = clEnqueueSVMUnmap(queue(), (void*)ptr_org, 0, 0, 0);
	}
	return 0;
}

/*
��srcָ���һ�������д��buffer
*/
template<class T>
int clBufferEx<T>::writeBlocks(int pos, vector<T*> *src, int block_size) {
	cl_int err;
	T *ptr, *ptr_org;
	if (svmMode == MODE_NO_SVM) {
		for (int i = 0; i < src->size(); i++) {
			ptr = (*src)[i];
			queue.enqueueWriteBuffer(buffer, CL_TRUE, i*block_size * sizeof(T),
				block_size * sizeof(T), ptr, 0, 0);
			queue.finish();
		}
	}
	else if (svmMode == MODE_COARSE_SVM) {
		ptr_org = bufferPtr.get();
		//ӳ�䵽����
		err = clEnqueueSVMMap(queue(), CL_TRUE, CL_MAP_WRITE,
			(void*)ptr_org, bufferSize * sizeof(T), 0, 0, 0);
		//��������
		for (int i = 0; i < src->size(); i++) {
			ptr = ptr_org + i*block_size;
			memcpy((void*)ptr, (void*)(*src)[i], block_size * sizeof(T));
		}
		//���ӳ��
		err = clEnqueueSVMUnmap(queue(), (void*)ptr_org, 0, 0, 0);
	}
	return 0;
}

//��buffer��ָ��λ�õ����ݶ�����dst
template<class T>
int clBufferEx<T>::read(int pos, T* dst, int size)
{
	cl_int err;
	T* ptr, *ptr_org;
	if (svmMode == MODE_NO_SVM) {
		queue.enqueueReadBuffer(buffer, true, 0, size * sizeof(T), (void*)dst, NULL, NULL);
		queue.finish();
	}
	else if (svmMode == MODE_COARSE_SVM) {
		ptr_org = bufferPtr.get();
		//ӳ�䵽����
		err = clEnqueueSVMMap(queue(), CL_TRUE, CL_MAP_READ,
			(void*)ptr_org, size * sizeof(T), 0, 0, 0);
		//��������
		ptr = bufferPtr.get() + pos;
		memcpy((void*)dst, (void*)ptr, size * sizeof(T));
		//���ӳ��
		err = clEnqueueSVMUnmap(queue(), (void*)ptr_org, 0, 0, 0);
	}

	return 0;
}

template<class T>
cl_int clBufferEx<T>::SetArgForKernel(cl::Kernel kernel, int num)
{
	cl_int err;
	if (svmMode == MODE_NO_SVM)
		kernel.setArg(num, buffer);
	else
		err = clSetKernelArgSVMPointer(kernel(), num, bufferPtr.get());
	return 0;
}

template<class T>
clBufferPtr<T> clBufferEx<T>::get_ptr(bool write)
{
	if (svmMode == MODE_NO_SVM) {
		//��new�����ռ�, ��ʹ������ָ��
		shared_ptr<T> ptr = shared_ptr<T>(new T[bufferSize], std::default_delete<T[]>());
		//��OpenCL�豸�˿�������
		queue.enqueueReadBuffer(buffer, CL_TRUE, 0, bufferSize * sizeof(T), ptr.get(), 0, 0);
		queue.finish();
		return clBufferPtr<T>(queue, buffer, ptr, bufferSize, write);
	}
	else{
		return clBufferPtr<T>(queue, bufferPtr.get(), bufferSize, write);
	}
}

