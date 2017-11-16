

/*
part 1, step 1 
���� b[i]=inv(L_ii)*b_i
*/
__kernel void dpotrs_v3_p1_s1(
	const int i,			//��ǰ�������
	const int matSize,
	__global double *L,
	__global double *DBIs,
	__global double *b		//Lx=b
)
{
	local double bi[TileSize];
	double sum = 0;
	const int u = get_local_id(0);
	double DBI_u[TileSize];
	int v;

	bi[u] = b[i*TileSize + u];
	barrier(CLK_LOCAL_MEM_FENCE);

	//read inv(L_ii)��u�� to DBI_u
	int addr = i*TileSize*TileSize+ u*TileSize;
	for (v = 0; v < TileSize; v++)
		DBI_u[v] = DBIs[addr++];

	//����inv(L_ii)��u����bi�ĵ��
	for (v = 0; v < TileSize; v++)
		sum = fma(bi[v], DBI_u[v], sum);
		//sum += bi[v]*DBI_u[v];

	b[i*TileSize + u] = sum;
}

/*
part 1 step 2
*/
__kernel void dpotrs_v3_p1_s2(
	const int i,			//��ǰ�������
	const int matSize,
	__global double *L,
	__global double *DBIs,
	__global double *b		//Lx=b
)
{
	double sum=0;
	int addr;
	const int k = get_group_id(0)+i+1;
	const int u = get_local_id(0);
	const int bAddr = k*TileSize+u;

	local double bi[TileSize];
	bi[u] = b[i*TileSize + u];
	barrier(CLK_LOCAL_MEM_FENCE);

	//����L_ki�ĵ�u��
	double Lki_u[TileSize];
	addr = (bAddr)*matSize + i*TileSize;
	for (int v = 0; v < TileSize; v++)
		Lki_u[v]=L[addr++];
		
	for( int v=0; v<TileSize; v++)
		sum = fma(Lki_u[v], bi[v], sum);

	//addr = kTileSize + u;
	b[bAddr] -= sum;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
part 2, step 1
���� b[i]=inv(L_ii^T)*b_i
*/
__kernel void dpotrs_v3_p2_s1(
	const int i,			//��ǰ�������
	const int matSize,
	__global double *L,
	__global double *DBIs,
	__global double *b		//Lx=b
)
{
	local double bi[TileSize];
	double sum = 0;
	const int u = get_local_id(0);
	double DBI_u[TileSize];
	int v;

	bi[u] = b[i*TileSize + u];
	barrier(CLK_LOCAL_MEM_FENCE);

	//read inv(L_ii)��u�� to DBI_u
	int addr = i*TileSize*TileSize + u;
	for (v = 0; v < TileSize; v++) {
		DBI_u[v] = DBIs[addr];
		addr += TileSize;
	}

	//����inv(L_ii)��u����bi�ĵ��
	for (v = 0; v < TileSize; v++)
		sum = fma(bi[v], DBI_u[v], sum);
	//sum += bi[v]*DBI_u[v];

	b[i*TileSize + u] = sum;
}


__kernel void dpotrs_v3_p2_s2(
	const int i,			//��ǰ�������
	const int matSize,
	__global double *L,
	__global double *DBIs,
	__global double *b		//Lx=b
	)
{
	double sum = 0;
	int addr;
	const int k = get_group_id(0);
	const int u = get_local_id(0);
	const int kTileSize = k*TileSize;


	local double bi[TileSize];
	bi[u] = b[i*TileSize + u];
	barrier(CLK_LOCAL_MEM_FENCE);

	//����L_ik�ĵ�u��
	double Lik_u[TileSize];
	addr = (i*TileSize)*matSize + kTileSize + u;
	for (int v = 0; v < TileSize; v++) {
		Lik_u[v] = L[addr];
		addr += matSize;
	}

	for (int v = 0; v<TileSize; v++)
		sum = fma(Lik_u[v], bi[v], sum);

	//addr = kTileSize + u;
	b[kTileSize+u] -= sum;
	//b[kTileSize + u] = 8.8;
}