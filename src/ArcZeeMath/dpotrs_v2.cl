

/*
Step1 
*/
__kernel void dpotrs_v2_p1(
	const int i,			//��ǰ�������
	const int matSize,
	const int n,			//�����ĳߴ磬 n=matSize/tileSize
	__global double *L,
	__global double *DBIs,
	__global double *b,		//Lx=b
	__global double *x		//results
)
{
	const int lid = get_local_id(0);
	const int base_addr = (i*TileSize+lid)*matSize;			//�ù������ȡLik�ĵ�lid��

	local double T[TileSize][TileSize];
	local double total_sum[TileSize];
	double x_k[TileSize];
	double sum=0;
	const int i_TileSize = i*TileSize;

	//����t_i=b_i-sum(Lin*xn)
	for (int k = 0; k < i; k++) {
		//��ȡLin�鵽T
		int k_TileSize = k*TileSize;
		int addr = base_addr + k_TileSize;
		for (int t = 0; t < TileSize; t++)
			T[lid][t] = L[addr+t];
		barrier(CLK_LOCAL_MEM_FENCE);

		//��ȡx_k
		for (int t = 0; t < TileSize; t++)
			x_k[t] = x[k_TileSize+t];

		//����Lin�ĵ�lid����x_n�ĵ��
		for (int t = 0; t < TileSize; t++)
			sum += T[lid][t] * x_k[t];
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	total_sum[lid] =b[i_TileSize+lid]-sum;
	
	barrier(CLK_LOCAL_MEM_FENCE);
	
	//����inv(Lii)*t_i
	//��ȡinv(Lii)�鵽T
	for (int t = 0; t < TileSize; t++)
		T[lid][t] = DBIs[i_TileSize*TileSize + lid*TileSize + t];

	barrier(CLK_LOCAL_MEM_FENCE);

	sum = 0;
	for (int t = 0; t < TileSize; t++)
		sum += T[lid][t] * total_sum[t];

	x[i_TileSize + lid] = sum;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
Step 2. 
*/
__kernel void dpotrs_v2_p2(
	const int i,			//��ǰ�������
	const int matSize,
	const int n,			//�����ĳߴ磬n=matSize/tileSize
	__global double *L,
	__global double *DBIs,
	__global double *b,		//Lx=b
	__global double *x		//results
)
{
	const int lid = get_local_id(0);							//�ù��������x_i�ĵ�lid��Ԫ��
	local double T[TileSize][TileSize];
	local double total_sum[TileSize];
	double x_k[TileSize];
	double sum = 0;
	const int i_TileSize = i*TileSize;

	//����t_i=b_i-sum(Lki*xk)
	for (int k = i+1; k< n; k++) {
		int k_TileSize = k*TileSize;
		//��ȡLki�ĵ�lid��, ����T�ĵ�lid��(����Lik��ת��)
		int addr = (k_TileSize+lid)*matSize + i_TileSize;
		for (int t = 0; t < TileSize; t++)
			T[t][lid] = L[addr+t];
		barrier(CLK_LOCAL_MEM_FENCE);

		//��ȡx_k
		for (int t = 0; t < TileSize; t++)
			x_k[t] = x[k_TileSize+t];

		barrier(CLK_LOCAL_MEM_FENCE);
		//����Lki�ĵ�lid����x_k�ĵ����Ҳ��T�ĵ�lid����x_k�ĵ����
		for (int t = 0; t < TileSize; t++)
			sum += T[lid][t] * x_k[t];
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	total_sum[lid] = b[i_TileSize + lid] - sum;

	barrier(CLK_LOCAL_MEM_FENCE);

	//����inv(Lii)*t_i
	//��ȡinv(Lii)�鵽T
	for (int t = 0; t < TileSize; t++)
		T[t][lid] = DBIs[i_TileSize*TileSize + lid*TileSize + t];

	barrier(CLK_LOCAL_MEM_FENCE);

	sum = 0;
	for (int t = 0; t < TileSize; t++)
		sum += T[lid][t] * total_sum[t];

	x[i_TileSize + lid] = sum;
}
