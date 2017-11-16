
/*
Step1 ����U_j=Lij*x_j, j=[1...i-1]. ����浽[n-i-1,n-j-1]��ĵ�һ��
*/
__kernel void dpotrs_v1_p1_s1(
	const int i,			//��ǰ�������
	const int NWR,
	const int tileSize,
	const int matSize,
	const int n,			//�����ĳߴ磬 n=matSize/tileSize
	__global double *L,
	__global double *DBIs,
	__global double *b,		//Lx=b
	__global double *x		//results
)
{
	int j = get_group_id(0);
	int u = get_local_id(0);
	int bandSize = matSize*tileSize;
	int ijuAddr = i*bandSize + u*matSize + j*tileSize;

	int resultAddr = (n - i - 1)*bandSize + (n - j - 1)*tileSize;
	int xjAddr = j*tileSize;

	double sum = 0;
	for (int k = 0; k < tileSize; k++)
		sum += L[ijuAddr + k] * x[xjAddr + k];
	L[resultAddr + u] = sum;
}

/*
Step1 ����V_i=sum(Uj)��reduction algorithm����ʼ��ַΪ[n-i-1,n-i]��
*/
__kernel void dpotrs_v1_p1_s2(
	const int i,			//��ǰ�������
	const int NWR,
	const int nrb,
	const int nCols,
	const int loop,
	const int tileSize,
	const int matSize,
	const int n,			//�����ĳߴ磬 n=matSize/tileSize
	const int g_stride,
	__global double *L,
	__global double *DBIs,
	__global double *b,		//Lx=b
	__global double *x		//results
)
{
	int j = get_global_id(0);		//block 
	int local_id = get_local_id(0);

	int bandSize = matSize*tileSize;
	int baseAddr = (n - i - 1)*bandSize + (n - i)*tileSize;
	int jAddr;
	int ijpAddr;
	int jj;

	for (uint stride = NWR / 2; stride > 0; stride /= 2) {
		barrier(CLK_GLOBAL_MEM_FENCE);
		jAddr = baseAddr + j*tileSize;
		//��i+stride��ӵ�i����
		if (local_id < stride) {
			jj = (j + stride*g_stride);
			if (jj > nrb - 1) continue;
			ijpAddr = baseAddr + jj*tileSize;

			for (int v = 0; v < tileSize; v++) {
				L[jAddr + v] += L[ijpAddr + v];
				//L[jAddr + v] = 8.8;
			}
		}
	}
}


/*
Step1 ����x_j=inv(Lii)*(b_i-v_i), v_i�����[n-i-1, n-i]
*/
__kernel void dpotrs_v1_p1_s3(
	const int i,			//��ǰ�������
	const int NWR,
	const int tileSize,
	const int matSize,
	const int n,			//�����ĳߴ磬 n=matSize/tileSize
	__global double *L,
	__global double *DBIs,
	__global double *b,		//Lx=b
	__global double *x,		//results
	__local double *TT
)
{
	double sum;
	int u = get_local_id(0);
	int iiuAddr = i*tileSize*tileSize + u*tileSize;		//inv(Lii)��u��
	int vAddr = (n - i - 1)*matSize*tileSize + (n - i)*tileSize;
	int iAddr = i*tileSize;

	//����b_i-v_i
	if (get_local_id(0) == 0) {
		if (i > 0) {
			for (int k = 0; k < tileSize; k++)
				TT[k] = b[iAddr + k] - L[vAddr + k];
		}
		else {
			for (int k = 0; k < tileSize; k++)
				TT[k] = b[iAddr + k];
		}
	}
	barrier(CLK_LOCAL_MEM_FENCE);
	sum = 0;
	for (int k = 0; k < tileSize; k++) {
		sum += DBIs[iiuAddr+k] * TT[k];
	}
	x[iAddr + u] = sum;// TT[u];
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
Step1 ����U_j=Lt_ji*x_j, j=[i+1, n]. ����浽[i,j]��ĵ�һ��
*/
__kernel void dpotrs_v1_p2_s1(
	const int i,			//��ǰ�������
	const int NWR,
	const int tileSize,
	const int matSize,
	const int n,			//�����ĳߴ磬n=matSize/tileSize
	__global double *L,
	__global double *DBIs,
	__global double *b,		//Lx=b
	__global double *x		//results
)
{
	int j = i + 1 + get_group_id(0);
	int u = get_local_id(0);

	int bandSize = matSize*tileSize;
	int ijuAddr = j*bandSize + i*tileSize + u;	//Lji

	int resultAddr = i*bandSize + j*tileSize;
	int xjAddr = j*tileSize;

	double sum = 0;
	for (int k = 0; k < tileSize; k++) {
		sum += L[ijuAddr] * x[xjAddr + k];
		ijuAddr += matSize;
	}
	L[resultAddr + u] = sum;

}


/*
Step1 ����V_i=sum(Uj)��reduction algorithm����ʼ��ַΪ[i,i+1]��
*/
__kernel void dpotrs_v1_p2_s2(
	const int i,			//��ǰ�������
	const int NWR,
	const int nrb,
	const int nCols,
	const int loop,
	const int tileSize,
	const int matSize,
	const int n,			//�����ĳߴ磬 n=matSize/tileSize
	const int g_stride,
	__global double *L,
	__global double *DBIs,
	__global double *b,		//Lx=b
	__global double *x		//results
)
{
	int j = get_global_id(0);			//block
	int local_id = get_local_id(0);

	int bandSize = matSize*tileSize;
	int baseAddr = i*bandSize + (i + 1)*tileSize;
	int jAddr;
	int ijpAddr;
	int jj;

	for (uint stride = NWR / 2; stride > 0; stride /= 2) {
		barrier(CLK_GLOBAL_MEM_FENCE);
		jAddr = baseAddr + j*tileSize;
		//��i+stride��ӵ�i����
		if (local_id < stride) {
			jj = (j + stride*g_stride);
			if (jj > nrb - 1) continue;
			ijpAddr = baseAddr + jj*tileSize;
			for (int v = 0; v < tileSize; v++) {
				L[jAddr + v] += L[ijpAddr + v];
			}
		}
	}
}


/*
Step1 ����x_j=inv(Lii)*(b_i-v_i), v_i�����[i, i+1]
*/
__kernel void dpotrs_v1_p2_s3(
	const int i,			//��ǰ�������
	const int NWR,
	const int tileSize,
	const int matSize,
	const int n,			//�����ĳߴ磬 n=matSize/tileSize
	__global double *L,
	__global double *DBIs,
	__global double *b,		//Lx=b
	__global double *x,		//results
	__local double *TT
)
{
	double sum;
	int u = get_local_id(0);
	int iiuAddr = i*tileSize*tileSize + u;		//inv(Lii)��u��
	int vAddr = i*matSize*tileSize + (i + 1)*tileSize;
	int iAddr = i*tileSize;

	//����b_i-v_i
	if (get_local_id(0) == 0) {
		if (i < n - 1) {
			for (int k = 0; k < tileSize; k++)
				TT[k] = b[iAddr + k] - L[vAddr + k];
		}
		else {
			for (int k = 0; k < tileSize; k++)
				TT[k] = b[iAddr + k];
		}
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	sum = 0;
	for (int k = 0; k < tileSize; k++)	{
		sum += DBIs[iiuAddr] * TT[k];
		iiuAddr += tileSize;
	}
	x[iAddr + u] = sum;// TT[u];
}