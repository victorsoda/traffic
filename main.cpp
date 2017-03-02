#include <stdio.h>
#include <fstream>
#include <math.h>
#include "Rib.h"
#include <memory.h>
#include <string.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <queue>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#define IP_LEN		32
#define FIFONUM		5
#define MAXIPNUM	5000000
using namespace std;

struct cache{
	string ip;
	int nexthop;
};

string ipstr[MAXIPNUM];
CRib tRib;
queue<string> fifo[FIFONUM + 2];
pthread_t thread[FIFONUM + 2];
cache compare[FIFONUM + 2];
int j[FIFONUM + 2];
int hitcnt[FIFONUM + 2];
int fifoIPcnt[FIFONUM + 2];
bool finished_distribution = false;
timeval tv1,tv2;	//for timer
unsigned long tv;	//for timer
pthread_mutex_t thread_mutex;


//given the root of rib trie---m_trie and the prefix---insert_C, return nexthop
int FindNextHop(RibTrie * m_trie,char * insert_C)
{
	int nextHop=-1;//init the return value
	RibTrie *insertNode=m_trie;

	bool IfNewBornNode=false;
	if (insertNode->iNextHop!=0)nextHop=insertNode->iNextHop;
	int len=(int)strlen(insert_C);
	for (int i=0;i<len+1;i++)
	{		
		if ('0'==insert_C[i])
		{//if 0, turn left
			if (NULL!=insertNode->pLeftChild)	insertNode=insertNode->pLeftChild;
			else								break;			
		}
		else
		{//if 1, turn right
			if (NULL!=insertNode->pRightChild)	insertNode=insertNode->pRightChild;
			else								break;			
		}
		if (insertNode->iNextHop!=0)			nextHop=insertNode->iNextHop;
	}
	return	nextHop;
}

//convert int to string
string Int_to_String(int n)
{
	ostringstream  ostr;
	ostr << n ;
 
	string str(ostr.str());
	return str;
}

//readin the rib（路由表）
void init()
{
	string tmpfile = "rib.txt"; //路由表文件名
	int filenamelen = tmpfile.length();
	char* ribfile = new char[filenamelen + 1];
	strcpy(ribfile, tmpfile.c_str());  
	
	FILE *fp_rib = fopen(ribfile, "r"); //打开路由表文件
	if (fp_rib == 0) 
	{
		printf("\tSource rib file doesn't exist，press any key to quit...\n");
		getchar();
	}
	fclose(fp_rib);

	tRib = CRib();		//define a rib struct
	int iEntryCount = tRib.BuildRibFromFile(ribfile);
	unsigned int iNodeCount = tRib.GetNodeCount(tRib.m_pTrie);
	
	for(int i = 0; i < FIFONUM; ++i) {
		compare[i].ip = "0.0.0.0";
		compare[i].nexthop = -1;
	}
	
	memset(hitcnt, 0, sizeof(hitcnt));
	memset(fifoIPcnt, 0, sizeof(fifoIPcnt));
}

//a nice hash algorithm, convert ip to hash number
int DJBHash(string ip) 
{
	unsigned int hash, seed = 5381;
	int c, i;
	char *pt;

	const int len = ip.length();
	pt = new char[len + 1];
	strcpy(pt, ip.c_str());
	unsigned char* ptr = (unsigned char*)pt;
	
	hash = seed;
	while (c = *ptr++) {
		hash = ((hash << 13) + hash) + c;
	}
	
	return hash;
}

//distributor: distribute an ip to its queue
void distribute(int hash, string ip)
{
	int k = hash % FIFONUM;
	if(k < 0) k += FIFONUM;
	fifo[k].push(ip);
}

//search ip's nexthop in the rib
int detectForFullIp(string ip)
{
	char new_tmp[IP_LEN+1];
	memset(new_tmp, 0, sizeof(new_tmp));
		

	int hop = 11111;
	if(new_tmp != NULL) {
		tRib.IpToBinary(ip, new_tmp);
		hop = FindNextHop(tRib.m_pTrie,new_tmp);
	}
	else cout << "null" << endl;	
	return hop;
}

