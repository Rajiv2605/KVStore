#include "server.hpp"
#include <iostream>
#include <sstream>
#include <string>

void Server::handle_get(string key)
{
    int idx = check_hit(key);
    if(idx != -1)
    {
        string value = LLC[idx].value;
        // respond here
        cout<<"Cache hit: "<<value<<endl;
        return;
    }

    // binary search on keys
    stringstream ss(key);
    int K;
    ss>>K;
    auto pos = keys.find(K);
    if(pos==keys.end())
    {
        cout<<"KEY NOT FOUND!"<<endl;
        return;
    }

    int offs = table[K];
    f_db.seekp(offs, ios::beg);
    string line;
    getline(f_db, line);
    stringstream getVal(line);
    string k, v;
    getVal>>k;
    getVal>>v;
    cout<<"value: "<<v<<endl;
    // respond here
    fill_cache(k, v);
}

void Server::handle_put(string key, string value)
{
    int idx = check_hit(key);
    if(idx > -1)
    {
        LLC[idx].value = value;
        LLC[idx].valid = 1;
    }

    stringstream conv(key);
    f_db.seekg(0, ios::beg);
    int K;
    conv>>K;
    string line;
    bool written = false;
    bool isEmpty = f_db.peek() == EOF;
    string newl = key + " " + value;
    if(isEmpty)
    {
        f_db.seekg(0, ios::beg);
        f_db<<newl<<endl;
        table[K] = 0;
        keys.insert(K);
        linesizes[K] = newl.size();

        return;
    }

    ofstream newdb;
    newdb.open("temp.txt", ofstream::out);

    // check if the key is already present in the DB
    bool isPresent = false;
    for(auto i=keys.begin(); i!=keys.end(); i++)
        if(*i==K)
        {
            isPresent = true;
            break;
        }

    while(getline(f_db, line))
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
                    for(auto x = keys.begin(); x != keys.end(); x++)
                    {
                        if(*x > K)
                            break;
                        maxkey = *x;
                    }
                    if(maxkey == -1)
                        table[K] = 0;
                    else
                        table[K] = table[maxkey] + linesizes[maxkey] + 1;
                }
            }

            newdb<<line<<endl;
            // update offsets here with newline.sze() + 1
            if(!isPresent)
                table[k] += newl.size() + 1;
        }
    }

    if(!written)
    {
        newdb<<newl<<endl;
        // calculate offset for current line here
        if(!isPresent)
        {
            int maxkey;
            for(auto x = keys.begin(); x != keys.end(); x++)
            {
                if(*x > K)
                    break;
                maxkey = *x;
            }
            table[K] = table[maxkey] + linesizes[maxkey] + 1;
        }
    }
    keys.insert(K);
    linesizes[K] = newl.size();
    newdb.close();
    f_db.close();
    remove("keydb.txt");
    rename("temp.txt", "keydb.txt");
    f_db.open("keydb.txt", ios::out | ios::app | ios::in);
}

void Server::handle_delete(string key)
{
    int idx = check_hit(key);
    if(idx > -1)
        LLC[idx].valid = 0;

    bool found = false;
    stringstream ss(key);
    int K;
    ss>>K;
    for(auto i=keys.begin(); i!=keys.end(); i++)
        if(*i==K)
        {
            found = true;
            break;
        }
    if(!found)
    {
        cout<<"KEY NOT FOUND!"<<endl;
        return;
    }

    // remove entries
    table.erase(K);
    int lsz = linesizes[K]+1;
    linesizes.erase(K);
    set<int>::iterator i;
    for(i=keys.begin(); i!=keys.end(); i++)
        if(*i==K)
            break;
    keys.erase(i);
    for(map<int, uint64_t>::iterator i=table.begin(); i!=table.end(); i++)
    {
        if(i->first > K)
            i->second -= lsz;
    }

    f_db.seekg(0, ios::beg);
    string line;
    ofstream newdb;
    newdb.open("temp.txt", ofstream::out);
    while(getline(f_db, line))
    {
        stringstream getVals(line);
        string k;
        getVals>>k;
        if(k != key)
            newdb<<line<<endl;
    }

    newdb.close();
    f_db.close();
    remove("keydb.txt");
    rename("temp.txt", "keydb.txt");
    f_db.open("keydb.txt", ios::out | ios::app | ios::in);
}

// int main()
// {
//     // WRITE PARSER
    
//     // sample received string
//     string s, key, method;

//     Server sr;
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