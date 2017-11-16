///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
����tileSize*tileSize�Խǿ��cholesky�ֽ�
ȫ���������ھֲ���DB��
*/
inline int compute_Ljj(local double* DB, int uvAddr, int tileSize, int u, int v)
{ 
	//int addr = u*tileSize + v;
	int addr2;
	int addr3;
	int ret = 0;
	double sum=0;
	for (int j = 0; j< tileSize; j++)	//���еĴ���
	{
		//�Խ�Ԫ, d_v=a[v,k]*a[v,k] k=1,...,v-1
		sum = DB[uvAddr];
		if(u==v && j==v)
		{
			if (v > 0) {
				addr2 = v*tileSize;
				for (int n = 0;n < v; n++) {
					//sum -= DB[addr2] * DB[addr2];
					sum = -fma(DB[addr2], DB[addr2], -sum);
					addr2++;
				}
			}
			if (sum<0) ret = -1;
			else DB[uvAddr] = sqrt(sum);
		}
		barrier(CLK_LOCAL_MEM_FENCE);
		//�ǶԽ�Ԫ, t_v=a[u,k]*a[v,k] k=1,...,v-1
		if (u > v && j == v)
		{
			if (v > 0) {
				addr2 = u*tileSize;
				addr3 = v*tileSize;
				for (int n = 0;n < v; n++) {
					sum -= DB[addr2++] * DB[addr3++];
				}
			}
			DB[uvAddr] = sum / DB[v*tileSize + v];
		}
		if(u<v && j==v)
			DB[uvAddr] = 0;
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	return ret;
}

/*
����tileSize*tileSize�Խǿ�Ljj����
*/
inline void compute_LjjInv(local double* DB,local double* DBI, int uvAddr, int tileSize, int u, int v)
{ 
	//int addr = u*tileSize + v;
	//int addr2;
	//int addr3;
	double sum = 0;
	for (int k = 0; k < tileSize; k++)	//����diagonal�Ĵ���, (main diagonal, non-main diagonal)
	{
		if ((u - v) == k) {		//ȷ���ǸöԽ����ϵ�Ԫ��
			if (k == 0)
				DBI[uvAddr] = 1 / DB[uvAddr];
			else{ 
				for (int s = 0; s < u; s++)	//sum(L_is*X_sj )
					sum -= DB[u*tileSize + s] * DBI[s*tileSize + v];
				DBI[uvAddr] = sum / DB[u*tileSize + u];
			}
		}
		if (u < v) DBI[uvAddr] = 0;
		barrier(CLK_LOCAL_MEM_FENCE);
	}

}