#include "storage.hpp"
#include <iostream>
#include <sstream>
#include <string>

string Storage::handle_get(string key)
{
    int idx = check_hit(key);
    if(idx != -1)
    {
        string value = LLC[idx].value;
        // respond here
        cout<<"Cache hit: "<<value<<endl;
        return value;
    }

    // binary search on keys
    stringstream ss(key);
    int K;
    ss>>K;
    int kidx = K%10;
    auto pos = keys[kidx].find(K);
    if(pos==keys[kidx].end())
    {
        cout<<"KEY NOT FOUND!"<<endl;
        return "ERROR";
    }

    int offs = table[kidx][K];
    f_db[kidx].seekp(offs, ios::beg);
    string line;
    getline(f_db[kidx], line);
    stringstream getVal(line);
    string k, v;
    getVal>>k;
    getVal>>v;
    cout<<"value: "<<v<<endl;
    // respond here
    fill_cache(k, v);
    return v;
}

void Storage::handle_put(string key, string value)
{
    int idx = check_hit(key);
    for(int i=0; i<cache_set; i++)
        if(LLC[i].key==key)
        {
            idx = i;
            break;
        }
    if(idx > -1)
    {
        LLC[idx].value = value;
        LLC[idx].valid = 1;
    }

    stringstream conv(key);
    int K;
    conv>>K;
    int kidx = K%10;
    string line;
    f_db[kidx].seekg(0, ios::beg);
    bool written = false;
    // bool isEmpty = f_db.peek() == EOF;
    string newl = key + " " + value;
    if(isEmpty[kidx])
    {
        f_db[kidx].seekg(0, ios::beg);
        f_db[kidx]<<newl<<endl;
        table[kidx][K] = 0;
        keys[kidx].insert(K);
        linesizes[kidx][K] = newl.size();
        isEmpty[kidx] = false;
        return;
    }

    ofstream newdb;
    newdb.open("temp.txt", ofstream::out);

    // check if the key is already present in the DB
    bool isPresent = false;
    for(auto i=keys[kidx].begin(); i!=keys[kidx].end(); i++)
        if(*i==K)
        {
            isPresent = true;
            break;
        }

    while(getline(f_db[kidx], line))
    {
        stringstream getVals(line);
        int k;
        getVals>>k;
        if(k < K)
        {
            newdb<<line<<endl;
        }
        else if(k > K)
        {
            if(!written)
            {
                newdb<<newl<<endl;
                written = true;

                // calculate offset for current line here
                if(!isPresent)
                {   
                    // cout<<"Inserting new: "<<K<<endl;
                    int maxkey=-1;
                    for(auto x = keys[kidx].begin(); x != keys[kidx].end(); x++)
                    {
                        if(*x > K)
                            break;
                        maxkey = *x;
                    }
                    if(maxkey == -1)
                        table[kidx][K] = 0;
                    else
                        table[kidx][K] = table[kidx][maxkey] + linesizes[kidx][maxkey] + 1;
                }
            }

            newdb<<line<<endl;
            // update offsets here with newline.sze() + 1
            if(!isPresent)
                table[kidx][k] += newl.size() + 1;
        }
    }

    if(!written)
    {
        newdb<<newl<<endl;
        // calculate offset for current line here
        if(!isPresent)
        {
            int maxkey;
            for(auto x = keys[kidx].begin(); x != keys[kidx].end(); x++)
            {
                if(*x > K)
                    break;
                maxkey = *x;
            }
            table[kidx][K] = table[kidx][maxkey] + linesizes[kidx][maxkey] + 1;
        }
    }
    keys[kidx].insert(K);
    linesizes[kidx][K] = newl.size();
    newdb.close();
    f_db[kidx].close();
    string dbfname = "keydb" + to_string(kidx) + ".txt";
    remove(dbfname.c_str());
    rename("temp.txt", dbfname.c_str());
    f_db[kidx].open(dbfname, ios::out | ios::app | ios::in);
}

string Storage::handle_delete(string key)
{
    int idx = check_hit(key);
    if(idx > -1)
        LLC[idx].valid = 0;

    bool found = false;
    stringstream ss(key);
    int K;
    ss>>K;
    int kidx = K%10;
    for(auto i=keys[kidx].begin(); i!=keys[kidx].end(); i++)
        if(*i==K)
        {
            found = true;
            break;
        }
    if(!found)
    {
        cout<<"KEY NOT FOUND!"<<endl;
        return "ERROR";
    }

    // remove entries
    table[kidx].erase(K);
    int lsz = linesizes[kidx][K]+1;
    linesizes[kidx].erase(K);
    set<int>::iterator i;
    for(i=keys[kidx].begin(); i!=keys[kidx].end(); i++)
        if(*i==K)
            break;
    keys[kidx].erase(i);
    for(map<int, uint64_t>::iterator i=table[kidx].begin(); i!=table[kidx].end(); i++)
    {
        if(i->first > K)
            i->second -= lsz;
    }

    f_db[kidx].seekg(0, ios::beg);
    string line;
    ofstream newdb;
    newdb.open("temp.txt", ofstream::out);
    while(getline(f_db[kidx], line))
    {
        stringstream getVals(line);
        string k;
        getVals>>k;
        if(k != key)
            newdb<<line<<endl;
    }

    newdb.close();
    f_db[kidx].close();
    string dbfname = "keydb" + to_string(kidx) + ".txt";
    remove(dbfname.c_str());
    rename("temp.txt", dbfname.c_str());
    f_db[kidx].open(dbfname, ios::out | ios::app | ios::in);
    return "SUCCESS";
}

// int main()
// {
//     // WRITE PARSER
    
//     // sample received string
//     string s, key, method;

//     Storage sr;
//     sr.handle_put("5", "500");
//     sr.handle_put("4", "400");
//     sr.handle_put("3", "300");
//     sr.handle_put("2", "200");

//     sr.handle_delete("3");
//     // sr.handle_put("2", "100");
//     // sr.handle_delete("2");
//     // sr.handle_put("2", "200");

//     // // sr.handle_delete("4");
//     // sr.handle_get("1");
//     // sr.handle_get("3");
//     sr.handle_get("2");
//     // sr.handle_get("4");
//     // sr.handle_get("5");
//     // sr.handle_delete("13");

//     for(auto i=sr.table.begin(); i!=sr.table.end(); i++)
//         cout<<i->first<<" "<<i->second<<endl;

//     return 0;
// }