bool fileExists(string filename)
{	
	char path[80];
	getcwd(path, sizeof(path));
	string pathstr = path;
	pathstr = pathstr + filename;
	if(access(pathstr.c_str(), F_OK)) return true;
	return false;
}


//what thread[threadnum] should do
void* mythread(void* arg) 
{
	int threadnum = *(int *)arg;
	/*	
	string myfilename = Int_to_String(threadnum) + ".txt";	//the out-put filename: e.g. "7.txt" for fifo[6]
	if(fileExists(myfilename))
	{ //if the output-file exists
		remove((myfilename).c_str());
	}
	ofstream fout((Int_to_String(threadnum) + ".txt").c_str(), ios::app);
	*/	
	
	//cout << threadnum << endl;	
	
	
	int ret;

	while(true) {
		pthread_mutex_lock(&thread_mutex);
		//ret = pthread_mutex_trylock(&thread_mutex);		
		//if(ret!=EBUSY) {		
			if(!fifo[threadnum].empty()) {
				string nextip = fifo[threadnum].front();
				fifoIPcnt[threadnum]++;
				int nexthop_toprint = -1;
				if(nextip == compare[threadnum].ip) {
					nexthop_toprint = compare[threadnum].nexthop;
					hitcnt[threadnum]++;
				} 
				else {
					int _nexthop = detectForFullIp(nextip);
					//int _nexthop = 12345;
					nexthop_toprint = _nexthop;
					compare[threadnum].ip = nextip;
					compare[threadnum].nexthop = _nexthop;
				}
				fifo[threadnum].pop();
				//fout << nextip << " " << nexthop_toprint << endl;
			}
			pthread_mutex_unlock(&thread_mutex);
			if(finished_distribution && fifo[threadnum].empty()) break;
		//}
	}
	//fout.close();
}


int main(int argc, char **argv)
{
	init();
	pthread_mutex_init(&thread_mutex, NULL);
	
	for(int i = 0; i < FIFONUM; ++i) {
		j[i] = i;
		int ret = pthread_create(&thread[i], NULL, mythread, &j[i]);
		if(ret) {
			cout << "Create pthread error!" << endl;
			return 0;
		}
	}
	
	
	//readin all the IPs to be looked up later
	string filenamestr = "DIP.txt";
	char filename[30];
	strcpy(filename, filenamestr.c_str());
	ifstream fin(filename);
	string theTime; //timestamp
	cout << "read in from DIP file..." << endl;
	int ipcnt = 0;
	while(fin >> theTime) {
		fin >> ipstr[ipcnt];
		ipcnt++;
	}
	cout << "ipcnt = " << ipcnt << endl;
	
	//start timer
	cout << "start looking up while distributing..." << endl;
	gettimeofday(&tv1,NULL);
	
	int ret;
	for(int l = 0; l < ipcnt; l++) {
		pthread_mutex_lock(&thread_mutex);
		//int ret = pthread_mutex_trylock(&thread_mutex);		
		//if(ret!=EBUSY) {
			int hash = DJBHash(ipstr[l]);
			distribute(hash, ipstr[l]);
			pthread_mutex_unlock(&thread_mutex);
		//}
	}
	finished_distribution = true;

	cout << "waiting for all threads over..." << endl;
	for(int i = 0; i < FIFONUM; ++i) {
		pthread_join(thread[i], NULL);
	}
	
	//end timer
	gettimeofday(&tv2,NULL);
	tv = (tv2.tv_sec - tv1.tv_sec) * 1000000 + tv2.tv_usec - tv1.tv_usec;
	cout << "TOTAL TIME = " << tv << "us" << endl;
	
	cout << "HANDLING CAPACITY = " << (double)ipcnt/(double)tv << " M DIPs per second" << endl;

	//calculate the hit rate
	int totalHitCnt = 0;
	for(int i = 0; i < FIFONUM; i++) {
		totalHitCnt += hitcnt[i];
		double hitRate = (double)hitcnt[i]/(double)fifoIPcnt[i];
		cout << "hit rate of thread[" << i << "] = " << hitRate << endl;
	}
	double totalHitRate = (double)totalHitCnt/(double)ipcnt;
	cout << "TOTAL HIT RATE = " << totalHitRate << endl;
	
	return 1;
}


