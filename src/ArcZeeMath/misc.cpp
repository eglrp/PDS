
#include "misc.h"


#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define MAX_LINE_DATA_NUM	1000*100
#define MAX_LINE_CHARS		1000*100*20


char line_buff[MAX_LINE_CHARS];

int readDataFromLine(FILE *fp, double *data, int *nDatas);

/*
read m*n dense matrix from txt file
*/
int read_dmat(char *fileName, int m,int n, double* mat)
{
    int ret;
    int nDatas;
    FILE *fp;

    //double *datas = new double[MAX_LINE_DATA_NUM];
    //��ȡ�����ļ�
#ifdef _MSC_VER
    if (fopen_s(&fp, fileName, "r") != 0)
    {
#else
    if ((fp=fopen(fileName, "r")) != 0)
    {
#endif // _MSC_VER
        fprintf(stderr, "cannot open file %s, exiting\n", fileName);
        system("pause");
        exit(1);
    }

    //��ÿһ�ж�ȡ����
    int cnt = 0;
    while (1)
    {
        ret = readDataFromLine(fp, &mat[cnt*n], &nDatas);
        if (ret != 1) cnt++;
        else break; //�ļ�β
        if (n != nDatas) return -1;
    }
    if (cnt != m) return -1;

    //delete[] datas;
    return 0;
}

/*
���ļ��ĵ�ǰ�ж�ȡnDatas������, ������'\t' ','��ո�ָ�
======����==========
fp		==>�ļ�������
data	==>�ṩ��MAX_LINE_DATA_NUM�����ڴ�����ݵĿռ�
======���==========

======����ֵ========
==1 ����Ϊ�ļ�β�� ==0 ��ʾ������ ==2
*/
int readDataFromLine(FILE *fp, double *data, int *nDatas)
{
    char *ret;
    int nChars, pos, start;
    double val;

    *nDatas = 0;
    start = 0;
    pos = 0;
    //�����ж���line_buff
    ret = fgets(line_buff, MAX_LINE_CHARS - 1, fp);
    if (ret == NULL) return 1;
    nChars = (int)strlen(line_buff); //����'\n'
    while (pos < nChars)
    {
        //��������ķָ��
        while ((line_buff[pos] == '\t' || line_buff[pos] == ',' || line_buff[pos] == ' ') && line_buff[pos] != '\n')
            pos++;
        if (line_buff[pos] == '\n' || line_buff[pos] == '#') break;

        //pos��ʼ���ַ�������һ������, ���н���
#ifdef _MSC_VER
        sscanf_s(&line_buff[pos], "%lf", &val);
#else
        sscanf(&line_buff[pos], "%lf", &val);
#endif // _MSC_VER
        data[(*nDatas)++] = val;

        //�������ݣ�ֱ�������ָ���
        while (line_buff[pos] != '\t' && line_buff[pos] != ',' && line_buff[pos] != ' ' && line_buff[pos] != '\n')
            pos++;
    }

    return 0;
}
