#include <stdio.h>
#include <string.h>
#include <iostream> 
#include <bits/stdc++.h>
#include "storage.hpp"

using namespace std; 

int Storage::check_hit(string key)
{
	for(int i=0;i<cache_set;i++)
	{
		if(LLC[i].key== key && LLC[i].valid==1){
			if (!policy.compare("lru"))
				llc_lru_update(i);
			else if (!policy.compare("lfu"))
				llc_lfu_update(i);
			return i;
		}
			
	}
	return -1;
}
int Storage::llc_lru_find_victim()
{
	for(int i=0;i<cache_set;i++)
		if(LLC[i].valid == 0)
			return i;
	for(int i=0;i<cache_set;i++)
	{
		if(LLC[i].lru == cache_set-1)
			return i;
	}

	assert(0);
}

int Storage::llc_lfu_find_victim()
{
	int min= 9999999,index;
	for(int i=0;i<cache_set;i++)
		if(LLC[i].valid == 0)
			return i;
	for(int i=0;i<cache_set;i++)
	{
		if(LLC[i].lfu <= min)
		{
			min = LLC[i].lfu;
			index = i;		
		}
	}
	return index;
	
}

void Storage::llc_lru_update(int index)
{
	for(int i=0;i<cache_set;i++)
		if(LLC[i].lru < LLC[index].lru && LLC[i].valid == 1)
			 	LLC[i].lru++;
	LLC[index].lru=0;
}

void Storage::llc_lfu_update(int index)
{
	LLC[index].lfu++;
}

void Storage::fill_cache(string key,string value)
{
	int index;
	if(check_hit(key) > -1)
	{
		assert(0);
	}
	else
	{
		if(!policy.compare("lru"))
		{
			index = llc_lru_find_victim();
			LLC[index].valid=1;
			LLC[index].lru=(cache_set-1);
			llc_lru_update(index);
		}
		else if(!policy.compare("lfu"))
		{
			index = llc_lfu_find_victim();
			LLC[index].valid=1;
			LLC[index].lfu=0;
			llc_lfu_update(index);
		}
		LLC[index].valid=1;
		LLC[index].key=key;
		LLC[index].value=value;
	}	
}
void Storage::print_cache()
{
	for(int i=0;i<cache_set;i++)
		if(LLC[i].valid == 1)
			cout<<"Set : "<<i<<" key: "<<LLC[i].key<<" value : "<<LLC[i].value<<" lru : "<<LLC[i].lru<<" lfu : " <<LLC[i].lfu<<endl;
} 
/*int main()
{
string key;
string value;
int count =1;
while(count == 1){
	cout<<"Enter key  "<<endl;
	cin>>key;
	cout<<"Enter value  "<<endl;
	cin>>value;
	fill_cache(key,value);
	print_cache();
	cout<<"Do you want to enter more : ";
	cin>>count;
}
}*/







