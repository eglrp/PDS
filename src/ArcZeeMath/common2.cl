///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
����tileSize*tileSize�Խǿ��cholesky�ֽ�
ȫ���������ھֲ���DB��
*/
inline int compute2_Ljj(local double DB[TileSize][TileSize], int u, int v)
{ 
	//int addr = u*tileSize + v;
	int ret = 0;
	for (int j = 0; j< TileSize; j++) {		//���еĴ���
		//�Խ�Ԫ, d_v=a[v,k]*a[v,k] k=1,...,v-1
		if(u==v && j==v) {
			if (v > 0) {
				for (int n = 0;n < v; n++) {
					DB[u][v] -= DB[v][n] * DB[v][n];
				}
			}
			if (DB[u][v]<0) ret = -1;
			else DB[u][v] = sqrt(DB[u][v]);
		}
		barrier(CLK_LOCAL_MEM_FENCE);
		//�ǶԽ�Ԫ, t_v=a[u,k]*a[v,k] k=1,...,v-1
		if (u > v && j == v) {
			if (v > 0) {
				for (int n = 0;n < v; n++) {
					DB[u][v] -= DB[u][n] * DB[v][n];
				}
			}
			DB[u][v] = DB[u][v] / DB[v][v];
		}
		if(u<v && j==v)
			DB[u][v] = 0;
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	return ret;
}

/*
����tileSize*tileSize�Խǿ�Ljj����
*/
inline void compute2_LjjInv(local double DB[TileSize][TileSize], local double DBI[TileSize][TileSize], int u, int v)
{ 
	//int addr = u*tileSize + v;
	double sum = 0;
	for (int k = 0; k < TileSize; k++) {		//����diagonal�Ĵ���, (main diagonal, non-main diagonal)
		if ((u - v) == k) {		//ȷ���ǸöԽ����ϵ�Ԫ��
			if (k == 0)
				DBI[u][v] = 1 / DB[u][v];
			else { 
				for (int n = 0; n < u; n++)	//sum(L_is*X_sj )
					sum -= DB[u][n] * DBI[n][v];
				DBI[u][v] = sum / DB[u][u];
			}
		}
		if (u < v) DBI[u][v] = 0;
		barrier(CLK_LOCAL_MEM_FENCE);
	}

}